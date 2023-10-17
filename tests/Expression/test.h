SCENARIO( "Linear expression", "[data][static][expression]" ) {
  const std::string modelFile = "Expression/linearExpression.bpmn";
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

      auto instantiations = scenario->getInstantiations(0);
      THEN( "The result is correct" ) {
        for ( auto& [process,values] : instantiations ) {
          auto status = process->extensionElements->represents<Model::Status>();
          REQUIRE( values.size() == 3 + 2 ); // don't forget timestamp and instance id
          REQUIRE( values[status->attributeMap["x"]->index].value() == 8 );
          REQUIRE( values[status->attributeMap["y"]->index].value() == 15 );
          REQUIRE( values[status->attributeMap["z"]->index].has_value() == false );
          status->applyOperators(values);
          REQUIRE( values[status->attributeMap["x"]->index].value() == 8 );
          REQUIRE( values[status->attributeMap["y"]->index].value() == 15 );
          REQUIRE( values[status->attributeMap["z"]->index].value() == 3*8 + 5*15 );
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
      auto instantiations = scenario->getInstantiations(0);

      THEN( "The result is correct" ) {
        for ( auto& [process,values] : instantiations ) {
          auto status = process->extensionElements->represents<Model::Status>();

          REQUIRE( values.size() == 3 + 2); // don't forget timestamp and instance id
          REQUIRE( values[status->attributeMap["x"]->index].has_value() == false );
          REQUIRE( values[status->attributeMap["y"]->index].value() == 15 );
          REQUIRE( values[status->attributeMap["z"]->index].has_value() == false );
          status->applyOperators(values);
          REQUIRE( values[status->attributeMap["x"]->index].has_value() == false );
          REQUIRE( values[status->attributeMap["y"]->index].value() == 15 );
          REQUIRE( values[status->attributeMap["z"]->index].has_value() == false );
        }
      }
    }
  }
}

SCENARIO( "Another linear expression", "[data][static][expression]" ) {
  const std::string modelFile = "Expression/anotherLinearExpression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An expression z = 3 + y/5" ) {
    WHEN( "The expression is executed with x = undefined, y = 15" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Y,15\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instantiations = scenario->getInstantiations(0);

      THEN( "The result is correct" ) {
        for ( auto& [process,values] : instantiations ) {
          auto status = process->extensionElements->represents<Model::Status>();

          REQUIRE( values.size() == 3 + 2); // don't forget timestamp and instance id
          REQUIRE( values[status->attributeMap["x"]->index].has_value() == false );
          REQUIRE( values[status->attributeMap["y"]->index].value() == 15 );
          REQUIRE( values[status->attributeMap["z"]->index].has_value() == false );
          status->applyOperators(values);
          REQUIRE( values[status->attributeMap["x"]->index].has_value() == false );
          REQUIRE( values[status->attributeMap["y"]->index].value() == 15 );
          REQUIRE( values[status->attributeMap["z"]->index].value() == 3 + 15/5 );
        }
      }
    }
  }
}

SCENARIO( "Generic expression", "[data][static][expression]" ) {
  const std::string modelFile = "Expression/genericExpression.bpmn";
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
      auto instantiations = scenario->getInstantiations(0);

      THEN( "The result is correct" ) {
        for ( auto& [process,values] : instantiations ) {
          auto status = process->extensionElements->represents<Model::Status>();

          REQUIRE( values.size() == 3 + 2); // don't forget timestamp and instance id
          REQUIRE( values[status->attributeMap["x"]->index].value() == 8 );
          REQUIRE( values[status->attributeMap["y"]->index].value() == 15 );
          REQUIRE( values[status->attributeMap["z"]->index].has_value() == false );
          status->applyOperators(values);
          REQUIRE( values[status->attributeMap["x"]->index].value() == 8 );
          REQUIRE( values[status->attributeMap["y"]->index].value() == 15 );
          REQUIRE( values[status->attributeMap["z"]->index].value() == 3*8 + 5*15 );
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
      auto instantiations = scenario->getInstantiations(0);

      THEN( "The result is correct" ) {
        for ( auto& [process,values] : instantiations ) {
          auto status = process->extensionElements->represents<Model::Status>();

          REQUIRE( values.size() == 3 + 2); // don't forget timestamp and instance id
          REQUIRE( values[status->attributeMap["x"]->index].has_value() == false );
          REQUIRE( values[status->attributeMap["y"]->index].value() == 15 );
          REQUIRE( values[status->attributeMap["z"]->index].has_value() == false );
          status->applyOperators(values);
          REQUIRE( values[status->attributeMap["x"]->index].has_value() == false );
          REQUIRE( values[status->attributeMap["y"]->index].value() == 15 );
          REQUIRE( values[status->attributeMap["z"]->index].has_value() == false );
        }
      }
    }
  }
}
