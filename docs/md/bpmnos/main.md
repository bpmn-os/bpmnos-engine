# Model provider
@page bpmnos Model provider

The @ref BPMNOS::Model::Model class can be used to read a BPMN file with respective extension elements for optimisation and simulation. All visual elements of a BPMN process or collaboration diagram can be read, however, not all elements are considered by the @ref BPMNOS::Execution::Engine "execution engine".

## BPMN elements

### @ref BPMN::Process "Processes"
Processes are assumed to have exactly one @ref BPMN::UntypedStartEvent "untyped start event" and must be executable (i.e. `isExecutable = true`). 
@attention 
- @ref BPMN::TypedStartEvent "Typed start events" and multiple start events are **not** supported.
- Empty pools are **not** supported, in particular, each @ref XML::bpmn::tParticipant "participant" in a @ref XML::bpmn::tCollaboration "collaboration" must have a reference to a process.

### @ref BPMN::Task "Tasks"

Tasks may have parallel and sequential @ref XML::bpmn::tMultiInstanceLoopCharacteristics "multi-instance markers".

- @ref BPMN::AbstractTask "Abstract tasks"
  @par
  Tasks without type marker in the upper left corner of the task shape, are called @ref BPMN::AbstractTask "abstract tasks".
- @ref BPMNOS::Model::DecisionTask "Decision tasks"
  @todo

  @note The @ref BPMNOS::Model::DecisionTask class is an extension to the BPMN standard.
- @ref BPMN::ReceiveTask "Receive tasks" 
  @todo
  
- @ref BPMN::SendTask "Send tasks" 
  @todo

.
@attention
- @ref BPMN::BusinessRuleTask "Business rule tasks" are treated like @ref BPMN::AbstractTask "abstract tasks".
- @ref BPMN::ManualTask "Manual tasks" are treated like @ref BPMN::AbstractTask "abstract tasks".
- @ref BPMN::ScriptTask "Script tasks" are treated like @ref BPMN::AbstractTask "abstract tasks".
- @ref BPMN::UserTask "User tasks" are treated like @ref BPMN::AbstractTask "abstract tasks".
- @ref XML::bpmn::tStandardLoopCharacteristics "Loop markers" are not supported.


### @ref BPMN::SubProcess "Subprocesses"
Subprocesses are assumed to have exactly one @ref BPMN::UntypedStartEvent "untyped start event".
They may have parallel and sequential @ref XML::bpmn::tMultiInstanceLoopCharacteristics "multi-instance markers".

@attention 
- @ref XML::bpmn::tStandardLoopCharacteristics "Loop markers" are **not** supported.
- @ref BPMN::Transaction "Transactions" are treated like regular @ref BPMN::SubProcess "subprocesses".

### @ref BPMN::AdHocSubProcess "Ad-hoc subprocesses" 
Ad-hoc subprocesses must not have any events or gateways without incoming sequence flow.
They may have parallel and sequential @ref XML::bpmn::tMultiInstanceLoopCharacteristics "multi-instance markers".
Furthermore, ad-hoc subprocesses are expected to have a sequential ordering (i.e. `ordering = "Sequential"`), indicating that activities within the scope of the ad-hoc subprocess must not be executed in parallel. For each ad-hoc subprocess, the model provider iterates through the nodes in the XML tree, starting from the ad-hoc subprocess, and goes up the hierarchy until it finds another ad-hoc subprocess or a @ref BPMN::Scope "scope" that contains a @ref XML::bpmn::tPerformer "tPerformer" element with `name = "Sequential"`. If the latter is found, the ad-hoc subprocess is associated to this performer, and all activities of any ad-hoc subprocess associated to this performer must be conducted in sequential order. Otherwise, the sequential ordering only applies to the activities within the ad-hoc subprocess.
  @note The @ref BPMN::AdHocSubProcess class is derived from @ref BPMN::Activity and not from @ref BPMN::SubProcess.

@par
@attention
- @ref XML::bpmn::tStandardLoopCharacteristics "Loop markers" are **not** supported.

### @ref BPMN::CallActivity "Call activities"

@attention
- Call activities are **not** supported.


### Compensation activities
@todo

### @ref BPMN::EventSubProcess "Event-subprocesses"
Event-subprocesses are assumed to have exactly one @ref BPMN::TypedStartEvent "typed start event".
@note The @ref BPMN::EventSubProcess class is derived from @ref BPMN::Scope and not from @ref BPMN::SubProcess.


### @ref BPMN::Gateway "Gateways" 

Gateways supported are @ref BPMN::ExclusiveGateway "exclusive gateways", @ref BPMN::ParallelGateway "parallel gateways", @ref BPMN::InclusiveGateway "inclusive gateways", and @ref BPMN::EventBasedGateway "event-based gateways". Inclusive gateways are assumed to be diverging.

@par
@attention
- Converging @ref BPMN::InclusiveGateway "inclusive gateways" are **not** supported.
- @ref BPMN::ComplexGateway "Complex gateways" are **not** supported.

### @ref BPMN::CatchEvent "Catch events" 
- @ref BPMN::UntypedStartEvent "Untyped start events"
- @ref BPMN::TypedStartEvent "Typed start events" 
  - @ref BPMN::MessageStartEvent "Message start events" 
  - @ref BPMN::ConditionalStartEvent "Conditional start events" 
  - @ref BPMN::SignalStartEvent "Signal start events" 
  - @ref BPMN::ErrorStartEvent "Error start events" 
  - @ref BPMN::EscalationStartEvent "Escalation start events" 
  - @ref BPMN::TimerStartEvent "Timer start events" 
- Intermediate events
  - @ref BPMN::SignalEvent "Signal events" 
  - @ref BPMN::ConditionalEvent "Conditional events" 
  - @ref BPMN::CancelEvent "Cancel events"
- @ref BPMN::BoundaryEvent "Boundary events"

.
@attention
- Multiple @ref XML::bpmn::tEventDefinition "event definitions" are **not** supported.

### @ref BPMN::ThrowEvent "Throw events" 

.
@attention
- Multiple @ref XML::bpmn::tEventDefinition "event definitions" are **not** supported.

### @ref BPMN::SequenceFlow "Sequence flows"

### @ref BPMN::MessageFlow "Message flows"

### Others


- Data stores
- Data inputs
- Data outputs
- Data associations

## BPMN extension


### Attributes
- Status attributes
- Data attributes
- Global attributes

### Restrictions
- Enumeration
- Null condition
- String expression
- Generic expressions
- Linear expressions

### Operators
- Assign
- Unassign
- Expression
- Lookup

### Messages
- Parameter
- Content

### Gatekeeper
- Expression

### Timer
