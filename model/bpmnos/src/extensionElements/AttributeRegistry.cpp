#include "AttributeRegistry.h"

using namespace BPMNOS::Model;

void AttributeRegistry::add(Attribute* attribute) {
  if ( contains(attribute->name) ) {
    throw std::runtime_error("AttributeRegistry: duplicate attribute name '" + attribute->name + "'");
  }
  if ( attribute->category == Attribute::Category::STATUS ) {
    attribute->index = statusAttributes.size();
    statusAttributes[attribute->name] = attribute;
  }
  else if ( attribute->category == Attribute::Category::DATA ) {
    attribute->index = dataAttributes.size(); 
    dataAttributes[attribute->name] = attribute;
  }
  else /* if ( attribute->category == Attribute::Category::GLOBAL )*/ {
    attribute->index = globalAttributes.size(); 
    globalAttributes[attribute->name] = attribute;
  }
}

Attribute* AttributeRegistry::operator[](const std::string& name) const {
  if ( auto it = statusAttributes.find(name);
    it != statusAttributes.end()
  ) {
    return it->second;
  }
  else if ( auto it = dataAttributes.find(name);
    it != dataAttributes.end()
  ) {
    return it->second;
  }
  else if ( auto it = globalAttributes.find(name);
    it != globalAttributes.end()
  ) {
    return it->second;
  }
  else {
    throw std::runtime_error("AttributeRegistry: cannot find attribute with name '" + name + "'");
  }
  return nullptr;
}

bool AttributeRegistry::contains(const std::string& name) const {
  return statusAttributes.contains(name) || dataAttributes.contains(name) || globalAttributes.contains(name);
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
