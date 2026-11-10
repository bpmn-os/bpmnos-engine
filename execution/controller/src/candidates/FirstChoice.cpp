#include "FirstChoice.h"
#include "execution/engine/src/Notifier.h"
#include "execution/engine/src/SystemState.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <functional>
#include <limits>
#include <queue>

using namespace BPMNOS::Execution;

FirstChoice::FirstChoice(std::shared_ptr<Evaluator> evaluator, std::optional<size_t> bisect)
  : Candidates(std::move(evaluator))
  , bisect(bisect)
{
}

void FirstChoice::connect(Notifier* notifier) {
  notifier->addSubscriber(this, Observable::Type::SystemState);
}

void FirstChoice::evaluateCandidates() {
  // stateless: recompute from scratch; determineBestChoices populates `candidates` with the considered request's
  // evaluated alternatives, so stop at the first feasible request
  this->clearDecisions();
  for ( [[maybe_unused]] auto& [ pendingToken, pendingRequest ] : systemState->pendingChoiceDecisions ) {
    if ( auto request = pendingRequest.lock() ) {
      if ( determineBestChoices( request ) ) {
        return;
      }
    }
  }
}

std::shared_ptr<Decision> FirstChoice::determineBestChoices(std::shared_ptr<const DecisionRequest> request) {
  this->clearDecisions();

  auto token = request->token;
  assert( token->node->represents<BPMN::FlowNode>() );
  assert( token->node->represents<BPMNOS::Model::DecisionTask>() );
  assert( token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert( extensionElements->choices.size() );

  token_ptr = token->weak_from_this();
  request_ptr = request->weak_from_this();

  if ( extensionElements->choices.size() > 1 ) {
    return bestEnumeratedChoices(request);
  }

  auto& choice = extensionElements->choices[0];

  if ( !choice->enumeration.empty() ) {
    bounded = false;
    enumeration = choice->getEnumeration(token->status, *token->data);
    count = enumeration.size();
  }
  else {
    if ( !choice->lowerBound || !choice->upperBound ) {
      throw std::runtime_error("FirstChoice: choice of attribute '" + choice->attribute->name + "' requires an enumeration or bounds");
    }
    auto [LB, UB] = choice->getBounds(token->status, *token->data);
    // the step of a decimal attribute without a step is the precision of numbers
    step = BPMNOS_NUMBER_PRECISION;
    if ( choice->multipleOf ) {
      auto discretizer = choice->multipleOf->execute(token->status, *token->data);
      if ( !discretizer.has_value() ) {
        throw std::runtime_error("FirstChoice: cannot determine step for '" + choice->multipleOf->expression + "'");
      }
      step = std::abs(discretizer.value());
      if ( !(step > 0.0) ) {
        throw std::runtime_error("FirstChoice: step for '" + choice->multipleOf->expression + "' is not positive");
      }
    }
    // the multiples are counted from zero, as by Choice::getEnumeration
    bounded = true;
    firstMultiple = std::floor((double)LB / step);
    if ( BPMNOS::number(firstMultiple * step) < LB ) {
      firstMultiple += 1.0;
    }
    double lastMultiple = std::floor((double)UB / step);
    if ( BPMNOS::number(lastMultiple * step) > UB ) {
      lastMultiple -= 1.0;
    }
    if ( lastMultiple < firstMultiple ) {
      count = 0;
    }
    else if ( lastMultiple - firstMultiple + 1.0 >= (double)std::numeric_limits<size_t>::max() ) {
      throw std::runtime_error("FirstChoice: choice of attribute '" + choice->attribute->name + "' allows too many values");
    }
    else {
      count = (size_t)(lastMultiple - firstMultiple + 1.0);
    }
  }

  if ( count == 0 ) {
    return nullptr;
  }

  if ( bisect.has_value() ? count >= bisect.value() : !choice->multipleOf && choice->enumeration.empty() ) {
    return discreteBisection();
  }
  return bestEnumeratedChoice();
}

BPMNOS::number FirstChoice::value(size_t index) const {
  assert( index < count );
  return bounded ? BPMNOS::number((firstMultiple + (double)index) * step) : enumeration[index];
}

std::shared_ptr<Decision> FirstChoice::bestEnumeratedChoices(std::shared_ptr<const DecisionRequest> request) {
  auto token = request->token;
  auto decisionTask = token->node->as<BPMNOS::Model::DecisionTask>();

  auto alternativeChoices = decisionTask->enumerateAlternatives(token->status, *token->data);
  std::shared_ptr<Decision> bestDecision = nullptr;
  for ( auto& choices : alternativeChoices ) {
    auto decision = std::make_shared<ChoiceDecision>(request.get(), std::move(choices), evaluator.get());
    decision->evaluate();
    auto reward = decision->reward();
    if ( reward.has_value() && ( !bestDecision || reward.value() > bestDecision->reward().value() ) ) {
      bestDecision = decision;
    }
    // add every alternative to the reward-ordered candidates (infeasible last), taking ownership
    this->addCandidate( token_ptr, request_ptr, std::move(decision) );
  }
  return bestDecision;
}

std::shared_ptr<Decision> FirstChoice::bestEnumeratedChoice() {
  std::shared_ptr<Decision> bestDecision = nullptr;
  for ( size_t index = 0; index < count; index++ ) {
    auto candidate = evaluate(index);
    if ( candidate.isFeasible() && ( !bestDecision || candidate.reward() > bestDecision->reward().value() ) ) {
      bestDecision = candidate.decision;
    }
  }
  return bestDecision;
}

FirstChoice::Candidate FirstChoice::evaluate(size_t index) {
  auto request = request_ptr.lock();
  assert( request );
  auto decision = std::make_shared<ChoiceDecision>(request.get(), std::vector<number>{ value(index) }, evaluator.get());
  decision->evaluate();
  // add this sampled alternative to the reward-ordered candidates, taking ownership; keep the local for the result
  this->addCandidate( token_ptr, request_ptr, decision );
  return { index, decision };
}

std::tuple<size_t, FirstChoice::Candidate, size_t> FirstChoice::findFeasible(size_t first, size_t last) {
  const size_t npos = std::numeric_limits<size_t>::max();
  if ( first > last ) return { npos, { npos, nullptr }, npos };

  // BFS on intervals to find any feasible
  std::queue<std::pair<size_t, size_t>> intervals;
  intervals.push({first, last});

  while ( !intervals.empty() ) {
    auto [l, r] = intervals.front();
    intervals.pop();

    size_t mid = l + (r - l) / 2;
    auto candidate = evaluate(mid);
    if ( candidate.isFeasible() ) {
      // Found feasible - return with nearest known infeasible bounds
      size_t nearestLeft = (mid > first) ? l : npos;
      size_t nearestRight = (mid < last) ? r : npos;
      return { nearestLeft, candidate, nearestRight };
    }

    // mid is infeasible - continue searching both halves
    if ( mid > l ) intervals.push({l, mid - 1});
    if ( mid < r ) intervals.push({mid + 1, r});
  }

  return { npos, { npos, nullptr }, npos };
}

void FirstChoice::findBetweenFeasibleAndFeasible(Candidate left, Candidate right) {
  if ( right.index <= left.index + 1 ) return;

  size_t mid = left.index + (right.index - left.index) / 2;
  auto candidate = evaluate(mid);

  if ( !candidate.isFeasible() ) {
    // Mid is infeasible - search both sides
    findBetweenFeasibleAndInfeasible(left, mid);
    findBetweenInfeasibleAndFeasible(mid, right);
  }
  else {
    // Mid is feasible
    if ( candidate.reward() > best.reward() ) {
      best = candidate;
    }
    // Check if mid is worse than both boundaries -> peak is at a boundary
    if ( candidate.reward() <= left.reward() && candidate.reward() <= right.reward() ) {
      return; // Peak is at one of the boundaries, already checked
    }
    // Continue toward better side
    if ( left.reward() >= right.reward() ) {
      findBetweenFeasibleAndFeasible(left, candidate);
    }
    else {
      findBetweenFeasibleAndFeasible(candidate, right);
    }
  }
}

void FirstChoice::findBetweenFeasibleAndInfeasible(Candidate feasible, size_t infeasibleIndex) {
  while ( infeasibleIndex > feasible.index + 1 ) {
    size_t mid = feasible.index + (infeasibleIndex - feasible.index + 1) / 2;
    auto candidate = evaluate(mid);

    if ( !candidate.isFeasible() ) {
      // Still infeasible - narrow right boundary
      infeasibleIndex = mid;
    }
    else if ( candidate.reward() <= best.reward() ) {
      // Feasible but worse - done (past peak)
      break;
    }
    else {
      // Feasible and better - new best, continue toward infeasible
      best = candidate;
      feasible = candidate;
    }
  }
}

void FirstChoice::findBetweenInfeasibleAndFeasible(size_t infeasibleIndex, Candidate feasible) {
  while ( feasible.index > infeasibleIndex + 1 ) {
    size_t mid = infeasibleIndex + (feasible.index - infeasibleIndex) / 2;
    auto candidate = evaluate(mid);

    if ( !candidate.isFeasible() ) {
      // Still infeasible - narrow left boundary
      infeasibleIndex = mid;
    }
    else if ( candidate.reward() <= best.reward() ) {
      // Feasible but worse - done (past peak)
      break;
    }
    else {
      // Feasible and better - new best, continue toward infeasible
      best = candidate;
      feasible = candidate;
    }
  }
}

std::shared_ptr<Decision> FirstChoice::discreteBisection() {
  const size_t npos = std::numeric_limits<size_t>::max();
  best = { npos, nullptr };

  // Check both boundaries
  auto left = evaluate(0);
  auto right = (count > 1) ? evaluate(count - 1) : Candidate{ npos, nullptr };


  if ( left.isFeasible() ) {
    best = left;
  }
  if ( right.isFeasible() && (!best.isFeasible() || right.reward() > best.reward()) ) {
    best = right;
  }

  if ( left.isFeasible() && right.isFeasible() ) {
    // Both feasible - search between them
    findBetweenFeasibleAndFeasible(left, right);
  }
  else if ( left.isFeasible() && !right.isFeasible() ) {
    // Left feasible, right infeasible - search toward right
    findBetweenFeasibleAndInfeasible(left, count - 1);
  }
  else if ( !left.isFeasible() && right.isFeasible() ) {
    // Left infeasible, right feasible - search toward left
    findBetweenInfeasibleAndFeasible(0, right);
  }
  else {
    // Both infeasible - find any feasible first
    if ( count <= 2 ) {
      return nullptr;
    }
    auto [leftInfeasible, candidate, rightInfeasible] = findFeasible(1, count - 2);
    if ( !candidate.isFeasible() ) {
      return nullptr;
    }
    best = candidate;

    // Search both directions from feasible point
    if ( leftInfeasible != npos ) {
      findBetweenInfeasibleAndFeasible(leftInfeasible, best);
    }
    if ( rightInfeasible != npos ) {
      findBetweenFeasibleAndInfeasible(best, rightInfeasible);
    }
  }

  return best.decision;
}
