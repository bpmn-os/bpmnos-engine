#include "AttributeRegistry.h"

using namespace BPMNOS::Model;

void AttributeRegistry::emplace(Attribute* attribute) {
  if ( attribute->category == Attribute::Category::STATUS ) {
//    attribute->index = statusAttributes.size(); // TODO: check
    statusAttributes[attribute->name] = attribute;
  }
  else {
//    attribute->index = dataAttributes.size(); // TODO: check
    dataAttributes[attribute->name] = attribute;
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
  else {
    throw std::runtime_error("AttributeRegistry: cannot find attribute with name '" + name + "'");
  }
  return nullptr;
}

bool AttributeRegistry::contains(const std::string& name) const {
  return statusAttributes.contains(name) || dataAttributes.contains(name);
}

std::optional<BPMNOS::number> AttributeRegistry::getValue(const Attribute* attribute, const Values& status, const Values& data) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    return status[attribute->index];
  }
  else {
    return data[attribute->index];
  }
}

std::optional<BPMNOS::number> AttributeRegistry::getValue(const Attribute* attribute, const Values& status, const Globals& data) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    return status[attribute->index];
  }
  else {
    return data[attribute->index].get();
  }
}

void AttributeRegistry::setValue(const Attribute* attribute, Values& status, Values& data, std::optional<BPMNOS::number> value) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    status[attribute->index] = value;
  }
  else {
    data[attribute->index] = value;
  }
}

void AttributeRegistry::setValue(const Attribute* attribute, Values& status, Globals& data, std::optional<BPMNOS::number> value) const {
  if ( attribute->category == Attribute::Category::STATUS ) {
    status[attribute->index] = value;
  }
  else {
    data[attribute->index].get() = value;
  }
}


