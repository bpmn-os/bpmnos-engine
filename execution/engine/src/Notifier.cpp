#include "Notifier.h"

using namespace BPMNOS::Execution;

Notifier::Notifier() {
  subscribers.resize((size_t)Observable::Type::Count);
}

void Notifier::notify(const Observable* observable) const {
  for ( auto subscriber : subscribers[(size_t)observable->getObservableType()] ) {
    subscriber->notice(observable);
  }
}

void Notifier::notify(const Observable& observable) const {
  notify(&observable);
}
