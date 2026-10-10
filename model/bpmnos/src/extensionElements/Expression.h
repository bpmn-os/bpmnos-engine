#ifndef BPMNOS_Model_Expression_H
#define BPMNOS_Model_Expression_H

#include <limex.h>
#include <set>

#include "Attribute.h"
#include "AttributeRegistry.h"
#include "model/utility/src/InputEncoder.h"
#include "model/utility/src/Value.h"
#include "model/utility/src/StringRegistry.h"
#include "model/utility/src/Object.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing a mathematical expression.
 *
 * The expression is compiled by LIMEX. Its variables are scalar attributes, passed to LIMEX by value. An array
 * named by an attribute, on the right of `in`, as the argument of an aggregator or in a binding of an
 * aggregation, is a collection, passed as a view of the array the attribute holds: an object, or the constant
 * array a collection attribute holds as its index in the object registry. An access path such as
 * `facilities[i].cost`, `grid[i][j]` or `x[i]` addresses a part of such an array or object; LIMEX evaluates its
 * indices, counting from one, and the expression resolves the path against the layout of the object it
 * addresses, without copying any value.
 *
 * Every name and path is bound to its attribute when the expression is compiled and checked against the
 * attribute's schema: every index takes the next dimension, a field may only follow once every dimension is
 * indexed, a path used as a value must address a scalar value, and an array used as such must be an array of
 * scalar values with a single dimension. The argument of `size` may be any array, its length being all that is
 * read. An undefined element read as a value makes the expression undefined, except within an aggregation,
 * whose aggregator skips it.
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
  std::vector<const Attribute*> paths; ///< Vector containing the attribute addressed by each path, in the order of the paths.
  /**
   * @brief Evaluates the compiled expression for the values of its variables, the arrays its collections name
   * and the arrays or objects its paths address, returning nullopt if it reads an undefined value.
   */
  std::optional<double> evaluate(const std::vector<double>& variableValues, const std::vector<const BPMNOS::Object*>& collectionObjects, const std::vector<const BPMNOS::Object*>& pathObjects) const;
  const Attribute* isAttribute() const; ///< Returns pointer to the attribute if and only if expression contains nothing else
  template <typename DataType>
  std::optional<double> execute(const BPMNOS::Status& status, const DataType& data) const;
private:
  LIMEX::Expression<double> getExpression(const std::string& input) const;
  Type getType() const;
  /// How a path is used: as a value or as an array, as the argument of `size`, and within an aggregation.
  struct Use {
    bool array = false;
    bool sizeOnly = false;
    bool aggregated = false;
  };
  std::vector<Use> pathUses; ///< The use of each path
  std::vector<bool> sizeOnly; ///< True for a collection that is only the argument of `size`
  void determineUses();
  void bind(size_t path, const Attribute* attribute) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Expression_H
