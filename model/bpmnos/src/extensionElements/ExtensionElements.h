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
  ExtensionElements(XML::bpmn::tBaseElement* baseElement, const AttributeRegistry attributeRegistry_, BPMN::Scope* parent = nullptr, std::vector<std::reference_wrapper<XML::bpmnos::tAttribute>> = {});
  AttributeRegistry attributeRegistry; ///< Registry allowing to look up all status and data attributes by their names.
  const BPMN::Scope* parent;

  /// Index of the timestamp among the status attributes.
  struct Index {
    static constexpr size_t Timestamp = 0; 
  };
  /// Position of the instance among the data attributes a process declares; its index in the data of every
  /// scope is @ref BPMNOS::Model::Model::instanceIndex.
  struct Position {
    static constexpr size_t Instance = 0; 
  };
  
  std::vector< std::unique_ptr<Attribute> > attributes; ///< Vector containing new status attributes declared for the node.
  std::vector< std::unique_ptr<Restriction> > restrictions; ///< Vector containing new restrictions provided for the node.
  std::vector< std::unique_ptr<Operator> > operators;
  std::vector< std::unique_ptr<Choice> > choices;

  std::set<const Attribute*> entryDependencies; ///< Set containing all input attributes influencing the entry feasibility.
  std::set<const Attribute*> completionDependencies; ///< Set containing all input attributes influencing completion feasibility.
  std::set<const Attribute*> exitDependencies; ///< Set containing all input attributes influencing the exit feasibility.
  std::set<const Attribute*> operatorDependencies; ///< Set containing all input attributes influencing the result of applying all operators.
  std::set<const Attribute*> choiceDependencies; ///< Set containing all input attributes influencing the allowed alternatives when making choices.

  std::vector< std::unique_ptr<Attribute> > data;  ///< Vector containing data attributes declared for data objects within the node's scope.

  struct { std::vector<const Attribute*> attributes; } dataUpdate; ///< Struct containing the data attributes, global or not, that are modified through operators, choices, or message content.

  std::unique_ptr<MessageDefinition> messageDefinition; ///< Message definition provided for the node, or nullptr if the node exchanges no message.
  const MessageDefinition* getMessageDefinition() const;

  std::vector< const BPMN::FlowNode* > messageCandidates; ///< Vector containing all potential sending or receiving nodes of a message.

  std::vector< std::unique_ptr<Restriction> > conditions; ///< Vector containing conditions that may be provided for conditional events.

  std::optional< std::unique_ptr<Parameter> > loopCardinality;  ///< Number of instances to be generated.
  std::optional< std::unique_ptr<Parameter> > loopIndex; ///< Attribute holding the automatically generated loop index.
  std::optional< std::unique_ptr<Parameter> > loopCondition; ///< Boolean attribute indicating whether an exit condition holds.
  std::optional< std::unique_ptr<Parameter> > loopMaximum;  ///< Maximum number of iterations of a standard loop (requires loopIndex).
  
  bool hasSequentialPerformer; ///< Boolean indicating whether element has a performer with name "Sequential".

  template <typename DataType>
  bool feasibleEntry(const BPMNOS::Status& status, const DataType& data) const;

  template <typename DataType>
  bool feasibleCompletion(const BPMNOS::Status& status, const DataType& data) const;
  
  template <typename DataType>
  bool feasibleExit(const BPMNOS::Status& status, const DataType& data) const;
  
  template <typename DataType>
  bool satisfiesInheritedRestrictions(const BPMNOS::Status& status, const DataType& data) const;
  
  template <typename DataType>
  bool fullScopeRestrictionsSatisfied(const BPMNOS::Status& status, const DataType& data) const;
  
  bool isInstantaneous; ///< Boolean indicating whether operators may modify timestamp.

  /// @brief Method computing the values the model assigns and returning the change of the objective.
  template <typename DataType>
  BPMNOS::number computeInitialValues(BPMNOS::number currentTime, BPMNOS::Status& status, DataType& data) const;

  /// @brief Method applying the operators and returning the change of the objective.
  template <typename DataType>
  BPMNOS::number applyOperators(BPMNOS::Status& status, DataType& data) const;

  template <typename DataType>
  BPMNOS::number getObjective(const BPMNOS::Status& status, const DataType& data) const; ///< Returns the total objective of all attributes provided.

  template <typename DataType>
  std::vector<std::pair<const Attribute*, BPMNOS::number>> getContributionsToObjective(const BPMNOS::Status& status, const DataType& data) const; ///< Returns the contribution to the objective by the attributes declared for the node.
  
  std::optional< std::unique_ptr<Guidance> > messageDeliveryGuidance;
  std::optional< std::unique_ptr<Guidance> > entryGuidance;
  std::optional< std::unique_ptr<Guidance> > exitGuidance;
  std::optional< std::unique_ptr<Guidance> > choiceGuidance;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ExtensionElements_H
