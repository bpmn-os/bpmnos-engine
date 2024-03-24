#ifndef BPMNOS_Model_ExtensionElements_H
#define BPMNOS_Model_ExtensionElements_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Attribute.h"
#include "Restriction.h"
#include "Operator.h"
#include "Choice.h"
#include "MessageDefinition.h"
#include "Guidance.h"

namespace BPMNOS::Model {


/**
 * @brief Class holding extension elements representing execution data for nodes 
 **/
class ExtensionElements : public BPMN::ExtensionElements {
public:
  ExtensionElements(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent = nullptr);
  const BPMN::Scope* parent;
  AttributeMap statusAttributes; ///< Map allowing to look up all status attributes by their names.

  enum Index { Instance, Timestamp }; ///< Indices for instance and timestamp attribute.

  std::vector< std::unique_ptr<Attribute> > attributes; ///< Vector containing new status attributes declared for the node.
  std::vector< std::unique_ptr<Restriction> > restrictions; ///< Vector containing new restrictions provided for the node.
  std::vector< std::unique_ptr<Operator> > operators;
  std::vector< std::unique_ptr<Choice> > choices;

  std::vector< std::unique_ptr<MessageDefinition> > messageDefinitions; ///< Vector containing message definition(s) provided for the node.
  std::vector< const BPMN::FlowNode* > messageCandidates; ///< Vector containing all potential sending or receiving nodes of a message.

  std::optional< std::unique_ptr<Parameter> > loopCardinality;
  std::optional< std::unique_ptr<Parameter> > loopIndex;
  
  bool hasSequentialPerformer; ///< Boolean indicating whether element has a performer with name "Sequential".

  inline std::size_t size() const { return parentSize + attributes.size(); };

  bool feasibleEntry(const Values& status) const;
  bool feasibleExit(const Values& status) const;
  bool satisfiesInheritedRestrictions(const Values& status) const;
  bool fullScopeRestrictionsSatisfied(const Values& status) const;
  
  bool isInstantaneous; ///< Boolean indicating whether operators may modify timestamp.
  void applyOperators(Values& status) const;

  BPMNOS::number getObjective(const Values& status) const; ///< Returns the contribution to the objective.
  BPMNOS::number getContributionToObjective(const Values& status) const; ///< Returns the contribution to the objective by the attributes declared for the node.
  
  std::optional< std::unique_ptr<Guidance> > messageDeliveryGuidance;
  std::optional< std::unique_ptr<Guidance> > entryGuidance;
  std::optional< std::unique_ptr<Guidance> > exitGuidance;
  std::optional< std::unique_ptr<Guidance> > choiceGuidance;
protected:
  std::size_t parentSize;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ExtensionElements_H
