#include "Guidance.h"
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"
#include "model/bpmnos/src/xml/bpmnos/tRestrictions.h"
#include "model/bpmnos/src/xml/bpmnos/tRestriction.h"
#include "model/bpmnos/src/xml/bpmnos/tOperators.h"
#include "model/bpmnos/src/xml/bpmnos/tOperator.h"
#include "model/bpmnos/src/xml/bpmnos/tDeferral.h"
#include "model/utility/src/Keywords.h"
//#include <iostream>

using namespace BPMNOS::Model;

Guidance::Guidance(XML::bpmnos::tGuidance* guidance, const AttributeRegistry& attributeRegistry)
  : element(guidance)
  , attributeRegistry(attributeRegistry)
{
  if ( guidance->type.value.value == "message" ) {
    type = Type::MessageDelivery;
  }
  else if ( guidance->type.value.value == "entry" ) {
    type = Type::Entry;
  }
  else if ( guidance->type.value.value == "choice" ) {
    type = Type::Choice;
  }
  else {
    throw std::runtime_error("Guidance: illegal type '" + guidance->type.value.value + "'");
  }

  // add all attributes
  if ( guidance->attributes.has_value() ) {
    for ( XML::bpmnos::tAttribute& attributeElement : guidance->attributes.value().get().attribute ) {
      try {
        auto attribute = std::make_unique<Attribute>(&attributeElement, Attribute::Category::STATUS, this->attributeRegistry);
        if ( auto expression = std::get_if< std::unique_ptr<const Expression> >(&attribute->initialization) ) {
          for ( auto input : (*expression)->inputs ) {
            dependencies.insert(input);
          }
        }
        attributes.push_back(std::move(attribute));
      }
      catch ( const std::exception &error ){
        throw std::runtime_error("Guidance: illegal expression for attribute '" + (std::string)attributeElement.id.value + "'\n" + error.what() );
      }
    }
  }    
  // add all restrictions
  if ( guidance->restrictions.has_value() ) {
    for ( XML::bpmnos::tRestriction& restriction : guidance->restrictions.value().get().restriction ) {
      try {
        restrictions.push_back(std::make_unique<Restriction>(&restriction,this->attributeRegistry));
        for ( auto input : restrictions.back()->expression.inputs ) {
          dependencies.insert(input);
        }
      }
      catch ( const std::exception &error ){
        throw std::runtime_error("Guidance: illegal expression for restriction '" + (std::string)restriction.id.value + "'\n" + error.what() );
      }
    }
  }    
  // add all operators
  if ( guidance->operators.has_value() ) {
    for ( XML::bpmnos::tOperator& operator_ : guidance->operators.value().get().operator_ ) {
      try {
        operators.push_back(std::make_unique<Operator>(&operator_,this->attributeRegistry));
        if ( operators.back()->attribute->isImmutable ) {
          throw std::runtime_error("Operator: attribute '" + operators.back()->attribute->id + "' is immutable and must not be assigned");
        }
        for ( auto input : operators.back()->expression.inputs ) {
          dependencies.insert(input);
        }
      }
      catch ( const std::exception &error ){
        throw std::runtime_error("Guidance: illegal expression for operator '" + (std::string)operator_.id.value + "'" + "'\n" + error.what() );
      }
    }
  }    
  // add the deferral, which is read in the scope of the node and not of the attributes of the guidance, since it is
  // evaluated on the token of the decision point, to which the guidance has not been applied
  auto deferrals = guidance->getChildren<XML::bpmnos::tDeferral>();
  if ( deferrals.size() > 1 ) {
    throw std::runtime_error("Guidance: more than one deferral");
  }
  if ( deferrals.size() == 1 ) {
    if ( type == Type::Entry ) {
      throw std::runtime_error("Guidance: the deferral of entries is stated at the performer, not in an entry guidance");
    }
    try {
      deferral = std::make_unique<Deferral>(&deferrals.front().get(), attributeRegistry);
    }
    catch ( const std::exception &error ){
      throw std::runtime_error("Guidance: illegal deferral\n" + std::string(error.what()) );
    }
  }
}

template <typename DataType>
BPMNOS::number Guidance::getObjective(const BPMNOS::Status& status, const DataType& data) const {
  BPMNOS::number objective = 0;
  for ( auto attribute : attributeRegistry.statusAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data);
    if ( value.has_value() && attribute->weight != 0 ) {
//std::cerr << attribute->name << " contributes " <<  attribute->weight * value.value() << std::endl;
      objective += attribute->weight * value.value();
    }
  }
  for ( auto attribute : attributeRegistry.dataAttributes ) {
    auto value = attributeRegistry.getValue(attribute,status,data);
    if ( value.has_value() && attribute->weight != 0 ) {
//std::cerr << attribute->name << " contributes " <<  attribute->weight * value.value() << std::endl;
      objective += attribute->weight * value.value();
    }
  }
  return objective;
}

template BPMNOS::number Guidance::getObjective<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
//template BPMNOS::number Guidance::getObjective<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;

template <typename DataType>
bool Guidance::restrictionsSatisfied(const BPMNOS::Status& status, const DataType& data) const {
  for ( auto& restriction : restrictions ) {
    if ( !restriction->isSatisfied(status,data) ) {
      return false;
    }
  }
  
  return true;
}

template bool Guidance::restrictionsSatisfied<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
//template bool Guidance::restrictionsSatisfied<BPMNOS::SharedData>(const BPMN::FlowNode* node, const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;


template <typename DataType>
void Guidance::apply(BPMNOS::Status& status, DataType& data, const SchemaProvider& schemaProvider) const {
  // the status grows by the attributes of the guidance, so that it is reallocated once
  status.attributes.reserve(status.attributes.size() + attributes.size());

  for ( auto& attribute : attributes ) {
    status.attributes.push_back( std::nullopt );
    if ( auto expression = std::get_if< std::unique_ptr<const Expression> >(&attribute->initialization) ) {
      // compute initial value
      status.attributes.back() = BPMNOS::to_value( (*expression)->execute(status,data) );
    }
  }
  
  // apply operators
  for ( auto& operator_ : operators ) {
    operator_->apply(status,data,schemaProvider);
  }
}

template void Guidance::apply<BPMNOS::Data>(BPMNOS::Status& status, BPMNOS::Data& data, const SchemaProvider& schemaProvider) const;
//template void Guidance::apply<BPMNOS::SharedData>(BPMNOS::Status& status, BPMNOS::SharedData& data, const SchemaProvider& schemaProvider) const;

