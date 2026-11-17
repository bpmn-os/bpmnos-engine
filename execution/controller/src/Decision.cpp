#include "Decision.h"
#include "execution/engine/src/DecisionRequest.h"

using namespace BPMNOS::Execution;


Decision::Decision(const DecisionRequest* request, Evaluator* evaluator)
  : request(request->weak_from_this())
  , evaluator(evaluator)
{
}

bool Decision::expired() const {
  // Stale once the decision request is gone: it is superseded (token->decisionRequest.reset()) or its
  // owning token was destroyed (the request cannot outlive its token), so this also covers token death.
  return request.expired();
}

void Decision::determineDependencies(const std::set<const BPMNOS::Model::Attribute*>& dependencies) {
  // an evaluation depending on time depends on the global attribute `time`, which is a data attribute like any other
  for ( auto attribute : dependencies ) {
    if ( attribute->category != BPMNOS::Model::Attribute::Category::STATUS && !attribute->isImmutable ) {
      dataDependencies.insert(attribute);
    }
  }
}
