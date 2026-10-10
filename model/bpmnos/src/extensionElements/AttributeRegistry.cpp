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


std::optional<BPMNOS::number> AttributeRegistry::getValue(const Attribute* attribute, const Status& status, const Data& data) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.attributes.size());
    return status.attributes[attribute->index];
  }
  else /* if ( attribute->category == Attribute::Category::DATA )*/ {
    assert(attribute->index < data.attributes.size());
    return data.attributes[attribute->index];
  }
}

std::optional<BPMNOS::number> AttributeRegistry::getValue(const Attribute* attribute, const Status& status, const SharedData& data) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.attributes.size());
    return status.attributes[attribute->index];
  }
  else /* if ( attribute->category == Attribute::Category::DATA )*/ {
    assert(attribute->index < data.attributes.size());
    return data.attributes[attribute->index].get();
  }
}

BPMNOS::number AttributeRegistry::setValue(const Attribute* attribute, Status& status, Data& data, std::optional<BPMNOS::number> value) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.attributes.size());
    status.attributes[attribute->index] = value;
    // a status attribute is accounted in the objective when its scope ends
    return 0;
  }
  else /* if ( attribute->category == Attribute::Category::DATA )*/ {
    assert(attribute->index < data.attributes.size());
    auto previous = data.attributes[attribute->index];
    data.attributes[attribute->index] = value;
    return ( value.value_or(0) - previous.value_or(0) ) * attribute->weight;
  }
}

BPMNOS::number AttributeRegistry::setValue(const Attribute* attribute, Status& status, SharedData& data, std::optional<BPMNOS::number> value) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.attributes.size());
    status.attributes[attribute->index] = value;
    // a status attribute is accounted in the objective when its scope ends
    return 0;
  }
  else /* if ( attribute->category == Attribute::Category::DATA )*/ {
    assert(attribute->index < data.attributes.size());
    // writing through the reference into the storage of the scope owning the data object
    auto previous = data.attributes[attribute->index].get();
    data.attributes[attribute->index].get() = value;
    return ( value.value_or(0) - previous.value_or(0) ) * attribute->weight;
  }
}
