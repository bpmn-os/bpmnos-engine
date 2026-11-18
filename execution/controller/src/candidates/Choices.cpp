#include "Choices.h"
#include "execution/engine/src/Notifier.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/DataUpdate.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <functional>
#include <limits>
#include <queue>

using namespace BPMNOS::Execution;

Choices::Choices(std::shared_ptr<Evaluator> evaluator, std::optional<size_t> bisect)
  : CachedCandidates(std::move(evaluator))
  , bisect(bisect)
{
}

void Choices::connect(Notifier* notifier) {
  notifier->addSubscriber(this,
    Observable::Type::ChoiceRequest,
    Observable::Type::DataUpdate,
    Observable::Type::SystemState
  );
}

void Choices::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::ChoiceRequest ) {
    assert( dynamic_cast<const DecisionRequest*>(observable) );
    auto request = static_cast<const DecisionRequest*>(observable);
    queueSearch( request->token->weak_from_this(), request->weak_from_this() );
  }
  else if ( observable->getObservableType() == Observable::Type::SystemState ) {
    clear();   // start from a clean cache, then rebuild from the installed state
    systemState = static_cast<const SystemState*>(observable);
    // search the choice requests a freshly installed (e.g. resumed) state lists as pending
    for ( auto& [ token_ptr, request_ptr ] : systemState->pendingChoiceDecisions ) {
      queueSearch( token_ptr, request_ptr );
    }
  }
  else {
    if ( observable->getObservableType() == Observable::Type::DataUpdate ) {
      // a request whose search yielded no alternative holds no decision a data update could invalidate, and is
      // searched again when the update touches an attribute its choices depend on, for the instance of its token or,
      // for a global attribute, for every instance
      auto update = static_cast<const DataUpdate*>(observable);
      for ( auto it = requestsWithoutAlternatives.begin(); it != requestsWithoutAlternatives.end(); ) {
        auto token = std::get<0>(*it).lock();
        assert( token );
        auto& dependencies = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->choiceDependencies;
        bool concerned = std::ranges::any_of(update->attributes, [&](const BPMNOS::Model::Attribute* attribute) {
          return dependencies.contains(attribute) && ( attribute->isGlobal || token->owner->root->instance.value() == update->instanceId );
        });
        if ( concerned ) {
          queueSearch( std::get<0>(*it), std::get<1>(*it) );
          it = requestsWithoutAlternatives.erase(it);
        }
        else {
          ++it;
        }
      }
    }
    CachedCandidates::notice(observable);
  }
}

void Choices::clear() {
  CachedCandidates::clear();
  requestsToSearch.clear();
  requestsWithoutAlternatives.clear();
}

void Choices::evaluateCandidates() {
  // a decision invalidated by a data update has its request searched anew rather than being evaluated alone
  evaluateDecisions(
    [this]( std::weak_ptr<const Token> token_ptr, std::weak_ptr<const DecisionRequest> request_ptr, [[maybe_unused]] std::shared_ptr<Decision> decision ) -> std::shared_ptr<Event> {
      queueSearch( token_ptr, request_ptr );
      return nullptr;
    }
  );
  // the requests are searched in the order in which they arose
  for ( auto it = requestsToSearch.begin(); it != requestsToSearch.end(); it = requestsToSearch.erase(it) ) {
    auto request = std::get<1>(*it).lock();
    assert( request );
    search(request);
  }
}

void Choices::queueSearch(std::weak_ptr<const Token> token_ptr, std::weak_ptr<const DecisionRequest> request_ptr) {
  auto request = request_ptr.lock();
  if ( !request || requestsToSearch.find(request.get()) != requestsToSearch.end() ) {
    return;
  }
  requestsToSearch.emplace_back( std::move(token_ptr), std::move(request_ptr) );
}

void Choices::dropDecisions(const DecisionRequest* request) {
  // the candidates and the indexes for invalidation hold weak references only, so that they drop with the decisions
  for ( auto it = decisions.begin(); it != decisions.end(); ) {
    if ( std::get<1>(*it).lock().get() == request ) {
      it = decisions.erase(it);
    }
    else {
      ++it;
    }
  }
}

void Choices::search(std::shared_ptr<const DecisionRequest> request) {
  dropDecisions(request.get());
  requestsWithoutAlternatives.remove(request.get());
  alternatives = 0;
  determineBestChoices(request);
  if ( alternatives == 0 ) {
    requestsWithoutAlternatives.emplace_back( request->token->weak_from_this(), request->weak_from_this() );
  }
}

void Choices::addAlternative(std::shared_ptr<ChoiceDecision> decision) {
  // owned by the base without being queued for evaluation, since it is evaluated already, and listed as a candidate
  // indexed for invalidation by its dependencies
  Candidates< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest> >::addDecision( token_ptr, request_ptr, decision );
  addEvaluation( token_ptr, request_ptr, decision );
  alternatives++;
}

std::shared_ptr<Decision> Choices::determineBestChoices(std::shared_ptr<const DecisionRequest> request) {
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
      throw std::runtime_error("Choices: choice of attribute '" + choice->attribute->name + "' requires an enumeration or bounds");
    }
    auto [LB, UB] = choice->getBounds(token->status, *token->data);
    // the step of a decimal attribute without a step is the precision of numbers
    step = BPMNOS_NUMBER_PRECISION;
    if ( choice->multipleOf ) {
      auto discretizer = choice->multipleOf->execute(token->status, *token->data);
      if ( !discretizer.has_value() ) {
        throw std::runtime_error("Choices: cannot determine step for '" + choice->multipleOf->expression + "'");
      }
      step = std::abs(discretizer.value());
      if ( !(step > 0.0) ) {
        throw std::runtime_error("Choices: step for '" + choice->multipleOf->expression + "' is not positive");
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
      throw std::runtime_error("Choices: choice of attribute '" + choice->attribute->name + "' allows too many values");
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

BPMNOS::number Choices::value(size_t index) const {
  assert( index < count );
  return bounded ? BPMNOS::number((firstMultiple + (double)index) * step) : enumeration[index];
}

std::shared_ptr<Decision> Choices::bestEnumeratedChoices(std::shared_ptr<const DecisionRequest> request) {
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
    // every alternative is a candidate (infeasible last)
    addAlternative(decision);
  }
  return bestDecision;
}

std::shared_ptr<Decision> Choices::bestEnumeratedChoice() {
  std::shared_ptr<Decision> bestDecision = nullptr;
  for ( size_t index = 0; index < count; index++ ) {
    auto candidate = evaluate(index);
    if ( candidate.isFeasible() && ( !bestDecision || candidate.reward() > bestDecision->reward().value() ) ) {
      bestDecision = candidate.decision;
    }
  }
  return bestDecision;
}

Choices::Candidate Choices::evaluate(size_t index) {
  auto request = request_ptr.lock();
  assert( request );
  auto decision = std::make_shared<ChoiceDecision>(request.get(), std::vector<number>{ value(index) }, evaluator.get());
  decision->evaluate();
  // this sampled alternative is a candidate; keep the local for the result
  addAlternative(decision);
  return { index, decision };
}

std::tuple<size_t, Choices::Candidate, size_t> Choices::findFeasible(size_t first, size_t last) {
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

void Choices::findBetweenFeasibleAndFeasible(Candidate left, Candidate right) {
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

void Choices::findBetweenFeasibleAndInfeasible(Candidate feasible, size_t infeasibleIndex) {
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

void Choices::findBetweenInfeasibleAndFeasible(size_t infeasibleIndex, Candidate feasible) {
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

std::shared_ptr<Decision> Choices::discreteBisection() {
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
