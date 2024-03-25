SCENARIO( "Linear expression", "[model][expression]" ) {
  const std::string modelFile = "model/expression/linearExpression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An expression z = 3*x + 5*y" ) {
    WHEN( "The expression is executed with x = 8, y = 15" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,X,8\n"
        "Process_1, Instance_1,Y,15\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      auto instantiations = scenario->getCurrentInstantiations(0);
      THEN( "The result is correct" ) {
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();
          REQUIRE( status.size() == 3 + 2 ); // don't forget timestamp and instance id
          REQUIRE( status[extensionElements->statusAttributes["x"]->index].value() == 8 );
          REQUIRE( status[extensionElements->statusAttributes["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->statusAttributes["z"]->index].has_value() == false );
          extensionElements->applyOperators(status);
          REQUIRE( status[extensionElements->statusAttributes["x"]->index].value() == 8 );
          REQUIRE( status[extensionElements->statusAttributes["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->statusAttributes["z"]->index].value() == 3*8 + 5*15 );
        }
      }
    }
    WHEN( "The expression is executed with x = undefined, y = 15" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Y,15\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instantiations = scenario->getCurrentInstantiations(0);

      THEN( "The result is correct" ) {
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();

          REQUIRE( status.size() == 3 + 2); // don't forget timestamp and instance id
          REQUIRE( status[extensionElements->statusAttributes["x"]->index].has_value() == false );
          REQUIRE( status[extensionElements->statusAttributes["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->statusAttributes["z"]->index].has_value() == false );
          extensionElements->applyOperators(status);
          REQUIRE( status[extensionElements->statusAttributes["x"]->index].has_value() == false );
          REQUIRE( status[extensionElements->statusAttributes["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->statusAttributes["z"]->index].has_value() == false );
        }
      }
    }
  }
}

SCENARIO( "Another linear expression", "[model][expression]" ) {
  const std::string modelFile = "model/expression/anotherLinearExpression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An expression z = 3 + y/5" ) {
    WHEN( "The expression is executed with x = undefined, y = 15" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Y,15\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instantiations = scenario->getCurrentInstantiations(0);

      THEN( "The result is correct" ) {
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();

          REQUIRE( status.size() == 3 + 2); // don't forget timestamp and instance id
          REQUIRE( status[extensionElements->statusAttributes["x"]->index].has_value() == false );
          REQUIRE( status[extensionElements->statusAttributes["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->statusAttributes["z"]->index].has_value() == false );
          extensionElements->applyOperators(status);
          REQUIRE( status[extensionElements->statusAttributes["x"]->index].has_value() == false );
          REQUIRE( status[extensionElements->statusAttributes["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->statusAttributes["z"]->index].value() == 3 + 15/5 );
        }
      }
    }
  }
}

SCENARIO( "Generic expression", "[model][expression]" ) {
  const std::string modelFile = "model/expression/genericExpression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An expression z = 3*x + 5*y" ) {
    WHEN( "The expression is executed with x = 8, y = 15" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,X,8\n"
        "Process_1, Instance_1,Y,15\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instantiations = scenario->getCurrentInstantiations(0);

      THEN( "The result is correct" ) {
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();

          REQUIRE( status.size() == 3 + 2); // don't forget timestamp and instance id
          REQUIRE( status[extensionElements->statusAttributes["x"]->index].value() == 8 );
          REQUIRE( status[extensionElements->statusAttributes["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->statusAttributes["z"]->index].has_value() == false );
          extensionElements->applyOperators(status);
          REQUIRE( status[extensionElements->statusAttributes["x"]->index].value() == 8 );
          REQUIRE( status[extensionElements->statusAttributes["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->statusAttributes["z"]->index].value() == 3*8 + 5*15 );
        }
      }
    }
    WHEN( "The expression is executed with x = undefined, y = 15" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Y,15\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instantiations = scenario->getCurrentInstantiations(0);

      THEN( "The result is correct" ) {
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();

          REQUIRE( status.size() == 3 + 2); // don't forget timestamp and instance id
          REQUIRE( status[extensionElements->statusAttributes["x"]->index].has_value() == false );
          REQUIRE( status[extensionElements->statusAttributes["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->statusAttributes["z"]->index].has_value() == false );
          extensionElements->applyOperators(status);
          REQUIRE( status[extensionElements->statusAttributes["x"]->index].has_value() == false );
          REQUIRE( status[extensionElements->statusAttributes["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->statusAttributes["z"]->index].has_value() == false );
        }
      }
    }
  }
}
