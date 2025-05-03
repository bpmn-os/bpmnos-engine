#include "CPSeed.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include <cassert>

using namespace BPMNOS::Execution;
using Vertex = FlattenedGraph::Vertex;

CPSeed::CPSeed( SeededController* controller, std::list<size_t> seed)
  : controller(controller)
{
  initialize( std::move(seed) );
}

std::list<size_t> CPSeed::defaultSeed(size_t length) {
  std::list<size_t> values( length );
  // sequence positions start at 1
  std::iota(values.begin(), values.end(), 1);
  return values;
}

std::list<size_t> CPSeed::defaultSeed(std::list<size_t> partialSeed, size_t length) {
  auto seed = defaultSeed(length);
  while ( !partialSeed.empty() ) {
    seed.remove( partialSeed.back() );
    seed.push_front( partialSeed.back() ); 
    partialSeed.pop_back();
  }
std::cerr << "initial seed: ";
for ( auto i : seed ) std::cerr << i << ", ";
std::cerr << std::endl;

  return seed;
}

double CPSeed::coverage() const {
  return (double)sequence.size() / (double)controller->getVertices().size();
}

std::list<size_t> CPSeed::getSeed() const {
  return std::list<size_t>(sequence.begin(), sequence.end());
}

std::vector<size_t> CPSeed::getSequence() const {
  return sequence;
}


void CPSeed::initialize(std::list<size_t> seed) {
  assert( seed.size() == controller->getVertices().size() );
//  assert( !controller->getModel().getSequences().empty() );
//  assert( seed.size() == controller->getModel().getSequences().front().variables.size() );

  sequence.reserve( seed.size() );
  
  auto it = seed.begin();
  while ( it != seed.end() ) {
    if ( addSequencePosition( *it ) ) {
      seed.erase(it);
      it = seed.begin();
      continue;
    }
    else {
      it++;
    }
  }
}


bool CPSeed::addSequencePosition(size_t index) {
  auto vertex = controller->getVertices().at(index-1); // sequence positions start at 1
  if ( !sequence.empty() ) {
    auto last = controller->getVertices().at( sequence.back() - 1 );
    if ( last->entry<BPMN::TypedStartEvent>() && vertex != controller->exit(last) ) {
      return false;
    }
  }

  // check that all predecessors are already in the sequence
  for ( auto& [ sequenceFlow, predecessor ] : vertex->inflows ) {
    if ( !vertices.contains(&predecessor) ) {
//std::cerr << "[" << predecessor.reference() << " -> " << vertex->reference() << "]";
      return false;
    }
  }
  for ( Vertex& predecessor : vertex->predecessors ) {
    if ( !vertices.contains(&predecessor) ) {
//std::cerr << "[" << predecessor.reference() << " -> " << vertex->reference() << "]";
      return false;
    }
  }

  // check that no other sequential activity is currently conducted by the same performer
  if ( vertex->type == Vertex::Type::ENTRY ) {
    if ( auto activity = vertex->node->represents<BPMN::Activity>();
      activity &&
      activity->as<BPMN::Activity>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>()
    ) {
      auto performer = vertex->performer();
      for ( auto& [somePerformer,someSequentialActivities] : controller->flattenedGraph.sequentialActivities ) {
        if ( somePerformer != performer ) {
          continue;
        }
        for ( auto& [someEntry,someExit] : someSequentialActivities ) {
          if (
            &someEntry != vertex &&
            vertices.contains(&someEntry) && 
            !vertices.contains(&someExit)
          ) {
            return false;
          } 
        }
      } 
    }
  }

  vertices.insert(vertex);
  sequence.push_back(index);
//std::cerr << vertex->reference() << " -> ";
  return true;
}



