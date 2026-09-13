#include <unordered_set>
#include <cassert>
#include <stdexcept>
#include <utility>
#include <tuple>

#include "Model.h"
#include "extensionElements/ExtensionElements.h"
#include "extensionElements/Gatekeeper.h"
#include "extensionElements/Timer.h"
#include "extensionElements/SignalDefinition.h"
#include "extensionElements/Conditions.h"
#include "extensionElements/MessageDefinition.h"
#include "DecisionTask.h"
#include "SequentialAdHocSubProcess.h"
#include "model/bpmnos/src/xml/bpmnos/tAttributes.h"
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"
#include "model/bpmnos/src/xml/bpmnos/tTables.h"
#include "model/bpmnos/src/xml/bpmnos/tTable.h"
#include "model/utility/src/Keywords.h"

using namespace BPMNOS::Model;

Model::Model(const std::string filename, const std::vector<std::string> folders)
  : attributeRegistry(limexHandle)
  , lookupTableFolders(folders)
{
  root = createRoot(filename);
  build();
}

Model::Model(std::unique_ptr<XML::XMLObject> root, std::unordered_map<std::string, std::string> lookupTableContents)
  : attributeRegistry(limexHandle)
  , lookupTableContents(std::move(lookupTableContents))
{
  this->root = std::move(root);
  build();
}

std::vector<std::reference_wrapper<XML::bpmnos::tAttribute>> Model::getAttributes(XML::bpmn::tBaseElement* baseElement) {
  std::vector<std::reference_wrapper<XML::bpmnos::tAttribute>> attributes;
  if ( baseElement->extensionElements.has_value() ) {
    if ( auto elements = baseElement->extensionElements->get().getOptionalChild<XML::bpmnos::tAttributes>(); elements.has_value()) {
      for ( XML::bpmnos::tAttribute& attribute : elements.value().get().attribute ) {
        attributes.emplace_back( attribute );
      }
    }
  }
  return attributes;
}

std::vector<std::reference_wrapper<XML::bpmnos::tAttribute>> Model::getData(XML::bpmn::tBaseElement* element) {
  std::vector<std::reference_wrapper<XML::bpmnos::tAttribute>> attributes;
  auto dataObjects = element->getChildren<XML::bpmn::tDataObject>();
  for ( XML::bpmn::tDataObject& dataObject : dataObjects ) {
    for ( XML::bpmnos::tAttribute& attribute : getAttributes(&dataObject) ) {
      if ( attributes.size() && attribute.id.value.value == BPMNOS::Keyword::Instance ) {
        // make sure instance attribute is at first position
        attributes.emplace_back( std::move(attributes[0]) );
        attributes[0] = std::ref(attribute);
      }
      else {
        attributes.emplace_back( attribute );
      }
    }
  }
  return attributes;
}

std::vector<std::reference_wrapper<XML::bpmnos::tAttribute>> Model::getGlobals() {
  std::vector<std::reference_wrapper<XML::bpmnos::tAttribute>> attributes;
  for ( auto& dataStore : dataStores ) {
    for ( XML::bpmnos::tAttribute& attribute : getAttributes(dataStore->element) ) {
      if ( attributes.size() && attribute.id.value.value == BPMNOS::Keyword::Objective ) {
        // make sure objective attribute is at first position
        attributes.emplace_back( std::move(attributes[0]) );
        attributes[0] = std::ref(attribute);
      }
      else {
        attributes.emplace_back( attribute );
      }
    }
  }
  return attributes;
}

namespace {

/// @brief Returns the {name, source, header} of every lookup table a data store declares.
/// @throws std::runtime_error if a source contains a path separator (a source must be a bare file name).
std::vector<std::tuple<std::string, std::string, std::string>> lookupTablesOf(const XML::bpmn::tDataStore& dataStore) {
  std::vector<std::tuple<std::string, std::string, std::string>> lookups;
  auto extensionElements = dataStore.getOptionalChild<XML::bpmn::tExtensionElements>();
  if ( !extensionElements.has_value() ) {
    return lookups;
  }
  auto tables = extensionElements->get().getOptionalChild<XML::bpmnos::tTables>();
  if ( !tables.has_value() ) {
    return lookups;
  }
  for ( const XML::bpmnos::tTable& table : tables->get().find<XML::bpmnos::tTable>() ) {
    std::string name = table.getRequiredAttributeByName("name").value;
    std::string source = table.getRequiredAttributeByName("source").value;
    std::string header = table.getRequiredAttributeByName("header").value;
    if ( source.find('/') != std::string::npos || source.find('\\') != std::string::npos ) {
      throw std::runtime_error("Model: lookup table source '" + source + "' must be a file name, not a path");
    }
    lookups.emplace_back( std::move(name), std::move(source), std::move(header) );
  }
  return lookups;
}

} // namespace

std::vector<std::string> Model::getLookupTableNames(const XML::XMLObject& root) {
  std::vector<std::string> names;
  // a data store is a root element, so its declarations are reached without a model having been built
  for ( const XML::bpmn::tDataStore& dataStore : root.getChildren<XML::bpmn::tDataStore>() ) {
    for ( auto& [name, source, header] : lookupTablesOf(dataStore) ) {
      names.push_back( source );
    }
  }
  return names;
}

void Model::createDataStores() {
  BPMN::Model::createDataStores();
  createLookupTables();   // the tables the stores declare, registered as callables
  createGlobals();        // the global attributes the stores declare
}

void Model::createLookupTables() {
  // TODO: make sure that only built in callables exist
  for ( auto& dataStore : dataStores ) {
    for ( auto& [name, source, header] : lookupTablesOf(*dataStore->element) ) {
      if ( lookupTableContents.has_value() ) {
        // content mode: resolve each declared source from the supplied content map
        auto it = lookupTableContents->find(source);
        if ( it == lookupTableContents->end() ) {
          throw std::runtime_error("Model: content for lookup table '" + source + "' not provided");
        }
        lookupTables.push_back( std::make_unique<LookupTable>(name, it->second, header) );
      }
      else {
        // file mode: resolve each declared source against the folders
        lookupTables.push_back( std::make_unique<LookupTable>(name, source, header, lookupTableFolders) );
      }

      auto table = lookupTables.back().get();
      // TODO: should I use shared pointers?
      limexHandle.addFunction(
        table->name,
        [table](const std::vector<double>& args)
        {
          return table->at(args);
        }
      );
    }
  }
}

void Model::createGlobals() {
  for ( XML::bpmnos::tAttribute& attributeElement : getGlobals() ) {
    attributes.push_back( std::make_unique<Attribute>(&attributeElement, Attribute::Category::GLOBAL, attributeRegistry) );
  }
}
 
std::unique_ptr<BPMN::Process> Model::createProcess(XML::bpmn::tProcess* process) {
  auto baseElement = BPMN::Model::createProcess(process);
  if ( !baseElement->isExecutable ) {
    throw std::runtime_error("Model: process '" + baseElement->id + "' must be executable");
  }
  auto extensionElements = std::make_unique<BPMNOS::Model::ExtensionElements>(process, attributeRegistry, nullptr, getData(process) );
  // bind attributes, restrictions, and operators to all processes
  return bind<BPMN::Process>( std::move(baseElement), std::move(extensionElements) );
}

std::unique_ptr<BPMN::EventSubProcess> Model::createEventSubProcess(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createEventSubProcess(subProcess,parent);
  auto extensionElements = std::make_unique<BPMNOS::Model::ExtensionElements>(subProcess, parent->extensionElements->as<ExtensionElements>()->attributeRegistry, parent, getData(subProcess));
  // bind attributes, restrictions, and operators to all event subprocesses
  return bind<BPMN::EventSubProcess>( std::move(baseElement), std::move(extensionElements) );
}

std::unique_ptr<BPMN::FlowNode> Model::createActivity(XML::bpmn::tActivity* activity, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createActivity(activity,parent);
  auto extensionElements = std::make_unique<BPMNOS::Model::ExtensionElements>(activity, parent->extensionElements->as<ExtensionElements>()->attributeRegistry, parent, getData(activity));

  if ( baseElement->represents<BPMN::SendTask>() && !extensionElements->messageDefinition ) {
    throw std::runtime_error("Model: No message defined for send task '" + baseElement->id + "'");
  }

  if ( baseElement->represents<BPMN::ReceiveTask>() ) {
    if ( !extensionElements->messageDefinition ) {
      throw std::runtime_error("Model: No message defined for receive task '" + baseElement->id + "'");
    }
    for ( auto& [_,content] : extensionElements->messageDefinition->contentMap ) {
      auto attribute = content->attribute;
      if ( attribute->category == Attribute::Category::GLOBAL ) {
        throw std::runtime_error("Model: Message received by task '" + baseElement->id + "' attempts to modify global attribute '" + attribute->id + "'");
      }
      else if ( attribute->category == Attribute::Category::DATA ) {
        throw std::runtime_error("Model: Message received by task '" + baseElement->id + "' attempts to modify data attribute '" + attribute->id + "'");
      }
      attribute->isImmutable = false;
    }
  }

  if ( auto adHocSubProcess = baseElement->represents<SequentialAdHocSubProcess>();
    adHocSubProcess && adHocSubProcess->performer == adHocSubProcess
  ) {
    // set flag in case performer is not explicitly provided
    extensionElements->as<BPMNOS::Model::ExtensionElements>()->hasSequentialPerformer = true;
  }

  // bind attributes, restrictions, and operators to all activities
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(extensionElements) );;
}

std::unique_ptr<BPMN::SequenceFlow> Model::createSequenceFlow(XML::bpmn::tSequenceFlow* sequenceFlow, BPMN::Scope* scope) {
  // bind gatekeeper restrictions to all sequence flows
  return bind<BPMN::SequenceFlow>(
    BPMN::Model::createSequenceFlow(sequenceFlow,scope),
    std::make_unique<Gatekeeper>(sequenceFlow,scope)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createAdHocSubProcess(XML::bpmn::tAdHocSubProcess* adHocSubProcess, BPMN::Scope* parent) {
  return std::make_unique<SequentialAdHocSubProcess>(adHocSubProcess,parent);
}

std::unique_ptr<BPMN::FlowNode> Model::createTask(XML::bpmn::tTask* task, BPMN::Scope* parent) {
  if ( const auto& type = task->getOptionalAttributeByName("type"); 
       type.has_value() && type->get().xmlns == "https://bpmnos.telematique.eu" 
  ) {
    if ( type->get().value.value == "Decision" ) {
      // decisions are added with status
      return std::make_unique<DecisionTask>(task,parent);
    }
    else {
      throw std::runtime_error("Model: Illegal type '" + (std::string)type->get().value + "'");
    }   
  }
  return BPMN::Model::createTask(task, parent);
}


std::unique_ptr<BPMN::FlowNode> Model::createTimerStartEvent(XML::bpmn::tStartEvent* startEvent, BPMN::Scope* parent) {
  if ( parent->represents<BPMN::Process>() ) {
    throw std::runtime_error("Model: timer start event of process '" + parent->id + "' is not supported");
  }
  // bind timer
  return bind<BPMN::FlowNode>(
    BPMN::Model::createTimerStartEvent(startEvent,parent),
    std::make_unique<Timer>(startEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createTimerBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) {
  // bind timer
  return bind<BPMN::FlowNode>(
    BPMN::Model::createTimerBoundaryEvent(boundaryEvent,parent),
    std::make_unique<Timer>(boundaryEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createTimerCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  // bind timer
  return bind<BPMN::FlowNode>(
    BPMN::Model::createTimerCatchEvent(catchEvent,parent),
    std::make_unique<Timer>(catchEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createSignalStartEvent(XML::bpmn::tStartEvent* startEvent, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createSignalStartEvent(startEvent,parent);
  auto definition = std::make_unique<SignalDefinition>(startEvent,parent);

  if ( !definition->signal ) {
    throw std::runtime_error("Model: No signal defined for signal start event '" + baseElement->id + "'");
  }

  if ( auto process = parent->represents<BPMN::Process>() ) {
    // the process is instantiated whenever a signal with this name is thrown
    auto [ entry, inserted ] = processesTriggeredBySignal.emplace( definition->name, process );
    if ( !inserted ) {
      throw std::runtime_error("Model: signal '" + BPMNOS::to_string(definition->name,STRING) + "' instantiates process '" + entry->second->id + "' and process '" + process->id + "'");
    }
  }

  // bind signal
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(definition) );
}

std::unique_ptr<BPMN::FlowNode> Model::createSignalBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createSignalBoundaryEvent(boundaryEvent,parent);
  auto definition = std::make_unique<SignalDefinition>(boundaryEvent,parent);

  if ( !definition->signal ) {
    throw std::runtime_error("Model: No signal defined for signal boundary event '" + baseElement->id + "'");
  }

  // bind signal
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(definition) );
}

std::unique_ptr<BPMN::FlowNode> Model::createSignalCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createSignalCatchEvent(catchEvent,parent);
  auto definition = std::make_unique<SignalDefinition>(catchEvent,parent);

  if ( !definition->signal ) {
    throw std::runtime_error("Model: No signal defined for signal catch event '" + baseElement->id + "'");
  }

  // bind signal
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(definition) );
}

std::unique_ptr<BPMN::FlowNode> Model::createSignalThrowEvent(XML::bpmn::tThrowEvent* throwEvent, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createSignalThrowEvent(throwEvent,parent);
  auto definition = std::make_unique<SignalDefinition>(throwEvent,parent);

  if ( !definition->signal ) {
    throw std::runtime_error("Model: No signal defined for signal throw event '" + baseElement->id + "'");
  }

  // bind signal
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(definition) );
}

std::unique_ptr<BPMN::FlowNode> Model::createConditionalStartEvent(XML::bpmn::tStartEvent* startEvent, BPMN::Scope* parent) {
  if ( parent->represents<BPMN::Process>() ) {
    throw std::runtime_error("Model: conditional start event of process '" + parent->id + "' is not supported");
  }
  // bind conditions
  return bind<BPMN::FlowNode>(
    BPMN::Model::createConditionalStartEvent(startEvent,parent),
    std::make_unique<Conditions>(startEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createConditionalBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) {
  // bind conditions
  return bind<BPMN::FlowNode>(
    BPMN::Model::createConditionalBoundaryEvent(boundaryEvent,parent),
    std::make_unique<Conditions>(boundaryEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createConditionalCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  // bind conditions
  return bind<BPMN::FlowNode>(
    BPMN::Model::createConditionalCatchEvent(catchEvent,parent),
    std::make_unique<Conditions>(catchEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageStartEvent(XML::bpmn::tStartEvent* startEvent, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createMessageStartEvent(startEvent,parent);
  auto extensionElements = std::make_unique<BPMNOS::Model::ExtensionElements>(startEvent,parent->extensionElements->as<ExtensionElements>()->attributeRegistry,parent);
  
  // Lambda function to compare unique_ptrs by their raw pointers
  auto contains = [](const std::vector<std::unique_ptr<Attribute>>& attributes, Attribute* attribute) {
    return std::any_of(attributes.begin(), attributes.end(),[attribute](const std::unique_ptr<Attribute>& ptr) {
      return ptr.get() == attribute;
    });
  };
  
  if ( !extensionElements->messageDefinition ) {
    throw std::runtime_error("Model: No message defined for message start event '" + baseElement->id + "'");
  }

  if ( auto process = parent->represents<BPMN::Process>() ) {
    // the process is instantiated whenever a message with this name is thrown
    auto [ entry, inserted ] = processesTriggeredByMessage.emplace( extensionElements->messageDefinition->name, process );
    if ( !inserted ) {
      throw std::runtime_error("Model: message '" + BPMNOS::to_string(extensionElements->messageDefinition->name,STRING) + "' instantiates process '" + entry->second->id + "' and process '" + process->id + "'");
    }
  }

  for ( auto& [_,content] : extensionElements->messageDefinition->contentMap ) {
    Attribute* attribute = content->attribute;
    auto parentExtension = parent->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    if ( attribute->category == Attribute::Category::GLOBAL ) {
      throw std::runtime_error("Model: Message start event '" + baseElement->id + "' attempts to modify global attribute '" + attribute->id + "'");
    }
    else if ( attribute->category == Attribute::Category::DATA ) {
      if ( !contains(parentExtension->data,attribute) ) {
        throw std::runtime_error("Model: Message start event '" + baseElement->id + "' attempts to modify data attribute '" + attribute->id + "' which is not owned by the scope it starts");
      }
      // data attributes owned by event-subprocesses are considered immutable even if they are modified by the message start event
    }
    else if ( attribute->category == Attribute::Category::STATUS ) {
      // status attributes owned by event-subprocesses are considered immutable even if they are modified by the message start event
      if ( !contains(parentExtension->attributes,attribute) ) {
        attribute->isImmutable = false;
      }
    }
  }
  // bind attributes, restrictions, and operators to all event subprocesses
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(extensionElements) );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createMessageBoundaryEvent(boundaryEvent,parent);
  auto extensionElements = std::make_unique<BPMNOS::Model::ExtensionElements>(boundaryEvent,parent->extensionElements->as<ExtensionElements>()->attributeRegistry,parent);
  
  if ( !extensionElements->messageDefinition ) {
    throw std::runtime_error("Model: No message defined for message boundary event '" + baseElement->id + "'");
  }

  for ( auto& [_,content] : extensionElements->messageDefinition->contentMap ) {
    Attribute* attribute = content->attribute;
    if ( attribute->category == Attribute::Category::GLOBAL ) {
      throw std::runtime_error("Model: Message boundary event '" + baseElement->id + "' attempts to modify global attribute '" + attribute->id + "'");
    }
    else if ( attribute->category == Attribute::Category::DATA ) {
      throw std::runtime_error("Model: Message boundary event '" + baseElement->id + "' attempts to modify data attribute '" + attribute->id + "'");
    }
    attribute->isImmutable = false;
  }
  // bind attributes, restrictions, and operators to all event subprocesses
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(extensionElements) );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createMessageCatchEvent(catchEvent,parent);
  auto extensionElements = std::make_unique<BPMNOS::Model::ExtensionElements>(catchEvent,parent->extensionElements->as<ExtensionElements>()->attributeRegistry,parent);
  
  if ( !extensionElements->messageDefinition ) {
    throw std::runtime_error("Model: No message defined for message catch event '" + baseElement->id + "'");
  }

  for ( auto& [_,content] : extensionElements->messageDefinition->contentMap ) {
    Attribute* attribute = content->attribute;
    if ( attribute->category == Attribute::Category::GLOBAL ) {
      throw std::runtime_error("Model: Message catch event '" + baseElement->id + "' attempts to modify global attribute '" + attribute->id + "'");
    }
    else if ( attribute->category == Attribute::Category::DATA ) {
      throw std::runtime_error("Model: Message catch event '" + baseElement->id + "' attempts to modify data attribute '" + attribute->id + "'");
    }
    attribute->isImmutable = false;
  }
  // bind attributes, restrictions, and operators to all event subprocesses
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(extensionElements) );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageThrowEvent(XML::bpmn::tThrowEvent* throwEvent, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createMessageThrowEvent(throwEvent,parent);
  auto extensionElements = std::make_unique<BPMNOS::Model::ExtensionElements>(throwEvent,parent->extensionElements->as<ExtensionElements>()->attributeRegistry,parent);

  if ( !extensionElements->messageDefinition ) {
    throw std::runtime_error("Model: No message defined for message throw event '" + baseElement->id + "'");
  }

  // bind message content
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(extensionElements) );
}

void Model::createMessageFlows() {
  BPMN::Model::createMessageFlows();

  // Messages can only flow between message extensions with the same name and header.
  // If message flows are given in the model, the message flow closest to the meesage
  // event restricts the candidate catching or throwing message events.
  for ( auto& sendingProcess : processes ) {
    // find all throwing message events of the sending process
    auto throwingMessageEvents = sendingProcess->find_all(
      [](const BPMN::Node* node) { return node->represents<BPMN::MessageThrowEvent>();}
    );

    for ( auto& receivingProcess : processes ) {
      // only consider node pairs belonging to different processes
      if ( sendingProcess.get() != receivingProcess.get() ) {
        // find all catching message events of receiving process
        auto catchingMessageEvents = receivingProcess->find_all(
         [](const BPMN::Node* node) { return node->represents<BPMN::MessageCatchEvent>();}
        );

        for ( auto throwingMessageEvent : throwingMessageEvents ) {
          for ( auto catchingMessageEvent : catchingMessageEvents ) {
            createMessageCandidates(sendingProcess.get(), throwingMessageEvent->as<BPMN::FlowNode>(), receivingProcess.get(), catchingMessageEvent->as<BPMN::FlowNode>());
          }
        }
      }

    }
  }

  validateTriggeringMessages();
}

void Model::validateTriggeringMessages() {
  if ( processesTriggeredByMessage.empty() ) {
    return;
  }

  // A message instantiating a process is matched by its name alone, there being no instance to address
  // before the message arrives. The rules below keep that match unambiguous, and keep what the model
  // states about a message and what the engine does with it the same thing. They are checked here because
  // this is the first point at which every process has been read.

  // the standard keys "name", "sender", and "recipient" every message definition holds
  constexpr size_t standardHeaderKeys = size_t(MessageDefinition::Index::Recipient) + 1;

  for ( auto& process : processes ) {
    for ( auto node : process->find_all( [](const BPMN::Node* node) { return node->represents<BPMN::MessageCatchEvent>(); } ) ) {
      auto catchingMessageEvent = node->as<BPMN::FlowNode>();
      auto messageDefinition = catchingMessageEvent->extensionElements->as<ExtensionElements>()->getMessageDefinition();
      auto it = processesTriggeredByMessage.find( messageDefinition->name );
      if ( it == processesTriggeredByMessage.end() ) {
        continue;
      }
      auto name = BPMNOS::to_string(messageDefinition->name,STRING);

      if ( !catchingMessageEvent->represents<BPMN::MessageStartEvent>() || catchingMessageEvent->parent->represents<BPMN::Process>() != it->second ) {
        // the name would be matched by this element as well, and the engine has no decision in which to
        // choose between instantiating the process and delivering the message here
        throw std::runtime_error("Model: message '" + name + "' instantiates process '" + it->second->id + "' and is caught by '" + catchingMessageEvent->id + "'");
      }

      if ( !messageDefinition->parameterMap.empty() ) {
        // the message is matched by its name, so a header parameter would remain unread
        throw std::runtime_error("Model: message start event '" + catchingMessageEvent->id + "' of process '" + it->second->id + "' must not have header parameters");
      }
    }

    for ( auto node : process->find_all( [](const BPMN::Node* node) { return node->represents<BPMN::MessageThrowEvent>(); } ) ) {
      auto throwingMessageEvent = node->as<BPMN::FlowNode>();
      auto messageDefinition = throwingMessageEvent->extensionElements->as<ExtensionElements>()->getMessageDefinition();
      auto it = processesTriggeredByMessage.find( messageDefinition->name );
      if ( it == processesTriggeredByMessage.end() ) {
        continue;
      }
      auto name = BPMNOS::to_string(messageDefinition->name,STRING);

      if ( process.get() == it->second ) {
        // a message flow connects two participants, so a process cannot send itself a message
        throw std::runtime_error("Model: message '" + name + "' instantiating process '" + it->second->id + "' must not be thrown by '" + throwingMessageEvent->id + "' of that process");
      }

      if ( messageDefinition->header.size() > standardHeaderKeys ) {
        // the start event has no header parameters, so a header key here would prevent the two from ever
        // becoming candidates of each other while the message instantiated the process regardless
        throw std::runtime_error("Model: message '" + name + "' thrown by '" + throwingMessageEvent->id + "' instantiates process '" + it->second->id + "' and must not have header parameters");
      }

      if ( messageDefinition->parameterMap.contains("recipient") ) {
        // the instance is created by this message and cannot have been named before it is thrown
        throw std::runtime_error("Model: message '" + name + "' thrown by '" + throwingMessageEvent->id + "' instantiates process '" + it->second->id + "' and must not state a recipient");
      }
    }
  }
}

std::vector<BPMN::MessageFlow*>& Model::determineMessageFlows(BPMN::FlowNode* messageEvent, auto getMessageFlows) {
  auto& relevantFlows = getMessageFlows(messageEvent);
  if ( relevantFlows.empty() ) {
    BPMN::ChildNode* node = messageEvent;
    BPMN::Scope* scope = nullptr;
    do {
      // get next scope that may have message flows
      scope = node->parent;
      while ( auto eventSubProcess = scope->represents<BPMN::EventSubProcess>() ) {
        // skip event-subprocesses
        node = eventSubProcess;
        scope = eventSubProcess->parent;
      }

      relevantFlows = getMessageFlows(scope);
      node = scope->represents<BPMN::SubProcess>();

    } while ( relevantFlows.empty() && node);
  }
  return relevantFlows;
}
bool Model::messageMayBeCaught( [[maybe_unused]] BPMN::Process* sendingProcess, BPMN::FlowNode* throwingMessageEvent, BPMN::Process* receivingProcess, BPMN::FlowNode* catchingMessageEvent ) {
  // determine relevant message flows for throwing events
  auto& outgoingMessageFlows = determineMessageFlows(
    throwingMessageEvent, 
    [](BPMN::Node* node) -> std::vector<BPMN::MessageFlow*>& {
      return node->sending;
    }
  );

  if ( outgoingMessageFlows.empty() ) {
    // no message flow is provided that imposes a restriction
    return true;
  }

  // determine whether catching message event is in message flow target
  bool found = false;
  for ( auto messageFlow : outgoingMessageFlows ) {
    auto& [process,flowNode] = messageFlow->target;
    if ( process == receivingProcess ) {
      if ( flowNode == catchingMessageEvent ) {
        return true;
      }
      else {
        if ( flowNode ) {
          found = flowNode->find_all(
            [catchingMessageEvent](const BPMN::Node* node) { return node == catchingMessageEvent;}
          ).size();
        }
        else {
          found = process->find_all(
            [catchingMessageEvent](const BPMN::Node* node) { return node == catchingMessageEvent;}
          ).size();
        }
      }
      if ( found ) {
        return true;
      }
    }
  }
  return false;
}

bool Model::messageMayBeThrown( BPMN::Process* sendingProcess, BPMN::FlowNode* throwingMessageEvent, [[maybe_unused]] BPMN::Process* receivingProcess, BPMN::FlowNode* catchingMessageEvent ) {
  // determine relevant message flows for catching event
  auto incomingMessageFlows = determineMessageFlows(
    catchingMessageEvent, 
    [](BPMN::Node* node) -> std::vector<BPMN::MessageFlow*>& {
      return node->receiving;
    }
  );

  if ( incomingMessageFlows.empty() ) {
    // no message flow is provided that imposes a restriction
    return true;
  }

  // determine whether throwing message event is in message flow source
  bool found = false;
  for ( auto messageFlow : incomingMessageFlows ) {
    auto& [process,flowNode] = messageFlow->source;
    if ( process == sendingProcess ) {
      if ( flowNode == throwingMessageEvent ) {
        return true;
      }
      else {
        if ( flowNode ) {
          found = flowNode->find_all(
            [throwingMessageEvent](const BPMN::Node* node) { return node == throwingMessageEvent;}
          ).size();
        }
        else {
          found = process->find_all(
            [throwingMessageEvent](const BPMN::Node* node) { return node == throwingMessageEvent;}
          ).size();
        }
        if ( found ) {
          return true;
        }
      }
    }
  }
  return false;
}

void Model::createMessageCandidates( BPMN::Process* sendingProcess, BPMN::FlowNode* throwingMessageEvent, BPMN::Process* receivingProcess, BPMN::FlowNode* catchingMessageEvent ) {
  auto senderExtension = throwingMessageEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
  auto recipientExtension = catchingMessageEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>();

  auto& outgoingMessageDefinition = senderExtension->messageDefinition;
  auto& incomingMessageDefinition = recipientExtension->messageDefinition;

  assert( outgoingMessageDefinition.get() );
  assert( incomingMessageDefinition.get() );

  if ( outgoingMessageDefinition->name != incomingMessageDefinition->name ) {
    return;
  }
  if ( outgoingMessageDefinition->header != incomingMessageDefinition->header ) {
    return;
  }

  if ( messageMayBeCaught(sendingProcess, throwingMessageEvent, receivingProcess, catchingMessageEvent) &&
    messageMayBeThrown(sendingProcess, throwingMessageEvent, receivingProcess, catchingMessageEvent)
  ) {
    // add message events to collection of candidates of each other
    if( find(
        senderExtension->messageCandidates.begin(),
        senderExtension->messageCandidates.end(),
        catchingMessageEvent->as<BPMN::FlowNode>()
      ) == senderExtension->messageCandidates.end()
    ) {
//std::cerr << throwingMessageEvent->id << " -> " << catchingMessageEvent->id << std::endl;
      senderExtension->messageCandidates.push_back(catchingMessageEvent->as<BPMN::FlowNode>());
    }

    if( find(
        recipientExtension->messageCandidates.begin(),
        recipientExtension->messageCandidates.end(),
        throwingMessageEvent->as<BPMN::FlowNode>()
      ) == recipientExtension->messageCandidates.end()
    ) {
//std::cerr << throwingMessageEvent->id << " -> " << catchingMessageEvent->id << std::endl;
      recipientExtension->messageCandidates.push_back(throwingMessageEvent->as<BPMN::FlowNode>());
    }
  }
}

bool Model::hasSequentialPerformer(const std::vector< std::reference_wrapper<XML::bpmn::tResourceRole> >& resources) {
  for ( auto& resource : resources ) {
    if ( auto performer = resource.get().get<XML::bpmn::tPerformer>();
      performer && performer->name.has_value() && performer->name.value().get().value.value == "Sequential"
    ) {
      return true;
    } 
  }
  return false;
}



