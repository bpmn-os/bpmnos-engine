#include "Decision.h"

using namespace BPMNOS::Execution;


Decision::Decision()
  : timeDependent(false)
{
}


void Decision::determineDependencies(const std::set<const BPMNOS::Model::Attribute*>& dependencies) {
  for ( auto attribute : dependencies ) {
    if ( attribute->category == BPMNOS::Model::Attribute::Category::DATA ) {
      dataDependencies.insert(attribute);
    }
    else if ( attribute->index == BPMNOS::Model::ExtensionElements::Index::Timestamp ) {
      timeDependent = true;
    }
  }
}

