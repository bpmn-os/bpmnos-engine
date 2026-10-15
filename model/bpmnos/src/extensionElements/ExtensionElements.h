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
#include "SignalDefinition.h"
#include "Parameter.h"
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
  
  std::vector< std::unique_ptr<Attribute> > attributes; ///< Vector containing new scalar status attributes declared for the node.
  std::vector< std::unique_ptr<Attribute> > statusObjects; ///< Vector containing new status objects declared for the node.
  std::vector< std::unique_ptr<Restriction> > restrictions; ///< Vector containing new restrictions provided for the node.
  std::vector< std::unique_ptr<Operator> > operators;
  std::vector< std::unique_ptr<Choice> > choices;

  std::set<const Attribute*> entryDependencies; ///< Set containing all input attributes influencing the entry feasibility.
  std::set<const Attribute*> completionDependencies; ///< Set containing all input attributes influencing completion feasibility.
  std::set<const Attribute*> exitDependencies; ///< Set containing all input attributes influencing the exit feasibility.
  std::set<const Attribute*> operatorDependencies; ///< Set containing all input attributes influencing the result of applying all operators.
  std::set<const Attribute*> choiceDependencies; ///< Set containing all input attributes influencing the allowed alternatives when making choices.

  std::vector< std::unique_ptr<Attribute> > data;  ///< Vector containing scalar data attributes declared for data objects within the node's scope.
  std::vector< std::unique_ptr<Attribute> > dataObjects;  ///< Vector containing objects declared for data objects within the node's scope.

  struct { std::vector<const Attribute*> attributes; } dataUpdate; ///< Struct containing the data attributes, global or not, that are modified through operators or choices.

  /// The registry the operators are compiled with if the node states a message or signal: the registry of the node,
  /// augmented by the content object of the message or signal, so that only the operators see the content object;
  /// nullptr if the node states none, the operators being compiled with the registry of the node.
  std::unique_ptr<AttributeRegistry> augmentedAttributeRegistry;
  /// The content object of the message or signal the node states, a status object named after the message or
  /// signal and of its type, nullptr if the node states none. Its slot follows the status objects of the node, and
  /// it is held there only while the operators run: the operators of a throwing node write it, starting from an
  /// undefined object of its type, and those of a catching node read the object received.
  std::unique_ptr<Attribute> content;

  std::unique_ptr<MessageDefinition> messageDefinition; ///< Message definition provided for the node, or nullptr if the node exchanges no message.
  const MessageDefinition* getMessageDefinition() const;

  std::vector< const BPMN::FlowNode* > messageCandidates; ///< Vector containing all potential sending or receiving nodes of a message.

  std::unique_ptr<Parameter> trigger; ///< Parameter giving the time at which a timer event is triggered, or nullptr if the node declares no timer.

  std::unique_ptr<SignalDefinition> signalDefinition; ///< Signal definition provided for the node, or nullptr if the node throws or catches no signal.

  std::vector< std::unique_ptr<Restriction> > conditions; ///< Vector containing the conditions of a conditional event.
  std::set<const Attribute*> conditionDependencies; ///< Set containing all input attributes influencing whether the conditions are satisfied.

  template <typename DataType>
  bool conditionsSatisfied(const BPMNOS::Status& status, const DataType& data) const; ///< Returns true if and only if every condition is satisfied.

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

  /// @brief Method applying the operators and returning the change of the objective.
  ///
  /// If the node states a message or signal, its content object, which the operators write or read, must be given;
  /// otherwise the content object given, if any, is not used.
  template <typename DataType>
  BPMNOS::number applyOperators(BPMNOS::Status& status, DataType& data, std::shared_ptr<const BPMNOS::Object>* contentObject = nullptr) const;

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
