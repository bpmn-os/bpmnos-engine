#include "Recorder.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/SystemState.h"

using namespace BPMNOS::Execution;

Recorder::Recorder(size_t maxSize) : objective(0), os(std::nullopt), maxSize(maxSize)
{
  log = nlohmann::ordered_json::array();
}

Recorder::Recorder(std::ostream &os, size_t maxSize) : objective(0), os(os), maxSize(maxSize)
{
  log = nlohmann::ordered_json::array();
  if (this->os.has_value()) {
    this->os.value().get() << Color::Modifier(Color::FG_LIGHT_GRAY) << "[" << Color::Modifier(Color::FG_DEFAULT);
  }
  isFirst = true;
}

Recorder::~Recorder()
{
  if (os.has_value()) {
    os.value().get() << Color::Modifier(Color::FG_LIGHT_GRAY) << "]" << Color::Modifier(Color::FG_DEFAULT) << std::endl;
    os.value().get()  << "Objective (maximization): " << (float)objective << std::endl;
    os.value().get()  << "Objective (minimization): " << -(float)objective << std::endl;
  }
}

void Recorder::subscribe(Engine* engine) {
  engine->addSubscriber(this, 
    Execution::Observable::Type::Token,
    Execution::Observable::Type::Event
  );
}

void Recorder::notice(const Observable* observable) {
  if ( observable->getObservableType() ==  Execution::Observable::Type::Token ) {
    auto token = static_cast<const Token*>(observable);
    objective = token->owner->systemState->getObjective();
    auto json = token->jsonify();

    if (os.has_value()) {
      if ( !isFirst ) {
        os.value().get() << Color::Modifier(Color::FG_LIGHT_GRAY) <<"," << Color::Modifier(Color::FG_DEFAULT);
      }
      os.value().get() << Color::Modifier(Color::FG_LIGHT_GRAY) <<json.dump() << Color::Modifier(Color::FG_DEFAULT);
      isFirst = false;
    }

    log.push_back( json );

    if ( log.size() > maxSize) {
      log.erase(log.begin());
    }
  }
  else if ( observable->getObservableType() ==  Execution::Observable::Type::Event ) {
    auto event = static_cast<const Event*>(observable);
    auto json = event->jsonify();

    if (os.has_value()) {
      if ( !isFirst ) {
        os.value().get() << Color::Modifier(Color::FG_LIGHT_CYAN) <<"," << Color::Modifier(Color::FG_DEFAULT);
      }
      os.value().get() << Color::Modifier(Color::FG_LIGHT_CYAN) <<json.dump() << Color::Modifier(Color::FG_DEFAULT);
      isFirst = false;
    }

    log.push_back( json );

    if ( log.size() > maxSize) {
      log.erase(log.begin());
    }
  }

};

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

