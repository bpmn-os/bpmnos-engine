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
  // sequence positions start at 1
  std::iota(values.begin(), values.end(), 1);
  return values;
}

void CPSeed::initialize(std::list<size_t> seed) {
std::cerr << "CPSeed: ";
std::cerr << "given seed: ";
for ( auto i : seed ) std::cerr << i << ", ";
std::cerr << std::endl;
  assert( seed.size() == controller->getVertices().size() );
  assert( !controller->getModel().getSequences().empty() );
  assert( seed.size() == controller->getModel().getSequences().front().variables.size() );

  sequence.reserve( seed.size() );
  
std::cerr << "resulting seed: ";
  auto it = seed.begin();
  while ( it != seed.end() ) {
    if ( addSequencePosition( *it ) ) {
std::cerr << *it << " -> ";
      seed.erase(it);
      it = seed.begin();
      continue;
    }
    else {
      it++;
    }
  }
std::cerr << std::endl;
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

CP::Solution& CPSeed::createSolution() const {
  if ( coverage() != 1.0 ) {
    throw std::runtime_error("CPSeed: failed achieving full coverage");
  }
  auto& solution = controller->createSolution();
  auto& sequenceVariable = controller->getModel().getSequences().front();
  solution.setSequenceValues( sequenceVariable, sequence );

  return solution;
}

bool CPSeed::addSequencePosition(size_t index) {
  auto vertex = controller->getVertices().at(index-1); // sequence positions start at 1

  // check that all predecessors are already in the sequence
  for ( auto& [ sequenceFlow, predecessor ] : vertex->inflows ) {
    if ( !vertices.contains(&predecessor) ) {
std::cerr << "[" << predecessor.reference() << " -> " << vertex->reference() << "]";
      return false;
    }
  }
  for ( Vertex& predecessor : vertex->predecessors ) {
    if ( !vertices.contains(&predecessor) ) {
std::cerr << "[" << predecessor.reference() << " -> " << vertex->reference() << "]";
      return false;
    }
  }

  vertices.insert(vertex);
  sequence.push_back(index);
std::cerr << vertex->reference() << " -> ";
  return true;
}



