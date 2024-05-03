# Execution logic
@page execution_logic Execution logic

Execution of process instances is managed through @ref BPMNOS::Execution::StateMachine "state machines" containing @ref BPMNOS::Execution::Token "tokens" running through respective state diagrams. The @ref BPMNOS::Execution::SystemState "system state" provides access to all state machines and tokens.

Upon instantiation of a @ref BPMN::Process "process", a token is generated for the process instance. This token owns a state machine containing all token that may exist within the scope of the process. Moreover, a token at the unique @ref BPMN::UntypedStartEvent "start event" of the process is created, and for all event-subprocesses, a token at the unique @ref BPMN::TypedStartEvent "start event" of the event-subprocess is created. Theses tokens flow through the process model and create all required child state machines and tokens running through these.
The token flow logic depends on the node type and is described in below sections.

- @subpage token_flow_logic_processes

- Activities
  - @subpage token_flow_logic_subprocesses
  - @subpage token_flow_logic_tasks
  - @subpage token_flow_logic_multi_instance_activities "Multi-instance activities"
  - @subpage token_flow_logic_compensation_activities "Compensation activities"

  @par
  @note @ref BPMN::CallActivity "Call activities" are **not** supported. 

- Gateways
  - @subpage token_flow_logic_exclusive_gateways
  - @subpage token_flow_logic_parallel_gateways
  - @subpage token_flow_logic_eventbased_gateways

  @par
  @note @ref BPMN::InclusiveGateway "Inclusive gateways" and @ref BPMN::ComplexGateway "complex gateways" are **not** supported. 

- Events
  - @subpage token_flow_logic_untyped_start_events
  - @subpage token_flow_logic_typed_start_events
  - @subpage token_flow_logic_boundary_events
  - @subpage token_flow_logic_intermediate_catching_events
  - @subpage token_flow_logic_throwing_events

.
@note All tokens can be @ref  BPMNOS::Execution::Token::State::WITHDRAWN "withdrawn" at any time, e.g., when the respective state machine terminates.

