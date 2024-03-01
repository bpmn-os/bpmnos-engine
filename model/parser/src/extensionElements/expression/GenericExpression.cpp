#include "GenericExpression.h"

using namespace BPMNOS::Model;

GenericExpression::GenericExpression(XML::bpmnos::tParameter* parameter, const AttributeMap& attributeMap)
  : Expression(parameter, attributeMap)
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
      boundAttribute = attributeMap.at(variable);
    }
    catch (...) {
      throw std::runtime_error("GenericExpression: unknown expression variable");
    }

    if ( boundAttribute->type == ValueType::STRING ) {
      throw std::runtime_error("GenericExpression: non-numeric variable '" + boundAttribute->name + "'");
    }
    bindings.push_back({ symbolTable.variable_ref(variable), boundAttribute });
  }
}

std::optional<BPMNOS::number> GenericExpression::execute(const Values& values) const {
  for ( auto& [variable,boundAttribute] : bindings ) {
    if ( !values[boundAttribute->index].has_value() ) {
      // return nullopt because required attribute value is not given
      return std::nullopt;
    }
    variable = (NumericType)values[boundAttribute->index].value();
  }

  return number(compiledExpression.value());
}

