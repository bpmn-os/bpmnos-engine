#ifndef BPMNOS_InstanceData_H
#define BPMNOS_InstanceData_H

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <bpmn++.h>
#include "model/utility/src/Numeric.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/xml/bpmnos/tAttribute.h"
#include "model/parser/src/Status.h"
#include <cnl/all.h>
#include <limits>

namespace BPMNOS {


template <typename T>
class DataProvider;

/**
 * @brief Template class representing instance data associated with a BPMN process.
 *
 * This class stores and manages the instance data for a specific BPMN process.
 * It provides access to instance data in the numeric type provided as template parameter.
 * All strings are represented by an integer and all decimals are rounded to the nearest
 * value available in the given type.
 *
 * @tparam T The numeric type to be used for attribute values.
 */
template <typename T>
class InstanceData {
  friend class DataProvider<T>;
public:
  /**
   * @brief Constructor for InstanceData.
   *
   * @param process The BPMN process associated with the instance data.
   * @param id The unique identifier of the instance.
   */
  InstanceData(const BPMN::Process& process, const std::string& id) : process(process), id(id) {};

  const BPMN::Process& process;  ///< Reference to the associated BPMN process.
  const std::string id;          ///< Unique identifier of the instance.

  /**
   * @brief Initializes the attribute values for the given node.
   *
   * Method that initializes a value vector with the numeric values provided for the instance. The given
   * values will be placed at the end of the given `values` vector in the order of their declaration. 
   * The `values` vector must have sufficient space and may begin with additional attribute values 
   * declared for ancestor node(s).
   *
   * @param node The node for which to initialize the attribute values.
   * @param values The vector to store the initialized attribute values.
   */
  virtual void initializeAttributeValues(const BPMN::Node* node, std::vector<std::optional<T> >& values ) {
    // get attributes declared for node
    if ( auto it = givens.find(node); it != givens.end()) {
      auto& [key, givenValues] = *it;

      if ( values.size() < givenValues.size() ) {
        throw std::logic_error("InstanceData: not enough values in argument");
      }

      auto delta = values.size() - givenValues.size();
      for (unsigned int i = 0; i < givenValues.size(); i++ ) {
        if ( givenValues[i].has_value() ) {
          values[delta + i] = givenValues[i]->numeric;
        }
      }
    }
  };

protected:

  /**
   * @brief Struct holding a given attribute value as string and by its numeric representation.
   */
  struct GivenValue {
    std::string string;
    T numeric;
  };

  /**
   * @brief Map holding a vector of optional attribute definitions for a node.
   */
  std::unordered_map<const BPMN::Node*, std::vector< std::optional<GivenValue> > > givens;
 
  /**
   * @brief Updates the map holding the given values for attributes.
   *
   * @param node The node associated with the given values.
   * @param attributeId The unique identifier of the attribute.
   * @param value The value of the attribute.
   */
  void update(const BPMN::Node* node, const std::string& attributeId, const std::string& value ) {
    auto status = node->extensionElements->as<Status>();
    givens[node].resize(status->attributes.size());

    for (unsigned int i = 0; i < status->attributes.size(); i++) {
      // check if attribute id is found 
      if ( status->attributes[i].get()->id == attributeId ) {
        if ( value == "" ) {
          throw std::logic_error("InstanceData: no value provided for attribute '" + attributeId + "'");
        }

        if ( status->attributes[i]->type == Attribute::Type::STRING ) {
          // use string registry to obtain numeric value for string
          givens[node][i] = { .string = value, .numeric = numeric<T>(stringRegistry(value)) };
        }
        else {
          // convert value to number
          givens[node][i] = { .string = value, .numeric = numeric<T>(std::stod(value)) };
        }
        return;
      }
    }
    throw std::logic_error("InstanceData: cannot find attribute '" + attributeId + "'");
  };

};

} // namespace BPMNOS

#endif // BPMNOS_InstanceData_H
