# Observer
@page observer Observer

An @ref BPMNOS::Execution::Observer "observer" connects to the @ref BPMNOS::Execution::Engine "execution engine" to watch a run without influencing it. It implements a single method, @ref BPMNOS::Execution::Observer::notice "notice", which the engine calls with an @ref BPMNOS::Execution::Observable "observable", and it receives only the kinds of observable it subscribes to. An observer makes no decisions; a component that does is a @ref controller "controller".

## What can be observed

A @ref BPMNOS::Execution::Token "token" is reported whenever its @ref BPMNOS::Execution::Token::state "state" changes, which is how the progress of a run is followed.

A @ref BPMNOS::Execution::Message "message" is reported when it is created by a throwing element, when it is delivered to a recipient, and when it is withdrawn because neither can happen any more. Its header and its content are reported with it.

A @ref BPMNOS::Execution::Signal "signal" is reported when it is broadcast, whether it was thrown within the model or raised by the environment, and it is reported once however many elements receive it and also when none does. It names the signal and the content it carries. Unlike a message it names no sender, a signal being broadcast without correlation, so a model that wants the origin known declares it as part of the content.

A @ref BPMNOS::Execution::DataUpdate "data update" is reported whenever attribute values change, naming the instance and the attributes concerned, or naming no instance where the values are global. It is the mechanism by which the engine itself learns of changes, driving conditional events and the invalidation of cached candidate evaluations.

An @ref BPMNOS::Execution::Event "event" is reported when the engine processes it, a @ref BPMNOS::Execution::DecisionRequest "decision request" when the engine needs a decision of a given kind, and a @ref BPMNOS::Execution::SequentialPerformerUpdate "sequential performer update" when a performer becomes busy or idle. The @ref BPMNOS::Execution::SystemState "system state" is reported when one is installed, which lets an observer holding state of its own rebuild from it.

## The recorder

The @ref BPMNOS::Execution::Recorder "recorder" is the observer the tests use. It keeps a JSON log of what it notices and optionally writes it to a stream as the run proceeds. Which kinds it records is configurable, and by default it records tokens, events, messages and signals. Each entry is the observable rendered as JSON, and @ref BPMNOS::Execution::Recorder::find "find" selects the entries matching a set of keys and values, which is how a test states what a run must have done.

An observer must be subscribed before a run starts, because what nobody noticed is not kept and cannot be asked for afterwards.
