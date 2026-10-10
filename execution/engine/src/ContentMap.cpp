#include "ContentMap.h"
#include "model/utility/src/InputEncoder.h"
#include "model/utility/src/ObjectRegistry.h"

using namespace BPMNOS::Execution;

template <typename DataType>
BPMNOS::number BPMNOS::Execution::applyContent(const BPMNOS::Model::AttributeRegistry& attributeRegistry, const std::string& key, const BPMNOS::Model::Attribute* attribute, const ContentMap::mapped_type* content, BPMNOS::Status& status, DataType& data) {
  if ( attribute->isObject() ) {
    if ( !content ) {
      // an object the content does not mention keeps its value
      return 0;
    }
    std::shared_ptr<const BPMNOS::Object> object;
    if ( std::holds_alternative< std::shared_ptr<const BPMNOS::Object> >(*content) ) {
      object = std::get< std::shared_ptr<const BPMNOS::Object> >(*content);
    }
    else if ( std::holds_alternative<std::string>(*content) ) {
      // content raised from outside the model states an array or object as a literal
      auto& text = std::get<std::string>(*content);
      InputEncoder encoder(text);
      if ( !encoder.object().has_value() ) {
        throw std::runtime_error("Content: '" + text + "' given for key '" + key + "' is no literal of an array or object");
      }
      object = objectRegistry[encoder.object().value()];
    }
    else {
      throw std::runtime_error("Content: a value is sent for key '" + key + "', which object '" + attribute->name + "' receives");
    }
    attributeRegistry.setObject(attribute, status, data, object);
    // an object carries no objective
    return 0;
  }

  if ( !content ) {
    return attributeRegistry.setValue(attribute, status, data, std::nullopt);
  }
  if ( std::holds_alternative<BPMNOS::Value>(*content) ) {
    return attributeRegistry.setValue(attribute, status, data, std::get<BPMNOS::Value>(*content));
  }
  if ( std::holds_alternative<std::string>(*content) ) {
    return attributeRegistry.setValue(attribute, status, data, BPMNOS::to_number(std::get<std::string>(*content), attribute->type));
  }
  throw std::runtime_error("Content: an array or object is sent for key '" + key + "', which attribute '" + attribute->name + "' receives");
}

template BPMNOS::number BPMNOS::Execution::applyContent<BPMNOS::Data>(const BPMNOS::Model::AttributeRegistry& attributeRegistry, const std::string& key, const BPMNOS::Model::Attribute* attribute, const ContentMap::mapped_type* content, BPMNOS::Status& status, BPMNOS::Data& data);
template BPMNOS::number BPMNOS::Execution::applyContent<BPMNOS::SharedData>(const BPMNOS::Model::AttributeRegistry& attributeRegistry, const std::string& key, const BPMNOS::Model::Attribute* attribute, const ContentMap::mapped_type* content, BPMNOS::Status& status, BPMNOS::SharedData& data);
