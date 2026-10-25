#ifndef BPMNOS_Execution_InstanceDataReader_H
#define BPMNOS_Execution_InstanceDataReader_H

#include <string>
#include <vector>
#include <unordered_map>
#include <bpmn++.h>
#include <limex.h>
#include "model/bpmnos/src/Model.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/utility/src/Schema.h"
#include "model/utility/src/InputEncoder.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/Value.h"

namespace BPMNOS::Execution {

/**
 * @brief Class reading the instance data of a data provider from a CSV table.
 *
 * The data provider gives the names of the columns it reads in their order. The first three columns hold the
 * instance, the node, and the initialization, which has the form `attribute := expression`, and the further
 * columns are interpreted by the data provider alone. The first line of the table names its columns, which
 * must be the first three or more of the columns given by the data provider; a column the table lacks gives
 * no input. A row without instance and node gives the value of a global attribute and has no further cells,
 * and every other row belongs to an instance, the first row of which must name the process of the instance.
 * A row whose initialization has no `:=` is a size declaration instead, `name[6].flags[3]`, naming no instance
 * and naming the node declaring the object, or no node for a global object; it fixes the sizes of the object
 * for every instance. An initialization of an object gives a literal, which arrives as the index of a constant
 * object, or an object initializer, a literal whose members may be expressions, which arrives as it is written.
 *
 * The reader resolves the rows and offers what every data provider needs to interpret them: the lookup of
 * the attribute an initialization gives a value to, the evaluation of global values and of expressions of
 * an instance with a LIMEX handle chosen by the data provider, and the default values of the instance and
 * the timestamp. How the values of an instance are used is left to the data provider.
 */
class InstanceDataReader {
public:
  /**
   * @brief Row of the table, giving the value of a global attribute if it names no node.
   */
  struct Row {
    size_t instanceId; ///< The identifier of the instance, zero for a global value
    const BPMN::Node* node; ///< The node the row refers to, nullptr for a global value
    std::string initialization; ///< The initialization, possibly empty
    std::vector<std::string> cells; ///< The cells of the further columns in the order given by the data provider, empty for a column the table lacks
    bool declaresSizes = false; ///< True if the row is a size declaration
  };

  /**
   * @param model The model the instance data refers to.
   * @param instanceFileOrString The name of the CSV file or its content.
   * @param columns The names of the columns the data provider reads, at least three, in their order.
   */
  InstanceDataReader(const BPMNOS::Model::Model* model, const std::string& instanceFileOrString, const std::vector<std::string>& columns);

  const BPMNOS::Model::Model* const model;
  std::vector<Row> rows; ///< The rows of the table in their order
  std::unordered_map<size_t, const BPMN::Process*> processes; ///< The process of each instance

  /**
   * @brief Method splitting an initialization of the form `attribute := expression`.
   */
  static std::pair<std::string, std::string> splitInitialization(const std::string& initialization);

  /**
   * @brief Method returning the attribute of the node an initialization gives a value to, and the expression.
   *
   * It rejects a value for an event subprocess, which the engine creates itself, for an attribute of a guidance,
   * and for an attribute the model assigns a value to.
   */
  std::pair<const BPMNOS::Model::Attribute*, std::string> lookupAttribute(const BPMN::Node* node, const std::string& initialization) const;
  /**
   * @brief Method returning true if the initialization of the row gives a value to an object.
   */
  bool assignsObject(const Row& row) const;
  /**
   * @brief Method returning the object an initialization gives a value to, an object of the node or a global object
   * if the row names no node, and the initializer stating the value.
   *
   * It rejects a value for an object the model assigns a value to.
   */
  std::pair<const BPMNOS::Model::Attribute*, BPMNOS::ObjectInitializer> lookupObject(const Row& row) const;
  /**
   * @brief Method evaluating the initializer the row gives an object of an instance or a global object and
   * recording the object, to which later expressions may refer.
   *
   * An expression member may refer to global attributes and objects, and to attributes and objects of the instance,
   * whose values have been recorded before; it does not refer to another member of the same initializer.
   */
  void setObject(const Row& row, const LIMEX::Handle<double>& handle);
  /**
   * @brief Method returning the object an initializer states with the given values of its expression members,
   * kept or converted for the given schema of the object.
   *
   * Each value is converted to the type the schema gives at the position of its member.
   */
  static std::shared_ptr<const BPMNOS::Object> instantiate(const BPMNOS::Schema& schema, const BPMNOS::Model::Attribute* object, const BPMNOS::ObjectInitializer& initializer, const std::vector<double>& values);
  /**
   * @brief Method recording a size declaration.
   *
   * A declaration naming a node fixes the sizes of an object the node declares or of the content of a message,
   * error or escalation the node throws; one naming no node fixes the sizes of a global object or of the content
   * of a signal, which is the same wherever its name is thrown or raised and is therefore declared by its name.
   */
  void declareSizes(const Row& row);
  /**
   * @brief Method returning the schema of an object with the sizes the size declarations fix.
   */
  BPMNOS::Schema getSchema(const BPMNOS::Model::Attribute* object) const;
  /**
   * @brief Method returning the schema of the content of the signal with the given name, of the given type, with
   * the sizes the size declarations fix.
   */
  BPMNOS::Schema getSignalSchema(BPMNOS::number name, const BPMNOS::Schema& type) const;
  /**
   * @brief Method returning the object an object attribute holds for an expression of the instance data, given
   * by an earlier row or by the model, nullptr if it has no value yet.
   *
   * No instance is given for a global object.
   */
  std::shared_ptr<const BPMNOS::Object> knownObject(std::optional<size_t> instanceId, const BPMNOS::Model::Attribute* object) const;

  /**
   * @brief Method evaluating the initialization of a global attribute and recording its value.
   *
   * The expression may refer to global attributes whose values have been recorded before.
   */
  void evaluateGlobal(const std::string& initialization, const LIMEX::Handle<double>& handle);

  /**
   * @brief Method evaluating an expression of an instance at a node, converted to the given type.
   *
   * The expression may refer to global attributes and to attributes of the instance whose values have been
   * recorded before.
   */
  BPMNOS::number evaluate(size_t instanceId, const BPMN::Node* node, const std::string& expression, ValueType type, const LIMEX::Handle<double>& handle) const;

  /**
   * @brief Method recording the value of an attribute of an instance, to which later expressions may refer.
   */
  void setValue(size_t instanceId, const BPMNOS::Model::Attribute* attribute, BPMNOS::number value);

  /**
   * @brief Method adding the default values of the instance and the timestamp to the values of an instance.
   *
   * The instance takes its identifier and the timestamp zero, unless the values give them.
   */
  void addDefaultValues(size_t instanceId, std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>& instanceValues) const;

  std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number> globals; ///< The values of the global attributes recorded
  std::unordered_map<size_t, std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>> values; ///< The values of the attributes of each instance recorded
  std::unordered_map<size_t, std::unordered_map<const BPMNOS::Model::Attribute*, std::shared_ptr<const BPMNOS::Object>>> objects; ///< The object recorded for each object of each instance
  std::unordered_map<const BPMNOS::Model::Attribute*, std::shared_ptr<const BPMNOS::Object>> globalObjects; ///< The object recorded for each global object
  std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::Schema> schemas; ///< The schema of each object whose sizes are declared
  std::unordered_map<BPMNOS::number, BPMNOS::Schema> signalSchemas; ///< The schema of the content of each signal whose sizes are declared, by the name of the signal

private:
  const BPMN::Node* findNode(const std::string& nodeId) const;
  /// @brief Method evaluating an expression of the instance data at a node of an instance, or of the global values if
  /// no node is given, rejecting a reference to an attribute or object without a value.
  double execute(std::optional<size_t> instanceId, const BPMN::Node* node, const std::string& expression, const LIMEX::Handle<double>& handle) const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_InstanceDataReader_H
