#ifndef BPMNOS_Model_Expression_H
#define BPMNOS_Model_Expression_H

#include <limex.h>
#include <set>

#include "Attribute.h"
#include "AttributeRegistry.h"
#include "model/utility/src/InputEncoder.h"
#include "model/utility/src/Value.h"
#include "model/utility/src/StringRegistry.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing a mathematical expression.
 *
 * The expression is compiled by LIMEX. Its variables and collections are attributes, passed to LIMEX by
 * value and as views of the registered collections. An access path such as `x[i]` addresses an element of the
 * collection attribute `x`, counting from one; LIMEX evaluates the index and the expression resolves the path
 * by reading the registered collection. A path addresses an element of a collection by a single index; fields,
 * several indices, paths addressing an array and assignments to a path are rejected.
 **/
class Expression {
public:
  enum class Type { ASSIGN, UNASSIGN, IS_NULL, IS_NOT_NULL, OTHER };
  /// Construct expression using attributeRegistry's limexHandle.
  /// The text must be scanned by the caller, so that the text of an expression is analysed once.
  Expression(const InputEncoder& encoder, const AttributeRegistry& attributeRegistry,
             bool newTarget = false);
  /// Construct expression using a custom LIMEX handle (for stochastic expressions)
  Expression(const LIMEX::Handle<double>& handle, const InputEncoder& encoder,
             const AttributeRegistry& attributeRegistry, bool newTarget = false);
  Expression(const Expression&) = delete;
  Expression(Expression&&) = delete;
  Expression& operator=(const Expression&) = delete;
  Expression& operator=(Expression&&) = delete;
  const AttributeRegistry& attributeRegistry;
  const LIMEX::Handle<double>& handle;
  const std::string expression;
  const LIMEX::Expression<double> compiled;
  const Type type;
  std::optional<const Attribute*> target;
  std::set<const Attribute*> inputs; ///< Vector containing all input attributes and collections used by the expression.
  std::vector<const Attribute*> variables; ///< Vector containing all input attributes used by the expression.
  std::vector<const Attribute*> collections; ///< Vector containing all input collections used by the expression.
  std::vector<const Attribute*> paths; ///< Vector containing the collection indexed by each path, in the order of the paths.
  /// Evaluates the compiled expression for the values of its variables, the views of its collections and the
  /// registered collections its paths index, each given as its index in the collection registry.
  double evaluate(const std::vector<double>& variableValues, const std::vector< LIMEX::View<double> >& collectionValues, const std::vector<BPMNOS::number>& pathCollections) const;
  const Attribute* isAttribute() const; ///< Returns pointer to the attribute if and only if expression contains nothing else
  template <typename DataType>
  std::optional<double> execute(const BPMNOS::Status& status, const DataType& data) const;
private:
  LIMEX::Expression<double> getExpression(const std::string& input) const;
  Type getType() const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Expression_H
