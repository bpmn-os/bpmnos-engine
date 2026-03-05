#include "BisectionalChoice.h"
#include "execution/engine/src/Mediator.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "execution/engine/src/events/ChoiceEvent.h"
#include <cassert>
#include <stdexcept>
#include <functional>
#include <limits>
#include <queue>
//#include <iostream>

using namespace BPMNOS::Execution;

BisectionalChoice::BisectionalChoice(Evaluator* evaluator)
  : GreedyDispatcher(evaluator)
  , enumeratedChoice(evaluator)
{
}

void BisectionalChoice::connect(Mediator* mediator) {
  mediator->addSubscriber(this,
    Execution::Observable::Type::ChoiceRequest
  );
  GreedyDispatcher::connect(mediator);
}


void BisectionalChoice::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::ChoiceRequest ) {
    assert( dynamic_cast<const DecisionRequest*>(observable) );
    auto request = static_cast<const DecisionRequest*>(observable);
    // create pseudo decision
    auto decision = std::make_shared<ChoiceDecision>(request->token, Values(), evaluator);
    decisionsWithoutEvaluation.emplace_back( request->token->weak_from_this(), request->weak_from_this(), decision );
  }
  else {
    GreedyDispatcher::notice(observable);
  }
}

std::shared_ptr<Event> BisectionalChoice::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
//std::cout << "BisectionalChoice::dispatchEvent" << std::endl;
  if ( systemState->currentTime > timestamp ) {
    timestamp = systemState->currentTime;
    clockTick();
  }

  for ( auto& [ token_ptr, request_ptr, _ ] : decisionsWithoutEvaluation ) {
    auto request = request_ptr.lock();
    assert( request );
    // forget previous decision and find new best decision for the request
    auto decision = determineBestChoices( request );
    if ( decision ) {
      addEvaluation( token_ptr, request_ptr, std::move(decision), decision->reward );
    }
  }
  decisionsWithoutEvaluation.clear();

  for ( auto [ cost, token_ptr, request_ptr, event_ptr ] : evaluatedDecisions ) {
//std::cerr << "Best choice decision " << event_ptr.lock()->jsonify() << " evaluated with " << cost << std::endl;
    return event_ptr.lock();
  }

//std::cerr << "No evaluated choice decision" << std::endl;
  return nullptr;
}

std::shared_ptr<Decision> BisectionalChoice::determineBestChoices(std::shared_ptr<const DecisionRequest> request) {
  auto token = request->token;
  assert( token->node );
  assert( token->node->represents<BPMNOS::Model::DecisionTask>() );
  assert( token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert( extensionElements->choices.size() );

  if ( extensionElements->choices.size() > 1 ) {
    // delegate to BestEnumeratedChoice
//std::cerr << "delegate multiple choices" << std::endl; 
    return enumeratedChoice.determineBestChoices(request);
  }

  auto& choice = extensionElements->choices[0];

  if ( !choice->enumeration.empty() ) {
//std::cerr << "delegate enumeration" << std::endl; 
    // delegate to BestEnumeratedChoice
    return enumeratedChoice.determineBestChoices(request);
  }

  if ( !choice->lowerBound && !choice->upperBound ) {
    throw std::runtime_error("BisectionalChoice: choice requires bounds");
  }

  // Single choice with bounds (no enumeration) → use bisection
  if ( choice->multipleOf ) {
    return discreteBisection(request, choice.get());
  }
  else {
//    throw std::runtime_error("BisectionalChoice: continuous bisection is not yet supported");
    // delegate to BestEnumeratedChoice
    return enumeratedChoice.determineBestChoices(request);
  }
}

BisectionalChoice::Candidate BisectionalChoice::evaluate(size_t index) {
//std::cerr << "Evaluate " << index << ": " << values[index] << std::endl;
  auto decision = std::make_shared<ChoiceDecision>(token, Values{ values[index] }, evaluator);
  decision->evaluate();
  return { index, decision };
}

std::tuple<size_t, BisectionalChoice::Candidate, size_t> BisectionalChoice::findFeasible(size_t first, size_t last) {
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

void BisectionalChoice::findBetweenFeasibleAndFeasible(Candidate left, Candidate right) {
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

void BisectionalChoice::findBetweenFeasibleAndInfeasible(Candidate feasible, size_t infeasibleIndex) {
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

void BisectionalChoice::findBetweenInfeasibleAndFeasible(size_t infeasibleIndex, Candidate feasible) {
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

std::shared_ptr<Decision> BisectionalChoice::discreteBisection(std::shared_ptr<const DecisionRequest> request, const BPMNOS::Model::Choice* choice) {
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

//std::cerr << "Best " << best.index << std::endl;
  return best.decision;
}

