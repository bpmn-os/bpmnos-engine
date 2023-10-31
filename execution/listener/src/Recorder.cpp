#include "Recorder.h"
#include "model/utility/src/Number.h"
#include "model/parser/src/extensionElements/Status.h"
#include "execution/engine/src/StateMachine.h"

using namespace BPMNOS::Execution;

Recorder::Recorder(size_t maxSize) : Listener(), os(std::nullopt), maxSize(maxSize)
{
  log = nlohmann::ordered_json::array();
}

Recorder::Recorder(std::ostream &os, size_t maxSize) : Listener(), os(os), maxSize(maxSize)
{
  log = nlohmann::ordered_json::array();
  this->os.value().get() << "[";
  isFirst = true;
}

Recorder::~Recorder()
{
  if (os.has_value()) {
    os.value().get() << "]";
  }
}

void Recorder::update( const Token* token ) {
  auto json = token->jsonify();

  if (os.has_value()) {
    if ( !isFirst ) {
      os.value().get() << ",";
    }
    os.value().get() << json.dump();
    isFirst = false;
  }

  log.push_back( json );

  if ( log.size() > maxSize) {
    log.erase(log.begin());
  }
}
