#include "RandomChoice.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "execution/engine/src/events/ChoiceEvent.h"
#include "execution/engine/src/events/ErrorEvent.h"
#include "model/bpmnos/src/extensionElements/expression/LinearExpression.h"
#include "model/bpmnos/src/extensionElements/expression/Enumeration.h"
#include "model/utility/src/CollectionRegistry.h"
#include <cassert>

using namespace BPMNOS::Execution;

RandomChoice::RandomChoice()
  : randomGenerator{std::random_device{}()}
{
}

std::shared_ptr<Event> RandomChoice::dispatchEvent( const SystemState* systemState ) {
  for ( auto& [token_ptr, event] : systemState->pendingChoiceEvents ) {
    if( auto token = token_ptr.lock() )  {
      assert( token );
      assert( token->node );
      assert( token->node->represents<BPMNOS::Model::DecisionTask>() );
      assert( token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
      
      auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      
      BPMNOS::Values choices;
      auto status = token->status;
      auto data = *token->data;
      auto globals = token->globals;
      for ( auto& choice : extensionElements->choices ) {
             // make random choice
        if ( auto allowedValues = choice->getEnumeration(status,data,globals); allowedValues.has_value() ) {
          auto& values = allowedValues.value();
          if ( values.size() ) {

            if ( choice->attribute->type != STRING ) {
              auto [lb,ub] = choice->getBounds(status,data,globals);
              // remove values outside of bounds
              values.erase(
                std::remove_if(
                  values.begin(),
                  values.end(),
                  [&](const BPMNOS::number& value) { return (value < lb || value > ub); }
                ),
                values.end()
              );
            }

            std::uniform_int_distribution<> random_distribution(0,(int)values.size()-1);
            choices.push_back( values[ (size_t)random_distribution(randomGenerator) ] );
            choice->attributeRegistry.setValue(choice->attribute, status, data, globals, choices.back());
          }
          else {
            return std::make_shared<ErrorEvent>(token.get());
          }
        }
        else if ( choice->attribute->type != STRING ) {
          auto [min,max] = choice->getBounds(status,data,globals);
          if ( min > max ) {
            return std::make_shared<ErrorEvent>(token.get());
          }
          if ( choice->attribute->type == DECIMAL ) {
            std::uniform_real_distribution<> random_distribution((double)min,(double)max);
            choices.push_back( BPMNOS::to_number( random_distribution(randomGenerator), DECIMAL ) );
            choice->attributeRegistry.setValue(choice->attribute, status, data, globals, choices.back());
          }
          else {
            std::uniform_int_distribution<> random_distribution((int)min,(int)max);
            choices.push_back( BPMNOS::to_number( random_distribution(randomGenerator), INTEGER ) );
            choice->attributeRegistry.setValue(choice->attribute, status, data, globals, choices.back());
          }
        }
        else {
          return std::make_shared<ErrorEvent>(token.get());
        }
      }
      return std::make_shared<ChoiceEvent>(token.get(), std::move(choices));
    }
  }
  return nullptr;
}

