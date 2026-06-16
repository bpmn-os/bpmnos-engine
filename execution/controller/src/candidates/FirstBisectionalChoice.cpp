#include "FirstBisectionalChoice.h"
#include "execution/engine/src/SystemState.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include <cassert>
#include <stdexcept>
#include <functional>
#include <limits>
#include <queue>

using namespace BPMNOS::Execution;

FirstBisectionalChoice::FirstBisectionalChoice(Evaluator* evaluator)
  : evaluator(evaluator)
{
}

void FirstBisectionalChoice::evaluateCandidates(const SystemState* systemState) {
  // stateless: recompute from scratch; determineBestChoices populates `candidates` with the considered request's
  // full alternative set, so stop at the first feasible request
  this->clearDecisions();
  for ( [[maybe_unused]] auto& [ pendingToken, pendingRequest ] : systemState->pendingChoiceDecisions ) {
    if ( auto request = pendingRequest.lock() ) {
      if ( determineBestChoices( request ) ) {
        return;
      }
    }
  }
}

std::shared_ptr<Decision> FirstBisectionalChoice::determineBestChoices(std::shared_ptr<const DecisionRequest> request) {
  this->clearDecisions();

  auto token = request->token;
  assert( token->node );
  assert( token->node->represents<BPMNOS::Model::DecisionTask>() );
  assert( token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert( extensionElements->choices.size() );

  token_ptr = token->weak_from_this();
  request_ptr = request->weak_from_this();

  if ( extensionElements->choices.size() > 1 ) {
    return bestEnumeratedChoice(request);
  }

  auto& choice = extensionElements->choices[0];

  if ( !choice->enumeration.empty() ) {
    return bestEnumeratedChoice(request);
  }

  if ( !choice->lowerBound && !choice->upperBound ) {
    throw std::runtime_error("FirstBisectionalChoice: choice requires bounds");
  }

  // Single choice with bounds (no enumeration) → use bisection
  if ( choice->multipleOf ) {
    return discreteBisection(request, choice.get());
  }
  else {
    return bestEnumeratedChoice(request);
  }
}

std::shared_ptr<Decision> FirstBisectionalChoice::bestEnumeratedChoice(std::shared_ptr<const DecisionRequest> request) {
  auto token = request->token;
  auto decisionTask = token->node->as<BPMNOS::Model::DecisionTask>();

  auto alternativeChoices = decisionTask->enumerateAlternatives(token->status, *token->data, token->globals);
  std::shared_ptr<Decision> bestDecision = nullptr;
  for ( auto& choices : alternativeChoices ) {
    auto decision = std::make_shared<ChoiceDecision>(token, std::move(choices), evaluator);
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

FirstBisectionalChoice::Candidate FirstBisectionalChoice::evaluate(size_t index) {
  auto decision = std::make_shared<ChoiceDecision>(token, std::vector<number>{ values[index] }, evaluator);
  decision->evaluate();
  // add this sampled alternative to the reward-ordered candidates, taking ownership; keep the local for the result
  this->addCandidate( token_ptr, request_ptr, decision );
  return { index, decision };
}

std::tuple<size_t, FirstBisectionalChoice::Candidate, size_t> FirstBisectionalChoice::findFeasible(size_t first, size_t last) {
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

void FirstBisectionalChoice::findBetweenFeasibleAndFeasible(Candidate left, Candidate right) {
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

void FirstBisectionalChoice::findBetweenFeasibleAndInfeasible(Candidate feasible, size_t infeasibleIndex) {
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

void FirstBisectionalChoice::findBetweenInfeasibleAndFeasible(size_t infeasibleIndex, Candidate feasible) {
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

std::shared_ptr<Decision> FirstBisectionalChoice::discreteBisection(std::shared_ptr<const DecisionRequest> request, const BPMNOS::Model::Choice* choice) {
  const size_t npos = std::numeric_limits<size_t>::max();

  // Set member state for this search
  token = request->token;
  values = choice->getEnumeration(token->status, *token->data, token->globals);
  best = { npos, nullptr };

  if ( values.empty() ) {
    return nullptr;
  }

  // Check both boundaries
  auto left = evaluate(0);
  auto right = (values.size() > 1) ? evaluate(values.size() - 1) : Candidate{ npos, nullptr };


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
    findBetweenFeasibleAndInfeasible(left, values.size() - 1);
  }
  else if ( !left.isFeasible() && right.isFeasible() ) {
    // Left infeasible, right feasible - search toward left
    findBetweenInfeasibleAndFeasible(0, right);
  }
  else {
    // Both infeasible - find any feasible first
    if ( values.size() <= 2 ) {
      return nullptr;
    }
    auto [leftInfeasible, candidate, rightInfeasible] = findFeasible(1, values.size() - 2);
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
