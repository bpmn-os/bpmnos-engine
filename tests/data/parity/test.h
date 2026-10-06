/**
 * Parity of the existing and the new deterministic data providers: a model is run on the instance data
 * through the existing and the new data provider of one kind, both built on the model, with the same
 * dispatchers of the controller, and the token logs, without events and decisions, are compared instance by
 * instance, the order in which the entries of different instances interleave being undefined.
 */
enum class ParityDataProvider { STATIC, EXPECTED_VALUE, DYNAMIC };

// the dispatchers of the controller a case is run with, in the order in which they are connected
enum class ParityController {
  INSTANT, ///< immediate entry and exit
  INSTANT_WITH_CHOICE, ///< immediate entry and exit, and the first enumerated choice
  INSTANT_WITH_MESSAGES, ///< the first matching message delivery, immediate entry and exit, and the myopic termination of message tasks
  GREEDY_LOCAL, ///< the greedy controller with the local evaluator, and the myopic termination of message tasks
  GREEDY_GUIDED ///< the greedy controller with the guided evaluator, and the myopic termination of message tasks
};

std::vector<std::unique_ptr<Execution::EventDispatcher>> createParityDispatchers(ParityController controller) {
  std::vector<std::unique_ptr<Execution::EventDispatcher>> dispatchers;
  switch ( controller ) {
    case ParityController::INSTANT:
      dispatchers.push_back(std::make_unique<Execution::InstantEntry>());
      dispatchers.push_back(std::make_unique<Execution::InstantExit>());
      break;
    case ParityController::INSTANT_WITH_CHOICE:
      dispatchers.push_back(std::make_unique<Execution::InstantEntry>());
      dispatchers.push_back(std::make_unique<Execution::InstantExit>());
      dispatchers.push_back(std::make_unique<Execution::GreedyDispatcher<Execution::FirstEnumeratedChoice>>(std::make_shared<Execution::LocalEvaluator>()));
      break;
    case ParityController::INSTANT_WITH_MESSAGES:
      dispatchers.push_back(std::make_unique<Execution::FirstMatchingMessageDelivery>());
      dispatchers.push_back(std::make_unique<Execution::InstantEntry>());
      dispatchers.push_back(std::make_unique<Execution::InstantExit>());
      dispatchers.push_back(std::make_unique<Execution::MyopicMessageTaskTerminator>());
      break;
    case ParityController::GREEDY_LOCAL:
      dispatchers.push_back(std::make_unique<Execution::GreedyController>(std::make_shared<Execution::LocalEvaluator>()));
      dispatchers.push_back(std::make_unique<Execution::MyopicMessageTaskTerminator>());
      break;
    case ParityController::GREEDY_GUIDED:
      dispatchers.push_back(std::make_unique<Execution::GreedyController>(std::make_shared<Execution::GuidedEvaluator>()));
      dispatchers.push_back(std::make_unique<Execution::MyopicMessageTaskTerminator>());
      break;
  }
  return dispatchers;
}

// every run ends at this time at the latest, so that a run that would not end shows as a difference
const BPMNOS::number parityEndTime = 10000;

nlohmann::ordered_json runOnExistingDataProvider(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& csv, ParityDataProvider kind, ParityController controller) {
  std::unique_ptr<Model::DataProvider> dataProvider;
  switch ( kind ) {
    case ParityDataProvider::STATIC: dataProvider = std::make_unique<Model::StaticDataProvider>(modelFile, folders, csv); break;
    case ParityDataProvider::EXPECTED_VALUE: dataProvider = std::make_unique<Model::ExpectedValueDataProvider>(modelFile, folders, csv); break;
    case ParityDataProvider::DYNAMIC: dataProvider = std::make_unique<Model::DynamicDataProvider>(modelFile, folders, csv); break;
  }
  auto scenario = dataProvider->createScenario();
  Execution::Engine engine;
  auto dispatchers = createParityDispatchers(controller);
  for ( auto& dispatcher : dispatchers ) {
    dispatcher->connect(&engine);
  }
  Execution::TimeWarp timeHandler;
  timeHandler.connect(&engine);
  Execution::Recorder recorder;
  recorder.subscribe(&engine);
  engine.run(scenario.get(), 0, parityEndTime);
  return recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});
}

nlohmann::ordered_json runOnNewDataProvider(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& csv, ParityDataProvider kind, ParityController controller) {
  auto model = std::make_shared<const Model::Model>(modelFile, folders);
  std::shared_ptr<Execution::StaticDataProvider> dataProvider;
  switch ( kind ) {
    case ParityDataProvider::STATIC: dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv); break;
    case ParityDataProvider::EXPECTED_VALUE: dataProvider = std::make_shared<Execution::ExpectedValueDataProvider>(model, csv); break;
    case ParityDataProvider::DYNAMIC: dataProvider = std::make_shared<Execution::DynamicDataProvider>(model, csv); break;
  }
  dataProvider->setEndTime(parityEndTime);
  Execution::Engine engine(model);
  auto dispatchers = createParityDispatchers(controller);
  for ( auto& dispatcher : dispatchers ) {
    dispatcher->connect(&engine);
  }
  Execution::Recorder recorder;
  recorder.subscribe(&engine);
  engine.run(dataProvider->createScenario());
  return recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});
}

// the entries of each instance in their order, and the entries belonging to no instance, such as messages
// and signals, in their order under the empty key
std::map<std::string, nlohmann::ordered_json> byInstance(const nlohmann::ordered_json& tokenLog) {
  std::map<std::string, nlohmann::ordered_json> result;
  for ( auto& entry : tokenLog ) {
    auto instanceId = entry.find("instanceId");
    auto key = ( instanceId != entry.end() && instanceId->is_string() ) ? instanceId->get<std::string>() : std::string();
    result[key].push_back(entry);
  }
  return result;
}

SCENARIO( "Parity of the deterministic data providers", "[data][parity]" ) {
  // the cases are plain constants, a list of objects with strings being slow to compile
  struct Case {
    const char* modelFile;
    const char* csv;
    ParityDataProvider kind;
    ParityController controller = ParityController::INSTANT;
    const char* folder = nullptr; ///< The folder of the lookup tables, if any
  };
  static constexpr Case cases[] = {
    {
      "tests/execution/process/Empty_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/process/Empty_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 3\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/task/Task_with_linear_expression.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/timer/Timer.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; trigger := 10\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/eventsubprocess/Non-interrupting_escalation.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/process/Empty_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := uniform(2,4)\n",
      ParityDataProvider::EXPECTED_VALUE
    },
    {
      "tests/data/dynamic/Executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 15; 5\n"
      "Instance_1; Process_1; x := 1; 10\n"
      "Instance_1; Activity_1; data := 5; 15\n"
      "Instance_1; Activity_1; y := 2; 20\n",
      ParityDataProvider::DYNAMIC
    },
    // tests/execution/eventbasedgateway
    {
      "tests/execution/eventbasedgateway/Two_timer_events.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; trigger1 := 1\n"
      "Instance_1; Process_1; trigger2 := 2\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/eventbasedgateway/Two_timer_events.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; trigger1 := 2\n"
      "Instance_1; Process_1; trigger2 := 1\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/errorevent
    {
      "tests/execution/errorevent/Simple_error.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/signal
    {
      "tests/execution/signal/Signal.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 1\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/signal/Signal.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n"
      "Instance_2; Process_2; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/signal/Signal.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n"
      "Instance_2; Process_2; timestamp := 0\n"
      "Instance_3; Process_2; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/signal/Signal.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_0; Process_1; timestamp := 1\n"
      "Instance_1; Process_1; timestamp := 1\n"
      "Instance_2; Process_2; timestamp := 0\n"
      "Instance_3; Process_2; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/collection
    {
      "tests/execution/collection/Collection.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/expression
    {
      "tests/execution/expression/linearExpression.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; x := 8\n"
      "Instance_1; Process_1; y := 15\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/expression/divideAssignment.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; x := 5\n"
      "Instance_1; Process_1; y := 3\n"
      "Instance_1; Process_1; z := 45\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/task
    {
      "tests/execution/task/Task_with_linear_expression.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1;;;;\n"
      "Instance_1; Activity_1;;; timestamp := triangular(5,5,5); timestamp := triangular(10,10,10)\n",
      ParityDataProvider::EXPECTED_VALUE
    },
    // tests/execution/triggeredprocess
    {
      "tests/execution/triggeredprocess/Signal_triggered_process_with_operators.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Emitter; amount := 0\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/triggeredprocess
    {
      "tests/execution/triggeredprocess/Signal_triggered_process_with_operators.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; OtherEmitter; amount := 3\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/triggeredprocess
    {
      "tests/execution/triggeredprocess/Signal_triggered_process_with_operators.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Emitter; amount := 7\n"
      "Instance_2; Emitter; amount := 5\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/triggeredprocess/Message_triggered_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; ThrowEventEmitter; amount := 7\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/triggeredprocess/Message_triggered_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; ThrowEventEmitter; amount := 7\n"
      "Instance_2; ThrowEventEmitter; amount := 5\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/triggeredprocess/Message_triggered_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; SendTaskEmitter; amount := 3\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/triggeredprocess/Message_triggered_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; MultiSendEmitter; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/triggeredprocess/Message_triggered_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; OtherEmitter; amount := 3\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/exclusivegateway
    {
      "tests/execution/exclusivegateway/Symmetric.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/exclusivegateway/Symmetric.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/exclusivegateway/Symmetric.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 2\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/multiinstanceactivity
    {
      "tests/execution/multiinstanceactivity/Parallel_multi-instance_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/multiinstanceactivity/Sequential_multi-instance_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/multiinstanceactivity/Parallel_multi-instance_task_with_timeout.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/multiinstanceactivity/Sequential_multi-instance_task_with_timeout.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/multiinstanceactivity/Sequential_multi-instance_subprocess_with_error.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/multiinstanceactivity/Sequential_multi-instance_subprocess_with_escalation.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/multiinstanceactivity/Multi-instance_subprocess_with_operators.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/data
    {
      "tests/execution/data/Data.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; data1 := 8\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/escalationevent
    {
      "tests/execution/escalationevent/Uncaught_escalation.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/boundaryevent
    {
      "tests/execution/boundaryevent/Failed_Task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/boundaryevent/Failed_SubProcess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/status
    {
      "tests/execution/status/Nested_activities.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/subprocess
    {
      "tests/execution/subprocess/Empty_executable_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/subprocess/Trivial_executable_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/subprocess/Constrained_executable_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/subprocess/Constrained_executable_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 2\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/subprocess/Constrained_executable_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/subprocess/Subprocess_with_operators.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_1;\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/condition
    {
      "tests/execution/condition/Condition_on_data.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/condition/Condition_on_data.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; condition := false\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/condition/Condition_on_data.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; condition := true\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/condition/Condition_on_global.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/condition/Condition_on_global.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; condition := false\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/condition/Condition_on_global.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; condition := true\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/condition/Condition_on_global.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; condition := false\n"
      "Instance_1; Process_2;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/condition/Condition_on_global.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; condition := true\n"
      "Instance_1; Process_2;\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/compensationactivity
    {
      "tests/execution/compensationactivity/No_compensation.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/compensationactivity/Unused_compensation_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/compensationactivity/Compensation_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/compensationactivity/Compensation_triggered_by_error.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/compensationactivity/Compensation_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/compensationactivity/Named_compensation.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/compensationactivity/Multi-instance_compensation.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/compensationactivity/Failing_compensations_multi-instance_activity.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/compensationactivity/Two_compensations_triggered.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/compensationeventsubprocess
    {
      "tests/execution/compensationeventsubprocess/Simple_compensation_event_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/compensationeventsubprocess/Recursive_compensations.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/compensationeventsubprocess/Recursive_named_compensations.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/parallelgateway
    {
      "tests/execution/parallelgateway/Fork.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/parallelgateway/Symmetric.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/loopactivity
    {
      "tests/execution/loopactivity/Loop_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; LoopActivity_1; maximum := 2\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/loopactivity/Loop_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; LoopActivity_1; maximum := 4\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/loopactivity/Loop_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; LoopActivity_1; maximum := 2\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/loopactivity/Loop_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; LoopActivity_1; maximum := 4\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/loopactivity/Loop_subprocess_with_operators.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; LoopActivity_1; maximum := 3\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/process
    {
      "tests/execution/process/Trivial_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/process/Trivial_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 42\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/process/Simple_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 5\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/process/Simple_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 10; 5\n",
      ParityDataProvider::DYNAMIC
    },
    {
      "tests/execution/process/Simple_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := triangular(10,10,10); triangular(5,5,5)\n",
      ParityDataProvider::EXPECTED_VALUE
    },
    {
      "tests/execution/process/Constrained_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/process/Constrained_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 2\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/process/Constrained_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/process/Process_with_operators.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/process/Process_with_weighted_data.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 10\n"
      "Instance_1; Process_1; cost := 7\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/adhocsubprocess
    {
      "tests/execution/adhocsubprocess/AdHocSubProcess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/adhocsubprocess/AdHocSubProcesses_with_common_performer.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    // tests/execution/decisiontask
    {
      "tests/execution/decisiontask/DecisionTask_with_enumeration.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Activity_1; x := -2\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_CHOICE
    },
    {
      "tests/execution/decisiontask/DecisionTask_with_enumeration.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Activity_1; x := 4\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_CHOICE
    },
    {
      "tests/execution/decisiontask/DecisionTask_with_bounds.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_CHOICE
    },
    // tests/execution/message
    {
      "tests/execution/message/Simple_messaging.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 0\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Simple_messaging.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_X; Process_2; timestamp := 0\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Simple_messaging.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 1\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Simple_messaging.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_X; Process_2; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 0\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Message_tasks.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Message_tasks.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Message_tasks.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_2; Process_2;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Message_tasks_with_timer.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Message_tasks_with_timer.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Message_tasks_with_timer.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_2; Process_2;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Message_tasks.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 0; 0\n"
      "Instance_2; Process_2; timestamp := 1; 1\n",
      ParityDataProvider::DYNAMIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Message_tasks_with_timer.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 1\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Multi-instance_send_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n"
      "Instance_3; Process_2;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Multi-instance_send_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Multi-instance_receive_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n"
      "Instance_3; Process_2;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Multi-instance_receive_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    // tests/execution/eventsubprocess
    {
      "tests/execution/eventsubprocess/N-to-1-assignment.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_0; Process_1; expected := 1\n"
      "Instance_1; Process_2;\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED
    },
    {
      "tests/execution/eventsubprocess/N-to-1-assignment.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_0; Process_1; expected := 2\n"
      "Instance_1; Process_2;\n"
      "Instance_2; Process_2;\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED
    },
    {
      "tests/execution/eventsubprocess/N-to-1-assignment.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_0; Process_1; expected := 1\n"
      "Instance_1; Process_2;\n"
      "Instance_2; Process_2;\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED
    },
    // tests/examples/bin_packing_problem
    {
      "examples/bin_packing_problem/Bin_packing_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; bins := 3\n"
      "; ; items := 3\n"
      "Bin1; BinProcess; capacity := 40.0\n"
      "Bin2; BinProcess; capacity := 40.0\n"
      "Bin3; BinProcess; capacity := 40.0\n"
      "Item1; ItemProcess; size := 20.0\n"
      "Item2; ItemProcess; size := 15.0\n"
      "Item3; ItemProcess; size := 22.0\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED
    },
    {
      "examples/bin_packing_problem/Bin_packing_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; bins := 4\n"
      "; ; items := 4\n"
      "Bin1; BinProcess; capacity := 100.0\n"
      "Bin2; BinProcess; capacity := 100.0\n"
      "Bin3; BinProcess; capacity := 100.0\n"
      "Bin4; BinProcess; capacity := 100.0\n"
      "Item1; ItemProcess; size := 36.6\n"
      "Item2; ItemProcess; size := 26.8\n"
      "Item3; ItemProcess; size := 36.6\n"
      "Item4; ItemProcess; size := 43.0\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED
    },
    // tests/examples/job_shop_scheduling_problem
    {
      "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 2\n"
      "Machine2; MachineProcess; jobs := 3\n"
      "Machine3; MachineProcess; jobs := 3\n"
      "Order1; OrderProcess; machines := [\"Machine1\",\"Machine2\",\"Machine3\"]\n"
      "Order1; OrderProcess; durations := [3,2,2]\n"
      "Order2; OrderProcess; machines := [\"Machine1\",\"Machine3\",\"Machine2\"]\n"
      "Order2; OrderProcess; durations := [2,1,4]\n"
      "Order3; OrderProcess; machines := [\"Machine2\",\"Machine3\"]\n"
      "Order3; OrderProcess; durations := [4,3]\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED
    },
    // tests/examples/knapsack_problem
    {
      "examples/knapsack_problem/Knapsack_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Knapsack1; KnapsackProcess; items := 3\n"
      "Knapsack1; KnapsackProcess; capacity := 40\n"
      "Item1; ItemProcess; weight := 20\n"
      "Item1; ItemProcess; value := 100\n"
      "Item2; ItemProcess; weight := 15\n"
      "Item2; ItemProcess; value := 50\n"
      "Item3; ItemProcess; weight := 22\n"
      "Item3; ItemProcess; value := 120\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    // tests/examples/truck_driver_scheduling_problem
    {
      "examples/truck_driver_scheduling_problem/US_Truck_driver_scheduling_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Driver1; TruckDriverProcess; travel_times := [90,90]\n"
      "Driver1; TruckDriverProcess; service_times := [30,30]\n"
      "Driver1; TruckDriverProcess; earliest_visits := [0,0]\n"
      "Driver1; TruckDriverProcess; latest_visits := [1440,1440]\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED
    },
    {
      "examples/truck_driver_scheduling_problem/US_Truck_driver_scheduling_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Driver1; TruckDriverProcess; travel_times := [510]\n"
      "Driver1; TruckDriverProcess; service_times := [30]\n"
      "Driver1; TruckDriverProcess; earliest_visits := [900]\n"
      "Driver1; TruckDriverProcess; latest_visits := [1440]\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED
    },
    // tests/examples/assignment_problem
    {
      "examples/assignment_problem/Assignment_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Client1; ClientProcess;\n"
      "Server1; ServerProcess;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES,
      "tests/examples/assignment_problem"
    },
    {
      "examples/assignment_problem/Assignment_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Client1; ClientProcess;\n"
      "Client2; ClientProcess;\n"
      "Client3; ClientProcess;\n"
      "Server1; ServerProcess;\n"
      "Server2; ServerProcess;\n"
      "Server3; ServerProcess;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES,
      "tests/examples/assignment_problem"
    },
    {
      "examples/assignment_problem/Assignment_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Client1; ClientProcess;\n"
      "Client2; ClientProcess;\n"
      "Client3; ClientProcess;\n"
      "Server1; ServerProcess;\n"
      "Server2; ServerProcess;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES,
      "tests/examples/assignment_problem"
    },
    {
      "examples/assignment_problem/Assignment_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Client1; ClientProcess;\n"
      "Client2; ClientProcess;\n"
      "Server1; ServerProcess;\n"
      "Server2; ServerProcess;\n"
      "Server3; ServerProcess;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES,
      "tests/examples/assignment_problem"
    },
    {
      "examples/assignment_problem/Assignment_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Client1; ClientProcess;\n"
      "Server1; ServerProcess;\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_LOCAL,
      "tests/examples/assignment_problem"
    },
    // tests/examples/earliest_arrival_problem
    {
      "examples/earliest_arrival_problem/Earliest_arrival_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance1; EarliestArrival_Process; initial_location := \"Origin\"\n"
      "Instance1; EarliestArrival_Process; final_location := \"Destination\"\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED,
      "tests/examples/earliest_arrival_problem"
    },
    // tests/examples/pickup_delivery_problem
    {
      "examples/pickup_delivery_problem/Pickup_delivery_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 1\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 1\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
      "Customer1; CustomerProcess; quantity := 1\n"
      "Customer1; CustomerProcess; pickup_location := \"Berlin\"\n"
      "Customer1; CustomerProcess; pickup_duration := 10\n"
      "Customer1; CustomerProcess; earliest_pickup := 0\n"
      "Customer1; CustomerProcess; latest_pickup := 1440\n"
      "Customer1; CustomerProcess; late_pickup_penalty := 0\n"
      "Customer1; CustomerProcess; delivery_location := \"Munich\"\n"
      "Customer1; CustomerProcess; delivery_duration := 10\n"
      "Customer1; CustomerProcess; earliest_delivery := 0\n"
      "Customer1; CustomerProcess; latest_delivery := 1440\n"
      "Customer1; CustomerProcess; late_delivery_penalty := 0\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_LOCAL,
      "tests/examples/pickup_delivery_problem"
    },
    {
      "examples/pickup_delivery_problem/Pickup_delivery_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 2\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 1\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
      "Customer1; CustomerProcess; quantity := 1\n"
      "Customer1; CustomerProcess; pickup_location := \"Berlin\"\n"
      "Customer1; CustomerProcess; pickup_duration := 10\n"
      "Customer1; CustomerProcess; earliest_pickup := 0\n"
      "Customer1; CustomerProcess; latest_pickup := 1440\n"
      "Customer1; CustomerProcess; late_pickup_penalty := 0\n"
      "Customer1; CustomerProcess; delivery_location := \"Munich\"\n"
      "Customer1; CustomerProcess; delivery_duration := 10\n"
      "Customer1; CustomerProcess; earliest_delivery := 0\n"
      "Customer1; CustomerProcess; latest_delivery := 1440\n"
      "Customer1; CustomerProcess; late_delivery_penalty := 0\n"
      "Customer2; CustomerProcess; quantity := 1\n"
      "Customer2; CustomerProcess; pickup_location := \"Cologne\"\n"
      "Customer2; CustomerProcess; pickup_duration := 10\n"
      "Customer2; CustomerProcess; earliest_pickup := 0\n"
      "Customer2; CustomerProcess; latest_pickup := 1440\n"
      "Customer2; CustomerProcess; late_pickup_penalty := 0\n"
      "Customer2; CustomerProcess; delivery_location := \"Hamburg\"\n"
      "Customer2; CustomerProcess; delivery_duration := 10\n"
      "Customer2; CustomerProcess; earliest_delivery := 0\n"
      "Customer2; CustomerProcess; latest_delivery := 1440\n"
      "Customer2; CustomerProcess; late_delivery_penalty := 0\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_LOCAL,
      "tests/examples/pickup_delivery_problem"
    },
    // tests/examples/travelling_salesperson_problem
    {
      "examples/travelling_salesperson_problem/Travelling_salesperson_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance1; TravellingSalesperson_Process; speed := 1\n"
      "Instance1; TravellingSalesperson_Process; origin := \"Hamburg\"\n"
      "Instance1; TravellingSalesperson_Process; locations := [\"Munich\",\"Berlin\",\"Cologne\"]\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES,
      "tests/examples/travelling_salesperson_problem"
    },
    {
      "examples/travelling_salesperson_problem/Travelling_salesperson_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance1; TravellingSalesperson_Process; speed := 1\n"
      "Instance1; TravellingSalesperson_Process; origin := \"Hamburg\"\n"
      "Instance1; TravellingSalesperson_Process; locations := [\"Munich\",\"Berlin\",\"Cologne\"]\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_LOCAL,
      "tests/examples/travelling_salesperson_problem"
    },
    // tests/examples/vehicle_routing_problem
    {
      "examples/vehicle_routing_problem/Vehicle_routing_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 2\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 2\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
      "Customer1; CustomerProcess; quantity := 1\n"
      "Customer1; CustomerProcess; location := \"Berlin\"\n"
      "Customer1; CustomerProcess; handling_duration := 10\n"
      "Customer1; CustomerProcess; earliest_visit := 0\n"
      "Customer1; CustomerProcess; latest_visit := 1440\n"
      "Customer1; CustomerProcess; lateness_penalty := 0\n"
      "Customer2; CustomerProcess; quantity := 1\n"
      "Customer2; CustomerProcess; location := \"Cologne\"\n"
      "Customer2; CustomerProcess; handling_duration := 10\n"
      "Customer2; CustomerProcess; earliest_visit := 0\n"
      "Customer2; CustomerProcess; latest_visit := 1440\n"
      "Customer2; CustomerProcess; lateness_penalty := 0\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED,
      "tests/examples/vehicle_routing_problem"
    },
    // tests/execution/expression, continued
    {
      "tests/execution/expression/stringExpression.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; name := \"Joe\"\n"
      "Instance_1; Process_1; example := \"Paul\"\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/expression/stringExpression.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; name := \"Mary\"\n"
      "Instance_1; Process_1; example := \"Paul\"\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/execution/expression/lookupTable.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT,
      "tests/execution/expression"
    },
    // tests/systemstate
    {
      "tests/systemstate/adhocsubprocess/AdHocSubProcess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/compensationactivity/Two_compensations_triggered.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/compensationeventsubprocess/Simple_compensation_event_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/condition/Condition_on_global.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; condition := false\n"
      "Instance_1; Process_2; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/eventbasedgateway/Two_timer_events.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_1; Process_1; trigger1 := 10\n"
      "Instance_1; Process_1; trigger2 := 20\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/eventsubprocess/Untriggered_evt_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/eventsubprocess/Triggered_interrupting_evt_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/eventsubprocess/Triggered_non-interrupting_evt_subprocess.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/message/Message_tasks_with_timer.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/systemstate/message/Message_tasks_with_timer.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_2; timestamp := 0\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/execution/message/Simple_messaging.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC,
      ParityController::INSTANT_WITH_MESSAGES
    },
    {
      "tests/systemstate/multiinstanceactivity/Parallel_multi-instance_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/multiinstanceactivity/Sequential_multi-instance_task.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/parallelgateway/Fork-Join.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/process/Simple_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/process/Simple_executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/process/Executable_process.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 0; 0\n"
      "Instance_1; Activity_1; y := 0; 10\n"
      "Instance_1; Activity_1; data := 0; 10\n",
      ParityDataProvider::DYNAMIC
    },
    {
      "tests/systemstate/signal/Signal.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_2; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/task/Task_with_linear_expression.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n",
      ParityDataProvider::STATIC
    },
    {
      "tests/systemstate/timer/Timer.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; trigger := 10\n",
      ParityDataProvider::STATIC
    },
    // tests/candidates
    {
      "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 1\n"
      "Order1; OrderProcess; machines := [\"Machine1\"]\n"
      "Order1; OrderProcess; durations := [2]\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED
    },
    {
      "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 2\n"
      "Order1; OrderProcess; machines := [\"Machine1\"]\n"
      "Order1; OrderProcess; durations := [2]\n"
      "Order2; OrderProcess; machines := [\"Machine1\"]\n"
      "Order2; OrderProcess; durations := [3]\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED
    },
    {
      "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 2\n"
      "Order1; OrderProcess; machines := [\"Machine1\",\"Machine1\"]\n"
      "Order1; OrderProcess; durations := [2,3]\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_GUIDED
    },
    {
      "examples/travelling_salesperson_problem/Travelling_salesperson_problem.bpmn",
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance1; TravellingSalesperson_Process; speed := 1\n"
      "Instance1; TravellingSalesperson_Process; origin := \"Hamburg\"\n"
      "Instance1; TravellingSalesperson_Process; locations := [\"Munich\",\"Berlin\",\"Cologne\"]\n",
      ParityDataProvider::STATIC,
      ParityController::GREEDY_LOCAL,
      "tests/examples/travelling_salesperson_problem"
    },
  };

  for ( auto& testCase : cases ) {
    GIVEN( std::string("The model ") + testCase.modelFile + " with the instance data\n" + testCase.csv ) {
      THEN( "The token logs of the existing and the new data provider agree" ) {
        std::vector<std::string> folders;
        if ( testCase.folder ) {
          folders.push_back(testCase.folder);
        }
        REQUIRE( byInstance(runOnNewDataProvider(testCase.modelFile, folders, testCase.csv, testCase.kind, testCase.controller)) == byInstance(runOnExistingDataProvider(testCase.modelFile, folders, testCase.csv, testCase.kind, testCase.controller)) );
      }
    }
  }
}
