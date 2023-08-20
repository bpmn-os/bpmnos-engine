#ifndef BPMNOS_DataProvider_H
#define BPMNOS_DataProvider_H

#include <string>
#include <memory>
#include <unordered_map>
#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/Model.h"
#include "InstanceData.h"

namespace BPMNOS {

/**
 * @brief Abstract base class representing a data provider for BPMN instance data.
 *
 * The DataProvider class is responsible for providing and managing instance data
 * for BPMN processes. 
 */
class DataProvider {
public:
 /**
   * @brief Constructor for DataProvider.
   *
   * @param modelFile The file path to the BPMN model file.
   */
  DataProvider(const std::string& modelFile);
  virtual ~DataProvider() = 0;
  const Model& getModel() const;
  const std::unordered_map<std::string, std::unique_ptr< InstanceData > >& getInstances() const;

  void appendActualValues( const InstanceData* instance, const BPMN::Node* node, Values& values) const;
  void appendPredictedValues( const InstanceData* instance, const BPMN::Node* node, Values& values ) const;
  void appendAssumedValues( const InstanceData* instance, const BPMN::Node* node, Values& values ) const;

protected:
  const std::unique_ptr<Model> model;  ///< Pointer to the BPMN model.
  std::unordered_map<std::string, const BPMN::Process*> processes; ///< Map of processes defined in the model with key being the process id.
  std::unordered_map<std::string, std::unique_ptr< InstanceData > > instances; ///< Map of instances with key being the instance id.
};

} // namespace BPMNOS

#endif // BPMNOS_DataProvider_H
