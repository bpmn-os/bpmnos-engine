/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "BPMN-OS", "index.html", [
    [ "Introduction", "index.html", "index" ],
    [ "Execution logic", "execution_logic.html", [
      [ "Processes", "token_flow_logic_processes.html", [
        [ "Token creation", "token_flow_logic_processes.html#token-creation", null ],
        [ "ENTERED", "token_flow_logic_processes.html#entered", null ],
        [ "BUSY", "token_flow_logic_processes.html#busy", null ],
        [ "COMPLETED", "token_flow_logic_processes.html#completed", null ],
        [ "DONE", "token_flow_logic_processes.html#done-1", null ],
        [ "FAILING", "token_flow_logic_processes.html#failing", null ],
        [ "FAILED", "token_flow_logic_processes.html#failed-1", null ]
      ] ],
      [ "Subprocesses and ad-hoc subprocesses", "token_flow_logic_subprocesses.html", [
        [ "Subprocesses and ad-hoc subprocesses (excluding multi-instance and compensation activities)", "token_flow_logic_subprocesses.html#subprocesses-and-ad-hoc-subprocesses-excluding-multi-instance-and-compensation-activities", [
          [ "ARRIVED / CREATED", "token_flow_logic_subprocesses.html#arrived--created-1", null ],
          [ "READY", "token_flow_logic_subprocesses.html#ready", null ],
          [ "ENTERED", "token_flow_logic_subprocesses.html#entered-1", null ],
          [ "BUSY", "token_flow_logic_subprocesses.html#busy-1", null ],
          [ "COMPLETED", "token_flow_logic_subprocesses.html#completed-1", null ],
          [ "EXITING", "token_flow_logic_subprocesses.html#exiting", null ],
          [ "DEPARTED", "token_flow_logic_subprocesses.html#departed-1", null ],
          [ "DONE", "token_flow_logic_subprocesses.html#done-2", null ],
          [ "FAILING", "token_flow_logic_subprocesses.html#failing-1", null ],
          [ "FAILED", "token_flow_logic_subprocesses.html#failed-2", null ]
        ] ]
      ] ],
      [ "Tasks", "token_flow_logic_tasks.html", [
        [ "Tasks (excluding multi-instance and compensation activities)", "token_flow_logic_tasks.html#tasks-excluding-multi-instance-and-compensation-activities", [
          [ "ARRIVED / CREATED", "token_flow_logic_tasks.html#arrived--created-2", null ],
          [ "READY", "token_flow_logic_tasks.html#ready-1", null ],
          [ "ENTERED", "token_flow_logic_tasks.html#entered-2", null ],
          [ "BUSY", "token_flow_logic_tasks.html#busy-2", null ],
          [ "COMPLETED", "token_flow_logic_tasks.html#completed-2", null ],
          [ "EXITING", "token_flow_logic_tasks.html#exiting-1", null ],
          [ "DEPARTED", "token_flow_logic_tasks.html#departed-2", null ],
          [ "DONE", "token_flow_logic_tasks.html#done-3", null ],
          [ "FAILED", "token_flow_logic_tasks.html#failed-3", null ]
        ] ]
      ] ],
      [ "Multi-instance activities", "token_flow_logic_multi_instance_activities.html", null ],
      [ "Compensation activities", "token_flow_logic_compensation_activities.html", null ],
      [ "Exclusive gateways", "token_flow_logic_exclusive_gateways.html", null ],
      [ "Parallel gateways", "token_flow_logic_parallel_gateways.html", null ],
      [ "Event-based gateways", "token_flow_logic_eventbased_gateways.html", [
        [ "States", "token_flow_logic_eventbased_gateways.html#states", null ]
      ] ],
      [ "Untyped start events", "token_flow_logic_untyped_start_events.html", null ],
      [ "Typed start events", "token_flow_logic_typed_start_events.html", null ],
      [ "Boundary events", "token_flow_logic_boundary_events.html", null ],
      [ "Intermediate catching events", "token_flow_logic_intermediate_catching_events.html", null ],
      [ "Throwing events", "token_flow_logic_throwing_events.html", null ]
    ] ],
    [ "Model provider", "bpmnos.html", [
      [ "BPMN elements", "elements.html", [
        [ "@ref BPMN::Process \"Processes\"", "elements.html#ref-bpmnprocess-processes", null ],
        [ "@ref BPMN::Activity \"Activities\"", "elements.html#ref-bpmnactivity-activities", [
          [ "@ref BPMN::Task \"Tasks\"", "elements.html#ref-bpmntask-tasks", null ],
          [ "@ref BPMN::SubProcess \"Subprocesses\"", "elements.html#ref-bpmnsubprocess-subprocesses", null ],
          [ "@ref BPMN::AdHocSubProcess \"Ad-hoc subprocesses\"", "elements.html#ref-bpmnadhocsubprocess-ad-hoc-subprocesses", null ],
          [ "Compensation activities", "elements.html#compensation-activities", null ]
        ] ],
        [ "@ref BPMN::EventSubProcess \"Event-subprocesses\"", "elements.html#ref-bpmneventsubprocess-event-subprocesses", null ],
        [ "@ref BPMN::Gateway \"Gateways\"", "elements.html#ref-bpmngateway-gateways", null ],
        [ "@ref BPMN::CatchEvent \"Catch events\"", "elements.html#ref-bpmncatchevent-catch-events", null ],
        [ "@ref BPMN::ThrowEvent \"Throw events\"", "elements.html#ref-bpmnthrowevent-throw-events", null ],
        [ "@ref BPMN::SequenceFlow \"Sequence flows\"", "elements.html#ref-bpmnsequenceflow-sequence-flows", null ],
        [ "@ref BPMN::MessageFlow \"Message flows\"", "elements.html#ref-bpmnmessageflow-message-flows", null ],
        [ "Data", "elements.html#data", null ]
      ] ],
      [ "BPMN extension", "extension.html", [
        [ "Attributes", "extension.html#attributes", [
          [ "Global attributes", "extension.html#global-attributes", null ],
          [ "Data attributes", "extension.html#data-attributes", null ],
          [ "Status attributes", "extension.html#status-attributes", null ]
        ] ],
        [ "Restrictions", "extension.html#restrictions", [
          [ "Node restrictions", "extension.html#node-restrictions", null ],
          [ "Gatekeeper restrictions", "extension.html#gatekeeper-restrictions", null ]
        ] ],
        [ "Operators", "extension.html#operators", null ],
        [ "Choices", "extension.html#choices", null ],
        [ "Messages", "extension.html#messages", null ],
        [ "Timer", "extension.html#timer", null ],
        [ "Lookup tables", "extension.html#lookup-tables", null ],
        [ "Loop parameters", "extension.html#loop-parameters", null ]
      ] ]
    ] ],
    [ "Data provider", "data.html", [
      [ "Static data provider", "data.html#static-data-provider", [
        [ "CSV Format", "data.html#csv-format", null ],
        [ "Example", "data.html#example", null ],
        [ "Global Attributes", "data.html#global-attributes-1", null ],
        [ "Expressions", "data.html#expressions", null ]
      ] ],
      [ "Dynamic data provider", "data.html#dynamic-data-provider", [
        [ "CSV Format", "data.html#csv-format-1", null ],
        [ "Example", "data.html#example-1", null ],
        [ "Disclosure Rules", "data.html#disclosure-rules", null ],
        [ "Global Attributes", "data.html#global-attributes-2", null ],
        [ "Usage", "data.html#usage", null ]
      ] ],
      [ "Stochastic data provider", "data.html#stochastic-data-provider", [
        [ "CSV Format", "data.html#csv-format-2", null ],
        [ "Random Functions", "data.html#random-functions", null ],
        [ "Example", "data.html#example-2", null ],
        [ "Expression Evaluation", "data.html#expression-evaluation", null ],
        [ "Attribute Initialization and Modification", "data.html#attribute-initialization-and-modification", null ],
        [ "Reproducibility", "data.html#reproducibility", null ],
        [ "Usage", "data.html#usage-1", null ],
        [ "Downward Compatibility", "data.html#downward-compatibility", null ]
      ] ],
      [ "Expected value data provider", "data.html#expected-value-data-provider", [
        [ "CSV Format", "data.html#csv-format-3", null ],
        [ "Behavior", "data.html#behavior", null ],
        [ "Expected Values", "data.html#expected-values", null ],
        [ "Example", "data.html#example-3", null ],
        [ "Usage", "data.html#usage-2", null ]
      ] ],
      [ "Real-life monitor", "data.html#real-life-monitor", null ]
    ] ],
    [ "Execution engine", "engine.html", null ],
    [ "Controller", "controller.html", [
      [ "Greedy controller", "controller.html#greedy-controller", null ],
      [ "Evaluator", "controller.html#evaluator", [
        [ "Local evaluator", "controller.html#local-evaluator", null ],
        [ "Guided evaluator", "controller.html#guided-evaluator", null ]
      ] ]
    ] ],
    [ "Observer", "observer.html", null ],
    [ "Models", "models.html", [
      [ "Travelling salesperson problem", "travelling_salesperson_problem.html", [
        [ "Travelling salesperson problem (TravellingSalesperson_Process)", "travelling_salesperson_problem.html#travelling-salesperson-problem-travellingsalesperson_process", [
          [ "Diagram", "travelling_salesperson_problem.html#diagram-10", null ],
          [ "Status", "travelling_salesperson_problem.html#status-212", null ],
          [ "Data", "travelling_salesperson_problem.html#data-213", null ],
          [ "Lookup tables", "travelling_salesperson_problem.html#lookup-tables-6", null ],
          [ "StartEvent (StartEvent_1)", "travelling_salesperson_problem.html#startevent-startevent_1-1", [
            [ "Status", "travelling_salesperson_problem.html#status-213", null ],
            [ "Data", "travelling_salesperson_problem.html#data-214", null ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcess)", "travelling_salesperson_problem.html#adhocsubprocess-adhocsubprocess-5", [
            [ "Status", "travelling_salesperson_problem.html#status-214", null ],
            [ "Data", "travelling_salesperson_problem.html#data-215", null ]
          ] ],
          [ "Task (VisitLocation)", "travelling_salesperson_problem.html#task-visitlocation", [
            [ "Status", "travelling_salesperson_problem.html#status-215", null ],
            [ "Data", "travelling_salesperson_problem.html#data-216", null ],
            [ "Multi-instance", "travelling_salesperson_problem.html#multi-instance-1", null ],
            [ "Operators", "travelling_salesperson_problem.html#operators-40", null ]
          ] ],
          [ "Task (ReturnTrip)", "travelling_salesperson_problem.html#task-returntrip-3", [
            [ "Status", "travelling_salesperson_problem.html#status-216", null ],
            [ "Data", "travelling_salesperson_problem.html#data-217", null ],
            [ "Operators", "travelling_salesperson_problem.html#operators-41", null ]
          ] ],
          [ "EndEvent (EndEvent_1)", "travelling_salesperson_problem.html#endevent-endevent_1-1", [
            [ "Status", "travelling_salesperson_problem.html#status-217", null ],
            [ "Data", "travelling_salesperson_problem.html#data-218", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_1db7c4q)", "travelling_salesperson_problem.html#dataobjectreference-dataobjectreference_1db7c4q", [
            [ "Status", "travelling_salesperson_problem.html#status-218", null ],
            [ "Data", "travelling_salesperson_problem.html#data-219", null ]
          ] ],
          [ "DataStoreReference (DataStoreReference_1ns10u7)", "travelling_salesperson_problem.html#datastorereference-datastorereference_1ns10u7", [
            [ "Status", "travelling_salesperson_problem.html#status-219", null ],
            [ "Data", "travelling_salesperson_problem.html#data-220", null ]
          ] ]
        ] ]
      ] ],
      [ "Earliest arrival problem", "earliest_arrival_problem.html", [
        [ "Process (EarliestArrival_Process)", "earliest_arrival_problem.html#process-earliestarrival_process", [
          [ "Diagram", "earliest_arrival_problem.html#diagram-2", null ],
          [ "Status", "earliest_arrival_problem.html#status-28", null ],
          [ "Data", "earliest_arrival_problem.html#data-29", null ],
          [ "Lookup tables", "earliest_arrival_problem.html#lookup-tables-2", null ],
          [ "StartEvent (StartEvent_1)", "earliest_arrival_problem.html#startevent-startevent_1", [
            [ "Status", "earliest_arrival_problem.html#status-29", null ],
            [ "Data", "earliest_arrival_problem.html#data-30", null ]
          ] ],
          [ "SubProcess (LoopActivity)", "earliest_arrival_problem.html#subprocess-loopactivity", [
            [ "Status", "earliest_arrival_problem.html#status-30", null ],
            [ "Data", "earliest_arrival_problem.html#data-31", null ],
            [ "Loop", "earliest_arrival_problem.html#loop", null ]
          ] ],
          [ "StartEvent (StartEventTrip)", "earliest_arrival_problem.html#startevent-starteventtrip", [
            [ "Status", "earliest_arrival_problem.html#status-31", null ],
            [ "Data", "earliest_arrival_problem.html#data-32", null ]
          ] ],
          [ "EndEvent (EndEventTrip)", "earliest_arrival_problem.html#endevent-endeventtrip", [
            [ "Status", "earliest_arrival_problem.html#status-32", null ],
            [ "Data", "earliest_arrival_problem.html#data-33", null ]
          ] ],
          [ "Task (SelectDestination)", "earliest_arrival_problem.html#task-selectdestination", [
            [ "Status", "earliest_arrival_problem.html#status-33", null ],
            [ "Data", "earliest_arrival_problem.html#data-34", null ],
            [ "Choices", "earliest_arrival_problem.html#choices-1", null ],
            [ "Operators", "earliest_arrival_problem.html#operators-5", null ],
            [ "Exit restrictions", "earliest_arrival_problem.html#exit-restrictions-15", null ],
            [ "Choice guidance", "earliest_arrival_problem.html#choice-guidance", [
              [ "Attributes", "earliest_arrival_problem.html#attributes-2", null ],
              [ "Operators", "earliest_arrival_problem.html#operators-6", null ]
            ] ]
          ] ],
          [ "Task (Travel)", "earliest_arrival_problem.html#task-travel", [
            [ "Status", "earliest_arrival_problem.html#status-34", null ],
            [ "Data", "earliest_arrival_problem.html#data-35", null ],
            [ "Operators", "earliest_arrival_problem.html#operators-7", null ]
          ] ],
          [ "IntermediateCatchEvent (TimerEvent)", "earliest_arrival_problem.html#intermediatecatchevent-timerevent", [
            [ "Status", "earliest_arrival_problem.html#status-35", null ],
            [ "Data", "earliest_arrival_problem.html#data-36", null ],
            [ "Timer", "earliest_arrival_problem.html#timer-1", null ]
          ] ],
          [ "EndEvent (EndEvent_1)", "earliest_arrival_problem.html#endevent-endevent_1", [
            [ "Status", "earliest_arrival_problem.html#status-36", null ],
            [ "Data", "earliest_arrival_problem.html#data-37", null ]
          ] ],
          [ "DataStoreReference (DataStoreReference_0myx9a2)", "earliest_arrival_problem.html#datastorereference-datastorereference_0myx9a2", [
            [ "Status", "earliest_arrival_problem.html#status-37", null ],
            [ "Data", "earliest_arrival_problem.html#data-38", null ]
          ] ]
        ] ]
      ] ],
      [ "Truck driver scheduling problem", "truck_driver_scheduling_problem.html", [
        [ "U.S. Truck driver process (TruckDriverProcess)", "truck_driver_scheduling_problem.html#us-truck-driver-process-truckdriverprocess", [
          [ "Diagram", "truck_driver_scheduling_problem.html#diagram-11", null ],
          [ "Status", "truck_driver_scheduling_problem.html#status-220", null ],
          [ "Data", "truck_driver_scheduling_problem.html#data-221", null ],
          [ "StartEvent (StartProcessEvent)", "truck_driver_scheduling_problem.html#startevent-startprocessevent", [
            [ "Status", "truck_driver_scheduling_problem.html#status-221", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-222", null ]
          ] ],
          [ "SubProcess (CustomerServiceActivity)", "truck_driver_scheduling_problem.html#subprocess-customerserviceactivity", [
            [ "Status", "truck_driver_scheduling_problem.html#status-222", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-223", null ],
            [ "Multi-instance", "truck_driver_scheduling_problem.html#multi-instance-2", null ]
          ] ],
          [ "StartEvent (StartTripEvent)", "truck_driver_scheduling_problem.html#startevent-starttripevent", [
            [ "Status", "truck_driver_scheduling_problem.html#status-223", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-224", null ]
          ] ],
          [ "Task (VisitTask)", "truck_driver_scheduling_problem.html#task-visittask", [
            [ "Status", "truck_driver_scheduling_problem.html#status-224", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-225", null ],
            [ "Operators", "truck_driver_scheduling_problem.html#operators-42", null ]
          ] ],
          [ "EndEvent (EndVisitEvent)", "truck_driver_scheduling_problem.html#endevent-endvisitevent", [
            [ "Status", "truck_driver_scheduling_problem.html#status-225", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-226", null ]
          ] ],
          [ "SubProcess (WaitActivity)", "truck_driver_scheduling_problem.html#subprocess-waitactivity", [
            [ "Diagram", "truck_driver_scheduling_problem.html#diagram-12", null ],
            [ "Status", "truck_driver_scheduling_problem.html#status-226", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-227", null ]
          ] ],
          [ "ExclusiveGateway (WaitBeforeVisitGateway)", "truck_driver_scheduling_problem.html#exclusivegateway-waitbeforevisitgateway", [
            [ "Status", "truck_driver_scheduling_problem.html#status-227", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-228", null ]
          ] ],
          [ "ExclusiveGateway (VisitGateway)", "truck_driver_scheduling_problem.html#exclusivegateway-visitgateway", [
            [ "Status", "truck_driver_scheduling_problem.html#status-228", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-229", null ]
          ] ],
          [ "SubProcess (TripActivity)", "truck_driver_scheduling_problem.html#subprocess-tripactivity", [
            [ "Status", "truck_driver_scheduling_problem.html#status-229", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-230", null ],
            [ "Loop", "truck_driver_scheduling_problem.html#loop-1", null ]
          ] ],
          [ "StartEvent (StartDriveEvent)", "truck_driver_scheduling_problem.html#startevent-startdriveevent", [
            [ "Status", "truck_driver_scheduling_problem.html#status-230", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-231", null ]
          ] ],
          [ "Task (DriveTask)", "truck_driver_scheduling_problem.html#task-drivetask", [
            [ "Status", "truck_driver_scheduling_problem.html#status-231", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-232", null ],
            [ "Operators", "truck_driver_scheduling_problem.html#operators-43", null ]
          ] ],
          [ "ExclusiveGateway (ArrivalGateway)", "truck_driver_scheduling_problem.html#exclusivegateway-arrivalgateway", [
            [ "Status", "truck_driver_scheduling_problem.html#status-232", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-233", null ]
          ] ],
          [ "SubProcess (OffDutyActivity)", "truck_driver_scheduling_problem.html#subprocess-offdutyactivity", [
            [ "Diagram", "truck_driver_scheduling_problem.html#diagram-13", null ],
            [ "Status", "truck_driver_scheduling_problem.html#status-233", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-234", null ]
          ] ],
          [ "EndEvent (TripEndEvent)", "truck_driver_scheduling_problem.html#endevent-tripendevent", [
            [ "Status", "truck_driver_scheduling_problem.html#status-234", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-235", null ]
          ] ],
          [ "EndEvent (OffDutyEndEvent)", "truck_driver_scheduling_problem.html#endevent-offdutyendevent", [
            [ "Status", "truck_driver_scheduling_problem.html#status-235", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-236", null ]
          ] ],
          [ "EndEvent (EndProcessEvent)", "truck_driver_scheduling_problem.html#endevent-endprocessevent", [
            [ "Status", "truck_driver_scheduling_problem.html#status-236", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-237", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_15y3y37)", "truck_driver_scheduling_problem.html#dataobjectreference-dataobjectreference_15y3y37", [
            [ "Status", "truck_driver_scheduling_problem.html#status-237", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-238", null ]
          ] ],
          [ "SequenceFlow (Flow_12gilga)", "truck_driver_scheduling_problem.html#sequenceflow-flow_12gilga", [
            [ "Gatekeeper", "truck_driver_scheduling_problem.html#gatekeeper-6", null ]
          ] ],
          [ "SequenceFlow (Flow_14ovnm9)", "truck_driver_scheduling_problem.html#sequenceflow-flow_14ovnm9", [
            [ "Gatekeeper", "truck_driver_scheduling_problem.html#gatekeeper-7", null ]
          ] ],
          [ "SequenceFlow (Flow_0s8gidz)", "truck_driver_scheduling_problem.html#sequenceflow-flow_0s8gidz", [
            [ "Gatekeeper", "truck_driver_scheduling_problem.html#gatekeeper-8", null ]
          ] ],
          [ "SequenceFlow (Flow_09sie24)", "truck_driver_scheduling_problem.html#sequenceflow-flow_09sie24", [
            [ "Gatekeeper", "truck_driver_scheduling_problem.html#gatekeeper-9", null ]
          ] ],
          [ "StartEvent (OffDutyStartEvent)", "truck_driver_scheduling_problem.html#startevent-offdutystartevent", [
            [ "Status", "truck_driver_scheduling_problem.html#status-238", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-239", null ]
          ] ],
          [ "Task (DecideDuringTripTask)", "truck_driver_scheduling_problem.html#task-decideduringtriptask", [
            [ "Status", "truck_driver_scheduling_problem.html#status-239", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-240", null ],
            [ "Choices", "truck_driver_scheduling_problem.html#choices-2", null ],
            [ "Completion restrictions", "truck_driver_scheduling_problem.html#completion-restrictions-25", null ]
          ] ],
          [ "ExclusiveGateway (TripGateway)", "truck_driver_scheduling_problem.html#exclusivegateway-tripgateway", [
            [ "Status", "truck_driver_scheduling_problem.html#status-240", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-241", null ]
          ] ],
          [ "Task (BreakDuringTripTask)", "truck_driver_scheduling_problem.html#task-breakduringtriptask", [
            [ "Status", "truck_driver_scheduling_problem.html#status-241", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-242", null ],
            [ "Operators", "truck_driver_scheduling_problem.html#operators-44", null ]
          ] ],
          [ "Task (RestDuringTripTask)", "truck_driver_scheduling_problem.html#task-restduringtriptask", [
            [ "Status", "truck_driver_scheduling_problem.html#status-242", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-243", null ],
            [ "Operators", "truck_driver_scheduling_problem.html#operators-45", null ]
          ] ],
          [ "EndEvent (TripEndEventBreak)", "truck_driver_scheduling_problem.html#endevent-tripendeventbreak", [
            [ "Status", "truck_driver_scheduling_problem.html#status-243", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-244", null ]
          ] ],
          [ "EndEvent (TripEndEventRest)", "truck_driver_scheduling_problem.html#endevent-tripendeventrest", [
            [ "Status", "truck_driver_scheduling_problem.html#status-244", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-245", null ]
          ] ],
          [ "Task (BreakDuringTripDuration)", "truck_driver_scheduling_problem.html#task-breakduringtripduration", [
            [ "Status", "truck_driver_scheduling_problem.html#status-245", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-246", null ],
            [ "Choices", "truck_driver_scheduling_problem.html#choices-3", null ],
            [ "Completion restrictions", "truck_driver_scheduling_problem.html#completion-restrictions-26", null ],
            [ "Choice guidance", "truck_driver_scheduling_problem.html#choice-guidance-1", [
              [ "Attributes", "truck_driver_scheduling_problem.html#attributes-11", null ]
            ] ]
          ] ],
          [ "Task (RestDuringTripDuration)", "truck_driver_scheduling_problem.html#task-restduringtripduration", [
            [ "Status", "truck_driver_scheduling_problem.html#status-246", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-247", null ],
            [ "Choices", "truck_driver_scheduling_problem.html#choices-4", null ],
            [ "Choice guidance", "truck_driver_scheduling_problem.html#choice-guidance-2", [
              [ "Attributes", "truck_driver_scheduling_problem.html#attributes-12", null ]
            ] ]
          ] ],
          [ "SequenceFlow (Flow_00x8mrp)", "truck_driver_scheduling_problem.html#sequenceflow-flow_00x8mrp", [
            [ "Gatekeeper", "truck_driver_scheduling_problem.html#gatekeeper-10", null ]
          ] ],
          [ "SequenceFlow (Flow_16bmg1w)", "truck_driver_scheduling_problem.html#sequenceflow-flow_16bmg1w", [
            [ "Gatekeeper", "truck_driver_scheduling_problem.html#gatekeeper-11", null ]
          ] ],
          [ "StartEvent (WaitStartEvent)", "truck_driver_scheduling_problem.html#startevent-waitstartevent", [
            [ "Status", "truck_driver_scheduling_problem.html#status-247", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-248", null ]
          ] ],
          [ "Task (BreakAtDestinationTask)", "truck_driver_scheduling_problem.html#task-breakatdestinationtask", [
            [ "Status", "truck_driver_scheduling_problem.html#status-248", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-249", null ],
            [ "Operators", "truck_driver_scheduling_problem.html#operators-46", null ]
          ] ],
          [ "Task (RestAtDestinationTask)", "truck_driver_scheduling_problem.html#task-restatdestinationtask", [
            [ "Status", "truck_driver_scheduling_problem.html#status-249", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-250", null ],
            [ "Operators", "truck_driver_scheduling_problem.html#operators-47", null ]
          ] ],
          [ "Task (DecideAtDestinationTask)", "truck_driver_scheduling_problem.html#task-decideatdestinationtask", [
            [ "Status", "truck_driver_scheduling_problem.html#status-250", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-251", null ],
            [ "Choices", "truck_driver_scheduling_problem.html#choices-5", null ],
            [ "Completion restrictions", "truck_driver_scheduling_problem.html#completion-restrictions-27", null ]
          ] ],
          [ "ExclusiveGateway (WaitGateway_1)", "truck_driver_scheduling_problem.html#exclusivegateway-waitgateway_1", [
            [ "Status", "truck_driver_scheduling_problem.html#status-251", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-252", null ]
          ] ],
          [ "IntermediateCatchEvent (TimerEvent)", "truck_driver_scheduling_problem.html#intermediatecatchevent-timerevent-1", [
            [ "Status", "truck_driver_scheduling_problem.html#status-252", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-253", null ],
            [ "Timer", "truck_driver_scheduling_problem.html#timer-7", null ]
          ] ],
          [ "EndEvent (WaitEndEvent)", "truck_driver_scheduling_problem.html#endevent-waitendevent", [
            [ "Status", "truck_driver_scheduling_problem.html#status-253", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-254", null ]
          ] ],
          [ "Task (BreakAtDestinationDuration)", "truck_driver_scheduling_problem.html#task-breakatdestinationduration", [
            [ "Status", "truck_driver_scheduling_problem.html#status-254", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-255", null ],
            [ "Choices", "truck_driver_scheduling_problem.html#choices-6", null ],
            [ "Completion restrictions", "truck_driver_scheduling_problem.html#completion-restrictions-28", null ],
            [ "Choice guidance", "truck_driver_scheduling_problem.html#choice-guidance-3", [
              [ "Attributes", "truck_driver_scheduling_problem.html#attributes-13", null ]
            ] ]
          ] ],
          [ "Task (RestAtDestinationDuration)", "truck_driver_scheduling_problem.html#task-restatdestinationduration", [
            [ "Status", "truck_driver_scheduling_problem.html#status-255", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-256", null ],
            [ "Choices", "truck_driver_scheduling_problem.html#choices-7", null ],
            [ "Choice guidance", "truck_driver_scheduling_problem.html#choice-guidance-4", [
              [ "Attributes", "truck_driver_scheduling_problem.html#attributes-14", null ]
            ] ]
          ] ],
          [ "ExclusiveGateway (WaitGateway_2)", "truck_driver_scheduling_problem.html#exclusivegateway-waitgateway_2", [
            [ "Status", "truck_driver_scheduling_problem.html#status-256", null ],
            [ "Data", "truck_driver_scheduling_problem.html#data-257", null ]
          ] ],
          [ "SequenceFlow (Flow_08rnlb8)", "truck_driver_scheduling_problem.html#sequenceflow-flow_08rnlb8", [
            [ "Gatekeeper", "truck_driver_scheduling_problem.html#gatekeeper-12", null ]
          ] ],
          [ "SequenceFlow (Flow_08pr2zc)", "truck_driver_scheduling_problem.html#sequenceflow-flow_08pr2zc", [
            [ "Gatekeeper", "truck_driver_scheduling_problem.html#gatekeeper-13", null ]
          ] ],
          [ "SequenceFlow (Flow_1at46ok)", "truck_driver_scheduling_problem.html#sequenceflow-flow_1at46ok", [
            [ "Gatekeeper", "truck_driver_scheduling_problem.html#gatekeeper-14", null ]
          ] ]
        ] ]
      ] ],
      [ "Assignment problem", "assignment_problem.html", [
        [ "Collaboration (Assignment_problem)", "assignment_problem.html#collaboration-assignment_problem", [
          [ "Diagram", "assignment_problem.html#diagram", null ],
          [ "Lookup tables", "assignment_problem.html#lookup-tables-1", null ],
          [ "Client process (ClientProcess)", "assignment_problem.html#client-process-clientprocess", [
            [ "Status", "assignment_problem.html#status", null ],
            [ "Data", "assignment_problem.html#data-1", null ]
          ] ],
          [ "StartEvent (StartEventClient)", "assignment_problem.html#startevent-starteventclient", [
            [ "Status", "assignment_problem.html#status-1", null ],
            [ "Data", "assignment_problem.html#data-2", null ]
          ] ],
          [ "SendTask (SendRequestTask)", "assignment_problem.html#sendtask-sendrequesttask", [
            [ "Status", "assignment_problem.html#status-2", null ],
            [ "Data", "assignment_problem.html#data-3", null ],
            [ "Message", "assignment_problem.html#message", [
              [ "<em>Name</em> Message", "assignment_problem.html#name-message", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventClient)", "assignment_problem.html#endevent-endeventclient", [
            [ "Status", "assignment_problem.html#status-3", null ],
            [ "Data", "assignment_problem.html#data-4", null ]
          ] ],
          [ "Server process (ServerProcess)", "assignment_problem.html#server-process-serverprocess", [
            [ "Status", "assignment_problem.html#status-4", null ],
            [ "Data", "assignment_problem.html#data-5", null ]
          ] ],
          [ "StartEvent (StartEventServer)", "assignment_problem.html#startevent-starteventserver", [
            [ "Status", "assignment_problem.html#status-5", null ],
            [ "Data", "assignment_problem.html#data-6", null ]
          ] ],
          [ "ReceiveTask (ReceiveRequestTask)", "assignment_problem.html#receivetask-receiverequesttask", [
            [ "Status", "assignment_problem.html#status-6", null ],
            [ "Data", "assignment_problem.html#data-7", null ],
            [ "Message", "assignment_problem.html#message-1", [
              [ "<em>Name</em> Message", "assignment_problem.html#name-message-1", null ]
            ] ],
            [ "Operators", "assignment_problem.html#operators-1", null ]
          ] ],
          [ "EndEvent (EndEventServer)", "assignment_problem.html#endevent-endeventserver", [
            [ "Status", "assignment_problem.html#status-7", null ],
            [ "Data", "assignment_problem.html#data-8", null ]
          ] ],
          [ "DataStoreReference (DataStoreReference_13uqyvh)", "assignment_problem.html#datastorereference-datastorereference_13uqyvh", [
            [ "Status", "assignment_problem.html#status-8", null ],
            [ "Data", "assignment_problem.html#data-9", null ]
          ] ]
        ] ]
      ] ],
      [ "Knapsack problem", "knapsack_problem.html", [
        [ "Collaboration (Knapsack_problem)", "knapsack_problem.html#collaboration-knapsack_problem-1", [
          [ "Diagram", "knapsack_problem.html#diagram-8", null ],
          [ "Item process (ItemProcess)", "knapsack_problem.html#item-process-itemprocess-3", [
            [ "Status", "knapsack_problem.html#status-154", null ],
            [ "Data", "knapsack_problem.html#data-155", null ]
          ] ],
          [ "StartEvent (StartEventItem)", "knapsack_problem.html#startevent-starteventitem-4", [
            [ "Status", "knapsack_problem.html#status-155", null ],
            [ "Data", "knapsack_problem.html#data-156", null ]
          ] ],
          [ "SendTask (SendRequestTask)", "knapsack_problem.html#sendtask-sendrequesttask-2", [
            [ "Status", "knapsack_problem.html#status-156", null ],
            [ "Data", "knapsack_problem.html#data-157", null ],
            [ "Message", "knapsack_problem.html#message-32", [
              [ "<em>Name</em> Request", "knapsack_problem.html#name-request-12", null ]
            ] ]
          ] ],
          [ "EventBasedGateway (Event-basedGateway)", "knapsack_problem.html#eventbasedgateway-event-basedgateway-1", [
            [ "Status", "knapsack_problem.html#status-157", null ],
            [ "Data", "knapsack_problem.html#data-158", null ]
          ] ],
          [ "IntermediateCatchEvent (CatchAcceptanceMessage)", "knapsack_problem.html#intermediatecatchevent-catchacceptancemessage-1", [
            [ "Status", "knapsack_problem.html#status-158", null ],
            [ "Data", "knapsack_problem.html#data-159", null ],
            [ "Message", "knapsack_problem.html#message-33", [
              [ "<em>Name</em> Acceptance", "knapsack_problem.html#name-acceptance-2", null ]
            ] ]
          ] ],
          [ "EndEvent (ItemAccepted)", "knapsack_problem.html#endevent-itemaccepted-1", [
            [ "Status", "knapsack_problem.html#status-159", null ],
            [ "Data", "knapsack_problem.html#data-160", null ]
          ] ],
          [ "IntermediateCatchEvent (CatchRejectionMessage)", "knapsack_problem.html#intermediatecatchevent-catchrejectionmessage-1", [
            [ "Status", "knapsack_problem.html#status-160", null ],
            [ "Data", "knapsack_problem.html#data-161", null ],
            [ "Message", "knapsack_problem.html#message-34", [
              [ "<em>Name</em> Rejection", "knapsack_problem.html#name-rejection-2", null ]
            ] ]
          ] ],
          [ "EndEvent (ItemRejected)", "knapsack_problem.html#endevent-itemrejected-1", [
            [ "Status", "knapsack_problem.html#status-161", null ],
            [ "Data", "knapsack_problem.html#data-162", null ]
          ] ],
          [ "Knapsack process (KnapsackProcess)", "knapsack_problem.html#knapsack-process-knapsackprocess-1", [
            [ "Status", "knapsack_problem.html#status-162", null ],
            [ "Data", "knapsack_problem.html#data-163", null ]
          ] ],
          [ "SubProcess (EventSubProcess)", "knapsack_problem.html#subprocess-eventsubprocess-6", [
            [ "Status", "knapsack_problem.html#status-163", null ],
            [ "Data", "knapsack_problem.html#data-164", null ],
            [ "Operators", "knapsack_problem.html#operators-30", null ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcess)", "knapsack_problem.html#adhocsubprocess-adhocsubprocess-4", [
            [ "Status", "knapsack_problem.html#status-164", null ],
            [ "Data", "knapsack_problem.html#data-165", null ]
          ] ],
          [ "SubProcess (HandleItemActivity)", "knapsack_problem.html#subprocess-handleitemactivity-2", [
            [ "Status", "knapsack_problem.html#status-165", null ],
            [ "Data", "knapsack_problem.html#data-166", null ]
          ] ],
          [ "StartEvent (StartHandleItemEvent)", "knapsack_problem.html#startevent-starthandleitemevent-1", [
            [ "Status", "knapsack_problem.html#status-166", null ],
            [ "Data", "knapsack_problem.html#data-167", null ]
          ] ],
          [ "ExclusiveGateway (CapacityGateway)", "knapsack_problem.html#exclusivegateway-capacitygateway-1", [
            [ "Status", "knapsack_problem.html#status-167", null ],
            [ "Data", "knapsack_problem.html#data-168", null ]
          ] ],
          [ "EndEvent (ThrowRejectionMessage)", "knapsack_problem.html#endevent-throwrejectionmessage-1", [
            [ "Status", "knapsack_problem.html#status-168", null ],
            [ "Data", "knapsack_problem.html#data-169", null ],
            [ "Message", "knapsack_problem.html#message-35", [
              [ "<em>Name</em> Rejection", "knapsack_problem.html#name-rejection-3", null ]
            ] ]
          ] ],
          [ "Task (IncludeItemActivity)", "knapsack_problem.html#task-includeitemactivity-2", [
            [ "Status", "knapsack_problem.html#status-169", null ],
            [ "Data", "knapsack_problem.html#data-170", null ],
            [ "Operators", "knapsack_problem.html#operators-31", null ]
          ] ],
          [ "EndEvent (ThrowAcceptanceMessage)", "knapsack_problem.html#endevent-throwacceptancemessage-1", [
            [ "Status", "knapsack_problem.html#status-170", null ],
            [ "Data", "knapsack_problem.html#data-171", null ],
            [ "Message", "knapsack_problem.html#message-36", [
              [ "<em>Name</em> Acceptance", "knapsack_problem.html#name-acceptance-3", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventSubProcess)", "knapsack_problem.html#endevent-endeventsubprocess-3", [
            [ "Status", "knapsack_problem.html#status-171", null ],
            [ "Data", "knapsack_problem.html#data-172", null ]
          ] ],
          [ "StartEvent (CatchRequestMessage)", "knapsack_problem.html#startevent-catchrequestmessage-4", [
            [ "Status", "knapsack_problem.html#status-172", null ],
            [ "Data", "knapsack_problem.html#data-173", null ],
            [ "Message", "knapsack_problem.html#message-37", [
              [ "<em>Name</em> Request", "knapsack_problem.html#name-request-13", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventKnapsack)", "knapsack_problem.html#endevent-endeventknapsack-1", [
            [ "Status", "knapsack_problem.html#status-173", null ],
            [ "Data", "knapsack_problem.html#data-174", null ]
          ] ],
          [ "Task (WaitActivity)", "knapsack_problem.html#task-waitactivity-3", [
            [ "Status", "knapsack_problem.html#status-174", null ],
            [ "Data", "knapsack_problem.html#data-175", null ],
            [ "Exit restrictions", "knapsack_problem.html#exit-restrictions-27", null ]
          ] ],
          [ "StartEvent (StartEventKnapsack)", "knapsack_problem.html#startevent-starteventknapsack-1", [
            [ "Status", "knapsack_problem.html#status-175", null ],
            [ "Data", "knapsack_problem.html#data-176", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_Knapsack)", "knapsack_problem.html#dataobjectreference-dataobjectreference_knapsack", [
            [ "Status", "knapsack_problem.html#status-176", null ],
            [ "Data", "knapsack_problem.html#data-177", null ]
          ] ],
          [ "SequenceFlow (Flow_0y2js91)", "knapsack_problem.html#sequenceflow-flow_0y2js91-1", [
            [ "Gatekeeper", "knapsack_problem.html#gatekeeper-4", null ]
          ] ],
          [ "SequenceFlow (Flow_1r0tr4t)", "knapsack_problem.html#sequenceflow-flow_1r0tr4t-1", [
            [ "Gatekeeper", "knapsack_problem.html#gatekeeper-5", null ]
          ] ]
        ] ]
      ] ],
      [ "Knapsack problem (guided)", "guided_knapsack_problem.html", [
        [ "Collaboration (Knapsack_problem)", "guided_knapsack_problem.html#collaboration-knapsack_problem", [
          [ "Diagram", "guided_knapsack_problem.html#diagram-4", null ],
          [ "Item process (ItemProcess)", "guided_knapsack_problem.html#item-process-itemprocess-2", [
            [ "Status", "guided_knapsack_problem.html#status-52", null ],
            [ "Data", "guided_knapsack_problem.html#data-53", null ]
          ] ],
          [ "StartEvent (StartEventItem)", "guided_knapsack_problem.html#startevent-starteventitem-2", [
            [ "Status", "guided_knapsack_problem.html#status-53", null ],
            [ "Data", "guided_knapsack_problem.html#data-54", null ]
          ] ],
          [ "SendTask (SendRequestTask)", "guided_knapsack_problem.html#sendtask-sendrequesttask-1", [
            [ "Status", "guided_knapsack_problem.html#status-54", null ],
            [ "Data", "guided_knapsack_problem.html#data-55", null ],
            [ "Message", "guided_knapsack_problem.html#message-6", [
              [ "<em>Name</em> Request", "guided_knapsack_problem.html#name-request-4", null ]
            ] ]
          ] ],
          [ "EventBasedGateway (Event-basedGateway)", "guided_knapsack_problem.html#eventbasedgateway-event-basedgateway", [
            [ "Status", "guided_knapsack_problem.html#status-55", null ],
            [ "Data", "guided_knapsack_problem.html#data-56", null ]
          ] ],
          [ "IntermediateCatchEvent (CatchAcceptanceMessage)", "guided_knapsack_problem.html#intermediatecatchevent-catchacceptancemessage", [
            [ "Status", "guided_knapsack_problem.html#status-56", null ],
            [ "Data", "guided_knapsack_problem.html#data-57", null ],
            [ "Message", "guided_knapsack_problem.html#message-7", [
              [ "<em>Name</em> Acceptance", "guided_knapsack_problem.html#name-acceptance", null ]
            ] ]
          ] ],
          [ "EndEvent (ItemAccepted)", "guided_knapsack_problem.html#endevent-itemaccepted", [
            [ "Status", "guided_knapsack_problem.html#status-57", null ],
            [ "Data", "guided_knapsack_problem.html#data-58", null ]
          ] ],
          [ "IntermediateCatchEvent (CatchRejectionMessage)", "guided_knapsack_problem.html#intermediatecatchevent-catchrejectionmessage", [
            [ "Status", "guided_knapsack_problem.html#status-58", null ],
            [ "Data", "guided_knapsack_problem.html#data-59", null ],
            [ "Message", "guided_knapsack_problem.html#message-8", [
              [ "<em>Name</em> Rejection", "guided_knapsack_problem.html#name-rejection", null ]
            ] ]
          ] ],
          [ "EndEvent (ItemRejected)", "guided_knapsack_problem.html#endevent-itemrejected", [
            [ "Status", "guided_knapsack_problem.html#status-59", null ],
            [ "Data", "guided_knapsack_problem.html#data-60", null ]
          ] ],
          [ "Knapsack process (KnapsackProcess)", "guided_knapsack_problem.html#knapsack-process-knapsackprocess", [
            [ "Status", "guided_knapsack_problem.html#status-60", null ],
            [ "Data", "guided_knapsack_problem.html#data-61", null ]
          ] ],
          [ "SubProcess (EventSubProcess)", "guided_knapsack_problem.html#subprocess-eventsubprocess-2", [
            [ "Status", "guided_knapsack_problem.html#status-61", null ],
            [ "Data", "guided_knapsack_problem.html#data-62", null ],
            [ "Operators", "guided_knapsack_problem.html#operators-13", null ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcess)", "guided_knapsack_problem.html#adhocsubprocess-adhocsubprocess-2", [
            [ "Status", "guided_knapsack_problem.html#status-62", null ],
            [ "Data", "guided_knapsack_problem.html#data-63", null ]
          ] ],
          [ "SubProcess (HandleItemActivity)", "guided_knapsack_problem.html#subprocess-handleitemactivity-1", [
            [ "Status", "guided_knapsack_problem.html#status-63", null ],
            [ "Data", "guided_knapsack_problem.html#data-64", null ],
            [ "Entry guidance", "guided_knapsack_problem.html#entry-guidance", [
              [ "Attributes", "guided_knapsack_problem.html#attributes-4", null ],
              [ "Restrictions", "guided_knapsack_problem.html#restrictions-2", null ]
            ] ]
          ] ],
          [ "StartEvent (StartHandleItemEvent)", "guided_knapsack_problem.html#startevent-starthandleitemevent", [
            [ "Status", "guided_knapsack_problem.html#status-64", null ],
            [ "Data", "guided_knapsack_problem.html#data-65", null ]
          ] ],
          [ "ExclusiveGateway (CapacityGateway)", "guided_knapsack_problem.html#exclusivegateway-capacitygateway", [
            [ "Status", "guided_knapsack_problem.html#status-65", null ],
            [ "Data", "guided_knapsack_problem.html#data-66", null ]
          ] ],
          [ "EndEvent (ThrowRejectionMessage)", "guided_knapsack_problem.html#endevent-throwrejectionmessage", [
            [ "Status", "guided_knapsack_problem.html#status-66", null ],
            [ "Data", "guided_knapsack_problem.html#data-67", null ],
            [ "Message", "guided_knapsack_problem.html#message-9", [
              [ "<em>Name</em> Rejection", "guided_knapsack_problem.html#name-rejection-1", null ]
            ] ]
          ] ],
          [ "Task (IncludeItemActivity)", "guided_knapsack_problem.html#task-includeitemactivity-1", [
            [ "Status", "guided_knapsack_problem.html#status-67", null ],
            [ "Data", "guided_knapsack_problem.html#data-68", null ],
            [ "Operators", "guided_knapsack_problem.html#operators-14", null ]
          ] ],
          [ "EndEvent (ThrowAcceptanceMessage)", "guided_knapsack_problem.html#endevent-throwacceptancemessage", [
            [ "Status", "guided_knapsack_problem.html#status-68", null ],
            [ "Data", "guided_knapsack_problem.html#data-69", null ],
            [ "Message", "guided_knapsack_problem.html#message-10", [
              [ "<em>Name</em> Acceptance", "guided_knapsack_problem.html#name-acceptance-1", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventSubProcess)", "guided_knapsack_problem.html#endevent-endeventsubprocess-2", [
            [ "Status", "guided_knapsack_problem.html#status-69", null ],
            [ "Data", "guided_knapsack_problem.html#data-70", null ]
          ] ],
          [ "StartEvent (CatchRequestMessage)", "guided_knapsack_problem.html#startevent-catchrequestmessage-2", [
            [ "Status", "guided_knapsack_problem.html#status-70", null ],
            [ "Data", "guided_knapsack_problem.html#data-71", null ],
            [ "Message", "guided_knapsack_problem.html#message-11", [
              [ "<em>Name</em> Request", "guided_knapsack_problem.html#name-request-5", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventKnapsack)", "guided_knapsack_problem.html#endevent-endeventknapsack", [
            [ "Status", "guided_knapsack_problem.html#status-71", null ],
            [ "Data", "guided_knapsack_problem.html#data-72", null ]
          ] ],
          [ "Task (WaitActivity)", "guided_knapsack_problem.html#task-waitactivity-2", [
            [ "Status", "guided_knapsack_problem.html#status-72", null ],
            [ "Data", "guided_knapsack_problem.html#data-73", null ],
            [ "Exit guidance", "guided_knapsack_problem.html#exit-guidance", [
              [ "Restrictions", "guided_knapsack_problem.html#restrictions-3", null ]
            ] ]
          ] ],
          [ "StartEvent (StartEventKnapsack)", "guided_knapsack_problem.html#startevent-starteventknapsack", [
            [ "Status", "guided_knapsack_problem.html#status-73", null ],
            [ "Data", "guided_knapsack_problem.html#data-74", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_18bke9g)", "guided_knapsack_problem.html#dataobjectreference-dataobjectreference_18bke9g", [
            [ "Status", "guided_knapsack_problem.html#status-74", null ],
            [ "Data", "guided_knapsack_problem.html#data-75", null ]
          ] ],
          [ "SequenceFlow (Flow_0y2js91)", "guided_knapsack_problem.html#sequenceflow-flow_0y2js91", [
            [ "Gatekeeper", "guided_knapsack_problem.html#gatekeeper-2", null ]
          ] ],
          [ "SequenceFlow (Flow_1r0tr4t)", "guided_knapsack_problem.html#sequenceflow-flow_1r0tr4t", [
            [ "Gatekeeper", "guided_knapsack_problem.html#gatekeeper-3", null ]
          ] ]
        ] ]
      ] ],
      [ "Bin packing problem", "bin_packing_problem.html", [
        [ "Collaboration (Bin_packing_problem)", "bin_packing_problem.html#collaboration-bin_packing_problem", [
          [ "Diagram", "bin_packing_problem.html#diagram-1", null ],
          [ "Globals", "bin_packing_problem.html#globals", null ],
          [ "Item process (ItemProcess)", "bin_packing_problem.html#item-process-itemprocess", [
            [ "Status", "bin_packing_problem.html#status-9", null ],
            [ "Data", "bin_packing_problem.html#data-10", null ],
            [ "Globals", "bin_packing_problem.html#globals-1", null ]
          ] ],
          [ "StartEvent (StartEventItem)", "bin_packing_problem.html#startevent-starteventitem", [
            [ "Status", "bin_packing_problem.html#status-10", null ],
            [ "Data", "bin_packing_problem.html#data-11", null ],
            [ "Globals", "bin_packing_problem.html#globals-2", null ]
          ] ],
          [ "EndEvent (EndEventItem)", "bin_packing_problem.html#endevent-endeventitem", [
            [ "Status", "bin_packing_problem.html#status-11", null ],
            [ "Data", "bin_packing_problem.html#data-12", null ],
            [ "Globals", "bin_packing_problem.html#globals-3", null ]
          ] ],
          [ "SendTask (RequestActivity)", "bin_packing_problem.html#sendtask-requestactivity", [
            [ "Status", "bin_packing_problem.html#status-12", null ],
            [ "Data", "bin_packing_problem.html#data-13", null ],
            [ "Globals", "bin_packing_problem.html#globals-4", null ],
            [ "Message", "bin_packing_problem.html#message-2", [
              [ "<em>Name</em> Request", "bin_packing_problem.html#name-request", null ]
            ] ]
          ] ],
          [ "Bin process (BinProcess)", "bin_packing_problem.html#bin-process-binprocess", [
            [ "Status", "bin_packing_problem.html#status-13", null ],
            [ "Data", "bin_packing_problem.html#data-14", null ],
            [ "Globals", "bin_packing_problem.html#globals-5", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions", null ]
          ] ],
          [ "EndEvent (EndEventBin)", "bin_packing_problem.html#endevent-endeventbin", [
            [ "Status", "bin_packing_problem.html#status-14", null ],
            [ "Data", "bin_packing_problem.html#data-15", null ],
            [ "Globals", "bin_packing_problem.html#globals-6", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-1", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-1", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-1", null ]
          ] ],
          [ "Task (WaitActivity)", "bin_packing_problem.html#task-waitactivity", [
            [ "Status", "bin_packing_problem.html#status-15", null ],
            [ "Data", "bin_packing_problem.html#data-16", null ],
            [ "Globals", "bin_packing_problem.html#globals-7", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-2", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-2", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-2", null ]
          ] ],
          [ "StartEvent (StartEvent)", "bin_packing_problem.html#startevent-startevent", [
            [ "Status", "bin_packing_problem.html#status-16", null ],
            [ "Data", "bin_packing_problem.html#data-17", null ],
            [ "Globals", "bin_packing_problem.html#globals-8", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-3", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-3", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-3", null ]
          ] ],
          [ "SubProcess (EventSubProcess)", "bin_packing_problem.html#subprocess-eventsubprocess", [
            [ "Status", "bin_packing_problem.html#status-17", null ],
            [ "Data", "bin_packing_problem.html#data-18", null ],
            [ "Globals", "bin_packing_problem.html#globals-9", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-4", null ],
            [ "Operators", "bin_packing_problem.html#operators-2", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-4", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-4", null ]
          ] ],
          [ "EndEvent (EndEventSubProcess)", "bin_packing_problem.html#endevent-endeventsubprocess", [
            [ "Status", "bin_packing_problem.html#status-18", null ],
            [ "Data", "bin_packing_problem.html#data-19", null ],
            [ "Globals", "bin_packing_problem.html#globals-10", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-5", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-5", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-5", null ]
          ] ],
          [ "StartEvent (CatchRequestMessage)", "bin_packing_problem.html#startevent-catchrequestmessage", [
            [ "Status", "bin_packing_problem.html#status-19", null ],
            [ "Data", "bin_packing_problem.html#data-20", null ],
            [ "Globals", "bin_packing_problem.html#globals-11", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-6", null ],
            [ "Message", "bin_packing_problem.html#message-3", [
              [ "<em>Name</em> Request", "bin_packing_problem.html#name-request-1", null ]
            ] ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-6", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-6", null ],
            [ "Message guidance", "bin_packing_problem.html#message-guidance", [
              [ "Attributes", "bin_packing_problem.html#attributes-1", null ],
              [ "Operators", "bin_packing_problem.html#operators-3", null ]
            ] ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcess)", "bin_packing_problem.html#adhocsubprocess-adhocsubprocess", [
            [ "Status", "bin_packing_problem.html#status-20", null ],
            [ "Data", "bin_packing_problem.html#data-21", null ],
            [ "Globals", "bin_packing_problem.html#globals-12", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-7", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-7", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-7", null ]
          ] ],
          [ "SubProcess (HandleItemActivity)", "bin_packing_problem.html#subprocess-handleitemactivity", [
            [ "Status", "bin_packing_problem.html#status-21", null ],
            [ "Data", "bin_packing_problem.html#data-22", null ],
            [ "Globals", "bin_packing_problem.html#globals-13", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-8", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-8", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-8", null ]
          ] ],
          [ "StartEvent (HandleItemStartEvent)", "bin_packing_problem.html#startevent-handleitemstartevent", [
            [ "Status", "bin_packing_problem.html#status-22", null ],
            [ "Data", "bin_packing_problem.html#data-23", null ],
            [ "Globals", "bin_packing_problem.html#globals-14", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-9", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-9", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-9", null ]
          ] ],
          [ "ExclusiveGateway (SplitGateway)", "bin_packing_problem.html#exclusivegateway-splitgateway", [
            [ "Status", "bin_packing_problem.html#status-23", null ],
            [ "Data", "bin_packing_problem.html#data-24", null ],
            [ "Globals", "bin_packing_problem.html#globals-15", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-10", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-10", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-10", null ]
          ] ],
          [ "Task (ClearBinTask)", "bin_packing_problem.html#task-clearbintask", [
            [ "Status", "bin_packing_problem.html#status-24", null ],
            [ "Data", "bin_packing_problem.html#data-25", null ],
            [ "Globals", "bin_packing_problem.html#globals-16", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-11", null ],
            [ "Operators", "bin_packing_problem.html#operators-4", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-11", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-11", null ]
          ] ],
          [ "ExclusiveGateway (MergeGateway)", "bin_packing_problem.html#exclusivegateway-mergegateway", [
            [ "Status", "bin_packing_problem.html#status-25", null ],
            [ "Data", "bin_packing_problem.html#data-26", null ],
            [ "Globals", "bin_packing_problem.html#globals-17", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-12", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-12", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-12", null ]
          ] ],
          [ "EndEvent (HandleItemEndEvent)", "bin_packing_problem.html#endevent-handleitemendevent", [
            [ "Status", "bin_packing_problem.html#status-26", null ],
            [ "Data", "bin_packing_problem.html#data-27", null ],
            [ "Globals", "bin_packing_problem.html#globals-18", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-13", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-13", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-13", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_04cxu4w)", "bin_packing_problem.html#dataobjectreference-dataobjectreference_04cxu4w", [
            [ "Status", "bin_packing_problem.html#status-27", null ],
            [ "Data", "bin_packing_problem.html#data-28", null ],
            [ "Globals", "bin_packing_problem.html#globals-19", null ],
            [ "Entry restrictions", "bin_packing_problem.html#entry-restrictions-14", null ],
            [ "Completion restrictions", "bin_packing_problem.html#completion-restrictions-14", null ],
            [ "Exit restrictions", "bin_packing_problem.html#exit-restrictions-14", null ]
          ] ],
          [ "SequenceFlow (Flow_0an2vmk)", "bin_packing_problem.html#sequenceflow-flow_0an2vmk", [
            [ "Gatekeeper", "bin_packing_problem.html#gatekeeper", null ]
          ] ],
          [ "SequenceFlow (Flow_0kc2iyd)", "bin_packing_problem.html#sequenceflow-flow_0kc2iyd", [
            [ "Gatekeeper", "bin_packing_problem.html#gatekeeper-1", null ]
          ] ]
        ] ]
      ] ],
      [ "Bin packing problem (guided)", "guided_bin_packing_problem.html", [
        [ "Collaboration (Bin_packing_problem)", "guided_bin_packing_problem.html#collaboration-bin_packing_problem-1", [
          [ "Diagram", "guided_bin_packing_problem.html#diagram-3", null ],
          [ "Globals", "guided_bin_packing_problem.html#globals-20", null ],
          [ "Item process (ItemProcess)", "guided_bin_packing_problem.html#item-process-itemprocess-1", [
            [ "Status", "guided_bin_packing_problem.html#status-38", null ],
            [ "Data", "guided_bin_packing_problem.html#data-39", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-21", null ]
          ] ],
          [ "StartEvent (StartEventItem)", "guided_bin_packing_problem.html#startevent-starteventitem-1", [
            [ "Status", "guided_bin_packing_problem.html#status-39", null ],
            [ "Data", "guided_bin_packing_problem.html#data-40", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-22", null ]
          ] ],
          [ "EndEvent (EndEventItem)", "guided_bin_packing_problem.html#endevent-endeventitem-1", [
            [ "Status", "guided_bin_packing_problem.html#status-40", null ],
            [ "Data", "guided_bin_packing_problem.html#data-41", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-23", null ]
          ] ],
          [ "SendTask (RequestActivity)", "guided_bin_packing_problem.html#sendtask-requestactivity-1", [
            [ "Status", "guided_bin_packing_problem.html#status-41", null ],
            [ "Data", "guided_bin_packing_problem.html#data-42", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-24", null ],
            [ "Operators", "guided_bin_packing_problem.html#operators-8", null ],
            [ "Message", "guided_bin_packing_problem.html#message-4", [
              [ "<em>Name</em> Request", "guided_bin_packing_problem.html#name-request-2", null ]
            ] ]
          ] ],
          [ "Bin process (BinProcess)", "guided_bin_packing_problem.html#bin-process-binprocess-1", [
            [ "Status", "guided_bin_packing_problem.html#status-42", null ],
            [ "Data", "guided_bin_packing_problem.html#data-43", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-25", null ],
            [ "Entry restrictions", "guided_bin_packing_problem.html#entry-restrictions-15", null ],
            [ "Completion restrictions", "guided_bin_packing_problem.html#completion-restrictions-15", null ],
            [ "Exit restrictions", "guided_bin_packing_problem.html#exit-restrictions-16", null ]
          ] ],
          [ "SubProcess (EventSubProcess)", "guided_bin_packing_problem.html#subprocess-eventsubprocess-1", [
            [ "Status", "guided_bin_packing_problem.html#status-43", null ],
            [ "Data", "guided_bin_packing_problem.html#data-44", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-26", null ],
            [ "Entry restrictions", "guided_bin_packing_problem.html#entry-restrictions-16", null ],
            [ "Operators", "guided_bin_packing_problem.html#operators-9", null ],
            [ "Completion restrictions", "guided_bin_packing_problem.html#completion-restrictions-16", null ],
            [ "Exit restrictions", "guided_bin_packing_problem.html#exit-restrictions-17", null ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcess)", "guided_bin_packing_problem.html#adhocsubprocess-adhocsubprocess-1", [
            [ "Status", "guided_bin_packing_problem.html#status-44", null ],
            [ "Data", "guided_bin_packing_problem.html#data-45", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-27", null ],
            [ "Entry restrictions", "guided_bin_packing_problem.html#entry-restrictions-17", null ],
            [ "Completion restrictions", "guided_bin_packing_problem.html#completion-restrictions-17", null ],
            [ "Exit restrictions", "guided_bin_packing_problem.html#exit-restrictions-18", null ]
          ] ],
          [ "Task (IncludeItemActivity)", "guided_bin_packing_problem.html#task-includeitemactivity", [
            [ "Status", "guided_bin_packing_problem.html#status-45", null ],
            [ "Data", "guided_bin_packing_problem.html#data-46", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-28", null ],
            [ "Entry restrictions", "guided_bin_packing_problem.html#entry-restrictions-18", null ],
            [ "Operators", "guided_bin_packing_problem.html#operators-10", null ],
            [ "Completion restrictions", "guided_bin_packing_problem.html#completion-restrictions-18", null ],
            [ "Exit restrictions", "guided_bin_packing_problem.html#exit-restrictions-19", null ]
          ] ],
          [ "EndEvent (EndEventSubProcess)", "guided_bin_packing_problem.html#endevent-endeventsubprocess-1", [
            [ "Status", "guided_bin_packing_problem.html#status-46", null ],
            [ "Data", "guided_bin_packing_problem.html#data-47", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-29", null ],
            [ "Entry restrictions", "guided_bin_packing_problem.html#entry-restrictions-19", null ],
            [ "Completion restrictions", "guided_bin_packing_problem.html#completion-restrictions-19", null ],
            [ "Exit restrictions", "guided_bin_packing_problem.html#exit-restrictions-20", null ]
          ] ],
          [ "StartEvent (CatchRequestMessage)", "guided_bin_packing_problem.html#startevent-catchrequestmessage-1", [
            [ "Status", "guided_bin_packing_problem.html#status-47", null ],
            [ "Data", "guided_bin_packing_problem.html#data-48", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-30", null ],
            [ "Entry restrictions", "guided_bin_packing_problem.html#entry-restrictions-20", null ],
            [ "Message", "guided_bin_packing_problem.html#message-5", [
              [ "<em>Name</em> Request", "guided_bin_packing_problem.html#name-request-3", null ]
            ] ],
            [ "Completion restrictions", "guided_bin_packing_problem.html#completion-restrictions-20", null ],
            [ "Exit restrictions", "guided_bin_packing_problem.html#exit-restrictions-21", null ],
            [ "Message guidance", "guided_bin_packing_problem.html#message-guidance-1", [
              [ "Attributes", "guided_bin_packing_problem.html#attributes-3", null ],
              [ "Operators", "guided_bin_packing_problem.html#operators-11", null ],
              [ "Restrictions", "guided_bin_packing_problem.html#restrictions-1", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventBin)", "guided_bin_packing_problem.html#endevent-endeventbin-1", [
            [ "Status", "guided_bin_packing_problem.html#status-48", null ],
            [ "Data", "guided_bin_packing_problem.html#data-49", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-31", null ],
            [ "Entry restrictions", "guided_bin_packing_problem.html#entry-restrictions-21", null ],
            [ "Completion restrictions", "guided_bin_packing_problem.html#completion-restrictions-21", null ],
            [ "Exit restrictions", "guided_bin_packing_problem.html#exit-restrictions-22", null ]
          ] ],
          [ "Task (WaitActivity)", "guided_bin_packing_problem.html#task-waitactivity-1", [
            [ "Status", "guided_bin_packing_problem.html#status-49", null ],
            [ "Data", "guided_bin_packing_problem.html#data-50", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-32", null ],
            [ "Entry restrictions", "guided_bin_packing_problem.html#entry-restrictions-22", null ],
            [ "Operators", "guided_bin_packing_problem.html#operators-12", null ],
            [ "Completion restrictions", "guided_bin_packing_problem.html#completion-restrictions-22", null ],
            [ "Exit restrictions", "guided_bin_packing_problem.html#exit-restrictions-23", null ]
          ] ],
          [ "StartEvent (StartEvent)", "guided_bin_packing_problem.html#startevent-startevent-1", [
            [ "Status", "guided_bin_packing_problem.html#status-50", null ],
            [ "Data", "guided_bin_packing_problem.html#data-51", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-33", null ],
            [ "Entry restrictions", "guided_bin_packing_problem.html#entry-restrictions-23", null ],
            [ "Completion restrictions", "guided_bin_packing_problem.html#completion-restrictions-23", null ],
            [ "Exit restrictions", "guided_bin_packing_problem.html#exit-restrictions-24", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_04cxu4w)", "guided_bin_packing_problem.html#dataobjectreference-dataobjectreference_04cxu4w-1", [
            [ "Status", "guided_bin_packing_problem.html#status-51", null ],
            [ "Data", "guided_bin_packing_problem.html#data-52", null ],
            [ "Globals", "guided_bin_packing_problem.html#globals-34", null ],
            [ "Entry restrictions", "guided_bin_packing_problem.html#entry-restrictions-24", null ],
            [ "Completion restrictions", "guided_bin_packing_problem.html#completion-restrictions-24", null ],
            [ "Exit restrictions", "guided_bin_packing_problem.html#exit-restrictions-25", null ]
          ] ]
        ] ]
      ] ],
      [ "Job shop scheduling problem", "job_shop_scheduling_problem.html", [
        [ "Collaboration (Job_shop_scheduling_problem)", "job_shop_scheduling_problem.html#collaboration-job_shop_scheduling_problem", [
          [ "Diagram", "job_shop_scheduling_problem.html#diagram-7", null ],
          [ "Globals", "job_shop_scheduling_problem.html#globals-97", null ],
          [ "Order process (OrderProcess)", "job_shop_scheduling_problem.html#order-process-orderprocess", [
            [ "Status", "job_shop_scheduling_problem.html#status-135", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-136", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-98", null ]
          ] ],
          [ "StartEvent (StartEventItem)", "job_shop_scheduling_problem.html#startevent-starteventitem-3", [
            [ "Status", "job_shop_scheduling_problem.html#status-136", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-137", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-99", null ]
          ] ],
          [ "EndEvent (EndEventItem)", "job_shop_scheduling_problem.html#endevent-endeventitem-2", [
            [ "Status", "job_shop_scheduling_problem.html#status-137", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-138", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-100", null ]
          ] ],
          [ "SubProcess (JobActivity)", "job_shop_scheduling_problem.html#subprocess-jobactivity", [
            [ "Status", "job_shop_scheduling_problem.html#status-138", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-139", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-101", null ],
            [ "Multi-instance", "job_shop_scheduling_problem.html#multi-instance", null ]
          ] ],
          [ "StartEvent (JobStartEvent)", "job_shop_scheduling_problem.html#startevent-jobstartevent", [
            [ "Status", "job_shop_scheduling_problem.html#status-139", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-140", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-102", null ]
          ] ],
          [ "EndEvent (JobEndEvent)", "job_shop_scheduling_problem.html#endevent-jobendevent", [
            [ "Status", "job_shop_scheduling_problem.html#status-140", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-141", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-103", null ]
          ] ],
          [ "SendTask (SendJobTask)", "job_shop_scheduling_problem.html#sendtask-sendjobtask", [
            [ "Status", "job_shop_scheduling_problem.html#status-141", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-142", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-104", null ],
            [ "Message", "job_shop_scheduling_problem.html#message-28", [
              [ "<em>Name</em> Request", "job_shop_scheduling_problem.html#name-request-10", null ]
            ] ]
          ] ],
          [ "ReceiveTask (NoticeJobCompletionTask)", "job_shop_scheduling_problem.html#receivetask-noticejobcompletiontask", [
            [ "Status", "job_shop_scheduling_problem.html#status-142", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-143", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-105", null ],
            [ "Message", "job_shop_scheduling_problem.html#message-29", [
              [ "<em>Name</em> Completion", "job_shop_scheduling_problem.html#name-completion", null ]
            ] ],
            [ "Operators", "job_shop_scheduling_problem.html#operators-27", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_0nc3n7b)", "job_shop_scheduling_problem.html#dataobjectreference-dataobjectreference_0nc3n7b", [
            [ "Status", "job_shop_scheduling_problem.html#status-143", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-144", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-106", null ]
          ] ],
          [ "Machine process (MachineProcess)", "job_shop_scheduling_problem.html#machine-process-machineprocess", [
            [ "Status", "job_shop_scheduling_problem.html#status-144", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-145", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-107", null ]
          ] ],
          [ "SubProcess (EventSubProcess)", "job_shop_scheduling_problem.html#subprocess-eventsubprocess-5", [
            [ "Status", "job_shop_scheduling_problem.html#status-145", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-146", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-108", null ],
            [ "Operators", "job_shop_scheduling_problem.html#operators-28", null ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcess)", "job_shop_scheduling_problem.html#adhocsubprocess-adhocsubprocess-3", [
            [ "Status", "job_shop_scheduling_problem.html#status-146", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-147", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-109", null ]
          ] ],
          [ "Task (ConductJobTask)", "job_shop_scheduling_problem.html#task-conductjobtask", [
            [ "Status", "job_shop_scheduling_problem.html#status-147", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-148", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-110", null ],
            [ "Operators", "job_shop_scheduling_problem.html#operators-29", null ]
          ] ],
          [ "StartEvent (CatchRequestMessage)", "job_shop_scheduling_problem.html#startevent-catchrequestmessage-3", [
            [ "Status", "job_shop_scheduling_problem.html#status-148", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-149", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-111", null ],
            [ "Message", "job_shop_scheduling_problem.html#message-30", [
              [ "<em>Name</em> Request", "job_shop_scheduling_problem.html#name-request-11", null ]
            ] ]
          ] ],
          [ "EndEvent (ThrowCompletionMessage)", "job_shop_scheduling_problem.html#endevent-throwcompletionmessage", [
            [ "Status", "job_shop_scheduling_problem.html#status-149", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-150", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-112", null ],
            [ "Message", "job_shop_scheduling_problem.html#message-31", [
              [ "<em>Name</em> Completion", "job_shop_scheduling_problem.html#name-completion-1", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventMachine)", "job_shop_scheduling_problem.html#endevent-endeventmachine", [
            [ "Status", "job_shop_scheduling_problem.html#status-150", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-151", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-113", null ]
          ] ],
          [ "StartEvent (StartEventMachine)", "job_shop_scheduling_problem.html#startevent-starteventmachine", [
            [ "Status", "job_shop_scheduling_problem.html#status-151", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-152", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-114", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_0j8smsl)", "job_shop_scheduling_problem.html#dataobjectreference-dataobjectreference_0j8smsl", [
            [ "Status", "job_shop_scheduling_problem.html#status-152", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-153", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-115", null ]
          ] ],
          [ "IntermediateCatchEvent (ConditionalEvent)", "job_shop_scheduling_problem.html#intermediatecatchevent-conditionalevent", [
            [ "Status", "job_shop_scheduling_problem.html#status-153", null ],
            [ "Data", "job_shop_scheduling_problem.html#data-154", null ],
            [ "Globals", "job_shop_scheduling_problem.html#globals-116", null ],
            [ "Conditions", "job_shop_scheduling_problem.html#conditions", null ]
          ] ]
        ] ]
      ] ],
      [ "Vehicle routing problem", "vehicle_routing_problem.html", [
        [ "Collaboration (VehicleRoutingProblem)", "vehicle_routing_problem.html#collaboration-vehicleroutingproblem-1", [
          [ "Diagram", "vehicle_routing_problem.html#diagram-14", null ],
          [ "Globals", "vehicle_routing_problem.html#globals-153", null ],
          [ "Lookup tables", "vehicle_routing_problem.html#lookup-tables-7", null ],
          [ "Customer process (CustomerProcess)", "vehicle_routing_problem.html#customer-process-customerprocess-3", [
            [ "Status", "vehicle_routing_problem.html#status-257", null ],
            [ "Data", "vehicle_routing_problem.html#data-258", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-154", null ]
          ] ],
          [ "StartEvent (StartEventCustomer)", "vehicle_routing_problem.html#startevent-starteventcustomer-3", [
            [ "Status", "vehicle_routing_problem.html#status-258", null ],
            [ "Data", "vehicle_routing_problem.html#data-259", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-155", null ]
          ] ],
          [ "SendTask (RequestActivity)", "vehicle_routing_problem.html#sendtask-requestactivity-5", [
            [ "Status", "vehicle_routing_problem.html#status-259", null ],
            [ "Data", "vehicle_routing_problem.html#data-260", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-156", null ],
            [ "Message", "vehicle_routing_problem.html#message-48", [
              [ "<em>Name</em> Request", "vehicle_routing_problem.html#name-request-16", null ]
            ] ]
          ] ],
          [ "IntermediateCatchEvent (CatchArrival)", "vehicle_routing_problem.html#intermediatecatchevent-catcharrival-1", [
            [ "Status", "vehicle_routing_problem.html#status-260", null ],
            [ "Data", "vehicle_routing_problem.html#data-261", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-157", null ],
            [ "Message", "vehicle_routing_problem.html#message-49", [
              [ "<em>Name</em> Arrival", "vehicle_routing_problem.html#name-arrival-10", null ]
            ] ]
          ] ],
          [ "IntermediateCatchEvent (WaitEvent)", "vehicle_routing_problem.html#intermediatecatchevent-waitevent-1", [
            [ "Status", "vehicle_routing_problem.html#status-261", null ],
            [ "Data", "vehicle_routing_problem.html#data-262", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-158", null ],
            [ "Timer", "vehicle_routing_problem.html#timer-8", null ]
          ] ],
          [ "EndEvent (EndEventCustomer)", "vehicle_routing_problem.html#endevent-endeventcustomer-3", [
            [ "Status", "vehicle_routing_problem.html#status-262", null ],
            [ "Data", "vehicle_routing_problem.html#data-263", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-159", null ]
          ] ],
          [ "Task (LoadActivity)", "vehicle_routing_problem.html#task-loadactivity-3", [
            [ "Status", "vehicle_routing_problem.html#status-263", null ],
            [ "Data", "vehicle_routing_problem.html#data-264", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-160", null ],
            [ "Operators", "vehicle_routing_problem.html#operators-48", null ]
          ] ],
          [ "IntermediateThrowEvent (ThrowShipmentHandled)", "vehicle_routing_problem.html#intermediatethrowevent-throwshipmenthandled-1", [
            [ "Status", "vehicle_routing_problem.html#status-264", null ],
            [ "Data", "vehicle_routing_problem.html#data-265", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-161", null ],
            [ "Message", "vehicle_routing_problem.html#message-50", [
              [ "<em>Name</em> Ready", "vehicle_routing_problem.html#name-ready-2", null ]
            ] ]
          ] ],
          [ "Vehicle process (VehicleProcess)", "vehicle_routing_problem.html#vehicle-process-vehicleprocess-3", [
            [ "Status", "vehicle_routing_problem.html#status-265", null ],
            [ "Data", "vehicle_routing_problem.html#data-266", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-162", null ]
          ] ],
          [ "EndEvent (EndEventVehicle)", "vehicle_routing_problem.html#endevent-endeventvehicle-3", [
            [ "Status", "vehicle_routing_problem.html#status-266", null ],
            [ "Data", "vehicle_routing_problem.html#data-267", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-163", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_0tut0wl)", "vehicle_routing_problem.html#dataobjectreference-dataobjectreference_0tut0wl-3", [
            [ "Status", "vehicle_routing_problem.html#status-267", null ],
            [ "Data", "vehicle_routing_problem.html#data-268", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-164", null ]
          ] ],
          [ "StartEvent (StartEventVehicle)", "vehicle_routing_problem.html#startevent-starteventvehicle-3", [
            [ "Status", "vehicle_routing_problem.html#status-268", null ],
            [ "Data", "vehicle_routing_problem.html#data-269", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-165", null ]
          ] ],
          [ "AdHocSubProcess (Activity_03cfvja)", "vehicle_routing_problem.html#adhocsubprocess-activity_03cfvja-1", [
            [ "Status", "vehicle_routing_problem.html#status-269", null ],
            [ "Data", "vehicle_routing_problem.html#data-270", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-166", null ]
          ] ],
          [ "Task (ReturnTrip)", "vehicle_routing_problem.html#task-returntrip-4", [
            [ "Status", "vehicle_routing_problem.html#status-270", null ],
            [ "Data", "vehicle_routing_problem.html#data-271", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-167", null ],
            [ "Entry restrictions", "vehicle_routing_problem.html#entry-restrictions-30", null ],
            [ "Operators", "vehicle_routing_problem.html#operators-49", null ]
          ] ],
          [ "SubProcess (EventSubProcess)", "vehicle_routing_problem.html#subprocess-eventsubprocess-8", [
            [ "Status", "vehicle_routing_problem.html#status-271", null ],
            [ "Data", "vehicle_routing_problem.html#data-272", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-168", null ],
            [ "Operators", "vehicle_routing_problem.html#operators-50", null ],
            [ "Exit restrictions", "vehicle_routing_problem.html#exit-restrictions-28", null ]
          ] ],
          [ "StartEvent (StartEventRequest)", "vehicle_routing_problem.html#startevent-starteventrequest-3", [
            [ "Status", "vehicle_routing_problem.html#status-272", null ],
            [ "Data", "vehicle_routing_problem.html#data-273", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-169", null ],
            [ "Message", "vehicle_routing_problem.html#message-51", [
              [ "<em>Name</em> Request", "vehicle_routing_problem.html#name-request-17", null ]
            ] ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcessRequest)", "vehicle_routing_problem.html#adhocsubprocess-adhocsubprocessrequest-3", [
            [ "Status", "vehicle_routing_problem.html#status-273", null ],
            [ "Data", "vehicle_routing_problem.html#data-274", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-170", null ]
          ] ],
          [ "SubProcess (VisitCustomerActivity)", "vehicle_routing_problem.html#subprocess-visitcustomeractivity-1", [
            [ "Status", "vehicle_routing_problem.html#status-274", null ],
            [ "Data", "vehicle_routing_problem.html#data-275", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-171", null ]
          ] ],
          [ "StartEvent (StartEventVisit)", "vehicle_routing_problem.html#startevent-starteventvisit-1", [
            [ "Status", "vehicle_routing_problem.html#status-275", null ],
            [ "Data", "vehicle_routing_problem.html#data-276", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-172", null ]
          ] ],
          [ "Task (CustomerTrip)", "vehicle_routing_problem.html#task-customertrip-1", [
            [ "Status", "vehicle_routing_problem.html#status-276", null ],
            [ "Data", "vehicle_routing_problem.html#data-277", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-173", null ],
            [ "Operators", "vehicle_routing_problem.html#operators-51", null ]
          ] ],
          [ "IntermediateThrowEvent (ThrowArrival)", "vehicle_routing_problem.html#intermediatethrowevent-throwarrival-1", [
            [ "Status", "vehicle_routing_problem.html#status-277", null ],
            [ "Data", "vehicle_routing_problem.html#data-278", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-174", null ],
            [ "Message", "vehicle_routing_problem.html#message-52", [
              [ "<em>Name</em> Arrival", "vehicle_routing_problem.html#name-arrival-11", null ]
            ] ]
          ] ],
          [ "IntermediateCatchEvent (CatchShipmentHandled)", "vehicle_routing_problem.html#intermediatecatchevent-catchshipmenthandled-1", [
            [ "Status", "vehicle_routing_problem.html#status-278", null ],
            [ "Data", "vehicle_routing_problem.html#data-279", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-175", null ],
            [ "Message", "vehicle_routing_problem.html#message-53", [
              [ "<em>Name</em> Ready", "vehicle_routing_problem.html#name-ready-3", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventVisit)", "vehicle_routing_problem.html#endevent-endeventvisit-1", [
            [ "Status", "vehicle_routing_problem.html#status-279", null ],
            [ "Data", "vehicle_routing_problem.html#data-280", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-176", null ]
          ] ],
          [ "EndEvent (EndEventRequest)", "vehicle_routing_problem.html#endevent-endeventrequest-3", [
            [ "Status", "vehicle_routing_problem.html#status-280", null ],
            [ "Data", "vehicle_routing_problem.html#data-281", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-177", null ]
          ] ],
          [ "DataStoreReference (DataStoreReference_0kxfq21)", "vehicle_routing_problem.html#datastorereference-datastorereference_0kxfq21", [
            [ "Status", "vehicle_routing_problem.html#status-281", null ],
            [ "Data", "vehicle_routing_problem.html#data-282", null ],
            [ "Globals", "vehicle_routing_problem.html#globals-178", null ]
          ] ]
        ] ]
      ] ],
      [ "Vehicle routing problem (guided)", "guided_vehicle_routing_problem.html", [
        [ "Collaboration (VehicleRoutingProblem)", "guided_vehicle_routing_problem.html#collaboration-vehicleroutingproblem", [
          [ "Diagram", "guided_vehicle_routing_problem.html#diagram-6", null ],
          [ "Globals", "guided_vehicle_routing_problem.html#globals-71", null ],
          [ "Lookup tables", "guided_vehicle_routing_problem.html#lookup-tables-4", null ],
          [ "Customer process (CustomerProcess)", "guided_vehicle_routing_problem.html#customer-process-customerprocess-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-110", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-111", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-72", null ]
          ] ],
          [ "StartEvent (StartEventCustomer)", "guided_vehicle_routing_problem.html#startevent-starteventcustomer-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-111", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-112", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-73", null ]
          ] ],
          [ "SendTask (RequestActivity)", "guided_vehicle_routing_problem.html#sendtask-requestactivity-3", [
            [ "Status", "guided_vehicle_routing_problem.html#status-112", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-113", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-74", null ],
            [ "Message", "guided_vehicle_routing_problem.html#message-22", [
              [ "<em>Name</em> Request", "guided_vehicle_routing_problem.html#name-request-8", null ]
            ] ]
          ] ],
          [ "IntermediateCatchEvent (CatchArrival)", "guided_vehicle_routing_problem.html#intermediatecatchevent-catcharrival", [
            [ "Status", "guided_vehicle_routing_problem.html#status-113", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-114", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-75", null ],
            [ "Message", "guided_vehicle_routing_problem.html#message-23", [
              [ "<em>Name</em> Arrival", "guided_vehicle_routing_problem.html#name-arrival-4", null ]
            ] ]
          ] ],
          [ "IntermediateCatchEvent (WaitEvent)", "guided_vehicle_routing_problem.html#intermediatecatchevent-waitevent", [
            [ "Status", "guided_vehicle_routing_problem.html#status-114", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-115", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-76", null ],
            [ "Timer", "guided_vehicle_routing_problem.html#timer-4", null ]
          ] ],
          [ "EndEvent (EndEventCustomer)", "guided_vehicle_routing_problem.html#endevent-endeventcustomer-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-115", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-116", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-77", null ]
          ] ],
          [ "Task (LoadActivity)", "guided_vehicle_routing_problem.html#task-loadactivity-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-116", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-117", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-78", null ],
            [ "Operators", "guided_vehicle_routing_problem.html#operators-23", null ]
          ] ],
          [ "IntermediateThrowEvent (ThrowShipmentHandled)", "guided_vehicle_routing_problem.html#intermediatethrowevent-throwshipmenthandled", [
            [ "Status", "guided_vehicle_routing_problem.html#status-117", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-118", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-79", null ],
            [ "Message", "guided_vehicle_routing_problem.html#message-24", [
              [ "<em>Name</em> Ready", "guided_vehicle_routing_problem.html#name-ready", null ]
            ] ]
          ] ],
          [ "Vehicle process (VehicleProcess)", "guided_vehicle_routing_problem.html#vehicle-process-vehicleprocess-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-118", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-119", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-80", null ]
          ] ],
          [ "EndEvent (EndEventVehicle)", "guided_vehicle_routing_problem.html#endevent-endeventvehicle-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-119", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-120", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-81", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_0tut0wl)", "guided_vehicle_routing_problem.html#dataobjectreference-dataobjectreference_0tut0wl-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-120", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-121", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-82", null ]
          ] ],
          [ "StartEvent (StartEventVehicle)", "guided_vehicle_routing_problem.html#startevent-starteventvehicle-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-121", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-122", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-83", null ]
          ] ],
          [ "AdHocSubProcess (Activity_03cfvja)", "guided_vehicle_routing_problem.html#adhocsubprocess-activity_03cfvja", [
            [ "Status", "guided_vehicle_routing_problem.html#status-122", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-123", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-84", null ]
          ] ],
          [ "Task (ReturnTrip)", "guided_vehicle_routing_problem.html#task-returntrip-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-123", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-124", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-85", null ],
            [ "Entry restrictions", "guided_vehicle_routing_problem.html#entry-restrictions-27", null ],
            [ "Operators", "guided_vehicle_routing_problem.html#operators-24", null ]
          ] ],
          [ "SubProcess (EventSubProcess)", "guided_vehicle_routing_problem.html#subprocess-eventsubprocess-4", [
            [ "Status", "guided_vehicle_routing_problem.html#status-124", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-125", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-86", null ],
            [ "Operators", "guided_vehicle_routing_problem.html#operators-25", null ],
            [ "Exit restrictions", "guided_vehicle_routing_problem.html#exit-restrictions-26", null ]
          ] ],
          [ "StartEvent (StartEventRequest)", "guided_vehicle_routing_problem.html#startevent-starteventrequest-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-125", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-126", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-87", null ],
            [ "Message", "guided_vehicle_routing_problem.html#message-25", [
              [ "<em>Name</em> Request", "guided_vehicle_routing_problem.html#name-request-9", null ]
            ] ],
            [ "Message guidance", "guided_vehicle_routing_problem.html#message-guidance-3", [
              [ "Attributes", "guided_vehicle_routing_problem.html#attributes-9", null ],
              [ "Restrictions", "guided_vehicle_routing_problem.html#restrictions-5", null ]
            ] ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcessRequest)", "guided_vehicle_routing_problem.html#adhocsubprocess-adhocsubprocessrequest-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-126", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-127", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-88", null ]
          ] ],
          [ "SubProcess (VisitCustomerActivity)", "guided_vehicle_routing_problem.html#subprocess-visitcustomeractivity", [
            [ "Status", "guided_vehicle_routing_problem.html#status-127", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-128", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-89", null ]
          ] ],
          [ "StartEvent (StartEventVisit)", "guided_vehicle_routing_problem.html#startevent-starteventvisit", [
            [ "Status", "guided_vehicle_routing_problem.html#status-128", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-129", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-90", null ]
          ] ],
          [ "Task (CustomerTrip)", "guided_vehicle_routing_problem.html#task-customertrip", [
            [ "Status", "guided_vehicle_routing_problem.html#status-129", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-130", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-91", null ],
            [ "Operators", "guided_vehicle_routing_problem.html#operators-26", null ],
            [ "Entry guidance", "guided_vehicle_routing_problem.html#entry-guidance-4", [
              [ "Attributes", "guided_vehicle_routing_problem.html#attributes-10", null ]
            ] ]
          ] ],
          [ "IntermediateThrowEvent (ThrowArrival)", "guided_vehicle_routing_problem.html#intermediatethrowevent-throwarrival", [
            [ "Status", "guided_vehicle_routing_problem.html#status-130", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-131", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-92", null ],
            [ "Message", "guided_vehicle_routing_problem.html#message-26", [
              [ "<em>Name</em> Arrival", "guided_vehicle_routing_problem.html#name-arrival-5", null ]
            ] ]
          ] ],
          [ "IntermediateCatchEvent (CatchShipmentHandled)", "guided_vehicle_routing_problem.html#intermediatecatchevent-catchshipmenthandled", [
            [ "Status", "guided_vehicle_routing_problem.html#status-131", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-132", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-93", null ],
            [ "Message", "guided_vehicle_routing_problem.html#message-27", [
              [ "<em>Name</em> Ready", "guided_vehicle_routing_problem.html#name-ready-1", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventVisit)", "guided_vehicle_routing_problem.html#endevent-endeventvisit", [
            [ "Status", "guided_vehicle_routing_problem.html#status-132", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-133", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-94", null ]
          ] ],
          [ "EndEvent (EndEventRequest)", "guided_vehicle_routing_problem.html#endevent-endeventrequest-1", [
            [ "Status", "guided_vehicle_routing_problem.html#status-133", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-134", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-95", null ]
          ] ],
          [ "DataStoreReference (DataStoreReference_1o75s8b)", "guided_vehicle_routing_problem.html#datastorereference-datastorereference_1o75s8b", [
            [ "Status", "guided_vehicle_routing_problem.html#status-134", null ],
            [ "Data", "guided_vehicle_routing_problem.html#data-135", null ],
            [ "Globals", "guided_vehicle_routing_problem.html#globals-96", null ]
          ] ]
        ] ]
      ] ],
      [ "Pickup and delivery problem", "pickup_delivery_problem.html", [
        [ "Collaboration (PickupDeliveryProblem)", "pickup_delivery_problem.html#collaboration-pickupdeliveryproblem-1", [
          [ "Diagram", "pickup_delivery_problem.html#diagram-9", null ],
          [ "Globals", "pickup_delivery_problem.html#globals-117", null ],
          [ "Lookup tables", "pickup_delivery_problem.html#lookup-tables-5", null ],
          [ "Customer process (CustomerProcess)", "pickup_delivery_problem.html#customer-process-customerprocess-2", [
            [ "Status", "pickup_delivery_problem.html#status-177", null ],
            [ "Data", "pickup_delivery_problem.html#data-178", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-118", null ]
          ] ],
          [ "StartEvent (StartEventCustomer)", "pickup_delivery_problem.html#startevent-starteventcustomer-2", [
            [ "Status", "pickup_delivery_problem.html#status-178", null ],
            [ "Data", "pickup_delivery_problem.html#data-179", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-119", null ]
          ] ],
          [ "IntermediateThrowEvent (ThrowShipmentUnloaded)", "pickup_delivery_problem.html#intermediatethrowevent-throwshipmentunloaded-1", [
            [ "Status", "pickup_delivery_problem.html#status-179", null ],
            [ "Data", "pickup_delivery_problem.html#data-180", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-120", null ],
            [ "Message", "pickup_delivery_problem.html#message-38", [
              [ "<em>Name</em> Unloaded", "pickup_delivery_problem.html#name-unloaded-2", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventCustomer)", "pickup_delivery_problem.html#endevent-endeventcustomer-2", [
            [ "Status", "pickup_delivery_problem.html#status-180", null ],
            [ "Data", "pickup_delivery_problem.html#data-181", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-121", null ]
          ] ],
          [ "Task (UnloadActivity)", "pickup_delivery_problem.html#task-unloadactivity-1", [
            [ "Status", "pickup_delivery_problem.html#status-181", null ],
            [ "Data", "pickup_delivery_problem.html#data-182", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-122", null ],
            [ "Operators", "pickup_delivery_problem.html#operators-32", null ]
          ] ],
          [ "IntermediateCatchEvent (DeliveryWaitEvent)", "pickup_delivery_problem.html#intermediatecatchevent-deliverywaitevent-1", [
            [ "Status", "pickup_delivery_problem.html#status-182", null ],
            [ "Data", "pickup_delivery_problem.html#data-183", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-123", null ],
            [ "Timer", "pickup_delivery_problem.html#timer-5", null ]
          ] ],
          [ "Task (LoadActivity)", "pickup_delivery_problem.html#task-loadactivity-2", [
            [ "Status", "pickup_delivery_problem.html#status-183", null ],
            [ "Data", "pickup_delivery_problem.html#data-184", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-124", null ],
            [ "Operators", "pickup_delivery_problem.html#operators-33", null ]
          ] ],
          [ "IntermediateCatchEvent (PickupWaitEvent)", "pickup_delivery_problem.html#intermediatecatchevent-pickupwaitevent-1", [
            [ "Status", "pickup_delivery_problem.html#status-184", null ],
            [ "Data", "pickup_delivery_problem.html#data-185", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-125", null ],
            [ "Timer", "pickup_delivery_problem.html#timer-6", null ]
          ] ],
          [ "IntermediateCatchEvent (CatchPickupArrival)", "pickup_delivery_problem.html#intermediatecatchevent-catchpickuparrival-1", [
            [ "Status", "pickup_delivery_problem.html#status-185", null ],
            [ "Data", "pickup_delivery_problem.html#data-186", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-126", null ],
            [ "Message", "pickup_delivery_problem.html#message-39", [
              [ "<em>Name</em> Arrival", "pickup_delivery_problem.html#name-arrival-6", null ]
            ] ]
          ] ],
          [ "IntermediateThrowEvent (ThrowShipmentLoaded)", "pickup_delivery_problem.html#intermediatethrowevent-throwshipmentloaded-1", [
            [ "Status", "pickup_delivery_problem.html#status-186", null ],
            [ "Data", "pickup_delivery_problem.html#data-187", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-127", null ],
            [ "Message", "pickup_delivery_problem.html#message-40", [
              [ "<em>Name</em> Loaded", "pickup_delivery_problem.html#name-loaded-2", null ]
            ] ]
          ] ],
          [ "IntermediateCatchEvent (CatchDestinationArrival)", "pickup_delivery_problem.html#intermediatecatchevent-catchdestinationarrival-1", [
            [ "Status", "pickup_delivery_problem.html#status-187", null ],
            [ "Data", "pickup_delivery_problem.html#data-188", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-128", null ],
            [ "Message", "pickup_delivery_problem.html#message-41", [
              [ "<em>Name</em> Arrival", "pickup_delivery_problem.html#name-arrival-7", null ]
            ] ]
          ] ],
          [ "SendTask (RequestActivity)", "pickup_delivery_problem.html#sendtask-requestactivity-4", [
            [ "Status", "pickup_delivery_problem.html#status-188", null ],
            [ "Data", "pickup_delivery_problem.html#data-189", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-129", null ],
            [ "Message", "pickup_delivery_problem.html#message-42", [
              [ "<em>Name</em> Request", "pickup_delivery_problem.html#name-request-14", null ]
            ] ]
          ] ],
          [ "Vehicle process (VehicleProcess)", "pickup_delivery_problem.html#vehicle-process-vehicleprocess-2", [
            [ "Status", "pickup_delivery_problem.html#status-189", null ],
            [ "Data", "pickup_delivery_problem.html#data-190", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-130", null ]
          ] ],
          [ "SubProcess (EventSubProcess)", "pickup_delivery_problem.html#subprocess-eventsubprocess-7", [
            [ "Status", "pickup_delivery_problem.html#status-190", null ],
            [ "Data", "pickup_delivery_problem.html#data-191", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-131", null ],
            [ "Operators", "pickup_delivery_problem.html#operators-34", null ]
          ] ],
          [ "StartEvent (StartEventRequest)", "pickup_delivery_problem.html#startevent-starteventrequest-2", [
            [ "Status", "pickup_delivery_problem.html#status-191", null ],
            [ "Data", "pickup_delivery_problem.html#data-192", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-132", null ],
            [ "Message", "pickup_delivery_problem.html#message-43", [
              [ "<em>Name</em> Request", "pickup_delivery_problem.html#name-request-15", null ]
            ] ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcessRequest)", "pickup_delivery_problem.html#adhocsubprocess-adhocsubprocessrequest-2", [
            [ "Status", "pickup_delivery_problem.html#status-192", null ],
            [ "Data", "pickup_delivery_problem.html#data-193", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-133", null ]
          ] ],
          [ "SubProcess (PickupActivity)", "pickup_delivery_problem.html#subprocess-pickupactivity-1", [
            [ "Status", "pickup_delivery_problem.html#status-193", null ],
            [ "Data", "pickup_delivery_problem.html#data-194", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-134", null ],
            [ "Entry restrictions", "pickup_delivery_problem.html#entry-restrictions-28", null ]
          ] ],
          [ "StartEvent (StartEventPickup)", "pickup_delivery_problem.html#startevent-starteventpickup-1", [
            [ "Status", "pickup_delivery_problem.html#status-194", null ],
            [ "Data", "pickup_delivery_problem.html#data-195", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-135", null ]
          ] ],
          [ "Task (PickupTrip)", "pickup_delivery_problem.html#task-pickuptrip-1", [
            [ "Status", "pickup_delivery_problem.html#status-195", null ],
            [ "Data", "pickup_delivery_problem.html#data-196", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-136", null ],
            [ "Operators", "pickup_delivery_problem.html#operators-35", null ]
          ] ],
          [ "IntermediateThrowEvent (ThrowPickupArrival)", "pickup_delivery_problem.html#intermediatethrowevent-throwpickuparrival-1", [
            [ "Status", "pickup_delivery_problem.html#status-196", null ],
            [ "Data", "pickup_delivery_problem.html#data-197", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-137", null ],
            [ "Message", "pickup_delivery_problem.html#message-44", [
              [ "<em>Name</em> Arrival", "pickup_delivery_problem.html#name-arrival-8", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventPickup)", "pickup_delivery_problem.html#endevent-endeventpickup-1", [
            [ "Status", "pickup_delivery_problem.html#status-197", null ],
            [ "Data", "pickup_delivery_problem.html#data-198", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-138", null ]
          ] ],
          [ "ReceiveTask (LoadReceiveTask)", "pickup_delivery_problem.html#receivetask-loadreceivetask-1", [
            [ "Status", "pickup_delivery_problem.html#status-198", null ],
            [ "Data", "pickup_delivery_problem.html#data-199", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-139", null ],
            [ "Message", "pickup_delivery_problem.html#message-45", [
              [ "<em>Name</em> Loaded", "pickup_delivery_problem.html#name-loaded-3", null ]
            ] ],
            [ "Operators", "pickup_delivery_problem.html#operators-36", null ]
          ] ],
          [ "SubProcess (DeliveryActivity)", "pickup_delivery_problem.html#subprocess-deliveryactivity-1", [
            [ "Status", "pickup_delivery_problem.html#status-199", null ],
            [ "Data", "pickup_delivery_problem.html#data-200", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-140", null ]
          ] ],
          [ "StartEvent (StartEventDelivery)", "pickup_delivery_problem.html#startevent-starteventdelivery-1", [
            [ "Status", "pickup_delivery_problem.html#status-200", null ],
            [ "Data", "pickup_delivery_problem.html#data-201", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-141", null ]
          ] ],
          [ "Task (DeliveryTrip)", "pickup_delivery_problem.html#task-deliverytrip-1", [
            [ "Status", "pickup_delivery_problem.html#status-201", null ],
            [ "Data", "pickup_delivery_problem.html#data-202", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-142", null ],
            [ "Operators", "pickup_delivery_problem.html#operators-37", null ]
          ] ],
          [ "IntermediateThrowEvent (CatchArrival)", "pickup_delivery_problem.html#intermediatethrowevent-catcharrival-1", [
            [ "Status", "pickup_delivery_problem.html#status-202", null ],
            [ "Data", "pickup_delivery_problem.html#data-203", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-143", null ],
            [ "Message", "pickup_delivery_problem.html#message-46", [
              [ "<em>Name</em> Arrival", "pickup_delivery_problem.html#name-arrival-9", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventDelivery)", "pickup_delivery_problem.html#endevent-endeventdelivery-1", [
            [ "Status", "pickup_delivery_problem.html#status-203", null ],
            [ "Data", "pickup_delivery_problem.html#data-204", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-144", null ]
          ] ],
          [ "ReceiveTask (UnloadReceiveTask)", "pickup_delivery_problem.html#receivetask-unloadreceivetask-1", [
            [ "Status", "pickup_delivery_problem.html#status-204", null ],
            [ "Data", "pickup_delivery_problem.html#data-205", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-145", null ],
            [ "Message", "pickup_delivery_problem.html#message-47", [
              [ "<em>Name</em> Unloaded", "pickup_delivery_problem.html#name-unloaded-3", null ]
            ] ],
            [ "Operators", "pickup_delivery_problem.html#operators-38", null ]
          ] ],
          [ "EndEvent (EndEventRequest)", "pickup_delivery_problem.html#endevent-endeventrequest-2", [
            [ "Status", "pickup_delivery_problem.html#status-205", null ],
            [ "Data", "pickup_delivery_problem.html#data-206", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-146", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_0tut0wl)", "pickup_delivery_problem.html#dataobjectreference-dataobjectreference_0tut0wl-2", [
            [ "Status", "pickup_delivery_problem.html#status-206", null ],
            [ "Data", "pickup_delivery_problem.html#data-207", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-147", null ]
          ] ],
          [ "StartEvent (StartEventVehicle)", "pickup_delivery_problem.html#startevent-starteventvehicle-2", [
            [ "Status", "pickup_delivery_problem.html#status-207", null ],
            [ "Data", "pickup_delivery_problem.html#data-208", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-148", null ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcessMain)", "pickup_delivery_problem.html#adhocsubprocess-adhocsubprocessmain-1", [
            [ "Status", "pickup_delivery_problem.html#status-208", null ],
            [ "Data", "pickup_delivery_problem.html#data-209", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-149", null ]
          ] ],
          [ "Task (ReturnTrip)", "pickup_delivery_problem.html#task-returntrip-2", [
            [ "Status", "pickup_delivery_problem.html#status-209", null ],
            [ "Data", "pickup_delivery_problem.html#data-210", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-150", null ],
            [ "Entry restrictions", "pickup_delivery_problem.html#entry-restrictions-29", null ],
            [ "Operators", "pickup_delivery_problem.html#operators-39", null ]
          ] ],
          [ "EndEvent (EndEventVehicle)", "pickup_delivery_problem.html#endevent-endeventvehicle-2", [
            [ "Status", "pickup_delivery_problem.html#status-210", null ],
            [ "Data", "pickup_delivery_problem.html#data-211", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-151", null ]
          ] ],
          [ "DataStoreReference (DataStoreReference_1uwtm3x)", "pickup_delivery_problem.html#datastorereference-datastorereference_1uwtm3x-1", [
            [ "Status", "pickup_delivery_problem.html#status-211", null ],
            [ "Data", "pickup_delivery_problem.html#data-212", null ],
            [ "Globals", "pickup_delivery_problem.html#globals-152", null ]
          ] ]
        ] ]
      ] ],
      [ "Pickup and delivery problem (guided)", "guided_pickup_delivery_problem.html", [
        [ "Collaboration (PickupDeliveryProblem)", "guided_pickup_delivery_problem.html#collaboration-pickupdeliveryproblem", [
          [ "Diagram", "guided_pickup_delivery_problem.html#diagram-5", null ],
          [ "Globals", "guided_pickup_delivery_problem.html#globals-35", null ],
          [ "Lookup tables", "guided_pickup_delivery_problem.html#lookup-tables-3", null ],
          [ "Customer process (CustomerProcess)", "guided_pickup_delivery_problem.html#customer-process-customerprocess", [
            [ "Status", "guided_pickup_delivery_problem.html#status-75", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-76", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-36", null ]
          ] ],
          [ "StartEvent (StartEventCustomer)", "guided_pickup_delivery_problem.html#startevent-starteventcustomer", [
            [ "Status", "guided_pickup_delivery_problem.html#status-76", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-77", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-37", null ]
          ] ],
          [ "IntermediateThrowEvent (ThrowShipmentUnloaded)", "guided_pickup_delivery_problem.html#intermediatethrowevent-throwshipmentunloaded", [
            [ "Status", "guided_pickup_delivery_problem.html#status-77", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-78", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-38", null ],
            [ "Message", "guided_pickup_delivery_problem.html#message-12", [
              [ "<em>Name</em> Unloaded", "guided_pickup_delivery_problem.html#name-unloaded", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventCustomer)", "guided_pickup_delivery_problem.html#endevent-endeventcustomer", [
            [ "Status", "guided_pickup_delivery_problem.html#status-78", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-79", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-39", null ]
          ] ],
          [ "Task (UnloadActivity)", "guided_pickup_delivery_problem.html#task-unloadactivity", [
            [ "Status", "guided_pickup_delivery_problem.html#status-79", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-80", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-40", null ],
            [ "Operators", "guided_pickup_delivery_problem.html#operators-15", null ]
          ] ],
          [ "IntermediateCatchEvent (DeliveryWaitEvent)", "guided_pickup_delivery_problem.html#intermediatecatchevent-deliverywaitevent", [
            [ "Status", "guided_pickup_delivery_problem.html#status-80", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-81", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-41", null ],
            [ "Timer", "guided_pickup_delivery_problem.html#timer-2", null ]
          ] ],
          [ "Task (LoadActivity)", "guided_pickup_delivery_problem.html#task-loadactivity", [
            [ "Status", "guided_pickup_delivery_problem.html#status-81", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-82", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-42", null ],
            [ "Operators", "guided_pickup_delivery_problem.html#operators-16", null ]
          ] ],
          [ "IntermediateCatchEvent (PickupWaitEvent)", "guided_pickup_delivery_problem.html#intermediatecatchevent-pickupwaitevent", [
            [ "Status", "guided_pickup_delivery_problem.html#status-82", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-83", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-43", null ],
            [ "Timer", "guided_pickup_delivery_problem.html#timer-3", null ]
          ] ],
          [ "IntermediateCatchEvent (CatchPickupArrival)", "guided_pickup_delivery_problem.html#intermediatecatchevent-catchpickuparrival", [
            [ "Status", "guided_pickup_delivery_problem.html#status-83", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-84", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-44", null ],
            [ "Message", "guided_pickup_delivery_problem.html#message-13", [
              [ "<em>Name</em> Arrival", "guided_pickup_delivery_problem.html#name-arrival", null ]
            ] ]
          ] ],
          [ "IntermediateThrowEvent (ThrowShipmentLoaded)", "guided_pickup_delivery_problem.html#intermediatethrowevent-throwshipmentloaded", [
            [ "Status", "guided_pickup_delivery_problem.html#status-84", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-85", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-45", null ],
            [ "Message", "guided_pickup_delivery_problem.html#message-14", [
              [ "<em>Name</em> Loaded", "guided_pickup_delivery_problem.html#name-loaded", null ]
            ] ]
          ] ],
          [ "IntermediateCatchEvent (CatchDestinationArrival)", "guided_pickup_delivery_problem.html#intermediatecatchevent-catchdestinationarrival", [
            [ "Status", "guided_pickup_delivery_problem.html#status-85", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-86", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-46", null ],
            [ "Message", "guided_pickup_delivery_problem.html#message-15", [
              [ "<em>Name</em> Arrival", "guided_pickup_delivery_problem.html#name-arrival-1", null ]
            ] ]
          ] ],
          [ "SendTask (RequestActivity)", "guided_pickup_delivery_problem.html#sendtask-requestactivity-2", [
            [ "Status", "guided_pickup_delivery_problem.html#status-86", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-87", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-47", null ],
            [ "Message", "guided_pickup_delivery_problem.html#message-16", [
              [ "<em>Name</em> Request", "guided_pickup_delivery_problem.html#name-request-6", null ]
            ] ]
          ] ],
          [ "Vehicle process (VehicleProcess)", "guided_pickup_delivery_problem.html#vehicle-process-vehicleprocess", [
            [ "Status", "guided_pickup_delivery_problem.html#status-87", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-88", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-48", null ]
          ] ],
          [ "SubProcess (EventSubProcess)", "guided_pickup_delivery_problem.html#subprocess-eventsubprocess-3", [
            [ "Status", "guided_pickup_delivery_problem.html#status-88", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-89", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-49", null ],
            [ "Operators", "guided_pickup_delivery_problem.html#operators-17", null ]
          ] ],
          [ "StartEvent (StartEventRequest)", "guided_pickup_delivery_problem.html#startevent-starteventrequest", [
            [ "Status", "guided_pickup_delivery_problem.html#status-89", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-90", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-50", null ],
            [ "Message", "guided_pickup_delivery_problem.html#message-17", [
              [ "<em>Name</em> Request", "guided_pickup_delivery_problem.html#name-request-7", null ]
            ] ],
            [ "Message guidance", "guided_pickup_delivery_problem.html#message-guidance-2", [
              [ "Attributes", "guided_pickup_delivery_problem.html#attributes-5", null ],
              [ "Restrictions", "guided_pickup_delivery_problem.html#restrictions-4", null ]
            ] ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcessRequest)", "guided_pickup_delivery_problem.html#adhocsubprocess-adhocsubprocessrequest", [
            [ "Status", "guided_pickup_delivery_problem.html#status-90", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-91", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-51", null ]
          ] ],
          [ "SubProcess (PickupActivity)", "guided_pickup_delivery_problem.html#subprocess-pickupactivity", [
            [ "Status", "guided_pickup_delivery_problem.html#status-91", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-92", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-52", null ],
            [ "Entry restrictions", "guided_pickup_delivery_problem.html#entry-restrictions-25", null ]
          ] ],
          [ "StartEvent (StartEventPickup)", "guided_pickup_delivery_problem.html#startevent-starteventpickup", [
            [ "Status", "guided_pickup_delivery_problem.html#status-92", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-93", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-53", null ]
          ] ],
          [ "Task (PickupTrip)", "guided_pickup_delivery_problem.html#task-pickuptrip", [
            [ "Status", "guided_pickup_delivery_problem.html#status-93", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-94", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-54", null ],
            [ "Operators", "guided_pickup_delivery_problem.html#operators-18", null ],
            [ "Entry guidance", "guided_pickup_delivery_problem.html#entry-guidance-1", [
              [ "Attributes", "guided_pickup_delivery_problem.html#attributes-6", null ]
            ] ]
          ] ],
          [ "IntermediateThrowEvent (ThrowPickupArrival)", "guided_pickup_delivery_problem.html#intermediatethrowevent-throwpickuparrival", [
            [ "Status", "guided_pickup_delivery_problem.html#status-94", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-95", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-55", null ],
            [ "Message", "guided_pickup_delivery_problem.html#message-18", [
              [ "<em>Name</em> Arrival", "guided_pickup_delivery_problem.html#name-arrival-2", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventPickup)", "guided_pickup_delivery_problem.html#endevent-endeventpickup", [
            [ "Status", "guided_pickup_delivery_problem.html#status-95", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-96", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-56", null ]
          ] ],
          [ "ReceiveTask (LoadReceiveTask)", "guided_pickup_delivery_problem.html#receivetask-loadreceivetask", [
            [ "Status", "guided_pickup_delivery_problem.html#status-96", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-97", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-57", null ],
            [ "Message", "guided_pickup_delivery_problem.html#message-19", [
              [ "<em>Name</em> Loaded", "guided_pickup_delivery_problem.html#name-loaded-1", null ]
            ] ],
            [ "Operators", "guided_pickup_delivery_problem.html#operators-19", null ]
          ] ],
          [ "SubProcess (DeliveryActivity)", "guided_pickup_delivery_problem.html#subprocess-deliveryactivity", [
            [ "Status", "guided_pickup_delivery_problem.html#status-97", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-98", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-58", null ],
            [ "Entry guidance", "guided_pickup_delivery_problem.html#entry-guidance-2", [
              [ "Attributes", "guided_pickup_delivery_problem.html#attributes-7", null ]
            ] ]
          ] ],
          [ "StartEvent (StartEventDelivery)", "guided_pickup_delivery_problem.html#startevent-starteventdelivery", [
            [ "Status", "guided_pickup_delivery_problem.html#status-98", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-99", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-59", null ]
          ] ],
          [ "Task (DeliveryTrip)", "guided_pickup_delivery_problem.html#task-deliverytrip", [
            [ "Status", "guided_pickup_delivery_problem.html#status-99", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-100", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-60", null ],
            [ "Operators", "guided_pickup_delivery_problem.html#operators-20", null ],
            [ "Entry guidance", "guided_pickup_delivery_problem.html#entry-guidance-3", [
              [ "Attributes", "guided_pickup_delivery_problem.html#attributes-8", null ]
            ] ]
          ] ],
          [ "IntermediateThrowEvent (CatchArrival)", "guided_pickup_delivery_problem.html#intermediatethrowevent-catcharrival", [
            [ "Status", "guided_pickup_delivery_problem.html#status-100", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-101", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-61", null ],
            [ "Message", "guided_pickup_delivery_problem.html#message-20", [
              [ "<em>Name</em> Arrival", "guided_pickup_delivery_problem.html#name-arrival-3", null ]
            ] ]
          ] ],
          [ "EndEvent (EndEventDelivery)", "guided_pickup_delivery_problem.html#endevent-endeventdelivery", [
            [ "Status", "guided_pickup_delivery_problem.html#status-101", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-102", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-62", null ]
          ] ],
          [ "ReceiveTask (UnloadReceiveTask)", "guided_pickup_delivery_problem.html#receivetask-unloadreceivetask", [
            [ "Status", "guided_pickup_delivery_problem.html#status-102", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-103", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-63", null ],
            [ "Message", "guided_pickup_delivery_problem.html#message-21", [
              [ "<em>Name</em> Unloaded", "guided_pickup_delivery_problem.html#name-unloaded-1", null ]
            ] ],
            [ "Operators", "guided_pickup_delivery_problem.html#operators-21", null ]
          ] ],
          [ "EndEvent (EndEventRequest)", "guided_pickup_delivery_problem.html#endevent-endeventrequest", [
            [ "Status", "guided_pickup_delivery_problem.html#status-103", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-104", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-64", null ]
          ] ],
          [ "DataObjectReference (DataObjectReference_0tut0wl)", "guided_pickup_delivery_problem.html#dataobjectreference-dataobjectreference_0tut0wl", [
            [ "Status", "guided_pickup_delivery_problem.html#status-104", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-105", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-65", null ]
          ] ],
          [ "StartEvent (StartEventVehicle)", "guided_pickup_delivery_problem.html#startevent-starteventvehicle", [
            [ "Status", "guided_pickup_delivery_problem.html#status-105", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-106", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-66", null ]
          ] ],
          [ "AdHocSubProcess (AdHocSubProcessMain)", "guided_pickup_delivery_problem.html#adhocsubprocess-adhocsubprocessmain", [
            [ "Status", "guided_pickup_delivery_problem.html#status-106", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-107", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-67", null ]
          ] ],
          [ "Task (ReturnTrip)", "guided_pickup_delivery_problem.html#task-returntrip", [
            [ "Status", "guided_pickup_delivery_problem.html#status-107", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-108", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-68", null ],
            [ "Entry restrictions", "guided_pickup_delivery_problem.html#entry-restrictions-26", null ],
            [ "Operators", "guided_pickup_delivery_problem.html#operators-22", null ]
          ] ],
          [ "EndEvent (EndEventVehicle)", "guided_pickup_delivery_problem.html#endevent-endeventvehicle", [
            [ "Status", "guided_pickup_delivery_problem.html#status-108", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-109", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-69", null ]
          ] ],
          [ "DataStoreReference (DataStoreReference_1uwtm3x)", "guided_pickup_delivery_problem.html#datastorereference-datastorereference_1uwtm3x", [
            [ "Status", "guided_pickup_delivery_problem.html#status-109", null ],
            [ "Data", "guided_pickup_delivery_problem.html#data-110", null ],
            [ "Globals", "guided_pickup_delivery_problem.html#globals-70", null ]
          ] ]
        ] ]
      ] ]
    ] ],
    [ "Todo List", "todo.html", null ],
    [ "Code documentation", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Namespace List", "namespaces.html", "namespaces" ],
      [ "File List", "files.html", "files" ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"AttributeRegistry_8cpp.html",
"LocalEvaluator_8h_source.html",
"Value_8h.html#abd0d3034519d54779b4df3e9754cd681a09706f77d0e6b9162ae8d432dc8bc85c",
"classBPMNOS_1_1Execution_1_1Candidates.html#a42df47586be55c64023a5dc28a5f6ad6",
"classBPMNOS_1_1Execution_1_1FirstFeasibleExit.html",
"classBPMNOS_1_1Execution_1_1Message.html#a8cfd940f3553766aad62774987301c0f",
"classBPMNOS_1_1Execution_1_1StateMachine.html#a43428949b19eda4bba96815b54187c8b",
"classBPMNOS_1_1Execution_1_1Token.html#ae76677096c6a3ad900e8b39056d5e44d",
"classBPMNOS_1_1Model_1_1DynamicDataProvider.html#ad710a7cf6e823209055aa3f96b34f172",
"classBPMNOS_1_1Model_1_1Guidance.html#a746c1f40d7d6fcaecbb4becfcb27c16ca54513e02f6d93b88e68f6b8fe8f531f6",
"classBPMNOS_1_1Model_1_1Scenario.html#adb46b1bf8f7f6236f2ded3451fb154df",
"classBPMNOS_1_1RandomDistributionFactory.html#aba7afbbb8c62543d95f5ebcb73cfa1a4",
"classXML_1_1bpmnos_1_1tAttribute.html#add93aa550ec7f145a4cfc6aa13f6d373",
"classXML_1_1bpmnos_1_1tTimer.html#a5d753ae4b949420a09bf1fae5e6ec1f2",
"guided_bin_packing_problem.html#diagram-3",
"guided_pickup_delivery_problem.html#globals-61",
"job_shop_scheduling_problem.html#data-137",
"namespaceBPMNOS_1_1Execution_1_1Color.html#a84e60288a7925b003f7ce914018b3f14a61a4ccab7e62fa42091dfd910531c68c",
"signal_8cpp_source.html",
"structBPMNOS_1_1Execution_1_1Recorder_1_1Config.html#a23b009a6114acb59be515e49f30c89e0",
"tDecision_8h.html",
"truck_driver_scheduling_problem.html#sequenceflow-flow_14ovnm9"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';