#include "GenericExpression.h"

using namespace BPMNOS::Model;

GenericExpression::GenericExpression(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry)
  : Expression(parameter, attributeRegistry)
{
  if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
    throw std::runtime_error("GenericExpression: expression must be given by value");
  }

  exprtk::symbol_table<NumericType> symbolTable;
  compiledExpression.register_symbol_table(symbolTable);

  exprtk::parser<NumericType> parser;
  parser.enable_unknown_symbol_resolver();

  if (auto result = parser.compile(expression, compiledExpression); !result) {
    throw std::runtime_error("GenericExpression: compilation of expression failed with: " + parser.error());
  }

  // get variable names used in expression
  std::list<std::string> variables;
  symbolTable.get_variable_list(variables);
  // Create bindings of expression variables to attributes. 
  for ( auto variable : variables ) {
    Attribute* boundAttribute; 
    try {
      boundAttribute = attributeRegistry[variable];
    }
    catch (...) {
      throw std::runtime_error("GenericExpression: unknown expression variable");
    }

    if ( boundAttribute->type == ValueType::STRING || boundAttribute->type == ValueType::COLLECTION ) {
      throw std::runtime_error("GenericExpression: non-numeric variable '" + boundAttribute->name + "'");
    }
    bindings.push_back({ symbolTable.variable_ref(variable), boundAttribute });
    inputs.insert(boundAttribute);
  }
}

template <typename DataType>
std::optional<BPMNOS::number> GenericExpression::_execute(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  for ( auto& [variable,boundAttribute] : bindings ) {
    auto value = attributeRegistry.getValue(boundAttribute,status,data,globals);
    if ( !value.has_value() ) {
      // return nullopt because required attribute value is not given
      return std::nullopt;
    }
    variable = (NumericType)value.value();
  }

  return number(compiledExpression.value());
}

template std::optional<BPMNOS::number> GenericExpression::_execute<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template std::optional<BPMNOS::number> GenericExpression::_execute<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;


