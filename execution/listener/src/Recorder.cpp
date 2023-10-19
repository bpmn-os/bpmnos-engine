#include "Recorder.h"
#include "model/utility/src/Number.h"
#include "model/parser/src/extensionElements/Status.h"
#include "execution/engine/src/StateMachine.h"

using namespace BPMNOS::Execution;

Recorder::Recorder() : Listener()
{
  log = nlohmann::json::array();
}

void Recorder::update( const Token* token ) {
  std::string instanceId = BPMNOS::to_string(token->status[Model::Status::Index::Instance].value(),STRING);
  log.push_back( token->jsonify() );
}
