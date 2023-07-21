#include "Expression.h"
#include "model/parser/src/extensionElements/Operator.h"

using namespace BPMNOS;

Expression::Expression(Operator* base, Attribute* attribute)
  : base(base)
  , attribute(attribute)
{
  if ( attribute->type == Attribute::Type::STRING ) {
    throw std::runtime_error("Expression: non-numeric result of operator '" + base->id + "'");
  }

  try {
    parameter = base->parameterMap.at("expression").get();
  }
  catch ( ... ){
    throw std::runtime_error("Expression: required parameter 'expression' not provided for operator '" + base->id + "'");
  }

  if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
    throw std::runtime_error("Expression: expression of operator '" + base->id + "' must be given by value");
  }

  exprtk::symbol_table<NumericType> symbolTable;
  expression.register_symbol_table(symbolTable);

  exprtk::parser<NumericType> parser;
  parser.enable_unknown_symbol_resolver();

  if (auto result = parser.compile(parameter->value->get().value, expression); !result) {
    throw std::runtime_error("Expression: compilation of expression of operator '" + base->id + "' failed with: " + parser.error());
  }

  // get variable names used in expression
  std::list<std::string> variables;
  symbolTable.get_variable_list(variables);
  // Create bindings of expression variables to attributes. 
  for ( auto variable : variables ) {
    Attribute* boundAttribute; 
    try {
      boundAttribute = base->attributeMap.at(variable);
    }
    catch (...) {
      throw std::runtime_error("Expression: unknown expression variable of operator '" + base->id + "'");
    }

    if ( boundAttribute->type == Attribute::Type::STRING ) {
      throw std::runtime_error("Expression: non-numeric variable '" + boundAttribute->name + "' of operator '" + base->id + "'");
    }
    bindings.push_back({ symbolTable.variable_ref(variable), boundAttribute });
  }

}
