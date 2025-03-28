# BPMN elements
@page elements BPMN elements

In the following the BPMN element supported by the BPMNOS framework are listed and notes are given where relevant.

## @ref BPMN::Process "Processes"
![Process](BPMN/Process.svg)
@par
Processes are assumed to have exactly one @ref BPMN::UntypedStartEvent "untyped start event" and must be executable (i.e. `isExecutable = true`).
@attention
- @ref BPMN::TypedStartEvent "Typed start events" and multiple start events are **not** supported.
- Empty pools are **not** supported, in particular, each @ref XML::bpmn::tParticipant "participant" in a @ref XML::bpmn::tCollaboration "collaboration" must have a reference to a process.

## @ref BPMN::Activity "Activities"

### @ref BPMN::Task "Tasks"

Tasks may have standard  @ref XML::bpmn::tStandardLoopCharacteristics "loop markers" as well as parallel and sequential @ref XML::bpmn::tMultiInstanceLoopCharacteristics "multi-instance markers".

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


### @ref BPMN::SubProcess "Subprocesses"
![SubProcess](BPMN/SubProcess.svg)
@par
Subprocesses are assumed to have exactly one @ref BPMN::UntypedStartEvent "untyped start event".
They may have standard  @ref XML::bpmn::tStandardLoopCharacteristics "loop markers" as well as parallel and sequential @ref XML::bpmn::tMultiInstanceLoopCharacteristics "multi-instance markers".

@attention
- @ref BPMN::Transaction "Transactions" are treated like regular @ref BPMN::SubProcess "subprocesses".
- @ref BPMN::CallActivity "Call activities" are **not** supported.

### @ref BPMN::AdHocSubProcess "Ad-hoc subprocesses"
![AdHocSubProcess](BPMN/AdHocSubProcess.svg)
@par
Ad-hoc subprocesses must not contain any events or gateways without incoming sequence flow.
They may have standard  @ref XML::bpmn::tStandardLoopCharacteristics "loop markers" as well as parallel and sequential @ref XML::bpmn::tMultiInstanceLoopCharacteristics "multi-instance markers".
Furthermore, ad-hoc subprocesses are expected to have a sequential ordering (i.e. `ordering = "Sequential"`), indicating that activities within the scope of the ad-hoc subprocess must not be executed in parallel. For each ad-hoc subprocess, the model provider iterates through the nodes in the XML tree, starting from the ad-hoc subprocess, and goes up the hierarchy until it finds another ad-hoc subprocess or a @ref BPMN::Scope "scope" that contains a @ref XML::bpmn::tPerformer "tPerformer" element with `name = "Sequential"`. If the latter is found, the ad-hoc subprocess is associated to this performer, and all activities of any ad-hoc subprocess associated to this performer must be conducted in sequential order. Otherwise, the sequential ordering only applies to the activities within the ad-hoc subprocess.
  @note The @ref BPMN::AdHocSubProcess class is derived from @ref BPMN::Activity and not from @ref BPMN::SubProcess.

### Compensation activities
![CompensationActivity](BPMN/CompensationActivity.svg)
@par
Compensation activities are @ref BPMN::Activity "activities" with the @ref BPMN::Activity::isForCompensation "isForCompensation" field set to `true`.

@note It is assumed that no no activtiies are compensated within the scope of a compensation activity.


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
  - @ref BPMN::SignalStartEvent "Signal start events"
    @par
    ![SignalStartEvent](BPMN/SignalStartEvent.svg)
    ![Non-interrupting SignalStartEvent](BPMN/SignalStartEvent_non-interrupting.svg)
  - @ref BPMN::TimerStartEvent "Timer start events"
    @par
    ![TimerStartEvent](BPMN/TimerStartEvent.svg)
    ![Non-interrupting TimerStartEvent](BPMN/TimerStartEvent_non-interrupting.svg)
  - @ref BPMN::ConditionalStartEvent "Conditional start events"
    @par
    ![ConditionalStartEvent](BPMN/ConditionalStartEvent.svg)
    ![Non-interrupting ConditionalStartEvent](BPMN/ConditionalStartEvent_non-interrupting.svg)
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
  @attention Typed start events are only supported for @ref BPMN::EventSubProcess "event-subprocesses". 
  
- Intermediate events
  - @ref BPMN::MessageCatchEvent "Message catch events"
    @par
    ![IntermediateMessageCatchEvent](BPMN/IntermediateMessageCatchEvent.svg)
  - @ref BPMN::SignalCatchEvent "Signal catch events"
    @par
    ![IntermediateSignalCatchEvent](BPMN/IntermediateSignalCatchEvent.svg)
  - @ref BPMN::TimerCatchEvent "Timer catch events"
    @par
    ![IntermediateTimerCatchEvent](BPMN/IntermediateTimerCatchEvent.svg)
  - @ref BPMN::ConditionalCatchEvent "Conditional catch events"
    @par
    ![IntermediateConditionalCatchEvent](BPMN/IntermediateConditionalCatchEvent.svg)
  - @ref BPMN::LinkTargetEvent "Link target events"
    @par
    ![LinkTargetEvent](BPMN/LinkTargetEvent.svg)
    
  .
- @ref BPMN::BoundaryEvent "Boundary events"
  - @ref BPMN::MessageBoundaryEvent "Message boundary events"
    @par
    ![MessageBoundaryEvent](BPMN/MessageBoundaryEvent.svg)
    ![Non-interrupting MessageBoundaryEvent](BPMN/MessageBoundaryEvent_non-interrupting.svg)
  - @ref BPMN::SignalBoundaryEvent "Signal boundary events"
    @par
    ![SignalBoundaryEvent](BPMN/SignalBoundaryEvent.svg)
    ![Non-interrupting SignalBoundaryEvent](BPMN/SignalBoundaryEvent_non-interrupting.svg)
  - @ref BPMN::TimerBoundaryEvent "Timer boundary events"
    @par
    ![TimerBoundaryEvent](BPMN/TimerBoundaryEvent.svg)
    ![Non-interrupting TimerBoundaryEvent](BPMN/TimerBoundaryEvent_non-interrupting.svg)
  - @ref BPMN::ConditionalBoundaryEvent "Conditional boundary events"
    @par
    ![ConditionalBoundaryEvent](BPMN/ConditionalBoundaryEvent.svg)
    ![Non-interrupting ConditionalBoundaryEvent](BPMN/ConditionalBoundaryEvent_non-interrupting.svg)
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
- Multiple @ref XML::bpmn::tEventDefinition "event definitions" are **not** supported.

## @ref BPMN::ThrowEvent "Throw events"

- Intermediate events
  - @ref BPMN::MessageThrowEvent "Message throw events"
    @par
    ![IntermediateMessageThrowEvent](BPMN/IntermediateMessageThrowEvent.svg)
  - @ref BPMN::SignalThrowEvent "Signal throw events"
    @par
    ![IntermediateSignalThrowEvent](BPMN/IntermediateSignalThrowEvent.svg)
  - @ref BPMN::EscalationThrowEvent "Escalation throw events"
    @par
    ![IntermediateEscalationThrowEvent](BPMN/IntermediateEscalationThrowEvent.svg)
  - @ref BPMN::CompensateThrowEvent "Compensate throw events"
    @par
    ![CompensateThrowEvent](BPMN/IntermediateCompensateThrowEvent.svg)
  - @ref BPMN::LinkTargetEvent "Link source events"
    @par
    ![LinkSourceEvent](BPMN/LinkSourceEvent.svg)

- End events
  - @ref BPMN::MessageThrowEvent "Message end events"
    @par
    ![MessageEndEvent](BPMN/MessageEndEvent.svg)
  - @ref BPMN::SignalThrowEvent "Signal end events"
    @par
    ![SignalEndEvent](BPMN/SignalEndEvent.svg)
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
- Multiple @ref XML::bpmn::tEventDefinition "event definitions" are **not** supported.

## @ref BPMN::SequenceFlow "Sequence flows"
Only gateways are allowed to have multiple incoming or outgoing sequence flows.
Gatekeepers must be provided for diverging @ref BPMN::ExclusiveGateway "exclusive gateways" and @ref BPMN::InclusiveGateway "inclusive gateways". 

@attention
- Implicit gateways, i.e. multiple sequence flows entering or leaving a node that is not a gateway, are **not** supported.
- Default flows and conditional flows are **not** supported.

## @ref BPMN::MessageFlow "Message flows"
Message flows restrict the flow of messages to the events or (sub)processes the message flow is connected to.
All messages sent by a node subject to such a restriction may only be delivered to a node subject to the restriction, and vice versa.
A message sent by a node can also be received by another node if no message flow restricts messaging between the nodes.

## Data
![DataObject](BPMN/DataObject.svg)
@par

@ref XML::bpmn::tDataObjectReference "Data object references" can refer to @ref BPMN::DataObject "data objects" containing information relevant for execution.

@note The visual representation is only a reference to a @ref BPMN::DataObject "data object". A model may contain @ref BPMN::DataObject "data objects" without visual representations, and multiple visual representations may refer to the same @ref BPMN::DataObject "data object".
@attention
- @ref BPMN::DataObject::isCollection "Collection markers" are **not** supported for @ref BPMN::DataObject "data objects". 
- @ref XML::bpmn::tDataAssociation "Data associations" are **not** supported.
- @ref XML::bpmn::tDataInput "Data inputs" are **not** supported.
- @ref XML::bpmn::tDataOutput "Data outputs" are **not** supported.

![DataStore](BPMN/DataStore.svg)
@par

@ref XML::bpmn::tDataStoreReference "Data store references" can refer to data stores containing information that persists beyond the lifetime of processes.

@note The visual representation is linked to a process, however, the actual data store is not. All information of a data store is globally available.
@attention
- The actual @ref XML::bpmn::tDataStore "Data store" is **not** supported and the @ref XML::bpmn::tDataStoreReference "data store reference" is used directly to provide relevant information.

