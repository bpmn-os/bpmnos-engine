SCENARIO( "Job shop schedulng problem", "[examples][job_shop_scheduling_problem]" ) {
  const std::string modelFile = "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Three machines and three orders" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 2\n"
      "Machine2; MachineProcess; jobs := 3\n"
      "Machine3; MachineProcess; jobs := 3\n"
      "Order1; OrderProcess; machines := [\"Machine1\",\"Machine2\",\"Machine3\"]\n"
      "Order1; OrderProcess; durations := [3,2,2]\n"
      "Order2; OrderProcess; machines := [\"Machine1\",\"Machine3\",\"Machine2\"]\n"
      "Order2; OrderProcess; durations := [2,1,4]\n"
      "Order3; OrderProcess; machines := [\"Machine2\",\"Machine3\"]\n"
      "Order3; OrderProcess; durations := [4,3]\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      readyHandler.connect(&engine);

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
