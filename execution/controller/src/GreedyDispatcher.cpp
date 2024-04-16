#include "GreedyDispatcher.h"
#include "execution/engine/src/Engine.h"
#include <limits>
#include <cassert>
#include <iostream>

using namespace BPMNOS::Execution;

GreedyDispatcher::GreedyDispatcher(Evaluator* evaluator)
  : evaluator(evaluator)
{
  if ( !evaluator ) {
    throw std::runtime_error("GreedyDispatcher: missing evaluator");
  }
}

void GreedyDispatcher::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::DataUpdate,
    Observable::Type::ClockTick
  );
  EventDispatcher::connect(mediator);
}

void GreedyDispatcher::evaluate(std::weak_ptr<const Token> token_ptr, std::weak_ptr<const DecisionRequest> request_ptr, std::shared_ptr<Decision> decision) {
  assert ( !token_ptr.expired() );
  assert ( !request_ptr.expired() );
  assert ( decision );

  auto value = decision->evaluate(evaluator);
  // decisions without evaluation are assumed to be infeasible
  evaluatedDecisions.emplace( (value.has_value() ? (double)value.value() : std::numeric_limits<double>::max() ), token_ptr, request_ptr, decision->weak_from_this());
//std::cerr << value.value_or(-1) << "Evaluation " << decision->evaluation.value_or(-1) << " for " << token_ptr.lock()->jsonify() << std::endl;

  assert ( decision );

  // add evaluation to respective container
  if ( !decision->timeDependent && decision->dataDependencies.empty() ) {
    invariantEvaluations.emplace_back(token_ptr, request_ptr, std::move(decision) );
  }
  else if ( decision->timeDependent && decision->dataDependencies.empty() ) {
    timeDependentEvaluations.emplace_back(token_ptr, request_ptr, std::move(decision) );
  }
  else if ( !decision->timeDependent && decision->dataDependencies.size() ) {
    assert(decision->token);
    BPMNOS::number instanceId = decision->token->owner->root->instance.value();
    dataDependentEvaluations[(long unsigned int)instanceId].emplace_back(token_ptr, request_ptr, std::move(decision) );
  }
  else if ( decision->timeDependent && decision->dataDependencies.size() ) {
    assert(decision->token);
    BPMNOS::number instanceId = decision->token->owner->root->instance.value();
    timeAndDataDependentEvaluations[(long unsigned int)instanceId].emplace_back(token_ptr, request_ptr, std::move(decision) );
  }
}

std::shared_ptr<Event> GreedyDispatcher::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto& [ token_ptr, request_ptr, decision ] : decisionsWithoutEvaluation ) {
//std::cerr << "(Re-)evaluate " << token_ptr.lock()->jsonify() << std::endl;
    assert(decision);
    if ( decision && !decision->expired() ) {
      evaluate( token_ptr, request_ptr, std::move(decision) );
    }
  }
  decisionsWithoutEvaluation.clear();

  for ( auto [ cost, token_ptr, request_ptr, event_ptr ] : evaluatedDecisions ) {
//std::cerr << "Decide " << token_ptr.lock()->jsonify() << std::endl;
    if( auto event = event_ptr.lock();
      event && !event->expired()
    )  {
      return event;
    }
  }
  return nullptr;
}

void GreedyDispatcher::notice(const Observable* observable) {
  if ( observable->getObservableType() == Observable::Type::DataUpdate ) {
    dataUpdate(static_cast<const DataUpdate*>(observable));   
  }
  else if ( observable->getObservableType() == Observable::Type::ClockTick ) {
    clockTick();
  }
}

void GreedyDispatcher::dataUpdate(const DataUpdate* update) {
//std::cerr << "DataUpdate " << (int)update->instanceId << std::endl;
/*
for ( auto attribute : update->attributes ) {
std::cerr << "Updated " << attribute->name << std::endl;
}
*/
  auto removeDependentEvaluations = [this,&update](std::unordered_map< long unsigned int, auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> > >& evaluations) -> void {
    auto intersect = [](const std::vector<const BPMNOS::Model::Attribute*>& first, const std::set<const BPMNOS::Model::Attribute*>& second) -> bool {
      for ( auto lhs : first ) {
        if ( second.contains(lhs) ) {
          return true;
        }
      }
      return false;
    };

    if ( auto evaluationIt = evaluations.find((long unsigned int)update->instanceId);
      evaluationIt != evaluations.end()
    ) {
      // check whether evaluation has become obsolete
      for ( auto it = evaluationIt->second.begin(); it != evaluationIt->second.end(); ) {
        auto& [ token_ptr, request_ptr, decision ] = *it;
        if ( intersect(update->attributes, decision->dataDependencies) ) {
          if ( !decision->expired() ) {
            decision->evaluation = std::nullopt;
            decisionsWithoutEvaluation.emplace_back( token_ptr, request_ptr, std::move(decision) );
          }
          // remove evaluation
          it = evaluationIt->second.erase(it);
        }
        else {
          ++it;
        }
      }
    }
  };
    
  removeDependentEvaluations(dataDependentEvaluations);
  removeDependentEvaluations(timeAndDataDependentEvaluations);
}

void GreedyDispatcher::clockTick() {
  for ( auto& [token_ptr, request_ptr, decision ] : timeDependentEvaluations ) {
    if ( !decision->expired() ) {
      decision->evaluation = std::nullopt;
      decisionsWithoutEvaluation.emplace_back( token_ptr, request_ptr, std::move(decision) );
    }
  }
  timeDependentEvaluations.clear();

  for ( auto& [ instance, evaluations ] : timeAndDataDependentEvaluations ) {
    for ( auto& [token_ptr, request_ptr, decision ] : evaluations ) {
      if ( !decision->expired() ) {
        decision->evaluation = std::nullopt;
        decisionsWithoutEvaluation.emplace_back( token_ptr, request_ptr, std::move(decision) );
      }
    }
  }
  timeAndDataDependentEvaluations.clear();
}



