SCENARIO( "Linear expression", "[model][expression]" ) {
  const std::string modelFile = "tests/model/expression/linearExpression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An expression z := 3*x + 5*y" ) {
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
        Values globals;
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();
          REQUIRE( status.size() == 3 + 1 ); // don't forget timestamp
          REQUIRE( status[extensionElements->attributeRegistry["x"]->index].value() == 8 );
          REQUIRE( status[extensionElements->attributeRegistry["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].has_value() == false );
          extensionElements->applyOperators(status,data,globals);
          REQUIRE( status[extensionElements->attributeRegistry["x"]->index].value() == 8 );
          REQUIRE( status[extensionElements->attributeRegistry["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].value() == 3*8 + 5*15 );
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
        Values globals;
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();

          REQUIRE( status.size() == 3 + 1); // don't forget timestamp
          REQUIRE( status[extensionElements->attributeRegistry["x"]->index].has_value() == false );
          REQUIRE( status[extensionElements->attributeRegistry["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].has_value() == false );
          extensionElements->applyOperators(status,data,globals);
          REQUIRE( status[extensionElements->attributeRegistry["x"]->index].has_value() == false );
          REQUIRE( status[extensionElements->attributeRegistry["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].has_value() == false );
        }
      }
    }
  }
}

SCENARIO( "Divide assignment", "[model][expression]" ) {
  const std::string modelFile = "tests/model/expression/divideAssignment.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An expression z /= 3*x + 5*y" ) {
    WHEN( "The expression is executed with x = 5, y = 3, z = 45" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,X,5\n"
        "Process_1, Instance_1,Y,3\n"
        "Process_1, Instance_1,Z,45\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      auto instantiations = scenario->getCurrentInstantiations(0);
      THEN( "The result is correct" ) {
        Values globals;
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();
          REQUIRE( status.size() == 3 + 1 ); // don't forget timestamp
          REQUIRE( status[extensionElements->attributeRegistry["x"]->index].value() == 5 );
          REQUIRE( status[extensionElements->attributeRegistry["y"]->index].value() == 3 );
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].value() == 45 );
          extensionElements->applyOperators(status,data,globals);
          REQUIRE( status[extensionElements->attributeRegistry["x"]->index].value() == 5 );
          REQUIRE( status[extensionElements->attributeRegistry["y"]->index].value() == 3 );
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].value() == 45.0 / (3*5 + 5*3) );
        }
      }
    }
    WHEN( "The expression is executed with x = 7, y = 15, z = undefined" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,X,7\n"
        "Process_1, Instance_1,Y,15\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instantiations = scenario->getCurrentInstantiations(0);

      THEN( "The result is correct" ) {
        Values globals;
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();

          REQUIRE( status.size() == 3 + 1); // don't forget timestamp
          REQUIRE( status[extensionElements->attributeRegistry["x"]->index].value() == 7 );
          REQUIRE( status[extensionElements->attributeRegistry["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].has_value() == false );
          extensionElements->applyOperators(status,data,globals);
          REQUIRE( status[extensionElements->attributeRegistry["x"]->index].value() == 7 );
          REQUIRE( status[extensionElements->attributeRegistry["y"]->index].value() == 15 );
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].has_value() == false );
        }
      }
    }
  }
}
