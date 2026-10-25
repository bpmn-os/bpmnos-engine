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
#include "model/bpmnos/src/xml/bpmnos/tMessage.h"
#include "model/bpmnos/src/xml/bpmnos/tSignal.h"
#include "model/bpmnos/src/xml/bpmnos/tTimer.h"
#include "model/bpmnos/src/xml/bpmnos/tParameter.h"
#include "model/bpmnos/src/xml/bpmnos/tLoopCharacteristics.h"
#include "model/bpmnos/src/xml/bpmnos/tGuidance.h"
#include "model/utility/src/Keywords.h"
#include <algorithm>
#include <cassert>
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
//std::cerr << "Base: " << baseElement->id.value().get().value.value << std::endl;

  if ( element ) { 
    if ( auto status = element->getOptionalChild<XML::bpmnos::tStatus>(); status.has_value() ) {
      // add all attributes
      if ( status->get().attributes.has_value() ) {
        for ( XML::bpmnos::tAttribute& attributeElement : status->get().attributes.value().get().attribute ) {
          auto attribute = std::make_unique<Attribute>(&attributeElement, Attribute::Category::STATUS, attributeRegistry);
          if ( attribute->isObject() ) {
            // objects are held and numbered separately from scalar attributes
            if ( auto expression = std::get_if< std::unique_ptr<const Expression> >(&attribute->initialization) ) {
              for ( auto input : (*expression)->inputs ) {
                entryDependencies.insert(input);
              }
            }
            statusObjects.push_back(std::move(attribute));
          }
          else if ( attribute->id == Keyword::Instance ) {
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
            // TODO: add entry data dependencies
            if ( auto expression = std::get_if< std::unique_ptr<const Expression> >(&attribute->initialization) ) {
              for ( auto input : (*expression)->inputs ) {
                entryDependencies.insert(input);
              }
            }
            attributes.push_back(std::move(attribute));
          }
        }
      }    
    }
  }
  
  // add all data attributes
  for ( XML::bpmnos::tAttribute& attributeElement : dataAttributes ) {
    auto attribute = std::make_unique<Attribute>(&attributeElement, Attribute::Category::DATA, attributeRegistry);
    if ( attribute->isObject() ) {
      // objects are held and numbered separately from scalar attributes
      if ( auto expression = std::get_if< std::unique_ptr<const Expression> >(&attribute->initialization) ) {
        for ( auto input : (*expression)->inputs ) {
          entryDependencies.insert(input);
        }
      }
      dataObjects.push_back(std::move(attribute));
      continue;
    }
    data.push_back( std::move(attribute) );
    // TODO: add entry data dependencies
    if ( auto expression = std::get_if< std::unique_ptr<const Expression> >(&data.back()->initialization) ) {
      for ( auto input : (*expression)->inputs ) {
        entryDependencies.insert(input);
      }
    }
  }

  // error and compensate event subprocesses and compensation activities do not inherit restrictions, every other node
  // inherits the full-scope restrictions of its parent and those its parent inherits, the parent being created first
  auto inheritsRestrictions = [baseElement]() {
    if ( auto activity = baseElement->is<XML::bpmn::tActivity>();
      activity && activity->isForCompensation.has_value() && activity->isForCompensation.value().get().value
    ) {
      return false;
    }
    if ( auto subProcess = baseElement->is<XML::bpmn::tSubProcess>();
      subProcess && subProcess->triggeredByEvent.has_value() && (bool)subProcess->triggeredByEvent->get().value
    ) {
      for ( XML::bpmn::tStartEvent& startEvent : subProcess->getChildren<XML::bpmn::tStartEvent>() ) {
        for ( XML::bpmn::tEventDefinition& eventDefinition : startEvent.getChildren<XML::bpmn::tEventDefinition>() ) {
          if ( eventDefinition.is<XML::bpmn::tErrorEventDefinition>() || eventDefinition.is<XML::bpmn::tCompensateEventDefinition>() ) {
            return false;
          }
        }
      }
    }
    return true;
  };
  if ( parent && inheritsRestrictions() ) {
    assert( parent->extensionElements->represents<BPMNOS::Model::ExtensionElements>() );
    auto parentExtensionElements = parent->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    inheritedRestrictions = parentExtensionElements->inheritedRestrictions;
    for ( auto& restriction : parentExtensionElements->restrictions ) {
      if ( restriction->scope == Restriction::Scope::FULL ) {
        inheritedRestrictions.push_back(restriction.get());
      }
    }
    for ( auto restriction : inheritedRestrictions ) {
      for ( auto input : restriction->expression.inputs ) {
        entryDependencies.insert(input);
        completionDependencies.insert(input);
        exitDependencies.insert(input);
      }
    }
  }

  if ( !element ) return;

  // add message definition
  if ( auto message = element->getOptionalChild<XML::bpmnos::tMessage>(); message.has_value() ) {
    messageDefinition = std::make_unique<MessageDefinition>(&message->get(),attributeRegistry);
  }

  // add signal definition
  if ( auto signal = element->getOptionalChild<XML::bpmnos::tSignal>(); signal.has_value() ) {
    signalDefinition = std::make_unique<SignalDefinition>(&signal->get());
  }

  // add error definition
  if ( auto error = element->getOptionalChild<XML::bpmnos::tError>(); error.has_value() ) {
    errorDefinition = std::make_unique<ErrorDefinition>(&error->get());
  }

  // add escalation definition
  if ( auto escalation = element->getOptionalChild<XML::bpmnos::tEscalation>(); escalation.has_value() ) {
    escalationDefinition = std::make_unique<EscalationDefinition>(&escalation->get());
  }

  // the operators see the content object of the message, signal, error or escalation the node states, named after it
  if ( messageDefinition ) {
    augmentedAttributeRegistry = std::make_unique<AttributeRegistry>(attributeRegistry);
    content = std::make_unique<Attribute>(messageDefinition->element->name.value.value, messageDefinition->schema, *augmentedAttributeRegistry);
  }
  else if ( signalDefinition ) {
    augmentedAttributeRegistry = std::make_unique<AttributeRegistry>(attributeRegistry);
    content = std::make_unique<Attribute>(signalDefinition->element->name.value.value, signalDefinition->schema, *augmentedAttributeRegistry);
  }
  else if ( errorDefinition ) {
    augmentedAttributeRegistry = std::make_unique<AttributeRegistry>(attributeRegistry);
    content = std::make_unique<Attribute>(errorDefinition->element->name.value.value, errorDefinition->schema, *augmentedAttributeRegistry);
  }
  else if ( escalationDefinition ) {
    augmentedAttributeRegistry = std::make_unique<AttributeRegistry>(attributeRegistry);
    content = std::make_unique<Attribute>(escalationDefinition->element->name.value.value, escalationDefinition->schema, *augmentedAttributeRegistry);
  }

  if ( auto status = element->getOptionalChild<XML::bpmnos::tStatus>(); status.has_value() ) {
    // add all restrictions
    if ( status->get().restrictions.has_value() ) {
      for ( XML::bpmnos::tRestriction& restriction : status->get().restrictions.value().get().restriction ) {
        try {
          restrictions.push_back(std::make_unique<Restriction>(&restriction,attributeRegistry));
        }
        catch ( const std::exception& error ) {
          throw std::runtime_error("ExtensionElements: illegal restriction '" + (std::string)restriction.id.value + "'.\n" + error.what());
        }
        // add dependencies
        for ( auto input : restrictions.back()->expression.inputs ) {
          if ( restrictions.back()->scope == Restriction::Scope::ENTRY ) {
            entryDependencies.insert(input);
          }
          else if ( restrictions.back()->scope == Restriction::Scope::COMPLETION ) {
            completionDependencies.insert(input);
          }
          else if ( restrictions.back()->scope == Restriction::Scope::EXIT ) {
            exitDependencies.insert(input);
          }
          else {
            // Restriction::Scope::FULL
            entryDependencies.insert(input);
            completionDependencies.insert(input);
            exitDependencies.insert(input);
          }
        }
      }
    }

    // add all operators
    if ( status->get().operators.has_value() ) {
      // a scope, a process or a subprocess of any kind, has no operators; what is to happen when it starts is
      // stated by the operators of its start event
      if ( baseElement->is<XML::bpmn::tProcess>() || baseElement->is<XML::bpmn::tSubProcess>() ) {
        throw std::runtime_error("ExtensionElements: scope '" + (std::string)baseElement->id.value().get().value + "' must not have operators");
      }

      for ( XML::bpmnos::tOperator& operator_ : status->get().operators.value().get().operator_ ) {
        try {
          operators.push_back( std::make_unique<Operator>(&operator_, augmentedAttributeRegistry ? *augmentedAttributeRegistry : attributeRegistry) );
        }
        catch ( const std::exception& error ) {
          throw std::runtime_error("ExtensionElements: illegal operator '" + (std::string)operator_.id.value + "'.\n" + error.what() );
        }
        auto attribute = operators.back()->attribute;
        if ( attribute == content.get() && !baseElement->is<XML::bpmn::tThrowEvent>() && !baseElement->is<XML::bpmn::tSendTask>() ) {
          // the content object is written by the node that throws it, and only read by the node catching it
          throw std::runtime_error("ExtensionElements: operator '" + (std::string)operator_.id.value + "' of a catching node writes the content of '" + content->name + "'");
        }
        if ( attribute->category == Attribute::Category::STATUS && !attribute->isObject() && attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
          isInstantaneous = false;
        }
        if ( attribute->category == Attribute::Category::DATA && attribute->id == Keyword::Instance ) {
          throw std::runtime_error("ExtensionElements: operator '" + (std::string)operator_.id.value + "' modifies instance attribute.\n" );
        }
        
        for ( auto input : operators.back()->expression.inputs ) {
          // the content object is no attribute whose change could invalidate anything
          if ( input != content.get() ) {
            operatorDependencies.insert(input);
          }
        }
      }

      if ( !isInstantaneous ) {
        // a send task completes when its message is delivered, a receive task when a message is
        // delivered to it, and a decision task when the choices are made, so none of them may advance
        // the timestamp by an operator of its own
        bool isDecisionTask = false;
        if ( auto task = baseElement->is<XML::bpmn::tTask>() ) {
          if ( const auto& type = task->getOptionalAttributeByName("type");
            type.has_value() && type->get().xmlns == "https://bpmnos.telematique.eu"
          ) {
            isDecisionTask = ( type->get().value.value == "Decision" );
          }
        }
        if ( baseElement->is<XML::bpmn::tSendTask>() || baseElement->is<XML::bpmn::tReceiveTask>() || isDecisionTask ) {
          throw std::runtime_error("ExtensionElements: operators of task '" + (std::string)baseElement->id.value().get().value + "' must not modify timestamp");
        }

        // an event is instantaneous
        if ( baseElement->is<XML::bpmn::tEvent>() ) {
          throw std::runtime_error("ExtensionElements: operators of event '" + (std::string)baseElement->id.value().get().value + "' must not modify timestamp");
        }
      }
    }

    // add all choices to be made
    if ( status->get().decisions.has_value() ) {
      for ( XML::bpmnos::tDecision& decision : status->get().decisions.value().get().decision ) {
        try {
          choices.push_back(std::make_unique<Choice>(&decision,attributeRegistry));
        }
        catch ( const std::exception& error ) {
          throw std::runtime_error("ExtensionElements: illegal attributes for choice '" + (std::string)decision.id.value + "'.\n" + error.what());
        }
        for ( auto input : choices.back()->dependencies ) {
          choiceDependencies.insert(input);
        }
      }
    }    
  }

  // add data attributes, global or not, modified by operator to dataUpdate; operators are applied upon entry
  for ( auto& operator_ : operators ) {
    if ( operator_->attribute->category != Attribute::Category::STATUS ) {
      dataUpdate.attributes.push_back(operator_->attribute);
    }
  }

  // add data attributes, global or not, modified by choice to dataUpdate
  for ( auto& choice : choices ) {
    if ( choice->attribute->category != Attribute::Category::STATUS ) {
      dataUpdate.attributes.push_back(choice->attribute);
    }
  }

  // add timer trigger
  if ( auto timer = element->getOptionalChild<XML::bpmnos::tTimer>(); timer.has_value() ) {
    if ( auto& parameter = timer->get().parameter; parameter.name.value.value == "trigger" ) {
      trigger = std::make_unique<Parameter>(&parameter,attributeRegistry);
    }
  }

  // add conditions of a conditional event, restrictions placed directly in the extension elements; those of any other
  // node belong to its status
  auto conditionElements = get<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>();
  bool isConditionalEvent = baseElement->is<XML::bpmn::tCatchEvent>() && !baseElement->getChildren<XML::bpmn::tConditionalEventDefinition>().empty();
  if ( !conditionElements.empty() && !isConditionalEvent ) {
    throw std::runtime_error("ExtensionElements: restrictions placed directly in the extension elements of '" + (std::string)baseElement->id.value().get().value + "' are conditions, which only a conditional event states; restrictions of the node belong to its status");
  }
  for ( XML::bpmnos::tRestriction& condition : conditionElements ) {
    try {
      conditions.push_back(std::make_unique<Restriction>(&condition,attributeRegistry));
    }
    catch ( const std::exception& error ) {
      throw std::runtime_error("ExtensionElements: illegal parameters for condition '" + (std::string)condition.id.value + "'.\n" + error.what());
    }
    for ( auto input : conditions.back()->expression.inputs ) {
      if ( input->category == Attribute::Category::STATUS && !input->isObject() && input->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
        throw std::runtime_error("ExtensionElements: condition '" + (std::string)condition.id.value + "' is time dependent");
      }
      conditionDependencies.insert(input);
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

const MessageDefinition* ExtensionElements::getMessageDefinition() const {
  if ( !messageDefinition ) {
    throw std::runtime_error("ExtensionElements: no message definition provided for '" +  baseElement->id + "'" );
  }
  return messageDefinition.get();
}

template <typename DataType>
bool ExtensionElements::conditionsSatisfied(const BPMNOS::Status& status, const DataType& data) const {
  for ( auto& condition : conditions ) {
    if ( !condition->isSatisfied(status,data) ) {
      return false;
    }
  }
  return true;
}

template bool ExtensionElements::conditionsSatisfied<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template bool ExtensionElements::conditionsSatisfied<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;

template <typename DataType>
bool ExtensionElements::feasibleEntry(const BPMNOS::Status& status, const DataType& data) const {
  for ( auto& restriction : restrictions ) {
    if (
      ( restriction->scope == Restriction::Scope::ENTRY || restriction->scope == Restriction::Scope::FULL )
      && !restriction->isSatisfied(status,data)
    ) {
      return false;
    }
  }
  return satisfiesInheritedRestrictions(status,data);
}

template bool ExtensionElements::feasibleEntry<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template bool ExtensionElements::feasibleEntry<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;

template <typename DataType>
bool ExtensionElements::feasibleCompletion(const BPMNOS::Status& status, const DataType& data) const {
  for ( auto& restriction : restrictions ) {
    if (
      ( restriction->scope == Restriction::Scope::COMPLETION || restriction->scope == Restriction::Scope::FULL )
      && !restriction->isSatisfied(status,data)
    ) {
      return false;
    }
  }
  return satisfiesInheritedRestrictions(status,data);
}

template bool ExtensionElements::feasibleCompletion<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template bool ExtensionElements::feasibleCompletion<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;

template <typename DataType>
bool ExtensionElements::feasibleExit(const BPMNOS::Status& status, const DataType& data) const {
  for ( auto& restriction : restrictions ) {
    if (
      ( restriction->scope == Restriction::Scope::EXIT || restriction->scope == Restriction::Scope::FULL )
      && !restriction->isSatisfied(status,data)
    ) {
      return false;
    }
  }
  
  return satisfiesInheritedRestrictions(status,data);
}

template bool ExtensionElements::feasibleExit<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template bool ExtensionElements::feasibleExit<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;


template <typename DataType>
bool ExtensionElements::satisfiesInheritedRestrictions(const BPMNOS::Status& status, const DataType& data) const {
  for ( auto restriction : inheritedRestrictions ) {
    if ( !restriction->isSatisfied(status,data) ) {
      return false;
    }
  }
  return true;
}

template bool ExtensionElements::satisfiesInheritedRestrictions<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template bool ExtensionElements::satisfiesInheritedRestrictions<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;

template <typename DataType>
bool ExtensionElements::fullScopeRestrictionsSatisfied(const BPMNOS::Status& status, const DataType& data) const {
  for ( auto& restriction : restrictions ) {
    if ( restriction->scope == Restriction::Scope::FULL && !restriction->isSatisfied(status,data) ) {
      return false;
    }
  }
  return satisfiesInheritedRestrictions(status,data);
}

template bool ExtensionElements::fullScopeRestrictionsSatisfied<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template bool ExtensionElements::fullScopeRestrictionsSatisfied<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;

template <typename DataType>
BPMNOS::number ExtensionElements::applyOperators(BPMNOS::Status& status, DataType& data, const SchemaProvider& schemaProvider, std::shared_ptr<const BPMNOS::Object>* contentObject) const {
  // the operators of a node stating a message or signal may refer to its content object, which is held in the slot
  // following the status objects while they run
  if ( content ) {
    assert( contentObject && *contentObject );
    assert( status.objects.size() == content->index );
    status.objects.push_back( std::move(*contentObject) );
  }
  BPMNOS::number change = 0;
  for ( auto& operator_ : operators ) {
    change += operator_->apply(status,data,schemaProvider);
  }
  if ( content ) {
    *contentObject = std::move(status.objects.back());
    status.objects.pop_back();
  }
  return change;
}

template BPMNOS::number ExtensionElements::applyOperators<BPMNOS::Data>(Status& status, BPMNOS::Data& data, const SchemaProvider& schemaProvider, std::shared_ptr<const BPMNOS::Object>* contentObject) const;
template BPMNOS::number ExtensionElements::applyOperators<BPMNOS::SharedData>(Status& status, BPMNOS::SharedData& data, const SchemaProvider& schemaProvider, std::shared_ptr<const BPMNOS::Object>* contentObject) const;

template <typename DataType>
BPMNOS::number ExtensionElements::getObjective(const BPMNOS::Status& status, const DataType& data) const {
  BPMNOS::number objective = 0;
  for ( auto attribute : attributeRegistry.statusAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data);
    if ( value.has_value() ) {
//std::cerr << attribute->name << " contributes " <<  attribute->weight * value.value() << std::endl;
      objective += attribute->weight * value.value();
    }
  }
  for ( auto attribute : attributeRegistry.dataAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data);
    if ( value.has_value() ) {
//std::cerr << attribute->name << " contributes " <<  attribute->weight * value.value() << std::endl;
      objective += attribute->weight * value.value();
    }
  }
  return objective;
}

template BPMNOS::number ExtensionElements::getObjective<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template BPMNOS::number ExtensionElements::getObjective<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;


template <typename DataType>
std::vector<std::pair<const Attribute*, BPMNOS::number>> ExtensionElements::getContributionsToObjective(const BPMNOS::Status& status, const DataType& data) const {
  std::vector<std::pair<const Attribute*, BPMNOS::number>> contributions;
  // at most one contribution per attribute
  contributions.reserve(attributes.size() + this->data.size());
  for ( auto& attribute : attributes ) {
    auto value = attributeRegistry.getValue(attribute.get(),status,data);
    if ( value.has_value() ) {
      contributions.emplace_back(attribute.get(),value.value());
    }
  }
  for ( auto& attribute : this->data ) {
    auto value = attributeRegistry.getValue(attribute.get(),status,data);
    if ( value.has_value() ) {
      contributions.emplace_back(attribute.get(),value.value());
    }
  }
  return contributions;
}

template std::vector<std::pair<const Attribute*, BPMNOS::number>> ExtensionElements::getContributionsToObjective<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template std::vector<std::pair<const Attribute*, BPMNOS::number>> ExtensionElements::getContributionsToObjective<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;
