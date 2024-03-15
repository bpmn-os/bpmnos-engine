#include "RandomChoiceHandler.h"
#include "model/parser/src/DecisionTask.h"
#include "model/parser/src/extensionElements/Attribute.h"
#include "execution/engine/src/events/CompletionEvent.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include <cassert>

using namespace BPMNOS::Execution;

RandomChoiceHandler::RandomChoiceHandler()
  : randomGenerator{std::random_device{}()}
{
}

std::shared_ptr<Event> RandomChoiceHandler::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, event] : systemState->pendingChoiceDecisions ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      assert( token->node );
      assert( token->node->represents<BPMNOS::Model::DecisionTask>() );
      assert( token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
      
      // make random choice
      auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      
      auto updatedStatus = token->status;
      for ( auto& choice : extensionElements->choices ) {
        BPMNOS::number min = choice->min;
        BPMNOS::number max = choice->max;

        // deduce stricter limits from restrictions
        for ( auto& restriction : extensionElements->restrictions ) {
          auto [lb,ub] = restriction->expression->getBounds(choice->attribute, updatedStatus);
          if ( lb.has_value() && lb.value() > min ) {
            min = lb.value();
          }
          if ( ub.has_value() && ub.value() < max ) {
            max = ub.value();
          }
        }
        
        if ( min <= max ) {
          if ( choice->attribute->type == DECIMAL ) {
            std::uniform_real_distribution<> random_distribution((double)min,(double)max);
            updatedStatus[choice->attribute->index] = BPMNOS::to_number( random_distribution(randomGenerator), DECIMAL );
          }
          else {
            std::uniform_int_distribution<> random_distribution((int)min,(int)max);
            updatedStatus[choice->attribute->index] = BPMNOS::to_number( random_distribution(randomGenerator), INTEGER);
          }
        }
        else {
          return std::make_shared<ErrorEvent>(token.get());
        }
      }
      event->updatedStatus = std::move(updatedStatus);
      return event;
    }
  }
  return nullptr;
}

