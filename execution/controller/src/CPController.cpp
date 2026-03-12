#ifdef USE_CP

#include "CPController.h"
#include "execution/engine/src/events/EntryEvent.h"
#include "execution/engine/src/events/ExitEvent.h"
#include "execution/engine/src/events/ChoiceEvent.h"
#include "execution/engine/src/events/MessageDeliveryEvent.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"

using namespace BPMNOS::Execution;

CPController::CPController(const BPMNOS::Execution::FlattenedGraph* flattenedGraph)
  : SeededController(flattenedGraph)
  , constraintProgramm(flattenedGraph)
{
}

void CPController::setSolution(CP::Solution sol) {
  solution = std::make_unique<CP::Solution>(std::move(sol));
  if (&solution->model != &constraintProgramm.getModel()) {
    throw std::invalid_argument("CPController: Solution model mismatch");
  }
  initializeFromSolution();
}

void CPController::initializeFromSolution() {
  // Extract sequence from solution position variables
  std::vector<size_t> sequence(flattenedGraph->vertices.size());
  for (size_t i = 0; i < flattenedGraph->vertices.size(); i++) {
    auto& vertex = flattenedGraph->vertices[i];
    auto pos = solution->getVariableValue(
      constraintProgramm.position.at(vertex.get())
    ).value();
    sequence[(size_t)pos - 1] = i + 1;
  }
  setSeed(sequence);
}

bool CPController::isVisited(const Vertex* vertex) const {
  return solution->evaluate(
    constraintProgramm.visit.at(vertex)
  ).value_or(false);
}

BPMNOS::number CPController::getTimestamp(const Vertex* vertex) const {
  auto result = solution->evaluate(
    constraintProgramm.status.at(vertex)[BPMNOS::Model::ExtensionElements::Index::Timestamp].value
  );
  assert(result.has_value());
  return result.value();
}

void CPController::notice(const Observable* observable) {
  SeededController::notice(observable);
  if (observable->getObservableType() == Observable::Type::Message) {
    auto message = static_cast<const Message*>(observable);
    if (message->state == Message::State::CREATED) {
      // Get sender vertex from message header (same approach as CPSolutionObserver)
      auto senderId = message->header[BPMNOS::Model::MessageDefinition::Index::Sender].value();
      auto& [senderEntry, senderExit] = flattenedGraph->vertexMap.at({senderId, {}, message->origin});
      messages.emplace_back(message->weak_from_this(), senderEntry);
    }
  }
}

std::shared_ptr<Event> CPController::createEntryEvent(const SystemState* systemState, [[maybe_unused]] const Token* token, const Vertex* vertex) {
  if (!isVisited(vertex)) {
    throw std::runtime_error("CPController: Cannot enter unvisited vertex");
  }
  if (systemState->getTime() < getTimestamp(vertex)) {
    return nullptr;
  }
  return std::make_shared<EntryEvent>(token);
}

std::shared_ptr<Event> CPController::createExitEvent(const SystemState* systemState, const Token* token, const Vertex* vertex) {
  if (!isVisited(vertex)) {
    throw std::runtime_error("CPController: Cannot exit unvisited vertex");
  }
  if (systemState->getTime() < getTimestamp(vertex)) {
    return nullptr;
  }
  return std::make_shared<ExitEvent>(token);
}

std::shared_ptr<Event> CPController::createChoiceEvent([[maybe_unused]] const SystemState* systemState, const Token* token, const Vertex* vertex) {
  if (!isVisited(vertex)) {
    throw std::runtime_error("CPController: Cannot make choice for unvisited vertex");
  }

  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();

  // Extract choice values from CP solution status variables
  Values choices;
  choices.reserve(extensionElements->choices.size());

  auto& statusVars = constraintProgramm.status.at(vertex);
  for (auto& choice : extensionElements->choices) {
    auto& attrVars = statusVars[choice->attribute->index];
    auto value = solution->evaluate(attrVars.value);
    assert(value.has_value());
    choices.push_back(value.value());
  }

  return std::make_shared<ChoiceEvent>(token, std::move(choices));
}

std::shared_ptr<Event> CPController::createMessageDeliveryEvent([[maybe_unused]] const SystemState* systemState, const Token* token, const Vertex* vertex) {
  if (!isVisited(vertex)) {
    throw std::runtime_error("CPController: Cannot deliver message to unvisited vertex");
  }

  // Find sender from CP solution's messageFlow
  const Vertex* sender = nullptr;
  for (auto candidate : vertex->senders) {
    auto& messageFlowVar = constraintProgramm.messageFlow.at({candidate, vertex});
    if (solution->evaluate(messageFlowVar).value_or(false)) {
      sender = candidate;
      break;
    }
  }

  if (!sender) {
    throw std::runtime_error("CPController: No sender found in solution for message delivery");
  }

  // Find message from the designated sender vertex
  for (auto& [message_ptr, senderVertex] : messages) {
    if (auto message = message_ptr.lock();
        message && senderVertex == sender)
    {
      return std::make_shared<MessageDeliveryEvent>(token, message.get());
    }
  }

  // Message not yet available
  return nullptr;
}

#endif // USE_CP
