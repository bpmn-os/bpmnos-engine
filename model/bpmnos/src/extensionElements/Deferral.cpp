#include "Deferral.h"
#include "model/bpmnos/src/xml/bpmnos/tParameter.h"
#include "model/utility/src/InputEncoder.h"
#include "model/utility/src/string_utility.h"
#include <algorithm>

using namespace BPMNOS::Model;

Deferral::Deferral(XML::bpmnos::tDeferral* deferral, const AttributeRegistry& attributeRegistry)
  : element(deferral)
  , condition(Expression(InputEncoder(deferral->condition.value.value),attributeRegistry))
  , observables(getObservables(deferral,attributeRegistry))
{
  if ( condition.type == Expression::Type::ASSIGN || condition.type == Expression::Type::UNASSIGN ) {
    throw std::runtime_error("Deferral: illegal condition '" + condition.expression + "'");
  }
}

std::vector<const Attribute*> Deferral::getObservables(XML::bpmnos::tDeferral* deferral, const AttributeRegistry& attributeRegistry) {
  std::vector<const Attribute*> observables;
  for ( XML::bpmnos::tParameter& parameter : deferral->parameter ) {
    if ( parameter.name.value.value != "observable" ) {
      throw std::runtime_error("Deferral: illegal parameter '" + parameter.name.value.value + "'");
    }
    std::string name = parameter.value.has_value() ? BPMNOS::trim_copy(parameter.value->get().value) : std::string();
    if ( name.empty() ) {
      throw std::runtime_error("Deferral: observable without attribute");
    }
    // an observed attribute is a data or global attribute in the scope of the decision point; a status attribute never
    // reaches a data update, and whether data is immutable is known only once the whole model is read
    if ( !attributeRegistry.contains(name) ) {
      throw std::runtime_error("Deferral: observable '" + name + "' is no attribute in scope");
    }
    auto attribute = attributeRegistry[name];
    if ( attribute->category != Attribute::Category::DATA ) {
      throw std::runtime_error("Deferral: observable '" + name + "' is no data or global attribute");
    }
    if ( std::ranges::contains(observables, attribute) ) {
      throw std::runtime_error("Deferral: observable '" + name + "' stated twice");
    }
    observables.push_back(attribute);
  }
  return observables;
}

template <typename DataType>
bool Deferral::conditionHolds(const BPMNOS::Status& status, const DataType& data) const {
  auto value = condition.execute(status,data);
  return value.has_value() && (bool)value.value();
}

template bool Deferral::conditionHolds<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template bool Deferral::conditionHolds<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;
