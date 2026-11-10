#include "prelude.h"

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
      entryHandler.connect(&engine);
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
      entryHandler.connect(&engine);
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
      entryHandler.connect(&engine);
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
      entryHandler.connect(&engine);
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
      entryHandler.connect(&engine);
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
      entryHandler.connect(&engine);
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
      entryHandler.connect(&engine);
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
      entryHandler.connect(&engine);
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


SCENARIO( "Conditions stated by a node other than a conditional event", "[execution][condition]" ) {
  GIVEN( "A task placing restrictions directly in its extension elements" ) {
    std::string xml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" type="decimal" name="timestamp" />
          <bpmnos:attribute id="Ready" type="boolean" name="ready" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" type="string" name="instance" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1">
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="Task_1" />
    <bpmn2:task id="Task_1">
      <bpmn2:extensionElements>
        <bpmnos:restrictions>
          <bpmnos:restriction id="Restriction_1" expression="ready == true" />
        </bpmnos:restrictions>
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_1</bpmn2:incoming>
      <bpmn2:outgoing>Flow_2</bpmn2:outgoing>
    </bpmn2:task>
    <bpmn2:sequenceFlow id="Flow_2" sourceRef="Task_1" targetRef="End_1" />
    <bpmn2:endEvent id="End_1">
      <bpmn2:incoming>Flow_2</bpmn2:incoming>
    </bpmn2:endEvent>
  </bpmn2:process>
</bpmn2:definitions>
)xml";

    THEN( "The model is refused, conditions belonging to conditional events only" ) {
      auto root = XML::XMLObject::createFromString(xml);
      REQUIRE( root );
      REQUIRE_THROWS_WITH(
        Model::Model(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{}),
        Catch::Matchers::ContainsSubstring("which only a conditional event states")
      );
    }
  }
}

SCENARIO( "Condition on a data attribute chosen at a decision task without operators", "[execution][condition]" ) {
  const std::string modelFile = "tests/execution/condition/Choice_on_data.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance whose decision task chooses the level a conditional event waits for" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;
    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);

    // the choice is played back, so that its value is the one stated
    auto choose = [&](int level) {
      auto log = nlohmann::ordered_json::array();
      log.push_back( nlohmann::ordered_json{{"event", {{"decision","choice"},{"processId","Process_1"},{"instanceId","Instance_1"},{"nodeId","Activity_1"},{"choices", {{"level", level}}}}}} );
      return log;
    };

    WHEN( "The level the conditional event waits for is chosen" ) {
      Execution::Engine engine(model);
      Execution::PlaybackController controller(choose(2));
      controller.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());
      THEN( "The data update of the choice triggers the conditional event" ) {
        REQUIRE( controller.isComplete() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_2"},{"state", "COMPLETED"}}).size() == 1 );
      }
    }

    WHEN( "Another level is chosen" ) {
      Execution::Engine engine(model);
      Execution::PlaybackController controller(choose(1));
      controller.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());
      THEN( "The conditional event is not triggered" ) {
        REQUIRE( controller.isComplete() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state", "COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_2"},{"state", "COMPLETED"}}).empty() );
      }
    }
  }
}
