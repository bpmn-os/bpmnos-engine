SCENARIO( "Empty executable process", "[execution][process]" ) {
  const std::string modelFile = "execution/process//Empty_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started" ) {
      Execution::Engine engine;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&timeHandler);
      THEN( "The engine completes without error" ) {
        REQUIRE_NOTHROW( engine.run(scenario.get()) );
      }
    }
    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "The recorder log has exactly 4 entries" ) {
        REQUIRE( recorder.log.size() == 4 );
      }
      THEN( "The first entry of the recorder log has the correct data" ) {
        REQUIRE( recorder.log.front()["instanceId"] == "Instance_1");
        REQUIRE( recorder.log.front()["processId"] == "Process_1");
        REQUIRE( recorder.log.front()["state"] == "ENTERED");
        REQUIRE( recorder.log.front()["status"]["instance"] == "Instance_1");
        REQUIRE( recorder.log.front()["status"]["timestamp"] == 0.0);
      }
      THEN( "The dump of each entry of the recorder log is correct" ) {
        REQUIRE( recorder.log[0].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[1].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[2].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[3].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
      }
      THEN( "The dump of the entire recorder log is correct" ) {
        std::string expected = "["
          "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}},"
          "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}},"
          "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}},"
          "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}"
          "]";
        REQUIRE( recorder.log.dump() == expected );
      }
    }
  }
};

SCENARIO( "Trivial executable process", "[execution][process]" ) {
  const std::string modelFile = "execution/process/Trivial_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started" ) {
      Execution::Engine engine;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&timeHandler);
      THEN( "The engine completes without error" ) {
        REQUIRE_NOTHROW( engine.run(scenario.get()) );
      }
    }
    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "The recorder log has exactly 6 entries" ) {
        REQUIRE( recorder.log.size() == 6 );
      }
      THEN( "The dump of each entry of the recorder log is correct" ) {
        REQUIRE( recorder.log[0].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[1].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );

        REQUIRE( recorder.log[2].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[3].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );

        REQUIRE( recorder.log[4].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[5].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );

      }
    }
  }
}
