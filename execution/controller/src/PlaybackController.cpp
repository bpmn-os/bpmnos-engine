#include "PlaybackController.h"
#include "decisions/EntryDecision.h"
#include "decisions/ChoiceDecision.h"
#include "decisions/MessageDeliveryDecision.h"
#include "execution/data/src/Scenario.h"
#include "execution/data/src/DataProvider.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/Token.h"
#include "execution/engine/src/Message.h"
#include "execution/engine/src/DecisionRequest.h"
#include "execution/engine/src/MessageDeliveryRequest.h"
#include "execution/engine/src/events/TerminationEvent.h"
#include "model/bpmnos/src/Model.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_set>

using namespace BPMNOS::Execution;

PlaybackController::PlaybackController(const nlohmann::ordered_json& log, std::shared_ptr<Evaluator> evaluator)
  : evaluator(std::move(evaluator))
{
  if ( !log.is_array() ) {
    throw std::runtime_error("PlaybackController: the log must be an array of records");
  }

  // returns the string a decision states for a field it requires
  auto field = [](const nlohmann::ordered_json& object, const std::string& name) -> std::string {
    if ( !object.contains(name) || !object.at(name).is_string() ) {
      throw std::runtime_error("PlaybackController: decision without field '" + name + "'");
    }
    return object.at(name).get<std::string>();
  };

  // decisions before the first clock tick belong to the time the run starts
  BPMNOS::number time = std::numeric_limits<BPMNOS::number>::lowest();
  for ( auto& record : log ) {
    if ( !record.is_object() || !record.contains("event") ) {
      // neither a clock tick nor a decision
      continue;
    }
    auto& event = record.at("event");

    if ( event.contains("event") && event.at("event") == "clocktick" ) {
      if ( !event.contains("time") || !event.at("time").is_number() ) {
        throw std::runtime_error("PlaybackController: clock tick without time");
      }
      time = BPMNOS::number(event.at("time").get<double>());
      continue;
    }

    if ( !event.contains("decision") ) {
      // an event the engine carries out itself
      continue;
    }

    LoggedDecision decision;
    decision.time = time;
    auto kind = field(event, "decision");
    if ( kind == "entry" ) {
      decision.kind = LoggedDecision::Kind::Entry;
    }
    else if ( kind == "choice" ) {
      decision.kind = LoggedDecision::Kind::Choice;
      if ( !event.contains("choices") || !event.at("choices").is_object() ) {
        throw std::runtime_error("PlaybackController: choice without field 'choices'");
      }
      decision.choices = event.at("choices");
    }
    else if ( kind == "messagedelivery" ) {
      decision.kind = LoggedDecision::Kind::MessageDelivery;
      if ( !event.contains("message") || !event.at("message").is_object() ) {
        throw std::runtime_error("PlaybackController: message delivery without field 'message'");
      }
      decision.message = event.at("message");
      decision.origin = field(decision.message, "origin");
      if ( decision.message.contains("header") && decision.message.at("header").contains("sender") && decision.message.at("header").at("sender").is_string() ) {
        decision.sender = decision.message.at("header").at("sender").get<std::string>();
      }
    }
    else {
      throw std::runtime_error("PlaybackController: decision of unknown kind '" + kind + "'");
    }
    decision.processId = field(event, "processId");
    decision.instanceId = field(event, "instanceId");
    decision.nodeId = field(event, "nodeId");

    decisions[time].push_back(std::move(decision));
  }

  // the decisions are ordered by their time, and within it by the log
  for ( auto& [_, decisionsAtTime] : decisions ) {
    for ( auto& decision : decisionsAtTime ) {
      sequence.push_back(&decision);
    }
  }
  dispatched.assign(sequence.size(), false);
  due.assign(sequence.size(), std::nullopt);
}

const std::map< BPMNOS::number, std::vector<PlaybackController::LoggedDecision> >& PlaybackController::getDecisions() const {
  return decisions;
}

size_t PlaybackController::getDispatched() const {
  return dispatchedCount;
}

const std::vector<PlaybackController::Deviation>& PlaybackController::getDeviations() const {
  return deviations;
}

const std::optional<PlaybackController::Mismatch>& PlaybackController::getMismatch() const {
  return mismatch;
}

bool PlaybackController::isComplete() const {
  return !mismatch && dispatchedCount == sequence.size();
}

nlohmann::ordered_json PlaybackController::jsonify(const LoggedDecision& decision) {
  nlohmann::ordered_json jsonObject;
  switch ( decision.kind ) {
    case LoggedDecision::Kind::Entry:
      jsonObject["decision"] = "entry";
      break;
    case LoggedDecision::Kind::Choice:
      jsonObject["decision"] = "choice";
      break;
    case LoggedDecision::Kind::MessageDelivery:
      jsonObject["decision"] = "messagedelivery";
      break;
  }
  jsonObject["processId"] = decision.processId;
  jsonObject["instanceId"] = decision.instanceId;
  jsonObject["nodeId"] = decision.nodeId;
  if ( decision.kind == LoggedDecision::Kind::Choice ) {
    jsonObject["choices"] = decision.choices;
  }
  else if ( decision.kind == LoggedDecision::Kind::MessageDelivery ) {
    jsonObject["message"] = decision.message;
  }
  if ( decision.time == std::numeric_limits<BPMNOS::number>::lowest() ) {
    jsonObject["time"] = nullptr;
  }
  else {
    jsonObject["time"] = (double)decision.time;
  }
  return jsonObject;
}

nlohmann::ordered_json PlaybackController::jsonify() const {
  nlohmann::ordered_json jsonObject;
  jsonObject["decisions"] = sequence.size();
  jsonObject["dispatched"] = dispatchedCount;
  jsonObject["deviations"] = nlohmann::ordered_json::array();
  for ( auto& deviation : deviations ) {
    auto deviationObject = jsonify(*deviation.decision);
    deviationObject["dispatchedAt"] = (double)deviation.time;
    jsonObject["deviations"].push_back(deviationObject);
  }
  if ( mismatch ) {
    auto mismatchObject = jsonify(*mismatch->decision);
    mismatchObject["foundAt"] = (double)mismatch->time;
    mismatchObject["reason"] = mismatch->reason;
    jsonObject["mismatch"] = mismatchObject;
  }
  else {
    jsonObject["mismatch"] = nullptr;
  }
  return jsonObject;
}

const BPMN::Node* PlaybackController::findNode(const SystemState* systemState, const std::string& nodeId) {
  if ( auto it = nodes.find(nodeId); it != nodes.end() ) {
    return it->second;
  }
  const BPMN::Node* found = nullptr;
  for ( auto& process : systemState->scenario->dataProvider->getModel()->processes ) {
    const BPMN::Process* constProcess = process.get();
    if ( auto node = constProcess->find([&nodeId](const BPMN::Node* candidate) { return candidate->id == nodeId; }) ) {
      found = node;
      break;
    }
  }
  nodes[nodeId] = found;
  return found;
}

std::string PlaybackController::performerKey(const SystemState* systemState, const LoggedDecision& decision, const BPMN::Node* node) {
  auto performer = node->as<BPMN::FlowNode>()->parent->as<BPMNOS::Model::SequentialAdHocSubProcess>()->performer;

  // returns true if the scope lies within the performer
  auto isWithinPerformer = [performer](const BPMN::Node* scope) {
    for ( auto flowNode = scope->represents<BPMN::FlowNode>(); flowNode; flowNode = flowNode->parent->represents<BPMN::FlowNode>() ) {
      if ( flowNode->parent == performer ) {
        return true;
      }
    }
    return false;
  };

  // the instance of the decision is the instance of the performer, followed by the disambiguations of the event
  // subprocesses and multi-instance activities within the performer, which are removed
  std::string instance = decision.instanceId;
  for ( auto position = instance.rfind(StateMachine::delimiters[0]); position != std::string::npos; position = instance.rfind(StateMachine::delimiters[0]) ) {
    auto disambiguation = instance.substr(position + 1);
    auto scope = findNode(systemState, disambiguation.substr(0, disambiguation.rfind(StateMachine::delimiters[1])));
    if ( !scope || !isWithinPerformer(scope) ) {
      break;
    }
    instance = instance.substr(0, position);
  }
  return instance + "/" + performer->id;
}

std::shared_ptr<Event> PlaybackController::stop(const LoggedDecision* decision, BPMNOS::number time, std::string reason) {
  mismatch = Mismatch{ decision, time, std::move(reason) };
  // the objective of a run deviating from the log validates nothing, so that the run ends
  return std::make_shared<TerminationEvent>();
}

std::shared_ptr<Decision> PlaybackController::createDecision(const SystemState* systemState, const LoggedDecision& decision) {
  // returns true if the token is the deciding token the log states
  auto isDeciding = [&decision](const Token* token) {
    return
      token->node->id == decision.nodeId &&
      token->owner->root->scope->id == decision.processId &&
      BPMNOS::to_string(token->getInstanceId(),STRING) == decision.instanceId
    ;
  };

  if ( decision.kind == LoggedDecision::Kind::Entry ) {
    for ( auto& [token_ptr, request_ptr] : systemState->pendingEntryDecisions ) {
      auto token = token_ptr.lock();
      auto request = request_ptr.lock();
      if ( token && request && isDeciding(token.get()) ) {
        return std::make_shared<EntryDecision>(request.get(), evaluator.get());
      }
    }
  }
  else if ( decision.kind == LoggedDecision::Kind::Choice ) {
    for ( auto& [token_ptr, request_ptr] : systemState->pendingChoiceDecisions ) {
      auto token = token_ptr.lock();
      auto request = request_ptr.lock();
      if ( token && request && isDeciding(token.get()) ) {
        // the values are given by attribute name and are taken in the order of the choices of the decision task
        auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
        std::vector<BPMNOS::number> values;
        for ( auto& choice : extensionElements->choices ) {
          auto& name = choice->attribute->name;
          if ( !decision.choices.contains(name) ) {
            return nullptr;
          }
          auto& value = decision.choices.at(name);
          if ( value.is_string() ) {
            values.push_back( BPMNOS::to_number(value.get<std::string>(), STRING) );
          }
          else if ( value.is_boolean() ) {
            values.push_back( BPMNOS::number( value.get<bool>() ? 1 : 0 ) );
          }
          else {
            values.push_back( BPMNOS::number( value.get<double>() ) );
          }
        }
        return std::make_shared<ChoiceDecision>(request.get(), values, evaluator.get());
      }
    }
  }
  else {
    for ( auto& [token_ptr, request_ptr] : systemState->pendingMessageDeliveryDecisions ) {
      auto token = token_ptr.lock();
      auto request = request_ptr.lock();
      if ( !token || !request || !isDeciding(token.get()) ) {
        continue;
      }
      auto& senderCandidates = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates;
      for ( auto& message : systemState->messages ) {
        if (
          message->state != Message::State::CREATED ||
          message->origin->id != decision.origin ||
          !std::ranges::contains(senderCandidates, message->origin) ||
          !message->matches(request->recipientHeader)
        ) {
          continue;
        }
        // the message agrees with the header and the content the log states
        auto messageObject = message->jsonify();
        bool agrees = true;
        if ( decision.message.contains("header") ) {
          auto& header = decision.message.at("header");
          for ( auto it = header.begin(); it != header.end(); ++it ) {
            if ( !it.value().is_null() && ( !messageObject["header"].contains(it.key()) || messageObject["header"][it.key()] != it.value() ) ) {
              agrees = false;
            }
          }
        }
        if ( decision.message.contains("content") && messageObject["content"] != decision.message.at("content") ) {
          agrees = false;
        }
        if ( agrees ) {
          return std::make_shared<MessageDeliveryDecision>(request.get(), message.get(), evaluator.get());
        }
      }
    }
  }
  return nullptr;
}

std::shared_ptr<Event> PlaybackController::dispatchEvent(const SystemState* systemState) {
  if ( mismatch ) {
    return nullptr;
  }
  auto now = systemState->getTime();

  // a decision becomes due at the first time reached that is not earlier than its own time
  for ( size_t i = 0; i < sequence.size() && sequence[i]->time <= now; i++ ) {
    if ( !dispatched[i] && !due[i].has_value() ) {
      due[i] = now;
    }
  }

  // performers whose next entry is listed before the entry considered
  std::unordered_set<std::string> busyPerformers;
  for ( size_t i = 0; i < sequence.size() && due[i].has_value(); i++ ) {
    if ( dispatched[i] ) {
      continue;
    }
    auto& decision = *sequence[i];

    if ( decision.kind == LoggedDecision::Kind::Entry ) {
      auto node = findNode(systemState, decision.nodeId);
      if ( !node || !BPMNOS::Model::Model::isSequentialActivity(node) ) {
        return stop(&decision, now, "entry of an activity whose entry is not decided");
      }
      // the entries of a performer keep the order of the log
      if ( !busyPerformers.insert(performerKey(systemState, decision, node)).second ) {
        continue;
      }
    }

    auto event = createDecision(systemState, decision);
    if ( !event ) {
      if ( decision.kind == LoggedDecision::Kind::Choice ) {
        // a choice whose request is pending but which lacks the value of a choice attribute is never dispatched
        auto node = findNode(systemState, decision.nodeId);
        if ( node ) {
          for ( auto& choice : node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->choices ) {
            if ( !decision.choices.contains(choice->attribute->name) ) {
              return stop(&decision, now, "choice without value of attribute '" + choice->attribute->name + "'");
            }
          }
        }
      }
      continue;
    }

    event->evaluate();
    if ( !event->reward().has_value() ) {
      // a decision that is infeasible now waits until it becomes feasible, as any controller has to
      continue;
    }

    dispatched[i] = true;
    dispatchedCount++;
    if ( decision.time != std::numeric_limits<BPMNOS::number>::lowest() && now > decision.time ) {
      // the run does not reach the time of the decision
      deviations.push_back( Deviation{ &decision, now } );
    }
    return event;
  }

  return nullptr;
}
