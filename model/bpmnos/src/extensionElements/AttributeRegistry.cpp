#include "AttributeRegistry.h"

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
  else if ( attribute->category == Attribute::Category::DATA ) {
    attribute->index = dataAttributes.size(); 
    dataAttributes.push_back(attribute);
    dataMap[attribute->name] = attribute;
  }
  else /* if ( attribute->category == Attribute::Category::GLOBAL )*/ {
    attribute->index = globalAttributes.size(); 
    globalAttributes.push_back(attribute);
    globalMap[attribute->name] = attribute;
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
  else if ( it = globalMap.find(name);
    it != globalMap.end()
  ) {
    return it->second;
  }
  else {
    throw std::runtime_error("AttributeRegistry: cannot find attribute with name '" + name + "'");
  }
  return nullptr;
}

bool AttributeRegistry::contains(const std::string& name) const {
  return statusMap.contains(name) || dataMap.contains(name) || globalMap.contains(name);
}


std::optional<BPMNOS::number> AttributeRegistry::getValue(const Attribute* attribute, const Values& status, const Values& data, const Values& globals) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.size());
    return status[attribute->index];
  }
  else if ( attribute->category == Attribute::Category::DATA ) {
    assert(attribute->index < data.size());
    return data[attribute->index];
  }
  else /* if ( attribute->category == Attribute::Category::GLOBAL )*/ {
    assert(attribute->index < globals.size());
    return globals[attribute->index];
  }
}

std::optional<BPMNOS::number> AttributeRegistry::getValue(const Attribute* attribute, const Values& status, const SharedValues& data, const Values& globals) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.size());
    return status[attribute->index];
  }
  else if ( attribute->category == Attribute::Category::DATA ) {
    assert(attribute->index < data.size());
    return data[attribute->index].get();
  }
  else /* if ( attribute->category == Attribute::Category::GLOBAL )*/ {
    assert(attribute->index < globals.size());
    return globals[attribute->index];
  }
}

void AttributeRegistry::setValue(const Attribute* attribute, Values& status, Values& data, Values& globals, std::optional<BPMNOS::number> value) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.size());
    status[attribute->index] = value;
  }
  else if ( attribute->category == Attribute::Category::DATA ) {
    assert(attribute->index < data.size());
    data[attribute->index] = value;
  }
  else /* if ( attribute->category == Attribute::Category::GLOBAL )*/ {
    assert(attribute->index < globals.size());
    globals[attribute->index] = value;
  }
}

void AttributeRegistry::setValue(const Attribute* attribute, Values& status, SharedValues& data, Values& globals, std::optional<BPMNOS::number> value) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.size());
    status[attribute->index] = value;
  }
  else if ( attribute->category == Attribute::Category::DATA ) {
    assert(attribute->index < data.size());
    data[attribute->index].get() = value;
  }
  else /* if ( attribute->category == Attribute::Category::GLOBAL )*/ {
    assert(attribute->index < globals.size());
    globals[attribute->index] = value;
  }
}
