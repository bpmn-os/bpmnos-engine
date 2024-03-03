#ifndef BPMNOS_Model_ExtensionElements_H
#define BPMNOS_Model_ExtensionElements_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Attribute.h"
#include "Restriction.h"
#include "Operator.h"
#include "Decision.h"
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
  AttributeMap attributeMap; ///< Map allowing to look up all attributes by their names.

  enum Index { Instance, Timestamp }; ///< Indices for instance and timestamp attribute.

  std::vector< std::unique_ptr<Attribute> > attributes; ///< Vector containing new attributes declared for the node.
  std::vector< std::unique_ptr<Restriction> > restrictions; ///< Vector containing new restrictions provided for the node.
  std::vector< std::unique_ptr<Operator> > operators;
  std::vector< std::unique_ptr<Decision> > decisions;

  std::vector< std::unique_ptr<MessageDefinition> > messageDefinitions; ///< Vector containing message definition(s) provided for the node.
  std::vector< const BPMN::FlowNode* > messageCandidates; ///< Vector containing all potential sending or receiving nodes of a message.

  std::optional< std::unique_ptr<Parameter> > loopCardinality;
  std::optional< std::unique_ptr<Parameter> > loopIndex;
  
  bool hasSequentialPerformer; ///< Boolean indicating whether element has a performer with name "Sequential".

  inline std::size_t size() const { return parentSize + attributes.size(); };

  bool entryScopeRestrictionsSatisfied(const Values& values) const;
  bool exitScopeRestrictionsSatisfied(const Values& values) const;
  bool fullScopeRestrictionsSatisfied(const Values& values) const;
  
  bool isInstantaneous; ///< Boolean indicating whether operators may modify timestamp.
  void applyOperators(Values& values) const;

  BPMNOS::number getContributionToObjective(const Values& values) const; ///< Returns the contribution to the objective by the attributes declared for the node.
  
  std::optional< std::unique_ptr<Guidance> > messageGuidance;
  std::optional< std::unique_ptr<Guidance> > entryGuidance;
  std::optional< std::unique_ptr<Guidance> > exitGuidance;
  std::optional< std::unique_ptr<Guidance> > choiceGuidance;
protected:
  std::size_t parentSize;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ExtensionElements_H
