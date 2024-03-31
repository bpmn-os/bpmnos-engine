#ifndef BPMNOS_Model_ExtensionElements_H
#define BPMNOS_Model_ExtensionElements_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Attribute.h"
#include "AttributeRegistry.h"
#include "Restriction.h"
#include "Operator.h"
#include "Choice.h"
#include "MessageDefinition.h"
#include "Guidance.h"
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"

namespace BPMNOS::Model {


/**
 * @brief Class holding extension elements representing execution data for nodes 
 **/
class ExtensionElements : public BPMN::ExtensionElements {
public:
  ExtensionElements(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent = nullptr, std::vector<std::reference_wrapper<XML::bpmnos::tAttribute>> = {});
  const BPMN::Scope* parent;
  AttributeRegistry attributeRegistry; ///< Registry allowing to look up all status and data attributes by their names.

//  enum Index { Instance, Timestamp }; ///< Indices for instance and timestamp attribute.
  struct Index {
    static constexpr size_t Instance = 0; 
    static constexpr size_t Timestamp = 0; 
  };
  
  std::vector< std::unique_ptr<Attribute> > attributes; ///< Vector containing new status attributes declared for the node.
  std::vector< std::unique_ptr<Restriction> > restrictions; ///< Vector containing new restrictions provided for the node.
  std::vector< std::unique_ptr<Operator> > operators;
  std::vector< std::unique_ptr<Choice> > choices;

  std::vector< std::unique_ptr<Attribute> > data;  ///< Vector containing data attributes declared for data objects within the node's scope.

  std::vector< std::unique_ptr<MessageDefinition> > messageDefinitions; ///< Vector containing message definition(s) provided for the node.
  std::vector< const BPMN::FlowNode* > messageCandidates; ///< Vector containing all potential sending or receiving nodes of a message.

  std::optional< std::unique_ptr<Parameter> > loopCardinality;
  std::optional< std::unique_ptr<Parameter> > loopIndex;
  
  bool hasSequentialPerformer; ///< Boolean indicating whether element has a performer with name "Sequential".

  template <typename DataType>
  bool feasibleEntry(const BPMNOS::Values& status, const DataType& data) const;
  
  template <typename DataType>
  bool feasibleExit(const BPMNOS::Values& status, const DataType& data) const;
  
  template <typename DataType>
  bool satisfiesInheritedRestrictions(const BPMNOS::Values& status, const DataType& data) const;
  
  template <typename DataType>
  bool fullScopeRestrictionsSatisfied(const BPMNOS::Values& status, const DataType& data) const;
  
  bool isInstantaneous; ///< Boolean indicating whether operators may modify timestamp.

  template <typename DataType>
  void applyOperators(BPMNOS::Values& status, DataType& data) const;

  template <typename DataType>
  BPMNOS::number getObjective(const BPMNOS::Values& status, const DataType& data) const; ///< Returns the contribution to the objective.

  template <typename DataType>
  BPMNOS::number getContributionToObjective(const BPMNOS::Values& status, const DataType& data) const; ///< Returns the contribution to the objective by the attributes declared for the node.
  
  std::optional< std::unique_ptr<Guidance> > messageDeliveryGuidance;
  std::optional< std::unique_ptr<Guidance> > entryGuidance;
  std::optional< std::unique_ptr<Guidance> > exitGuidance;
  std::optional< std::unique_ptr<Guidance> > choiceGuidance;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ExtensionElements_H
