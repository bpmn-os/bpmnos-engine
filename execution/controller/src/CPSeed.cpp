#include "CPSeed.h"
#include <cassert>

using namespace BPMNOS::Execution;
using Vertex = FlattenedGraph::Vertex;

CPSeed::CPSeed( CPController* controller, std::list<size_t> seed)
  : controller(controller)
{
  initialize( std::move(seed) );
}

std::list<size_t> CPSeed::defaultSeed(size_t length) {
  std::list<size_t> values( length );
  std::iota(values.begin(), values.end(), 1);
  return values;
}

void CPSeed::initialize(std::list<size_t> seed) {
  assert( seed.size() == controller->getVertices().size() );
  assert( !controller->getModel().getSequences().empty() );
  assert( seed.size() == controller->getModel().getSequences().front().variables.size() );

  sequence.reserve( seed.size() );
  
  auto it = seed.begin();
  while ( it != seed.end() ) {
    if ( addSequencePosition( *it ) ) {
      it = seed.erase(it);
      continue;
    }
    else {
      it++;
    }
  }
}

bool CPSeed::isFeasible() const {
  return (sequence.size() == controller->getVertices().size() );
}

CP::Solution& CPSeed::createSolution() const {
  if ( !isFeasible() ) {
    throw std::runtime_error("CPSeed: cannot create infeasible solution");
  }
  auto& solution = controller->createSolution();
  auto& sequenceVariable = controller->getModel().getSequences().front();
  solution.setSequenceValues( sequenceVariable, sequence );

  return solution;
}

bool CPSeed::addSequencePosition(size_t index) {
  auto vertex = controller->getVertices()[index];

  // check that all predecessors are already in the sequence
  for ( auto& [ sequenceFlow, predecessor ] : vertex->inflows ) {
    if ( !vertices.contains(&predecessor) ) {
      return false;
    }
  }
  for ( Vertex& predecessor : vertex->predecessors ) {
    if ( !vertices.contains(&predecessor) ) {
      return false;
    }
  }

  // ensure that each message recipient can be matched to a message sender
  if ( vertex->entry<BPMN::MessageThrowEvent>() ) {
    senders.emplace_back(vertex);
  }

  if ( vertex->exit<BPMN::MessageCatchEvent>() ) {
    recipients.emplace_back(vertex,senders);
    // find augmenting path and update matching
    std::unordered_set<CPSeed::MessageVertex*> marked;
    auto augmentingPath = findAugmentingPath( &recipients.back() );
    if ( augmentingPath.empty() ) {
      return false;
    }
    else {
      // update matching
      updateMatching(augmentingPath);
    }
  }
  
  vertices.insert(vertex);
  sequence.push_back((double)index);

  return true;
}

void CPSeed::updateMatching(CPSeed::Matching& update) {
  for ( auto& [sender,recipient] : update ) {
    matching[sender] = recipient;
  }
}


CPSeed::Matching CPSeed::findAugmentingPath(CPSeed::MessageVertex* recipient) const {
  std::unordered_set<CPSeed::MessageVertex*> marked;
  return findAlternatingPath(recipient, marked);
}

CPSeed::Matching CPSeed::findAlternatingPath(CPSeed::MessageVertex* sender, CPSeed::MessageVertex* recipient) const {
  std::unordered_set<CPSeed::MessageVertex*> marked = { recipient };
  return updateMatch(sender, recipient, marked);
}

CPSeed::Matching CPSeed::findAlternatingPath(CPSeed::MessageVertex* recipient, std::unordered_set<CPSeed::MessageVertex*>& marked) const {
  if ( marked.contains(recipient) ) {
    return {}; // Avoid cycles
  } 
  marked.insert(recipient);
  
  for (MessageVertex* sender : recipient->candidates) {
    auto update = updateMatch(sender, recipient, marked);
    if ( !update.empty() ) {
      return update;
    }
  }
    
  return {};
}

CPSeed::Matching CPSeed::updateMatch(CPSeed::MessageVertex* sender, CPSeed::MessageVertex* recipient, std::unordered_set<CPSeed::MessageVertex*>& marked) const {
  auto it = matching.find(sender);
  if ( it == matching.end() ) {
    Matching update;
    update[sender] = recipient; // set match of sender to recipient
    return update;
  }
  else {
    auto update = findAlternatingPath( it->second, marked ); // recurse to update previous matching(s) 
    if ( !update.empty() ) {
      update[sender] = recipient; // change match of sender to recipient
    }
    return update;
  }
}


CPSeed::MessageVertex::MessageVertex(const FlattenedGraph::Vertex* vertex, std::vector<CPSeed::MessageVertex>& senders)
 : vertex(vertex) 
{
  for ( const FlattenedGraph::Vertex& candidate : vertex->senders ) {
    // check if sender representing candidate already exists
    auto it = std::find_if(senders.begin(), senders.end(), [&](const CPSeed::MessageVertex& sender) {
      return sender.vertex == &candidate;
    });
    if (it != senders.end()) {
      // add sender to candidates of recipient and vice versa
      auto& sender = *it;
      candidates.push_back(&sender);
      sender.candidates.push_back(this);
    }
  }
};




