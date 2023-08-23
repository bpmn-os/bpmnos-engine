#include "GenericExpression.h"

using namespace BPMNOS::Model;

GenericExpression::GenericExpression(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap)
  : Expression(operator_, attributeMap)
{
  if ( attribute->type == ValueType::STRING ) {
    throw std::runtime_error("GenericExpression: non-numeric result of operator '" + id + "'");
  }

  parameter = parameterMap.at("generic").get();

  if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
    throw std::runtime_error("GenericExpression: expression of operator '" + id + "' must be given by value");
  }

  exprtk::symbol_table<NumericType> symbolTable;
  expression.register_symbol_table(symbolTable);

  exprtk::parser<NumericType> parser;
  parser.enable_unknown_symbol_resolver();

  if (auto result = parser.compile(parameter->value->get().value, expression); !result) {
    throw std::runtime_error("GenericExpression: compilation of expression of operator '" + id + "' failed with: " + parser.error());
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
      throw std::runtime_error("GenericExpression: unknown expression variable of operator '" + id + "'");
    }

    if ( boundAttribute->type == ValueType::STRING ) {
      throw std::runtime_error("GenericExpression: non-numeric variable '" + boundAttribute->name + "' of operator '" + id + "'");
    }
    bindings.push_back({ symbolTable.variable_ref(variable), boundAttribute });
  }
}

void GenericExpression::apply(Values& status) const {
  for ( auto& [variable,boundAttribute] : bindings ) {
    if ( !status[boundAttribute->index].has_value() ) {
      // set attribute to undefined because required lookup value is not given
      status[attribute->index] = std::nullopt;
      return;
    }
    variable = (NumericType)status[boundAttribute->index].value();
  }

  status[attribute->index] = number(expression.value());
}

