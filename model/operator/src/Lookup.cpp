#include "Lookup.h"
#include "model/parser/src/Operator.h"

using namespace BPMNOS;

Lookup::Lookup(Operator* base, Attribute* attribute)
  : base(base)
  , attribute(attribute)
{
  try {
    base->parameterMap.at("source").get();
  }
  catch ( ... ){
    throw std::runtime_error("Lookup: required parameter 'source' not provided for operator '" + base->id + "'");
  }

  for ( auto &[parameterName,parameter] : base->parameterMap ) {
    if ( parameterName == "source" ) {
      if ( !parameter->value.has_value() || parameter->value.value().get().value != "file" ) {
        throw std::runtime_error("Lookup: unknown source of operator '" + base->id + "'");
      }
    }
    else if ( parameterName == "filename" ) {
      if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
        throw std::runtime_error("Lookup: filename of operator '" + base->id + "' must be given by value");
      }
      filename = parameter->value.value().get().value;
    }
    else if ( parameterName == "key" ) {
      if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
        throw std::runtime_error("Lookup: key of operator '" + base->id + "' must be given by value");
      }
      key = parameter->value.value().get().value;
    }
    else {
      if ( !parameter->attribute.has_value() || parameter->value.has_value() ) {
        throw std::runtime_error("Lookup: lookup argument of operator '" + base->id + "' must be given by attribute name");
      }
      lookups.push_back({ parameterName, &parameter->attribute->get() });
    }
  }

  Parameter* parameter = nullptr;


  if ( !parameter->value.has_value() || parameter->value.value().get().value == "file" ) {
    try {
      parameter = base->parameterMap.at("filename").get();
    }
    catch ( ... ){
      throw std::runtime_error("LookupOperator: required parameter 'filename' not provided for operator '" + base->id + "'");
    }
  }
  else {
    throw std::runtime_error("LookupOperator: unknown source of operator '" + base->id + "'");
  }


  if ( parameter->value.has_value() ) {
    filename = parameter->value.value().get().value;
  }
  else {
    throw std::runtime_error("LookupOperator: filename missing for operator '" + base->id + "'");
  }

  // make sure lookup table is read and available;
  table = getLookupTable(filename);
}


