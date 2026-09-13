#ifndef BPMNOS_Execution_Signal_H
#define BPMNOS_Execution_Signal_H

#include "Observable.h"
#include "model/utility/src/Value.h"
#include <nlohmann/json.hpp>

namespace BPMNOS::Execution {

/**
 * @brief A signal that is broadcast, whether thrown within the model or raised by the environment.
 *
 * The counterpart of @ref Message for signals, and deliberately far smaller. A message is an object with a
 * lifetime: it is owned by @ref SystemState::messages, referred to weakly by the containers that route it,
 * and moves from @ref Message::State::CREATED to delivery or withdrawal. A signal has none of that. It is
 * broadcast to whoever is listening, kept nowhere, and over once it has been delivered, so it is a value
 * that exists for the duration of its broadcast, as a @ref DataUpdate is.
 *
 * It holds no sender. A signal is broadcast without correlation, which is what distinguishes it from a
 * message, and a model that wants the thrower known declares it as part of the @ref content.
 *
 * The content is fixed when the signal arises and is never read again from whoever raised it, so that a
 * recipient writing a global cannot change what a later recipient obtains.
 */
class Signal : public Observable {
public:
  constexpr Type getObservableType() const override { return Type::Signal; };
  Signal(BPMNOS::number name, BPMNOS::VariedValueMap content);

  BPMNOS::number name;            ///< Signal name
  BPMNOS::VariedValueMap content; ///< Content the signal carries

  nlohmann::ordered_json jsonify() const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Signal_H
