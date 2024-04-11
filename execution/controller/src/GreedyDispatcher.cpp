#include "GreedyDispatcher.h"
#include "execution/engine/src/Engine.h"
#include <limits>
#include <cassert>
#include <iostream>

using namespace BPMNOS::Execution;

GreedyDispatcher::GreedyDispatcher( std::function<std::optional<double>(const Event* event)> evaluator )
  : evaluator(evaluator)
{
}

void GreedyDispatcher::connect(Mediator* mediator) {
  mediator->addSubscriber(this, 
    Observable::Type::DataUpdate,
    Observable::Type::ClockTick
  );
  EventDispatcher::connect(mediator);
}

void GreedyDispatcher::evaluate(std::weak_ptr<const Token> token_ptr, std::weak_ptr<const DecisionRequest> request_ptr, std::shared_ptr<Decision> decision) {
  // (re-)evaluate decision 
  auto value = decision->evaluate();
  // infeasible decisions are evaluated with high costs
  auto evaluation = std::make_shared<Evaluation>( (value.has_value() ? (double)value.value() : std::numeric_limits<double>::max() ), std::move(decision));
  evaluatedDecisions.emplace(value.value(), token_ptr, request_ptr, evaluation->weak_from_this());

  // add evaluation to respective container
  if ( !evaluation->decision->timeDependent && evaluation->decision->dataDependencies.empty() ) {
    invariantEvaluations.emplace_back(token_ptr, request_ptr, std::move(evaluation) );
  }
  else if ( evaluation->decision->timeDependent && evaluation->decision->dataDependencies.empty() ) {
    timeDependentEvaluations.emplace_back(token_ptr, request_ptr, std::move(evaluation) );
  }
  else if ( !evaluation->decision->timeDependent && evaluation->decision->dataDependencies.size() ) {
    assert(evaluation->decision->token);
    BPMNOS::number instanceId = evaluation->decision->token->owner->root->instance.value();
    dataDependentEvaluations[(long unsigned int)instanceId].emplace_back(token_ptr, request_ptr, evaluation->weak_from_this(), std::move(evaluation) );
  }
  else if ( evaluation->decision->timeDependent && evaluation->decision->dataDependencies.size() ) {
    assert(evaluation->decision->token);
    BPMNOS::number instanceId = evaluation->decision->token->owner->root->instance.value();
    timeAndDataDependentEvaluations[(long unsigned int)instanceId].emplace_back(token_ptr, request_ptr, evaluation->weak_from_this(), std::move(evaluation) );
  }
}

std::shared_ptr<Event> GreedyDispatcher::dispatchEvent( [[maybe_unused]] const SystemState* systemState ) {
  for ( auto& [ token_ptr, request_ptr, decision ] : decisionsWithoutEvaluation ) {
    assert(decision);
    if ( decision && !decision->expired() ) {
      evaluate( token_ptr, request_ptr, decision );
    }
  }
  decisionsWithoutEvaluation.clear();

  for ( auto [ cost, token_ptr, request_ptr, evaluation_ptr ] : evaluatedDecisions ) {
    if( auto evaluation = evaluation_ptr.lock();
      evaluation && !evaluation->decision->expired()
    )  {
      return evaluation->decision;
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
  auto removeDependentEvaluations = [this,&update](std::unordered_map< long unsigned int, auto_list< std::weak_ptr<const Token>, std::weak_ptr<const DecisionRequest>, std::weak_ptr<Evaluation>, std::shared_ptr<Evaluation> > >& evaluations) -> void {
    auto intersect = [](const std::vector<const BPMNOS::Model::Attribute*>& first, const std::vector<const BPMNOS::Model::Attribute*>& second) -> bool {
      for ( auto lhs : first ) {
        for ( auto rhs : second ) {
          if ( lhs == rhs ) {
            return true;
          }
        }
      }
      return false;
    };

    if ( auto it = evaluations.find((long unsigned int)update->instanceId);
      it != evaluations.end()
    ) {
      // check whether evaluation has become obsolete
      for ( auto& [ token_ptr, request_ptr, evaluation_ptr, evaluation ] : it->second ) { 
        if ( intersect(update->attributes, evaluation->decision->dataDependencies) ) {
          if ( !evaluation->decision->expired() ) {
            evaluation->decision->evaluation = std::nullopt;
            decisionsWithoutEvaluation.emplace_back( token_ptr, request_ptr, std::move(evaluation->decision) );
          }
          // invalidate evaluation_ptr for auto removal
          evaluation.reset();
        }
      }
    }
  };
    
  removeDependentEvaluations(dataDependentEvaluations);
  removeDependentEvaluations(timeAndDataDependentEvaluations);
}

void GreedyDispatcher::clockTick() {
  for ( auto& [token_ptr, request_ptr, evaluation ] : timeDependentEvaluations ) {
    if ( !evaluation->decision->expired() ) {
      evaluation->decision->evaluation = std::nullopt;
      decisionsWithoutEvaluation.emplace_back( token_ptr, request_ptr, std::move(evaluation->decision) );
    }
  }
  timeDependentEvaluations.clear();

  for ( auto& [ instance, evaluations ] : timeAndDataDependentEvaluations ) {
    for ( auto& [token_ptr, request_ptr, evaluation_ptr, evaluation ] : evaluations ) {
      if ( !evaluation->decision->expired() ) {
        evaluation->decision->evaluation = std::nullopt;
        decisionsWithoutEvaluation.emplace_back( token_ptr, request_ptr, std::move(evaluation->decision) );
      }
    }
  }
  timeAndDataDependentEvaluations.clear();
}



