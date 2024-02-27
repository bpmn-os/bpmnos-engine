#include "Lookup.h"
#include "model/parser/src/extensionElements/Operator.h"

using namespace BPMNOS::Model;

Lookup::Lookup(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap)
  : Operator(operator_, attributeMap)
{
  try {
    parameterMap.at("source").get();
  }
  catch ( ... ){
    throw std::runtime_error("Lookup: required parameter 'source' not provided for operator '" + id + "'");
  }

  for ( auto &[parameterName,parameter] : parameterMap ) {
    if ( parameterName == "source" ) {
      if ( !parameter->value.has_value() || parameter->value.value().get().value != "file" ) {
        throw std::runtime_error("Lookup: unknown source of operator '" + id + "'");
      }
    }
    else if ( parameterName == "filename" ) {
      if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
        throw std::runtime_error("Lookup: filename of operator '" + id + "' must be given by value");
      }
      filename = parameter->value.value().get().value;
    }
    else if ( parameterName == "key" ) {
      if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
        throw std::runtime_error("Lookup: key of operator '" + id + "' must be given by value");
      }
      key = parameter->value.value().get().value;
    }
    else {
      if ( !parameter->attribute.has_value() || parameter->value.has_value() ) {
        throw std::runtime_error("Lookup: lookup argument of operator '" + id + "' must be given by attribute name");
      }
      lookups.push_back({ parameterName, &parameter->attribute->get() });
    }
  }

  auto& filenameParameter = parameterMap["filename"];

  if ( filenameParameter->value.has_value() ) {
    filename = filenameParameter->value.value().get().value;
  }
  else {
    throw std::runtime_error("LookupOperator: filename missing for operator '" + id + "'");
  }

  // make sure lookup table is read and available;
  table = getLookupTable(filename);
}

void Lookup::apply(Values& status) const {
  std::unordered_map< std::string, Value > arguments;
  for ( auto& [name,lookupAttribute] : lookups) {
    if ( !status[lookupAttribute->index].has_value() ) {
      // set attribute to undefined because required lookup value is not given
      status[attribute->index] = std::nullopt;
      return;
    }

    arguments[name] = to_string(status[lookupAttribute->index].value(),lookupAttribute->type);
  }

  std::optional<std::string> value = table->lookup(key, arguments);
  if ( value.has_value() ) {
    // set value to the value looked up
    status[attribute->index] = to_number( value.value(), attribute->type );
  }
  else {
    // set value to undefined if no attribute with value is given and no explicit value is given
    status[attribute->index] = std::nullopt;
  }
}



