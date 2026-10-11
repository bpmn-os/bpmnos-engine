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
 * The expression is compiled by LIMEX. Its variables are scalar attributes, passed to LIMEX by value. An access
 * path such as `facilities[i].cost`, `grid[i][j]` or `x[i]` addresses a part of the array or object an object
 * attribute holds, and a name used as an array, on the right of `in`, as the argument of an aggregator or in a
 * binding of an aggregation, is a path without steps, addressing the whole array; LIMEX evaluates its
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
  std::set<const Attribute*> inputs; ///< Set containing all attributes read by the expression.
  std::vector<const Attribute*> variables; ///< Vector containing all input attributes used by the expression.
  std::vector<const Attribute*> paths; ///< Vector containing the attribute addressed by each path, in the order of the paths, nullptr for an input.
  std::vector<const Input*> pathInputs; ///< Vector containing the input addressed by each path, in the order of the paths, nullptr for an attribute.
  /**
   * @brief Evaluates the compiled expression for the values of its variables and the arrays or objects its paths
   * address, returning nullopt if it reads an undefined value.
   */
  std::optional<double> evaluate(const std::vector<double>& variableValues, const std::vector<const BPMNOS::Object*>& pathObjects) const;
  const Attribute* isAttribute() const; ///< Returns pointer to the attribute if and only if expression contains nothing else
  template <typename DataType>
  std::optional<double> execute(const BPMNOS::Status& status, const DataType& data) const;
  /// Returns true if the expression assigns an object, a part of one, or the length of one of its dimensions.
  bool writesObject() const { return objectTarget; }
  /**
   * @brief Writes what an assignment to an object states: a value or an array or object to an element, an array
   * or object to the whole object, or the length of a dimension.
   *
   * The object is copied first if anyone else holds it. An element keeps its lengths, except those not yet
   * determined, which the value determines for every element; the whole object keeps the lengths of its fixed
   * dimensions, a shorter value being padded with undefined values and a longer one being an error, and takes
   * the lengths of its open ones.
   */
  template <typename DataType>
  void write(BPMNOS::Status& status, DataType& data) const;
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
  bool objectTarget = false; ///< True if the target is an object
  bool resize = false; ///< True for `resize(path) := n`, which assigns the length of a dimension
  bool compound = false; ///< True for a compound assignment, such as `+=`
  bool objectValue = false; ///< True if an array or object is assigned
  std::optional<size_t> sourceLiteral; ///< The constant object assigned, by its index in the object registry
  const Attribute* sourceObject = nullptr; ///< The object attribute assigned
  const Input* sourceInput = nullptr; ///< The input whose object is assigned
  std::optional<size_t> sourcePath; ///< The path to the part of an object assigned
  const LIMEX::Node<double>* sourceCall = nullptr; ///< The call of a lookup returning the array assigned
  std::vector<std::string> targetFields; ///< The fields the target path names, one within the other
  size_t targetDimension = 0; ///< The number of dimensions the target path indexes after its last field
  std::vector<std::string> resizeFields; ///< The fields naming the array whose dimension is resized
  size_t resizeDimension = 0; ///< The position of the dimension resized
  /// The schema a path reaches, the number of its dimensions indexed, and the fields it names
  struct Step {
    const Schema* schema;
    size_t dimension;
    std::vector<std::string> fields;
  };
  void determineUses();
  /// Returns the schema of the object an attribute or input of the given name holds, nullptr for a scalar attribute.
  const Schema* schemaOf(const std::string& name) const;
  Step walk(size_t path) const;
  void bind(size_t path) const;
  void analyseTarget(const std::vector<size_t>& literals);
  static std::string withoutResize(const std::string& text);
  class PathResolver; ///< Resolves the paths of the expression in the objects they address
  template <typename DataType>
  bool gather(const BPMNOS::Status& status, const DataType& data, std::vector<double>& variableValues, std::vector<const BPMNOS::Object*>& pathObjects, bool undefinedAsNaN) const;
  template <typename DataType>
  std::string arguments(const BPMNOS::Status& status, const DataType& data) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Expression_H
