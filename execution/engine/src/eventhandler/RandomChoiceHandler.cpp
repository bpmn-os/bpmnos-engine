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

std::unique_ptr<Event> RandomChoiceHandler::fetchEvent( const SystemState* systemState ) {
  // assume that feasible choices are already made by an appropriate handler
  for ( auto& [ token_ptr ] : systemState->tokensAwaitingChoice ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      assert( token->node );
      assert( token->node->represents<BPMNOS::Model::DecisionTask>() );
      assert( token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
      
      // make random choice
      auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      
      auto updatedStatus = token->status;
      for ( auto& decision : extensionElements->decisions ) {
        BPMNOS::number min = decision->min;
        BPMNOS::number max = decision->max;
        // TODO: deduce stricter limits from restrictions
        
        if ( min <= max ) {
          if ( decision->attribute->type == DECIMAL ) {
            std::uniform_real_distribution<> random_distribution((double)min,(double)max);
            updatedStatus[decision->attribute->index] = BPMNOS::to_number( random_distribution(randomGenerator), DECIMAL );
          }
          else {
            std::uniform_int_distribution<> random_distribution((int)min,(int)max);
            updatedStatus[decision->attribute->index] = BPMNOS::to_number( random_distribution(randomGenerator), INTEGER);
          }
        }
        else {
          return std::make_unique<ErrorEvent>(token.get());
        }
      }
      return std::make_unique<CompletionEvent>(token.get(),std::move(updatedStatus));      
    }
  }
  return nullptr;
}

