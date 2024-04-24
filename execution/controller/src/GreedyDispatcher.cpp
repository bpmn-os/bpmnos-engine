#include "GreedyDispatcher.h"
#include "execution/engine/src/Engine.h"
#include <limits>
#include <cassert>
//#include <iostream>

using namespace BPMNOS::Execution;

GreedyDispatcher::GreedyDispatcher(Evaluator* evaluator)
  : evaluator(evaluator)
{
  if ( !evaluator ) {
    throw std::runtime_error("GreedyDispatcher: missing evaluator");
  }
  timestamp = std::numeric_limits<BPMNOS::number>::min();
}

void GreedyDispatcher::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::DataUpdate
  );
  EventDispatcher::connect(mediator);
}

void GreedyDispatcher::evaluate(std::weak_ptr<const Token> token_ptr, std::weak_ptr<const DecisionRequest> request_ptr, std::shared_ptr<Decision> decision) {
  assert ( !token_ptr.expired() );
  assert ( !request_ptr.expired() );
  assert ( decision );

  auto value = decision->evaluate();
  // decisions without evaluation are assumed to be infeasible
  evaluatedDecisions.emplace( (value.has_value() ? (double)value.value() : std::numeric_limits<double>::max() ), token_ptr, request_ptr, decision->weak_from_this());

  assert ( decision );

  // add evaluation to respective container
  if ( !decision->timeDependent && decision->dataDependencies.empty() ) {
//std::cerr << "GreedyDispatcher: Decision is invariant"<< std::endl;
    invariantEvaluations.emplace_back(token_ptr, request_ptr, std::move(decision) );
  }
  else if ( decision->timeDependent && decision->dataDependencies.empty() ) {
//std::cerr << "GreedyDispatcher: Decision is time dependent" << std::endl;
    timeDependentEvaluations.emplace_back(token_ptr, request_ptr, std::move(decision) );
  }
  else if ( !decision->timeDependent && decision->dataDependencies.size() ) {
    assert(decision->token);
    BPMNOS::number instanceId = decision->token->owner->root->instance.value();
//std::cerr << "GreedyDispatcher: Decision is data dependent with index " << (long unsigned int)instanceId << std::endl;
    dataDependentEvaluations[(long unsigned int)instanceId].emplace_back(token_ptr, request_ptr, std::move(decision) );
  }
  else if ( decision->timeDependent && decision->dataDependencies.size() ) {
    assert(decision->token);
    BPMNOS::number instanceId = decision->token->owner->root->instance.value();
//std::cerr << "GreedyDispatcher: Decision is time and data dependent with index " << (long unsigned int)instanceId << std::endl;
    timeAndDataDependentEvaluations[(long unsigned int)instanceId].emplace_back(token_ptr, request_ptr, std::move(decision) );
  }
}

std::shared_ptr<Event> GreedyDispatcher::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  if ( systemState->currentTime > timestamp ) {
    timestamp = systemState->currentTime;
    clockTick();
  }

  for ( auto& [ token_ptr, request_ptr, decision ] : decisionsWithoutEvaluation ) {
    assert(decision);
    if ( decision && !decision->expired() ) {
      evaluate( token_ptr, request_ptr, std::move(decision) );
    }
  }
  decisionsWithoutEvaluation.clear();

  for ( auto [ cost, token_ptr, request_ptr, event_ptr ] : evaluatedDecisions ) {
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
}

void GreedyDispatcher::dataUpdate(const DataUpdate* update) {
/*
std::cerr << "DataUpdate: ";
for ( auto attribute : update->attributes ) {
std::cerr << attribute->name << ", ";
}
std::cerr << std::endl;
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

    auto removeObsolete = [this,&update,intersect](auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::shared_ptr<Decision> >& evaluation) -> void {
      // check whether evaluation has become obsolete
      for ( auto it = evaluation.begin(); it != evaluation.end(); ) {
        auto& [ token_ptr, request_ptr, decision ] = *it;
        if ( intersect(update->attributes, decision->dataDependencies) ) {
          if ( !decision->expired() ) {
            decision->evaluation = std::nullopt;
            decisionsWithoutEvaluation.emplace_back( token_ptr, request_ptr, std::move(decision) );
          }
          // remove evaluation
          it = evaluation.erase(it);
        }
        else {
          ++it;
        }
      }
    };

    if ( update->instanceId >= 0 ) {
      // find instance that data update refers to
      if ( auto it = evaluations.find((long unsigned int)update->instanceId);
        it != evaluations.end()
      ) {
        removeObsolete(it->second);
      }
    }
    else {
      // update of global value may influence evaluations of all instances
      for ( auto it = evaluations.begin(); it != evaluations.end(); ++it) {
        removeObsolete(it->second);
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



