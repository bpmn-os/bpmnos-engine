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
  // a matrix or an object input is read by its name as an attribute is, whereas a lookup is always called, so that
  // only the former must not share its name with an attribute
  if ( inputs.contains(attribute->name) ) {
    throw std::runtime_error("AttributeRegistry: attribute name '" + attribute->name + "' is the name of an input");
  }
  attribute->declaration = declared++;
  // objects are numbered separately from scalar attributes, so that the index of a scalar attribute never
  // depends on objects
  if ( attribute->category == Attribute::Category::STATUS ) {
    auto& attributes = attribute->isObject() ? statusObjects : statusAttributes;
    attribute->index = attributes.size();
    attributes.push_back(attribute);
    statusMap[attribute->name] = attribute;
  }
  else /* if ( attribute->category == Attribute::Category::DATA )*/ {
    auto& attributes = attribute->isObject() ? dataObjects : dataAttributes;
    attribute->index = attributes.size(); 
    attributes.push_back(attribute);
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
    auto& attributes = attribute->isObject() ? statusObjects : statusAttributes;
    return attribute->index < attributes.size() &&
           attributes[attribute->index] == attribute;
  }
  else /* if (attribute->category == Attribute::Category::DATA) */ {
    auto& attributes = attribute->isObject() ? dataObjects : dataAttributes;
    return attribute->index < attributes.size() &&
           attributes[attribute->index] == attribute;
  }
}


std::optional<BPMNOS::number> AttributeRegistry::getValue(const Attribute* attribute, const Status& status, const Data& data) const {
  // the value of a scalar attribute; an object is held in the objects of the status or data
  assert( !attribute->isObject() );
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
  // the value of a scalar attribute; an object is held in the objects of the status or data
  assert( !attribute->isObject() );
  if ( attribute->category == Attribute::Category::STATUS ) {
    assert(attribute->index < status.attributes.size());
    return status.attributes[attribute->index];
  }
  else /* if ( attribute->category == Attribute::Category::DATA )*/ {
    assert(attribute->index < data.attributes.size());
    return data.attributes[attribute->index].get();
  }
}

const std::shared_ptr<const BPMNOS::Object>& AttributeRegistry::getObject(const Attribute* object, const Status& status, const Data& data) const {
  assert( object->isObject() );
  if ( object->category == Attribute::Category::STATUS ) {
    assert(object->index < status.objects.size());
    return status.objects[object->index];
  }
  assert(object->index < data.objects.size());
  return data.objects[object->index];
}

const std::shared_ptr<const BPMNOS::Object>& AttributeRegistry::getObject(const Attribute* object, const Status& status, const SharedData& data) const {
  assert( object->isObject() );
  if ( object->category == Attribute::Category::STATUS ) {
    assert(object->index < status.objects.size());
    return status.objects[object->index];
  }
  assert(object->index < data.objects.size());
  return data.objects[object->index].get();
}

std::shared_ptr<const BPMNOS::Object>& AttributeRegistry::getObjectSlot(const Attribute* object, Status& status, Data& data) const {
  assert( object->isObject() );
  if ( object->category == Attribute::Category::STATUS ) {
    assert(object->index < status.objects.size());
    return status.objects[object->index];
  }
  assert(object->index < data.objects.size());
  return data.objects[object->index];
}

std::shared_ptr<const BPMNOS::Object>& AttributeRegistry::getObjectSlot(const Attribute* object, Status& status, SharedData& data) const {
  assert( object->isObject() );
  if ( object->category == Attribute::Category::STATUS ) {
    assert(object->index < status.objects.size());
    return status.objects[object->index];
  }
  assert(object->index < data.objects.size());
  return data.objects[object->index].get();
}

namespace {

/// Returns the object a whole assignment of the value gives an object holding the current one.
std::shared_ptr<const BPMNOS::Object> assigned(const BPMNOS::Model::Attribute* object, const std::shared_ptr<const BPMNOS::Object>& current, const std::shared_ptr<const BPMNOS::Object>& value) {
  try {
    return Schema::of(*current->layout, false).conform(value);
  }
  catch ( const std::exception& error ) {
    throw std::runtime_error("AttributeRegistry: illegal value of object '" + object->name + "'.\n" + error.what());
  }
}

} // namespace

void AttributeRegistry::setObject(const Attribute* object, Status& status, Data& data, const std::shared_ptr<const Object>& value) const {
  auto& slot = getObjectSlot(object, status, data);
  slot = assigned(object, slot, value);
}

void AttributeRegistry::setObject(const Attribute* object, Status& status, SharedData& data, const std::shared_ptr<const Object>& value) const {
  auto& slot = getObjectSlot(object, status, data);
  slot = assigned(object, slot, value);
}

BPMNOS::number AttributeRegistry::setValue(const Attribute* attribute, Status& status, Data& data, std::optional<BPMNOS::number> value) const {
  // the value of a scalar attribute; an object is held in the objects of the status or data
  assert( !attribute->isObject() );
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
  // the value of a scalar attribute; an object is held in the objects of the status or data
  assert( !attribute->isObject() );
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
