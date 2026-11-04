#include "Recorder.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/Signal.h"
#include "execution/engine/src/Objective.h"
#include "execution/engine/src/DecisionRequest.h"
#include "execution/engine/src/MessageDeliveryRequest.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"

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
  if ( config.signal ) {
    engine->addSubscriber(this, Execution::Observable::Type::Signal);
  }
  if ( config.objective ) {
    engine->addSubscriber(this, Execution::Observable::Type::Objective);
  }
  if ( config.request ) {
    engine->addSubscriber(this, Execution::Observable::Type::EntryRequest);
    engine->addSubscriber(this, Execution::Observable::Type::ChoiceRequest);
    engine->addSubscriber(this, Execution::Observable::Type::MessageDeliveryRequest);
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
  else if ( observable->getObservableType() ==  Execution::Observable::Type::Signal ) {
    auto signal = static_cast<const Signal*>(observable);
    record( signal->jsonify(), "signal", Color::FG_YELLOW );
  }
  else if ( observable->getObservableType() ==  Execution::Observable::Type::Objective ) {
    auto objective = static_cast<const Objective*>(observable);
    record( objective->jsonify(), "objective", Color::FG_LIGHT_GREEN );
  }
  else if ( observable->getObservableType() ==  Execution::Observable::Type::EntryRequest ) {
    auto request = static_cast<const DecisionRequest*>(observable);
    record( request->token->jsonify(), "entryRequest", Color::FG_LIGHT_MAGENTA );
  }
  else if ( observable->getObservableType() ==  Execution::Observable::Type::ChoiceRequest ) {
    auto request = static_cast<const DecisionRequest*>(observable);
    record( request->token->jsonify(), "choiceRequest", Color::FG_LIGHT_MAGENTA );
  }
  else if ( observable->getObservableType() ==  Execution::Observable::Type::MessageDeliveryRequest ) {
    // the token awaiting a delivery, with the senders it accepts and the header it expects, so that a reader
    // of the log matches a message against them as the engine does
    auto request = static_cast<const MessageDeliveryRequest*>(observable);
    auto jsonObject = request->token->jsonify();
    jsonObject["senders"] = nlohmann::ordered_json::array();
    auto extensionElements = request->token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    for ( auto sender : extensionElements->messageCandidates ) {
      jsonObject["senders"].push_back(sender->id);
    }
    auto requestObject = request->jsonify();
    jsonObject["recipientHeader"] = requestObject.contains("header") ? requestObject["header"] : nlohmann::ordered_json::object();
    record( jsonObject, "messageDeliveryRequest", Color::FG_LIGHT_MAGENTA );
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

