#include "ExtensionElements.h"
#include "model/bpmnos/src/Model.h"
#include "model/bpmnos/src/xml/bpmnos/tStatus.h"
#include "model/bpmnos/src/xml/bpmnos/tAttributes.h"
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"
#include "model/bpmnos/src/xml/bpmnos/tRestrictions.h"
#include "model/bpmnos/src/xml/bpmnos/tRestriction.h"
#include "model/bpmnos/src/xml/bpmnos/tOperators.h"
#include "model/bpmnos/src/xml/bpmnos/tOperator.h"
#include "model/bpmnos/src/xml/bpmnos/tDecisions.h"
#include "model/bpmnos/src/xml/bpmnos/tDecision.h"
#include "model/bpmnos/src/xml/bpmnos/tMessages.h"
#include "model/bpmnos/src/xml/bpmnos/tMessage.h"
#include "model/bpmnos/src/xml/bpmnos/tLoopCharacteristics.h"
#include "model/bpmnos/src/xml/bpmnos/tGuidance.h"
#include "model/utility/src/Keywords.h"
#include <algorithm>
#include <iostream>

using namespace BPMNOS::Model;

ExtensionElements::ExtensionElements(XML::bpmn::tBaseElement* baseElement, const AttributeRegistry attributeRegistry_, BPMN::Scope* parent, std::vector<std::reference_wrapper<XML::bpmnos::tAttribute>> dataAttributes)
  : BPMN::ExtensionElements( baseElement )
  , attributeRegistry(attributeRegistry_)
  , parent(parent)
  , hasSequentialPerformer(false)
  , isInstantaneous(true)
{
  // @attention: this->baseElement is only set after constructed extension elements have been bound to the BPMN base element. 

  // add all data attributes
  for ( XML::bpmnos::tAttribute& attributeElement : dataAttributes ) {
    data.push_back( std::make_unique<Attribute>(&attributeElement, Attribute::Category::DATA, attributeRegistry) );
  }

  if ( !element ) return; 

  if ( auto status = element->getOptionalChild<XML::bpmnos::tStatus>(); status.has_value() ) {
    // add all attributes
    if ( status->get().attributes.has_value() ) {
      for ( XML::bpmnos::tAttribute& attributeElement : status->get().attributes.value().get().attribute ) {
        auto attribute = std::make_unique<Attribute>(&attributeElement, Attribute::Category::STATUS, attributeRegistry);
        if ( attribute->id == Keyword::Instance ) {
          // always insert instance attribute at first position
          attributes.insert(attributes.begin(), std::move(attribute));
          // fix indices
          for (size_t index = 0; index < attributes.size(); index ++) {
            attributes[index]->index = index;
          }
        }
        else if ( attribute->id == Keyword::Timestamp ) {
          if ( attributes.size() && attributes[0]->id == Keyword::Instance ) {
            // insert timestamp attribute at the second position
            attributes.insert(++attributes.begin(), std::move(attribute));
            // fix indices
            for (size_t index = 0; index < attributes.size(); index ++) {
              attributes[index]->index = index;
            }
          }
          else {
            // insert timestamp attribute at first position, expecting that 
            // instance attribute will be inserted before
            attributes.insert(attributes.begin(), std::move(attribute));
            // fix indices
            for (size_t index = 0; index < attributes.size(); index ++) {
              attributes[index]->index = index;
            }
          }
        }
        else {
          attributes.push_back(std::move(attribute));
        }
      }
    }    

    // add all restrictions
    if ( status->get().restrictions.has_value() ) {
      for ( XML::bpmnos::tRestriction& restriction : status->get().restrictions.value().get().restriction ) {
        try {
          restrictions.push_back(std::make_unique<Restriction>(&restriction,attributeRegistry));
        }
        catch ( const std::exception& error ) {
          throw std::runtime_error("ExtensionElements: illegal parameters for restriction '" + (std::string)restriction.id.value + "'.\n" + error.what());
        }
      }

      auto& restriction = restrictions.back();
      for ( auto input : restriction->expression->inputs ) {
        if ( restriction->scope != Restriction::Scope::EXIT ) {
          entryDependencies.insert(input);
        }
        if ( restriction->scope != Restriction::Scope::ENTRY ) {
          exitDependencies.insert(input);
        }
      }
    }
    // recursively determine entry and exit dependencies
    auto ancestor = parent;
    while ( ancestor ) {
      if ( auto extensionElements = ancestor->extensionElements->represents<BPMNOS::Model::ExtensionElements>() ) {
        for ( auto& restriction : extensionElements->restrictions ) {
          for ( auto input : restriction->expression->inputs ) {
            if ( restriction->scope == Restriction::Scope::FULL ) {
              entryDependencies.insert(input);
              exitDependencies.insert(input);
            }
          }
        }
      }
      if ( auto child = ancestor->represents<BPMN::ChildNode>() ) {
        ancestor = child->parent;
      }
      else {
        break;
      }
    }

    // add all operators
    if ( status->get().operators.has_value() ) {
      for ( XML::bpmnos::tOperator& operator_ : status->get().operators.value().get().operator_ ) {
        try {
          operators.push_back( Operator::create(&operator_,attributeRegistry) );
        }
        catch ( const std::exception& error ) {
          throw std::runtime_error("ExtensionElements: illegal parameters for operator '" + (std::string)operator_.id.value + "'.\n" + error.what() );
        }
        auto attribute = operators.back()->attribute;
        if ( attribute->category == Attribute::Category::STATUS && attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
          isInstantaneous = false;
        }
        if ( attribute->category == Attribute::Category::DATA && attribute->index == BPMNOS::Model::ExtensionElements::Index::Instance ) {
          throw std::runtime_error("ExtensionElements: operator '" + (std::string)operator_.id.value + "' modifies instance attribute.\n" );
        }
        
        for ( auto input : operators.back()->inputs ) {
          operatorDependencies.insert(input);
        }
      }
    }    

    // add all choices to be made
    if ( status->get().decisions.has_value() ) {
      for ( XML::bpmnos::tDecision& decision : status->get().decisions.value().get().decision ) {
        try {
          choices.push_back(std::make_unique<Choice>(&decision,attributeRegistry,restrictions));
        }
        catch ( const std::exception& error ) {
          throw std::runtime_error("ExtensionElements: illegal attributes for choice '" + (std::string)decision.id.value + "'.\n" + error.what());
        }
      }
    }    
  }

  // add data attributes and global values modified by operator to dataUpdateOnEntry or dataUpdateOnCompletion
  dataUpdateOnEntry.global = false; // data update does not affect all instances unless a global value is updated
  dataUpdateOnCompletion.global = false;  // data update does not affect all instances unless a global value is updated
  
  if ( baseElement->is<XML::bpmn::tTask>() && !baseElement->is<XML::bpmn::tSendTask>() ) {
    for ( auto& operator_ : operators ) {
      if ( operator_->attribute->category != Attribute::Category::STATUS ) {
        dataUpdateOnCompletion.attributes.push_back(operator_->attribute);
      }
      if ( operator_->attribute->category == Attribute::Category::GLOBAL ) {
        dataUpdateOnCompletion.global = true;
      }
    }
  }
  else if ( baseElement->is<XML::bpmn::tActivity>() ) {
    // baseElement is a send task or a subprocess
    for ( auto& operator_ : operators ) {
      if ( operator_->attribute->category != Attribute::Category::STATUS ) {
        dataUpdateOnEntry.attributes.push_back(operator_->attribute);
      }
      if ( operator_->attribute->category == Attribute::Category::GLOBAL ) {
        dataUpdateOnEntry.global = true;
      }
    }
  }

  // add data attributes and global values modified by choice to dataUpdateOnCompletion
  for ( auto& choice : choices ) {
    if ( choice->attribute->category != Attribute::Category::STATUS ) {
      dataUpdateOnCompletion.attributes.push_back(choice->attribute);
    }
    if ( choice->attribute->category == Attribute::Category::GLOBAL ) {
      dataUpdateOnCompletion.global = true;
    }
  }

  // add all message definitions
  if ( element->getOptionalChild<XML::bpmnos::tMessages>().has_value() ) {
    for ( XML::bpmnos::tMessage& message : element->getOptionalChild<XML::bpmnos::tMessages>()->get().message ) {
      messageDefinitions.push_back(std::make_unique<MessageDefinition>(&message,attributeRegistry));
    }
  }
  else if ( auto message = element->getOptionalChild<XML::bpmnos::tMessage>(); message.has_value() ) {
    messageDefinitions.push_back(std::make_unique<MessageDefinition>(&message->get(),attributeRegistry));
  }

  if ( baseElement->is<XML::bpmn::tReceiveTask>() || baseElement->is<XML::bpmn::tCatchEvent>() ) {
    // add data attributes modified by message to dataUpdateOnCompletion (global values must not be updated by messages)
    for ( auto& messageDefinition : messageDefinitions ) {
       for ( auto& [key,content] : messageDefinition->contentMap ) {
         if ( !content->attribute.has_value() ) {
           throw std::runtime_error("ExtensionElements: missing attribute for content '" + (std::string)content->id + "'.");
         }
         Attribute* attribute = &content->attribute.value().get();
         if ( attribute->category == Attribute::Category::DATA ) {
           dataUpdateOnCompletion.attributes.push_back(attribute);
         }
       }
    }
  }


  // add loop characteristics
  if ( element->getOptionalChild<XML::bpmnos::tLoopCharacteristics>().has_value() ) {
    for ( XML::bpmnos::tParameter& parameter : element->getOptionalChild<XML::bpmnos::tLoopCharacteristics>()->get().parameter ) {
      if ( parameter.name.value.value == "cardinality" ) {
        loopCardinality = std::make_unique<Parameter>(&parameter,attributeRegistry);
      }
      else if ( parameter.name.value.value == "index" ) {
        loopIndex = std::make_unique<Parameter>(&parameter,attributeRegistry);
      }
      else if ( parameter.name.value.value == "condition" ) {
        loopCondition = std::make_unique<Parameter>(&parameter,attributeRegistry);
      }
      else if ( parameter.name.value.value == "maximum" ) {
        loopMaximum = std::make_unique<Parameter>(&parameter,attributeRegistry);
      }
    }
  }

  if ( auto process = baseElement->is<XML::bpmn::tProcess>() ) {
    hasSequentialPerformer = BPMNOS::Model::Model::hasSequentialPerformer( process->resourceRole );
  }
  else if ( auto activity = baseElement->is<XML::bpmn::tActivity>() ) {
    hasSequentialPerformer = BPMNOS::Model::Model::hasSequentialPerformer( activity->resourceRole );
  }
  
  // add all guidances
  for ( XML::bpmnos::tGuidance& item : element->getChildren<XML::bpmnos::tGuidance>() ) {
    auto guidance = std::make_unique<Guidance>(&item,attributeRegistry);
    if ( guidance->type == Guidance::Type::Entry ) {
      entryGuidance = std::move(guidance);
    }
    else if ( guidance->type == Guidance::Type::Exit ) {
      exitGuidance = std::move(guidance);
    }
    else if ( guidance->type == Guidance::Type::Choice ) {
      choiceGuidance = std::move(guidance);
    }
    else if ( guidance->type == Guidance::Type::MessageDelivery ) {
      messageDeliveryGuidance = std::move(guidance);
    }
  }
}

template <typename DataType>
bool ExtensionElements::feasibleEntry(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  for ( auto& restriction : restrictions ) {
    if ( restriction->scope != Restriction::Scope::EXIT && !restriction->isSatisfied(status,data,globals) ) {
      return false;
    }
  }
  return satisfiesInheritedRestrictions(status,data,globals);
}

template bool ExtensionElements::feasibleEntry<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template bool ExtensionElements::feasibleEntry<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;

template <typename DataType>
bool ExtensionElements::feasibleExit(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  for ( auto& restriction : restrictions ) {
    if ( restriction->scope != Restriction::Scope::ENTRY && !restriction->isSatisfied(status,data,globals) ) {
      return false;
    }
  }
  
  return satisfiesInheritedRestrictions(status,data,globals);
}

template bool ExtensionElements::feasibleExit<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template bool ExtensionElements::feasibleExit<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;


template <typename DataType>
bool ExtensionElements::satisfiesInheritedRestrictions(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  auto base = baseElement->represents<BPMN::ChildNode>();
  
  if ( !base ) return true;
  
  // check restrictions within ancestor scopes
  const BPMN::Node* ancestor = base->parent;
  while ( ancestor ) {
    assert( ancestor->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
    if ( !ancestor->extensionElements->as<BPMNOS::Model::ExtensionElements>()->fullScopeRestrictionsSatisfied(status,data,globals) ) {
      return false;
    }
    if ( auto eventSubProcess = ancestor->represents<BPMN::EventSubProcess>();
      eventSubProcess && 
      ( eventSubProcess->startEvent->represents<BPMN::ErrorStartEvent>() || eventSubProcess->startEvent->represents<BPMN::CompensateStartEvent>() )
    ) {
      // error and compensate event subprocesses do not inherit restrictions
      break;
    }
    else if ( auto activity = ancestor->represents<BPMN::Activity>();
      activity && activity->isForCompensation
    ) {
      // compensation activities do not inherit restrictions
      break;
    }
    else if ( auto child = ancestor->represents<BPMN::ChildNode>() ) {
      ancestor = child->parent;
    }
    else {
      break;
    }
  }
  return true;
}

template bool ExtensionElements::satisfiesInheritedRestrictions<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template bool ExtensionElements::satisfiesInheritedRestrictions<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;

template <typename DataType>
bool ExtensionElements::fullScopeRestrictionsSatisfied(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  for ( auto& restriction : restrictions ) {
    if ( restriction->scope == Restriction::Scope::FULL && !restriction->isSatisfied(status,data,globals) ) {
      return false;
    }
  }
  return true;
}

template bool ExtensionElements::fullScopeRestrictionsSatisfied<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template bool ExtensionElements::fullScopeRestrictionsSatisfied<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;


template <typename DataType>
void ExtensionElements::applyOperators(BPMNOS::Values& status, DataType& data, BPMNOS::Values& globals) const {
  for ( auto& operator_ : operators ) {
    operator_->apply(status,data,globals);
  }
}

template void ExtensionElements::applyOperators<BPMNOS::Values>(Values& status, BPMNOS::Values& data, BPMNOS::Values& globals) const;
template void ExtensionElements::applyOperators<BPMNOS::SharedValues>(Values& status, BPMNOS::SharedValues& data, BPMNOS::Values& globals) const;

template <typename DataType>
BPMNOS::number ExtensionElements::getObjective(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  BPMNOS::number objective = 0;
  for ( auto& [name, attribute] : attributeRegistry.statusAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data,globals);
    if ( value.has_value() ) {
//std::cerr << attribute->name << " contributes " <<  attribute->weight * value.value() << std::endl;
      objective += attribute->weight * value.value();
    }
  }
  for ( auto& [name, attribute] : attributeRegistry.dataAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data,globals);
    if ( value.has_value() ) {
//std::cerr << attribute->name << " contributes " <<  attribute->weight * value.value() << std::endl;
      objective += attribute->weight * value.value();
    }
  }
  for ( auto& [name, attribute] : attributeRegistry.globalAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data,globals);
    if ( value.has_value() ) {
//std::cerr << attribute->name << " contributes " <<  attribute->weight * value.value() << std::endl;
      objective += attribute->weight * value.value();
    }
  }
  return objective;
}

template BPMNOS::number ExtensionElements::getObjective<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template BPMNOS::number ExtensionElements::getObjective<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;


template <typename DataType>
BPMNOS::number ExtensionElements::getContributionToObjective(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  BPMNOS::number contribution = 0;
  for ( auto& attribute : attributes ) {
    auto value = attributeRegistry.getValue(attribute.get(),status,data,globals);
    if ( value.has_value() ) {
      contribution += attribute->weight * value.value();
    }
  }
  for ( auto& attribute : this->data ) {
    auto value = attributeRegistry.getValue(attribute.get(),status,data,globals);
    if ( value.has_value() ) {
      contribution += attribute->weight * value.value();
    }
  }
  return contribution;
}

template BPMNOS::number ExtensionElements::getContributionToObjective<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template BPMNOS::number ExtensionElements::getContributionToObjective<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;

