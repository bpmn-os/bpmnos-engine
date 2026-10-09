#include "AttributeRegistry.h"
#include "ExtensionElements.h"

using namespace BPMNOS::Model;

AttributeRegistry::AttributeRegistry(const LIMEX::Handle<double>& limexHandle)
  : limexHandle(limexHandle)
{
}

void AttributeRegistry::add(Attribute* attribute) {
  if ( contains(attribute->name) ) {
    throw std::runtime_error("AttributeRegistry: duplicate attribute name '" + attribute->name + "'");
  }
  if ( attribute->category == Attribute::Category::STATUS ) {
    attribute->index = statusAttributes.size();
    statusAttributes.push_back(attribute);
    statusMap[attribute->name] = attribute;
  }
  else /* if ( attribute->category == Attribute::Category::DATA )*/ {
    attribute->index = dataAttributes.size(); 
    dataAttributes.push_back(attribute);
    dataMap[attribute->name] = attribute;
  }
}

Attribute* AttributeRegistry::operator[](const std::string& name) const {
  std::unordered_map< std::string, Attribute*>::const_iterator it;
  if ( it = statusMap.find(name);
    it != statusMap.end()
  ) {
    return it->second;
  }
  else if ( it = dataMap.find(name);
    it != dataMap.end()
  ) {
    return it->second;
  }
  else {
    throw std::runtime_error("AttributeRegistry: cannot find attribute with name '" + name + "'");
  }
  return nullptr;
}

bool AttributeRegistry::contains(const std::string& name) const {
  return statusMap.contains(name) || dataMap.contains(name);
}

bool AttributeRegistry::contains(const Attribute* attribute) const {
  if (attribute->category == Attribute::Category::STATUS) {
    return attribute->index < statusAttributes.size() &&
           statusAttributes[attribute->index] == attribute;
  }
  else /* if (attribute->category == Attribute::Category::DATA) */ {
    return attribute->index < dataAttributes.size() &&
           dataAttributes[attribute->index] == attribute;
  }
}


std::optional<BPMNOS::number> AttributeRegistry::getValue(const Attribute* attribute, const Values& status, const Values& data) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.size());
    return status[attribute->index];
  }
  else /* if ( attribute->category == Attribute::Category::DATA )*/ {
    assert(attribute->index < data.size());
    return data[attribute->index];
  }
}

std::optional<BPMNOS::number> AttributeRegistry::getValue(const Attribute* attribute, const Values& status, const SharedValues& data) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.size());
    return status[attribute->index];
  }
  else /* if ( attribute->category == Attribute::Category::DATA )*/ {
    assert(attribute->index < data.size());
    return data[attribute->index].get();
  }
}

BPMNOS::number AttributeRegistry::setValue(const Attribute* attribute, Values& status, Values& data, std::optional<BPMNOS::number> value) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.size());
    status[attribute->index] = value;
    // a status attribute is accounted in the objective when its scope ends
    return 0;
  }
  else /* if ( attribute->category == Attribute::Category::DATA )*/ {
    assert(attribute->index < data.size());
    auto previous = data[attribute->index];
    data[attribute->index] = value;
    return ( value.value_or(0) - previous.value_or(0) ) * attribute->weight;
  }
}

BPMNOS::number AttributeRegistry::setValue(const Attribute* attribute, Values& status, SharedValues& data, std::optional<BPMNOS::number> value) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.size());
    status[attribute->index] = value;
    // a status attribute is accounted in the objective when its scope ends
    return 0;
  }
  else /* if ( attribute->category == Attribute::Category::DATA )*/ {
    assert(attribute->index < data.size());
    // writing through the reference into the storage of the scope owning the data object
    auto previous = data[attribute->index].get();
    data[attribute->index].get() = value;
    return ( value.value_or(0) - previous.value_or(0) ) * attribute->weight;
  }
}
