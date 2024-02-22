# Compensation activities
@page token_flow_logic_compensation_activities Compensation activities

@warning Compensation Activities

# Token life-cycle

The life-cycle of a token visiting at a subprocess is as follows:

## ARRIVED

## CREATED
After initialization, the @ref BPMNOS::Model::ExtensionElements::operators "operators" are applied to update the @ref BPMNOS::Execution::Token::status "status" of the process token.
Furthermore, a token is created at the @ref BPMN::Process::startNodes "start node" of the process. 
This token inherits the @ref BPMNOS::Execution::Token::status "status attributes" of the process token.
Then, the @ref BPMNOS::Execution::Token::state "state" of the process token is set to @ref BPMNOS::Execution::Token::State::ENTERED "ENTERED". 
@attention All operators must be instantaneous, i.e., they must not change the timestamp
@par 
@attention It is assumed that each process has a unique @ref BPMN::UntypedStartEvent "blank start event".

## ENTERED
Feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If none of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::BUSY "BUSY".
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".

@note The failure occurring at this stage can not yet be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event".

## BUSY
A new token is created at the unique start node of the process.
This token inherits the @ref BPMNOS::Execution::Token::status "status attributes" of the original token.
The process token remains in @ref BPMNOS::Execution::Token::State::BUSY "BUSY" state as long as the process is neither terminated due to an uncaught failure nor successfully completed.

In the case that

- all tokens within the scope of the process have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state and no event subprocess is running, or
- the last @ref BPMN::EventSubProcess "event subprocess" with a @ref BPMN::TypedStartEvent "start event" that is @ref BPMN::TypedStartEvent::isInterrupting "not interrupting" completes successfully after all tokens within the scope of the process have reached @ref BPMNOS::Execution::Token::State::DONE "DONE" state,

the @ref BPMNOS::Execution::Token::status "status" of the process token is updated with the merged status of all tokens within its scope. All child tokens are disposed before the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

In the case that an @ref BPMN::EventSubProcess "event subprocess" with a @ref BPMN::TypedStartEvent "start event" that is @ref BPMN::TypedStartEvent::isInterrupting "interrupting" completes successfully, the @ref BPMNOS::Execution::Token::status "token status" of the process is analogously updated with the merged status of all tokens in @ref BPMNOS::Execution::Token::State::DONE "DONE" state within the scope of the interupting event subprocess. All of these tokens are disposed before the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::COMPLETED "COMPLETED".

In case of an uncaught failure, the @ref BPMNOS::Execution::Token::state "state" of the original token is updated to @ref BPMNOS::Execution::Token::State::FAILING "FAILING".

## COMPLETED
Feasibility of the @ref BPMNOS::Execution::Token::status "token status" is validated.
If none of the @ref BPMNOS::Model::ExtensionElements::restrictions "restrictions" is violated, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::DONE "DONE".
Otherwise, the @ref BPMNOS::Execution::Token::state "token state" is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED".

@note The failure occurring at this stage can no longer be caught by an @ref BPMN::EventSubProcess "event subprocess" with an @ref BPMN::ErrorStartEvent "error start event".

## DONE

When the process token reaches @ref BPMNOS::Execution::Token::State::DONE "DONE" state, it is disposed and the process instance has successfully completed.

## FAILING

When an uncaught failure has occurred within the scope of the process, the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::FAILING "FAILING",
the @ref BPMNOS::Execution::Token::status "status" of the process token is updated using the values of the token causing the failure,
all tokens within the scope of the process are withdrawn, 
and the @ref BPMNOS::Execution::Token::state "state" of the process token is updated to @ref BPMNOS::Execution::Token::State::FAILED "FAILED" after all relevant activities within the scope of the process have been compensated.

## FAILED

Upon failure, the process token is disposed.

