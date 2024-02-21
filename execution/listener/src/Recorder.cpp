#include "Recorder.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/SystemState.h"

using namespace BPMNOS::Execution;

Recorder::Recorder(size_t maxSize) : Listener(), objective(0), os(std::nullopt), maxSize(maxSize)
{
  log = nlohmann::ordered_json::array();
}

Recorder::Recorder(std::ostream &os, size_t maxSize) : Listener(), objective(0), os(os), maxSize(maxSize)
{
  log = nlohmann::ordered_json::array();
  if (this->os.has_value()) {
    this->os.value().get() << "[";
  }
  isFirst = true;
}

Recorder::~Recorder()
{
  if (os.has_value()) {
    os.value().get() << "]" << std::endl;
    os.value().get()  << "Objective (maximization): " << (float)objective << std::endl;
    os.value().get()  << "Objective (minimization): " << -(float)objective << std::endl;
  }
}

void Recorder::update( const Token* token ) {
  objective = token->owner->systemState->objective;
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
#include <iostream>
nlohmann::ordered_json Recorder::find(nlohmann::json include, nlohmann::json exclude) const {
  nlohmann::ordered_json result = nlohmann::ordered_json::array();
  std::copy_if(
    log.begin(),
    log.end(),
    std::back_inserter(result), [&include,&exclude](const nlohmann::json& item) {
      for ( auto& [key,value] : include.items() ) {
        if ( !item.contains(key) || item[key] != value ) {
          return false;
        } 
      }
      for ( auto& [key,value] : exclude.items() ) {
        if ( item.contains(key) && (value.is_null() || item[key] == value) ) {
          return false;
        } 
      }
      return true;
    }
  );
  return result;
}

