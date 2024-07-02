SCENARIO( "Linear expression", "[model][expression]" ) {
  const std::string modelFile = "tests/model/expression/linearExpression.bpmn";
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

SCENARIO( "Another linear expression", "[model][expression]" ) {
  const std::string modelFile = "tests/model/expression/anotherLinearExpression.bpmn";
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
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].value() == 3 + 15/5 );
        }
      }
    }
  }
}

SCENARIO( "Generic expression", "[model][expression]" ) {
  const std::string modelFile = "tests/model/expression/genericExpression.bpmn";
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
        Values globals;
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();

          REQUIRE( status.size() == 3 + 1); // don't forget timestamp
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

SCENARIO( "Another generic expression", "[model][expression]" ) {
  const std::string modelFile = "tests/model/expression/anotherGenericExpression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An expression if ( x )  ( y >= 3 ); else 5;" ) {
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
          extensionElements->applyOperators(status,data,globals);
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].has_value() == true );
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].value() == 1 );
        }
      }
    }
    WHEN( "The expression is executed with x = 8, y = 15" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,X,8\n"
        "Process_1, Instance_1,Y,0\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instantiations = scenario->getCurrentInstantiations(0);

      THEN( "The result is correct" ) {
        Values globals;
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();
          extensionElements->applyOperators(status,data,globals);
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].has_value() == true );
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].value() == 0 );
        }
      }
    }
    WHEN( "The expression is executed with x = 0, y = undefined" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,X,0\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instantiations = scenario->getCurrentInstantiations(0);

      THEN( "The result is correct" ) {
        Values globals;
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();
          extensionElements->applyOperators(status,data,globals);
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].has_value() == false ); // all inputs must be defined, even if they are not used in the end
        }
      }
    }
    WHEN( "The expression is executed with x = 0, y = 0" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,X,0\n"
        "Process_1, Instance_1,Y,0\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instantiations = scenario->getCurrentInstantiations(0);

      THEN( "The result is correct" ) {
        Values globals;
        for ( auto& [process,status,data] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();
          extensionElements->applyOperators(status,data,globals);
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].has_value() == true );
          REQUIRE( status[extensionElements->attributeRegistry["z"]->index].value() == 5 );
        }
      }
    }
  }
}
