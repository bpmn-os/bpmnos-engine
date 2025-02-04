#ifndef BPMNOS_Model_Model_H
#define BPMNOS_Model_Model_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include <limex.h>
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/AttributeRegistry.h"
#include "model/bpmnos/src/LookupTable.h"
#include "model/bpmnos/src/xml/bpmnos/tTable.h"

namespace BPMNOS::Model {

/**
 * @brief Represents a BPMN model with all its processes.
 *
 * The `Model` class encapsulates all processes with their nodes and sequence flows of a BPMN model.
 */
class Model : public BPMN::Model {
public:
  Model(const std::string filename, const std::vector<std::string> folders = {});
  const std::string filename; ///< File name of the BPMN model
  const std::vector<std::string> folders; ///< Folders containing lookup tables
  static inline LIMEX::Callables<double> callables = LIMEX::Callables<double>(); // TODO: this should not be static!

  std::vector<std::reference_wrapper<XML::bpmnos::tAttribute>> getAttributes(XML::bpmn::tBaseElement* element);
  std::vector<std::reference_wrapper<XML::bpmnos::tAttribute>> getData(XML::bpmn::tBaseElement* element);
  
  std::unique_ptr<LookupTable> createLookupTable(XML::bpmnos::tTable* table);
  std::unique_ptr<XML::XMLObject> createRoot(const std::string& filename) override;

  std::unique_ptr<BPMN::Process> createProcess(XML::bpmn::tProcess* process) override;
  std::unique_ptr<BPMN::EventSubProcess> createEventSubProcess(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createActivity(XML::bpmn::tActivity* activity, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::SequenceFlow> createSequenceFlow(XML::bpmn::tSequenceFlow* sequenceFlow, BPMN::Scope* scope) override;
  std::unique_ptr<BPMN::FlowNode> createAdHocSubProcess(XML::bpmn::tAdHocSubProcess* adHocSubProcess, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createTask(XML::bpmn::tTask* task, BPMN::Scope* parent) override;

  std::unique_ptr<BPMN::FlowNode> createTimerStartEvent(XML::bpmn::tStartEvent* startEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createTimerBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createTimerCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) override;

  std::unique_ptr<BPMN::FlowNode> createSignalStartEvent(XML::bpmn::tStartEvent* startEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createSignalBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createSignalCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createSignalThrowEvent(XML::bpmn::tThrowEvent* throwEvent, BPMN::Scope* parent) override;

  std::unique_ptr<BPMN::FlowNode> createConditionalStartEvent(XML::bpmn::tStartEvent* startEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createConditionalBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createConditionalCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) override;

  std::unique_ptr<BPMN::FlowNode> createMessageStartEvent(XML::bpmn::tStartEvent* startEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createMessageBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createMessageCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) override;
  std::unique_ptr<BPMN::FlowNode> createMessageThrowEvent(XML::bpmn::tThrowEvent* throwEvent, BPMN::Scope* parent) override;

  void createMessageFlows() override;
  bool messageMayBeCaught( BPMN::Process* sendingProcess, BPMN::FlowNode* throwingMessageEvent, BPMN::Process* receivingProcess, BPMN::FlowNode* catchingMessageEvent );
  bool messageMayBeThrown( BPMN::Process* sendingProcess, BPMN::FlowNode* throwingMessageEvent, BPMN::Process* receivingProcess, BPMN::FlowNode* catchingMessageEvent );
  void createMessageCandidates( BPMN::Process* sendingProcess, BPMN::FlowNode* throwingMessageEvent, BPMN::Process* receivingProcess, BPMN::FlowNode* catchingMessageEvent );
  std::vector<BPMN::MessageFlow*>& determineMessageFlows(BPMN::FlowNode* messageEvent, auto getMessageFlows);
  
  static bool hasSequentialPerformer(const std::vector< std::reference_wrapper<XML::bpmn::tResourceRole> >& resources);
  
  AttributeRegistry attributeRegistry; ///< Registry allowing to look up all status and data attributes by their names.
  std::vector< std::unique_ptr<LookupTable> > lookupTables; ///< Vector containing lookup tables declared in model.
  std::vector< std::unique_ptr<Attribute> > attributes; ///< Vector containing new global attributes declared for the model.
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Model_H
