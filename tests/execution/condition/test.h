SCENARIO( "Condition on data attribute", "[execution][condition]" ) {
  const std::string modelFile = "tests/execution/condition/Condition_on_data.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "The engine is started with an undefined condition attribute" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The activity is completed and the conditional event is not triggerd" ) {
        auto activityLog =recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state", "COMPLETED"}});
        REQUIRE( activityLog.size() == 1 ); 
        auto conditionLog =recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( conditionLog.size() == 0 ); 
      }
    }

    WHEN( "The engine is started with a false condition attribute" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; condition := false\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The activity is completed and the conditional event is triggerd" ) {
        auto activityLog =recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state", "BUSY"}});
        REQUIRE( activityLog.size() == 1 ); 
        auto conditionLog =recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( conditionLog.size() == 1 ); 
      }
    }

    WHEN( "The engine is started with a true condition attribute" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; condition := true\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The activity is withdrawn and the conditional event is triggerd" ) {
        auto activityLog =recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state", "BUSY"}});
        REQUIRE( activityLog.size() == 0 ); 
        auto conditionLog =recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( conditionLog.size() == 1 ); 
      }
    }
  }
}

SCENARIO( "Condition on global attribute", "[execution][condition]" ) {
  const std::string modelFile = "tests/execution/condition/Condition_on_global.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "Both instances" ) {
    WHEN( "The engine is started with an undefined condition attribute" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
        "Instance_2; Process_2;\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),1);
      THEN( "The activity is completed and the conditional event is not triggerd" ) {
        auto activityLog =recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state", "COMPLETED"}});
        REQUIRE( activityLog.size() == 1 ); 
        auto conditionLog =recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( conditionLog.size() == 0 ); 
      }
    }

    WHEN( "The engine is started with a false condition attribute" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "; ; condition := false\n"
        "Instance_1; Process_1;\n"
        "Instance_2; Process_2;\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The conditional event is triggerd" ) {
        auto activityLog =recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state", "COMPLETED"}});
        REQUIRE( activityLog.size() == 1 ); 
        auto conditionLog =recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( conditionLog.size() == 1 ); 
      }
    }

    WHEN( "The engine is started with a true condition attribute" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "; ; condition := true\n"
        "Instance_1; Process_1;\n"
        "Instance_2; Process_2;\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The conditional event is triggerd" ) {
        auto activityLog =recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state", "COMPLETED"}});
        REQUIRE( activityLog.size() == 1 ); 
        auto conditionLog =recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( conditionLog.size() == 1 ); 
      }
    }

  }


  GIVEN( "A single instance" ) {

    WHEN( "The engine is started with a false condition attribute" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "; ; condition := false\n"
        "Instance_1; Process_2;\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),1);
      THEN( "The conditional event is not triggerd" ) {
        auto conditionLog =recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( conditionLog.size() == 0 ); 
      }
    }

    WHEN( "The engine is started with a true condition attribute" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "; ; condition := true\n"
        "Instance_1; Process_2;\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The conditional event is triggerd" ) {
        auto conditionLog =recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( conditionLog.size() == 1 ); 
      }
    }

  }
}

