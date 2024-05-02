# BPMN elements
@page elements BPMN elements

## @ref BPMN::Process "Processes"
![Process](BPMN/Process.svg)
@par
Processes are assumed to have exactly one @ref BPMN::UntypedStartEvent "untyped start event" and must be executable (i.e. `isExecutable = true`).
@attention
- @ref BPMN::TypedStartEvent "Typed start events" and multiple start events are **not** supported.
- Empty pools are **not** supported, in particular, each @ref XML::bpmn::tParticipant "participant" in a @ref XML::bpmn::tCollaboration "collaboration" must have a reference to a process.

## @ref BPMN::Activity "Activities"

### @ref BPMN::Task "Tasks"

Tasks may have parallel and sequential @ref XML::bpmn::tMultiInstanceLoopCharacteristics "multi-instance markers".

- @ref BPMN::AbstractTask "Abstract tasks"
  @par
  ![AbstractTask](BPMN/AbstractTask.svg)
  @par
  Tasks without type marker in the upper left corner of the task shape, are called @ref BPMN::AbstractTask "abstract tasks".
- @ref BPMNOS::Model::DecisionTask "Decision tasks"
  @par
  ![DecisionTask](BPMNOS/DecisionTask.svg)
  @par
  Tasks with a branching arrow symbol in the upper left corner of the task shape, are called @ref BPMNOS::Model::DecisionTask "decision tasks".
  Decision tasks can be used to indicate that a decision has to be made. Unlike, @ref BPMN::BusinessRuleTask "business rule tasks", @ref BPMN::ScriptTask "script tasks", @ref BPMN::UserTask "user tasks" or @ref BPMN::ManualTask "manual tasks", decision tasks do neither prescribe how the task is performed and whether the task is performed by a human or not.
  @par
  @note The @ref BPMNOS::Model::DecisionTask class is an extension to the BPMN standard. In the XML model they are represented by a `<task>` element with an attribute `bpmnos::type = "Decision"`.
- @ref BPMN::ReceiveTask "Receive tasks"
  @par
  ![ReceiveTask](BPMN/ReceiveTask.svg)
  @par
  Tasks with a white envelope symbol in the upper left corner of the task shape, are called @ref BPMN::ReceiveTask "receive tasks".
  Receive tasks are similar to @ref BPMN::MessageCatchEvent "message catch events", however, they may have multi-instance markers and events attached to the boundary of the task.
- @ref BPMN::SendTask "Send tasks"
  @par
  ![SendTask](BPMN/SendTask.svg)
  @par
  Tasks with a black envelope symbol in the upper left corner of the task shape, are called @ref BPMN::SendTask "send tasks".
  Send tasks are similar to @ref BPMN::MessageThrowEvent "message throw events", however, they may have multi-instance markers and events attached to the boundary of the task. Furthermore, a token at a send task may only proceed when the message is delivered to the recipient.
  @note It is assumed that the completion of a send task requires that the message is delivered to the recipient. 

.
@attention
- @ref BPMN::BusinessRuleTask "Business rule tasks" are treated like @ref BPMN::AbstractTask "abstract tasks".
- @ref BPMN::ManualTask "Manual tasks" are treated like @ref BPMN::AbstractTask "abstract tasks".
- @ref BPMN::ScriptTask "Script tasks" are treated like @ref BPMN::AbstractTask "abstract tasks".
- @ref BPMN::UserTask "User tasks" are treated like @ref BPMN::AbstractTask "abstract tasks".
- @ref XML::bpmn::tStandardLoopCharacteristics "Loop markers" are not supported.


### @ref BPMN::SubProcess "Subprocesses"
![SubProcess](BPMN/SubProcess.svg)
@par
Subprocesses are assumed to have exactly one @ref BPMN::UntypedStartEvent "untyped start event".
They may have parallel and sequential @ref XML::bpmn::tMultiInstanceLoopCharacteristics "multi-instance markers".

@attention
- @ref XML::bpmn::tStandardLoopCharacteristics "Loop markers" are **not** supported.
- @ref BPMN::Transaction "Transactions" are treated like regular @ref BPMN::SubProcess "subprocesses".
- @ref BPMN::CallActivity "Call activities" are **not** supported.

### @ref BPMN::AdHocSubProcess "Ad-hoc subprocesses"
![AdHocSubProcess](BPMN/AdHocSubProcess.svg)
@par
Ad-hoc subprocesses must not contain any events or gateways without incoming sequence flow.
They may have parallel and sequential @ref XML::bpmn::tMultiInstanceLoopCharacteristics "multi-instance markers".
Furthermore, ad-hoc subprocesses are expected to have a sequential ordering (i.e. `ordering = "Sequential"`), indicating that activities within the scope of the ad-hoc subprocess must not be executed in parallel. For each ad-hoc subprocess, the model provider iterates through the nodes in the XML tree, starting from the ad-hoc subprocess, and goes up the hierarchy until it finds another ad-hoc subprocess or a @ref BPMN::Scope "scope" that contains a @ref XML::bpmn::tPerformer "tPerformer" element with `name = "Sequential"`. If the latter is found, the ad-hoc subprocess is associated to this performer, and all activities of any ad-hoc subprocess associated to this performer must be conducted in sequential order. Otherwise, the sequential ordering only applies to the activities within the ad-hoc subprocess.
  @note The @ref BPMN::AdHocSubProcess class is derived from @ref BPMN::Activity and not from @ref BPMN::SubProcess.

@par
@attention
- @ref XML::bpmn::tStandardLoopCharacteristics "Loop markers" are **not** supported.

### Compensation activities
@todo




## @ref BPMN::EventSubProcess "Event-subprocesses"
![EventSubProcess](BPMN/EventSubProcess.svg)
@par
Event-subprocesses are assumed to have exactly one @ref BPMN::TypedStartEvent "typed start event".
@note The @ref BPMN::EventSubProcess class is derived from @ref BPMN::Scope and not from @ref BPMN::SubProcess or @ref BPMN::Activity.


## @ref BPMN::Gateway "Gateways"
![ExclusiveGateway](BPMN/ExclusiveGateway.svg)
![ParallelGateway](BPMN/ParallelGateway.svg)
![InclusiveGateway](BPMN/InclusiveGateway.svg)
![EventBasedGateway](BPMN/EventBasedGateway.svg)
@par
Supported gateways are @ref BPMN::ExclusiveGateway "exclusive gateways", @ref BPMN::ParallelGateway "parallel gateways", @ref BPMN::InclusiveGateway "inclusive gateways", and @ref BPMN::EventBasedGateway "event-based gateways". Inclusive gateways are assumed to be diverging.

@par
@attention
- Converging @ref BPMN::InclusiveGateway "inclusive gateways" are **not** supported.
- @ref BPMN::ComplexGateway "Complex gateways" are **not** supported.

## @ref BPMN::CatchEvent "Catch events"
- @ref BPMN::UntypedStartEvent "Untyped start events"
  @par
  ![UntypedStartEvent](BPMN/UntypedStartEvent.svg)
- @ref BPMN::TypedStartEvent "Typed start events"
  - @ref BPMN::MessageStartEvent "Message start events"
    @par
    ![MessageStartEvent](BPMN/MessageStartEvent.svg)
    ![Non-interrupting MessageStartEvent](BPMN/MessageStartEvent_non-interrupting.svg)
  - @ref BPMN::TimerStartEvent "Timer start events"
    @par
    ![TimerStartEvent](BPMN/TimerStartEvent.svg)
    ![Non-interrupting TimerStartEvent](BPMN/TimerStartEvent_non-interrupting.svg)
  - @ref BPMN::EscalationStartEvent "Escalation start events"
    @par
    ![EscalationStartEvent](BPMN/EscalationStartEvent.svg)
    ![Non-interrupting EscalationStartEvent](BPMN/EscalationStartEvent_non-interrupting.svg)
  - @ref BPMN::ErrorStartEvent "Error start events"
    @par
    ![ErrorStartEvent](BPMN/ErrorStartEvent.svg)
  - @ref BPMN::CompensateStartEvent "Compensate start events"
    @par
    ![CompensateStartEvent](BPMN/CompensateStartEvent.svg)
  
  .
  @attention
  - @ref BPMN::ConditionalStartEvent "Conditional start events" are **not** supported.
  - @ref BPMN::SignalStartEvent "Signal start events" are **not** supported.

- Intermediate events
  - @ref BPMN::MessageCatchEvent "Message catch events"
    @par
    ![IntermediateMessageCatchEvent](BPMN/IntermediateMessageCatchEvent.svg)
  - @ref BPMN::TimerCatchEvent "Timer catch events"
    @par
    ![IntermediateTimerCatchEvent](BPMN/IntermediateTimerCatchEvent.svg)
  - @ref BPMN::LinkTargetEvent "Link target events"
    @par
    ![LinkTargetEvent](BPMN/LinkTargetEvent.svg)
    
  .
  @attention
  - @ref BPMN::ConditionalCatchEvent "Conditional catch events" are **not** supported.
  - @ref BPMN::SignalCatchEvent "Signal catch events" are **not** supported.
- @ref BPMN::BoundaryEvent "Boundary events"
  - @ref BPMN::MessageBoundaryEvent "Message boundary events"
    @par
    ![MessageBoundaryEvent](BPMN/MessageBoundaryEvent.svg)
    ![Non-interrupting MessageBoundaryEvent](BPMN/MessageBoundaryEvent_non-interrupting.svg)
  - @ref BPMN::TimerBoundaryEvent "Timer boundary events"
    @par
    ![TimerBoundaryEvent](BPMN/TimerBoundaryEvent.svg)
    ![Non-interrupting TimerBoundaryEvent](BPMN/TimerBoundaryEvent_non-interrupting.svg)
  - @ref BPMN::EscalationBoundaryEvent "Escalation boundary events"
    @par
    ![EscalationBoundaryEvent](BPMN/EscalationBoundaryEvent.svg)
    ![Non-interrupting EscalationBoundaryEvent](BPMN/EscalationBoundaryEvent_non-interrupting.svg)
  - @ref BPMN::ErrorBoundaryEvent "Error boundary events"
    @par
    ![ErrorBoundaryEvent](BPMN/ErrorBoundaryEvent.svg)
  - @ref BPMN::CompensateBoundaryEvent "Compensate boundary events"
    @par
    ![CompensateBoundaryEvent](BPMN/CompensateBoundaryEvent.svg)

  .
  @attention
  - @ref BPMN::ConditionalBoundaryEvent "Conditional boundary events" are **not** supported.
  - @ref BPMN::SignalBoundaryEvent "Signal boundary events" are **not** supported.

.
@attention
- Multiple @ref XML::bpmn::tEventDefinition "event definitions" are **not** supported.

## @ref BPMN::ThrowEvent "Throw events"

- Intermediate events
  - @ref BPMN::MessageThrowEvent "Message throw events"
    @par
    ![IntermediateMessageThrowEvent](BPMN/IntermediateMessageThrowEvent.svg)
  - @ref BPMN::EscalationThrowEvent "Escalation throw events"
    @par
    ![IntermediateEscalationThrowEvent](BPMN/IntermediateEscalationThrowEvent.svg)
  - @ref BPMN::CompensateThrowEvent "Compensate throw events"
    @par
    ![CompensateThrowEvent](BPMN/IntermediateCompensateThrowEvent.svg)
  - @ref BPMN::LinkTargetEvent "Link source events"
    @par
    ![LinkSourceEvent](BPMN/LinkSourceEvent.svg)

  .
  @attention
  - @ref BPMN::SignalThrowEvent "Signal throw events" are **not** supported.

- End events
  - @ref BPMN::MessageThrowEvent "Message end events"
    @par
    ![MessageEndEvent](BPMN/MessageEndEvent.svg)
  - @ref BPMN::CompensateThrowEvent "Compensate end events"
    @par
    ![CompensateEndEvent](BPMN/CompensateEndEvent.svg)
  - @ref BPMN::ErrorEndEvent "Error end events"
    @par
    ![ErrorEndEvent](BPMN/ErrorEndEvent.svg)
  - @ref BPMN::TerminateEvent "Terminate events"
    @par
    ![TerminateEvent](BPMN/TerminateEvent.svg)

  .
  @attention
  - @ref BPMN::SignalThrowEvent "Signal end events" are **not** supported.

@attention
- Multiple @ref XML::bpmn::tEventDefinition "event definitions" are **not** supported.

## @ref BPMN::SequenceFlow "Sequence flows"
@todo

@attention
- Implicit gateways, i.e. multiple sequence flows entering or leaving a node that is not a gateway, are **not** supported.
- Default flows and conditional flows are **not** supported.

## @ref BPMN::MessageFlow "Message flows"
@todo

## Data
![DataObject](BPMN/DataObject.svg)
@par

@note The visual representation is only a reference to a @ref BPMN::DataObject "data object". A model may contain @ref BPMN::DataObject "data objects" without visual representations, and multiple visual representations may refer to the same @ref BPMN::DataObject "data object".
@attention
- @ref XML::bpmn::tDataStore "Data stores" are **not** supported.
- @ref XML::bpmn::tDataAssociation "Data associations" are **not** supported.
- @ref XML::bpmn::tDataInput "Data inputs" are **not** supported.
- @ref XML::bpmn::tDataOutput "Data outputs" are **not** supported.

