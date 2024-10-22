#ifndef BPMNOS_Execution_CPController_H
#define BPMNOS_Execution_CPController_H

#include <bpmn++.h>
#include "Controller.h"
#include "Evaluator.h"
#include "execution/engine/src/Mediator.h"
#include <cp.h>
#include <unordered_map>
#include <utility>
#include <memory>



namespace BPMNOS::Execution {



/**
 * @brief A controller dispatching decisions obtained from a solution of a constraint program
 */
class CPController : public Controller {
public:
  CPController();
  void connect(Mediator* mediator);
//  std::vector< std::unique_ptr<EventDispatcher> > eventDispatchers;
protected:
  Evaluator* evaluator;
  std::shared_ptr<Event> dispatchEvent(const SystemState* systemState);
  const BPMNOS::Model::Scenario* scenario;
  std::vector< const BPMNOS::Model::Scenario::InstanceData* > instances;
  CP::Model model;
public:
  const CP::Model& createCP(const BPMNOS::Model::Scenario* scenario); /// Function creating a constraint program
protected:
/*
  void createEntryVariables(const BPMN::Process* process, const BPMNOS::Values& status, const BPMNOS::Values& data); /// Function creating process variables
  void createEntryVariables(const BPMN::FlowNode* flowNode); /// Function creating process variables

  struct Reference {
    Reference(const BPMN::Node* node, bool entered = false) : node(node), entered(entered) {};
    const BPMN::Node* node;
    bool entered;
  };
  std::list<Reference> pending; /// Map holding references of all entry/exit variables that have not yet been created
  struct Variables {
  };
*/

  std::unordered_map<const BPMN::MessageThrowEvent*, std::vector<BPMNOS::number> > originInstances; /// Map containing all instance identifiers for all message throw events

  std::unordered_map<const BPMN::Scope*, std::vector<const BPMN::FlowNode*> > reachableFlowNodes; /// Map containing all flow nodes in a scope that can be reached without traversing a boundary event

 
/*  
  auto getReachableEndNodes( const BPMN::Scope* scope ) {
    // Check if the given scope exists in the map
    auto it = reachableFlowNodes.find(scope);
    if (it == reachableFlowNodes.end()) {
      assert(!"Unknown scope");
    }

    // Use ranges and views to filter nodes with empty outgoing vectors
    auto reachableEndNodes = it->second | std::ranges::views::filter([](const BPMN::FlowNode* node) {
        return node->outgoing.empty();
    });

    return reachableEndNodes;
  };
*/
  
  struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        // Mainly for demonstration purposes, i.e. works but is overly simple
        // In the real world, use sth. like boost.hash_combine
        // return h1 ^ h2;
        return h1 * 31 + h2; // Prime multiplication
    }
  };
  
  struct AttributeVariables {
    const CP::Variable& defined;
    const CP::Variable& value;
  };
 
  std::unordered_map<const BPMNOS::Model::Attribute*, const BPMN::Scope* > dataOwner;/// Map allowing to look up the scope owning a data attribute
  std::unordered_map<const BPMN::Scope*, std::vector<const BPMN::Node* > > sequentialActivities;/// Map allowing to look up the sequential activities that may change a data attribute (assumimng that intermediate changes are not propagated)
  
  using NodeReference = std::pair<size_t, const BPMN::Node*>;
  using DataReference = std::pair<size_t, const BPMNOS::Model::Attribute*>;
  using MessageCatchReference = std::pair<size_t, const BPMN::MessageCatchEvent*>;
  using MessageThrowReference = std::pair<size_t, const BPMN::MessageThrowEvent*>;
  using ScopeReference = std::pair<size_t, const BPMN::Scope*>;
  using SequenceFlowReference = std::pair<size_t,const BPMN::SequenceFlow*>;

  template <typename T>
  std::string identifier(const std::pair<size_t, const T*>& reference) {
    return BPMNOS::to_string(reference.first,STRING) + "," + reference.second->id;
  }

  template <typename ReferenceType, typename ValueType>
  using variable_map = std::unordered_map<ReferenceType, ValueType, pair_hash>;


  variable_map<NodeReference, const CP::Variable&> nodeVariables; 
  variable_map<SequenceFlowReference, const CP::Variable&> sequenceFlowVariables; 
  variable_map<MessageCatchReference, variable_map<MessageThrowReference, const CP::Variable&> > messageFlowVariables; 
  
  std::vector<AttributeVariables> globals; 
  variable_map<DataReference, std::vector< AttributeVariables> > dataState; /// Variables representing data attributes after i-th activity of a sequential performer is conducted

  variable_map<NodeReference, variable_map<ScopeReference, std::vector< std::reference_wrapper< const CP::Variable > > > > entryDataState; /// Binary variables indicating the data states upon entry
  variable_map<NodeReference, variable_map<ScopeReference, std::vector< std::reference_wrapper< const CP::Variable > > > > exitDataState; /// Binary variables indicating the data states upon exit

  variable_map<NodeReference, std::vector<AttributeVariables> > entryData; /// Variables representing data state upon entry to a node
  variable_map<NodeReference, std::vector<AttributeVariables> > exitData;  /// Variables representing data state upon exit of a node

  variable_map<NodeReference, std::vector<AttributeVariables> > entryStatus;
  variable_map<NodeReference, std::vector<AttributeVariables> > exitStatus; 
  
  variable_map<NodeReference, std::vector< std::vector<AttributeVariables> > > interimData;  /// Variables representing data attributes after the i-th operator is applied
  variable_map<NodeReference, std::vector< std::vector<AttributeVariables> > > interimStatus;  /// Variables representing data attributes after i-th operator is applied


private:
  std::unordered_map<NodeReference, std::set<const BPMN::Scope*>, pair_hash > scopesOwningData;
  std::set<NodeReference> pendingEntry;
  std::list<NodeReference> pendingExit;
  void addChildren(ScopeReference reference);
  void addSuccessors(NodeReference reference);

  void addGlobalVariable( const BPMNOS::Model::Attribute* attribute, std::optional<BPMNOS::number> value );
  
  bool checkEntryDependencies(NodeReference reference);
  bool checkExitDependencies(NodeReference reference);
  
  void addEntryVariables(NodeReference reference);  
  void createNodeTraversalVariables(NodeReference reference); /// x_n
  void createEntryDataStateVariables(NodeReference reference); /// y^entry_{n,s,i} where n is a node, s is a scope with data, and i is a non-negative index for the entry data state at n for data belonging to s 
  void deduceEntryAttributeVariable(std::vector<AttributeVariables>& attributeVariables, const BPMNOS::Model::Attribute* attribute, NodeReference reference, variable_map<NodeReference, std::vector<AttributeVariables> >& entryAttributes, variable_map<NodeReference, std::vector<AttributeVariables> >& exitAttributes);

  void addExitVariables(NodeReference reference);
  void createExitDataStateVariables(NodeReference reference); /// y^exit_{n,s,i} where n is a node, s is a scope with data, and i is a non-negative index for the exit data state at n for data belonging to s 
  void constrainExitDataStateVariables(ScopeReference reference); /// y^exit_{n,s,i} where n is a node, s is a scope with data, and i is a non-negative index for the exit data state at n for data belonging to s 
//  void createSequenceFlowTraversalVariables(SequenceFlowReference reference);
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_CPController_H

