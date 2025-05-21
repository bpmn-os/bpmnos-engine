#include <unordered_set>
#include <cassert>

#include "Model.h"
#include "extensionElements/ExtensionElements.h"
#include "extensionElements/Gatekeeper.h"
#include "extensionElements/Timer.h"
#include "extensionElements/Signal.h"
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
  : filename(std::move(filename))
  , folders(std::move(folders))
  , attributeRegistry(limexHandle)
{
  stringRegistry.clear();
  collectionRegistry.clear();
  readBPMNFile(filename);
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

std::unique_ptr<LookupTable> Model::createLookupTable(XML::bpmnos::tTable* table) {
  std::string lookupName = table->getRequiredAttributeByName("name").value;
  std::string source = table->getRequiredAttributeByName("source").value;
  return std::make_unique<LookupTable>(lookupName,source,folders);
}

std::unique_ptr<XML::XMLObject> Model::createRoot(const std::string& filename) {
  auto root = BPMN::Model::createRoot(filename);
  // TODO: make sure that only built in callables exist 
  // create lookup tables
  for ( XML::bpmn::tDataStoreReference& dataStoreReference : root->find<XML::bpmn::tDataStoreReference>() ) {
    auto extensionElements = dataStoreReference.getOptionalChild<XML::bpmn::tExtensionElements>();
    if ( extensionElements.has_value() ) {
      auto tables = extensionElements->get().getOptionalChild<XML::bpmnos::tTables>();
      if ( tables.has_value() ) {
        for ( XML::bpmnos::tTable& table : tables->get().find<XML::bpmnos::tTable>() ) {
          lookupTables.push_back( createLookupTable(&table) );
          auto lookupTable = lookupTables.back().get();
          // register callable
          // TODO: should I use shared pointers?
          limexHandle.add(
            lookupTable->name, 
            [lookupTable](const std::vector<double>& args)
            {
              return lookupTable->at(args);
            }
          );
        }
      }
    }
  }

  // create global variables
  if ( auto collaboration = root->getOptionalChild<XML::bpmn::tCollaboration>();
    collaboration.has_value()
  ) {
    for ( XML::bpmnos::tAttribute& attributeElement : getAttributes(&collaboration.value().get()) ) {
      attributes.push_back( std::make_unique<Attribute>(&attributeElement, Attribute::Category::GLOBAL, attributeRegistry) );
    }
  }


  return root;
}
 
std::unique_ptr<BPMN::Process> Model::createProcess(XML::bpmn::tProcess* process) {
  auto baseElement = BPMN::Model::createProcess(process);
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

  if ( baseElement->represents<BPMN::ReceiveTask>() ) {
    for ( auto& messageDefinition : extensionElements->messageDefinitions ) {
      for ( auto& [_,content] : messageDefinition->contentMap ) {
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
  // bind signal
  return bind<BPMN::FlowNode>(
    BPMN::Model::createSignalStartEvent(startEvent,parent),
    std::make_unique<Signal>(startEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createSignalBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) {
  // bind signal
  return bind<BPMN::FlowNode>(
    BPMN::Model::createSignalBoundaryEvent(boundaryEvent,parent),
    std::make_unique<Signal>(boundaryEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createSignalCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  // bind signal
  return bind<BPMN::FlowNode>(
    BPMN::Model::createSignalCatchEvent(catchEvent,parent),
    std::make_unique<Signal>(catchEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createSignalThrowEvent(XML::bpmn::tThrowEvent* throwEvent, BPMN::Scope* parent) {
  // bind signal
  return bind<BPMN::FlowNode>(
    BPMN::Model::createSignalThrowEvent(throwEvent,parent),
    std::make_unique<Signal>(throwEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createConditionalStartEvent(XML::bpmn::tStartEvent* startEvent, BPMN::Scope* parent) {
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
  
  for ( auto& messageDefinition : extensionElements->messageDefinitions ) {
    for ( auto& [_,content] : messageDefinition->contentMap ) {
      Attribute* attribute = content->attribute;
      auto parentExtension = parent->extensionElements->as<BPMNOS::Model::ExtensionElements>();
      if ( attribute->category == Attribute::Category::GLOBAL ) {
        throw std::runtime_error("Model: Message start event '" + baseElement->id + "' attempts to modify global attribute '" + attribute->id + "'");
      }
      else if ( attribute->category == Attribute::Category::DATA ) {
        if ( !contains(parentExtension->data,attribute) ) {
          throw std::runtime_error("Model: Message start event '" + baseElement->id + "' attempts to modify data attribute '" + attribute->id + "' which is not owned by event-subprocess");
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
  }
  // bind attributes, restrictions, and operators to all event subprocesses
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(extensionElements) );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createMessageBoundaryEvent(boundaryEvent,parent);
  auto extensionElements = std::make_unique<BPMNOS::Model::ExtensionElements>(boundaryEvent,parent->extensionElements->as<ExtensionElements>()->attributeRegistry,parent);
  
  for ( auto& messageDefinition : extensionElements->messageDefinitions ) {
    for ( auto& [_,content] : messageDefinition->contentMap ) {
      Attribute* attribute = content->attribute;
      if ( attribute->category == Attribute::Category::GLOBAL ) {
        throw std::runtime_error("Model: Message boundary event '" + baseElement->id + "' attempts to modify global attribute '" + attribute->id + "'");
      } 
      else if ( attribute->category == Attribute::Category::DATA ) {
        throw std::runtime_error("Model: Message boundary event '" + baseElement->id + "' attempts to modify data attribute '" + attribute->id + "'");
      }
      attribute->isImmutable = false;
    }
  }
  // bind attributes, restrictions, and operators to all event subprocesses
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(extensionElements) );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  auto baseElement = BPMN::Model::createMessageCatchEvent(catchEvent,parent);
  auto extensionElements = std::make_unique<BPMNOS::Model::ExtensionElements>(catchEvent,parent->extensionElements->as<ExtensionElements>()->attributeRegistry,parent);
  
  for ( auto& messageDefinition : extensionElements->messageDefinitions ) {
    for ( auto& [_,content] : messageDefinition->contentMap ) {
      Attribute* attribute = content->attribute;
      if ( attribute->category == Attribute::Category::GLOBAL ) {
        throw std::runtime_error("Model: Message catch event '" + baseElement->id + "' attempts to modify global attribute '" + attribute->id + "'");
      }
      else if ( attribute->category == Attribute::Category::DATA ) {
        throw std::runtime_error("Model: Message catch event '" + baseElement->id + "' attempts to modify data attribute '" + attribute->id + "'");
      }
      attribute->isImmutable = false;
    }
  }
  // bind attributes, restrictions, and operators to all event subprocesses
  return bind<BPMN::FlowNode>( std::move(baseElement), std::move(extensionElements) );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageThrowEvent(XML::bpmn::tThrowEvent* throwEvent, BPMN::Scope* parent) {
  // bind message content
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageThrowEvent(throwEvent,parent),
    std::make_unique<BPMNOS::Model::ExtensionElements>(throwEvent,parent->extensionElements->as<ExtensionElements>()->attributeRegistry,parent)
  );
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
  for ( auto& outgoingMessageDefinition : senderExtension->messageDefinitions ) {
    auto recipientExtension = catchingMessageEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
    for ( auto& incomingMessageDefinition : recipientExtension->messageDefinitions) {

      assert( outgoingMessageDefinition.get() );
      assert( incomingMessageDefinition.get() );

      if ( outgoingMessageDefinition->name != incomingMessageDefinition->name ) {
        continue;
      }
      if ( outgoingMessageDefinition->header != incomingMessageDefinition->header ) {
        continue;
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



