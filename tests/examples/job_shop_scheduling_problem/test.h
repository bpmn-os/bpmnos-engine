SCENARIO( "Job shop schedulng problem", "[examples][job_shop_scheduling_problem]" ) {
  const std::string modelFile = "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Three machines and three orders" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "MachineProcess;Machine1;Jobs;2\n"
      "MachineProcess;Machine2;Jobs;3\n"
      "MachineProcess;Machine3;Jobs;3\n"
      "OrderProcess;Order1;Machines;[\"Machine1\",\"Machine2\",\"Machine3\"]\n"
      "OrderProcess;Order1;Durations;[3,2,2]\n"
      "OrderProcess;Order2;Machines;[\"Machine1\",\"Machine3\",\"Machine2\"]\n"
      "OrderProcess;Order2;Durations;[2,1,4]\n"
      "OrderProcess;Order3;Machines;[\"Machine2\",\"Machine3\"]\n"
      "OrderProcess;Order3;Durations;[4,3]\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      readyHandler.connect(&engine);
      completionHandler.connect(&engine);

      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::TimeWarp timeHandler;
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);

      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then no failure occurs" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
      THEN( "Then all process instances complete" ) {
        auto processLog = recorder.find({{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
//std::cerr << processLog.dump() << std::endl;
        REQUIRE( processLog.size() == 3 + 3 );
      }
    }
  }
}
