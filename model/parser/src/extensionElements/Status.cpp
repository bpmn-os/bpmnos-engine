#include "Status.h"
#include "model/parser/src/xml/bpmnos/tStatus.h"
#include "model/parser/src/xml/bpmnos/tAttribute.h"
#include "model/parser/src/xml/bpmnos/tRestrictions.h"
#include "model/parser/src/xml/bpmnos/tRestriction.h"
#include "model/parser/src/xml/bpmnos/tOperators.h"
#include "model/parser/src/xml/bpmnos/tOperator.h"
#include "model/parser/src/xml/bpmnos/tDecisions.h"
#include "model/parser/src/xml/bpmnos/tDecision.h"
#include "model/parser/src/xml/bpmnos/tMessages.h"
#include "model/parser/src/xml/bpmnos/tMessage.h"
#include "model/parser/src/xml/bpmnos/tLoopCharacteristics.h"
#include "model/parser/src/xml/bpmnos/tGuidance.h"
#include "model/utility/src/Keywords.h"

using namespace BPMNOS::Model;

Status::Status(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement )
  , parent(parent)
  , isInstantaneous(true)
{

  if ( parent ) {
    parentSize = parent->extensionElements->as<Status>()->size();
    attributeMap = parent->extensionElements->as<Status>()->attributeMap;
  }
  else {
    parentSize = 0;
  }

  if ( !element ) return; 

  if ( auto status = element->getOptionalChild<XML::bpmnos::tStatus>(); status.has_value() ) {
    // add all attributes
    if ( status->get().attributes.has_value() ) {
      for ( XML::bpmnos::tAttribute& attributeElement : status->get().attributes.value().get().attribute ) {
        auto attribute = std::make_unique<Attribute>(&attributeElement,attributeMap);
        attributeMap[attribute->name] = attribute.get();
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
          restrictions.push_back(std::make_unique<Restriction>(&restriction,attributeMap));
        }
        catch ( ... ){
          throw std::runtime_error("Status: illegal parameters for restriction '" + (std::string)restriction.id.value + "'");
        }
      }
    }    
    // add all operators
    if ( status->get().operators.has_value() ) {
      for ( XML::bpmnos::tOperator& operator_ : status->get().operators.value().get().operator_ ) {
        try {
          operators.push_back(Operator::create(&operator_,attributeMap));
          if ( operators.back()->attribute->index == BPMNOS::Model::Status::Index::Instance ) {
            throw;
          }
          if ( operators.back()->attribute->index == BPMNOS::Model::Status::Index::Timestamp ) {
            isInstantaneous = false;
          }
        }
        catch ( ... ){
          throw std::runtime_error("Status: illegal parameters for operator '" + (std::string)operator_.id.value + "'");
        }
      }
    }    
    // add all decisions
    if ( status->get().decisions.has_value() ) {
      for ( XML::bpmnos::tDecision& decision : status->get().decisions.value().get().decision ) {
        try {
          decisions.push_back(std::make_unique<Decision>(&decision,attributeMap));
        }
        catch ( ... ){
          throw std::runtime_error("Status: illegal attributes for decision '" + (std::string)decision.id.value + "'");
        }
      }
    }    
  }

  // add all message definitions
  if ( element->getOptionalChild<XML::bpmnos::tMessages>().has_value() ) {
    for ( XML::bpmnos::tMessage& message : element->getOptionalChild<XML::bpmnos::tMessages>()->get().message ) {
      messageDefinitions.push_back(std::make_unique<MessageDefinition>(&message,attributeMap));
    }
  }
  else if ( auto message = element->getOptionalChild<XML::bpmnos::tMessage>(); message.has_value() ) {
    messageDefinitions.push_back(std::make_unique<MessageDefinition>(&message->get(),attributeMap));
  }

  // add loop characteristics
  if ( element->getOptionalChild<XML::bpmnos::tLoopCharacteristics>().has_value() ) {
    for ( XML::bpmnos::tParameter& parameter : element->getOptionalChild<XML::bpmnos::tLoopCharacteristics>()->get().parameter ) {
      if ( parameter.name.value.value == "cardinality" ) {
        loopCardinality = std::make_unique<Parameter>(&parameter,attributeMap);
      }
      else if ( parameter.name.value.value == "index" ) {
        loopIndex = std::make_unique<Parameter>(&parameter,attributeMap);
      }
    }
  }

  // add all guidances
  for ( XML::bpmnos::tGuidance& item : element->getChildren<XML::bpmnos::tGuidance>() ) {
    auto guidance = std::make_unique<Guidance>(&item,attributeMap);
    if ( guidance->type == Guidance::Type::Message ) {
      messageGuidance = std::move(guidance);
    }
    else if ( guidance->type == Guidance::Type::Entry ) {
      entryGuidance = std::move(guidance);
    }
    else if ( guidance->type == Guidance::Type::Exit ) {
      exitGuidance = std::move(guidance);
    }
    else if ( guidance->type == Guidance::Type::Choice ) {
      choiceGuidance = std::move(guidance);
    }
  }
}

bool Status::isFeasible(const Values& values) const {
  for ( auto& restriction : restrictions ) {
    if ( !restriction->isSatisfied(values) ) {
      return false; 
    }
  }
  return true; 
}  

void Status::applyOperators(Values& values) const {
  for ( auto& operator_ : operators ) {
    operator_->apply(values);
  }
}

void Status::makeChoices(const std::unordered_map<Decision*,number>& choices, Values& values) const {
  for ( auto& [decision,value] : choices ) {
    values[decision->attribute->index] = value;
  }
}

BPMNOS::number Status::getContributionToObjective(const Values& values) const {
  BPMNOS::number contribution = 0;
  for ( auto& attribute : attributes ) {
    if ( values[attribute->index].has_value() ) {
      contribution += attribute->weight * values[attribute->index].value();
    }
  }
  return contribution;
}
