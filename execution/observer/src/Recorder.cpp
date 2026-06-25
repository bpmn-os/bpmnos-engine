#include "Recorder.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/SystemState.h"

using namespace BPMNOS::Execution;

Recorder::Recorder(Config config) : isFirst(true), config(config)
{
  log = nlohmann::ordered_json::array();
  emit("[", Color::FG_LIGHT_GRAY);
}

Recorder::~Recorder()
{
  emit("]", Color::FG_LIGHT_GRAY);
  if (config.stream.has_value()) {
    config.stream.value().get() << std::endl;
  }
}

void Recorder::emit(const std::string& text, Color::Code color)
{
  if (!config.stream.has_value()) {
    return;
  }
  auto& stream = config.stream.value().get();
  if (config.colored) {
    stream << Color::Modifier(color) << text << Color::Modifier(Color::FG_DEFAULT);
  }
  else {
    stream << text;
  }
}

void Recorder::record(const nlohmann::ordered_json& json, const std::string& type, Color::Code color)
{
  nlohmann::ordered_json item;
  if (config.tagged) {
    item[type] = json;
  }
  else {
    item = json;
  }

  if (config.stream.has_value()) {
    if (!isFirst) {
      emit(",", color);
    }
    emit(item.dump(), color);
    isFirst = false;
  }

  log.push_back( std::move(item) );

  if ( log.size() > config.maxSize) {
    log.erase(log.begin());
  }
}

void Recorder::inject(const std::string& tag, const nlohmann::ordered_json& json) {
  record( json, tag, Color::FG_WHITE );
}

void Recorder::subscribe(Engine* engine) {
  if ( config.token ) {
    engine->addSubscriber(this, Execution::Observable::Type::Token);
  }
  if ( config.event ) {
    engine->addSubscriber(this, Execution::Observable::Type::Event);
  }
  if ( config.message ) {
    engine->addSubscriber(this, Execution::Observable::Type::Message);
  }
}

void Recorder::notice(const Observable* observable) {
  if ( observable->getObservableType() ==  Execution::Observable::Type::Token ) {
    auto token = static_cast<const Token*>(observable);
    record( token->jsonify(), "token", Color::FG_LIGHT_GRAY );
  }
  else if ( observable->getObservableType() ==  Execution::Observable::Type::Event ) {
    auto event = static_cast<const Event*>(observable);
    record( event->jsonify(), "event", Color::FG_LIGHT_CYAN );
  }
  else if ( observable->getObservableType() ==  Execution::Observable::Type::Message ) {
    auto message = static_cast<const Message*>(observable);
    record( message->jsonify(), "message", Color::FG_LIGHT_YELLOW );
  }

};

nlohmann::ordered_json Recorder::find(nlohmann::json include, nlohmann::json exclude) const {
  nlohmann::ordered_json result = nlohmann::ordered_json::array();
  std::copy_if(
    log.begin(),
    log.end(),
    std::back_inserter(result), [&include,&exclude](const nlohmann::json& item) {
      for ( auto& [key,value] : include.items() ) {
        if ( !item.contains(key) || !(value.is_null() || item[key] == value) ) {
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

