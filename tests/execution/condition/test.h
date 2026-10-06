SCENARIO( "Condition on data attribute", "[execution][condition]" ) {
  const std::string modelFile = "tests/execution/condition/Condition_on_data.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "The engine is started with an undefined condition attribute" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
      ;

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
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

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
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

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
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

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      dataProvider->setEndTime(1);
      engine.run(std::move(scenario));
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

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
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

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
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

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      dataProvider->setEndTime(1);
      engine.run(std::move(scenario));
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

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The conditional event is triggerd" ) {
        auto conditionLog =recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( conditionLog.size() == 1 ); 
      }
    }

  }
}

