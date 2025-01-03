#include "Expression.h"
#include "model/utility/src/CollectionRegistry.h"
#include "model/utility/src/Keywords.h"

using namespace BPMNOS::Model;

Expression::Expression(std::string expression, const AttributeRegistry& attributeRegistry)
  : attributeRegistry(attributeRegistry)
  , expression(expression)
  , compiled(LIMEX::Expression<double>(expression))
  , type(getType())
{
  if ( auto name = compiled.getTarget(); name.has_value() ) {
    if ( name.value() == BPMNOS::Keyword::Undefined ) {
      throw std::runtime_error("Expression: illegal assignment '" + expression +"'");
    }
    target = attributeRegistry[ name.value() ];
  }
  
  for ( auto& name : compiled.getVariables() ) {
    if ( name != BPMNOS::Keyword::Undefined ) {
      auto attribute = attributeRegistry[ name ];
      inputs.push_back(attribute);
      variables.push_back(attribute);
    }
  }
  for ( auto& name : compiled.getCollections() ) {
    if ( name == BPMNOS::Keyword::Undefined ) {
      throw std::runtime_error("Expression: illegal expression '" + expression +"'");
    }
    auto attribute = attributeRegistry[ name ];
    inputs.push_back(attribute);
    collections.push_back(attribute);
  }
}

Expression::Type Expression::getType() {
  auto& variableNames = compiled.getVariables();
  if ( std::find( variableNames.begin(), variableNames.end(), BPMNOS::Keyword::Undefined ) != variableNames.end() ) {
    // only lhs == undefined, lhs != undefined, and lhs := undefined are allowed
    auto& root = compiled.getRoot(); 
    assert( root.operands.size() == 1 );
    assert( root.type == LIMEX::Type::group );
    auto& node = std::get< LIMEX::Node<double> >(root.operands[0]);
    
    if ( node.type == LIMEX::Type::assign ) {
      assert( node.operands.size() == 1 );
      if ( 
        !std::holds_alternative< LIMEX::Node<double> >(node.operands[0]) || 
        std::get< LIMEX::Node<double> >(node.operands[0]).type != LIMEX::Type::variable
      ) {
        throw std::runtime_error("Expression: illegal assignment '" + expression +"'");
      }
      return Type::UNASSIGN;
    }

    if ( node.type != LIMEX::Type::equal_to && node.type != LIMEX::Type::not_equal_to ) {
      throw std::runtime_error("Expression: illegal expression '" + expression +"'");
    }
    
    assert( node.operands.size() == 2 );
    assert( std::holds_alternative< LIMEX::Node<double> >(node.operands[0]) );
    assert( std::holds_alternative< LIMEX::Node<double> >(node.operands[1]) );
    auto& lhs = std::get< LIMEX::Node<double> >(node.operands[0]);
    auto& rhs = std::get< LIMEX::Node<double> >(node.operands[1]);
    assert( !lhs.operands.empty() );
    assert( !rhs.operands.empty() );
//  std::cout << node.operands.size() << std::endl; // Print the entire AST
    if (
      lhs.type != LIMEX::Type::variable ||
      variableNames.at( std::get< size_t >(lhs.operands[0]) ) == BPMNOS::Keyword::Undefined ||
      rhs.type != LIMEX::Type::variable ||
      variableNames.at( std::get< size_t >(rhs.operands[0]) ) != BPMNOS::Keyword::Undefined
    ) {
      throw std::runtime_error("Expression: illegal comparison '" + expression +"'");
    }

    return node.type == LIMEX::Type::equal_to ? Type::IS_NULL : Type::IS_NOT_NULL;
  }
  return Type::OTHER;
}

template <typename DataType>
std::optional<BPMNOS::number> Expression::execute(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  if ( type == Type::UNASSIGN ) {
    return std::nullopt;
  }
  if ( type == Type::IS_NULL ) {
    assert(variables.size() == 1);
    auto value = attributeRegistry.getValue(variables[0],status,data,globals);
    return number( (double)!value.has_value() );
  }
  if ( type == Type::IS_NOT_NULL ) {
    assert(variables.size() == 1);
    auto value = attributeRegistry.getValue(variables[0],status,data,globals);
    return number( (double)value.has_value() );
  }
  
  // collect variable values
  std::vector< double > variableValues;
  for ( auto attribute : variables ) {
    auto value = attributeRegistry.getValue(attribute,status,data,globals);
    if ( !value.has_value() ) {
      // return nullopt because required attribute value is not given
      return std::nullopt;
    }
    variableValues.push_back( (double)value.value() );
  }
  
  // collect values of all variables in collection
  std::vector< std::vector< double > > collectionValues;
  for ( auto attribute : collections ) {
    collectionValues.push_back( {} );
    auto collection = attributeRegistry.getValue(attribute,status,data,globals);
    if ( !collection.has_value() ) {
      // return nullopt because required collection is not given
      return std::nullopt;
    }
    for ( auto value : collectionRegistry[(size_t)collection.value()].values ) {
      if ( !value.has_value() ) {
        // return nullopt because a value in required collection is missing
        return std::nullopt;
      }
      collectionValues.back().push_back( (double)value.value() );
    }
  }
  
  return number(compiled.evaluate(variableValues,collectionValues));
}

template std::optional<BPMNOS::number> Expression::execute<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template std::optional<BPMNOS::number> Expression::execute<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;

