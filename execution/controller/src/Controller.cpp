#include "Controller.h"

using namespace BPMNOS::Execution;

Controller::Controller()
{
}

void Controller::connect(Mediator* mediator) {
  mediator->addSubscriber(this,
    Observable::Type::Message,
    Observable::Type::SequentialPerformerUpdate,
    Observable::Type::DataUpdate,
    Observable::Type::EntryRequest,
    Observable::Type::ChoiceRequest,
    Observable::Type::ExitRequest,
    Observable::Type::MessageDeliveryRequest
  );
  EventDispatcher::connect(mediator);
}

void Controller::notice(const Observable* observable) {
  // forward observables
  notify(observable);
}

