SCENARIO( "Linear expression", "[data][static][expression]" ) {
  const std::string modelFile = "Expression/linearExpression.bpmn";
  REQUIRE_NOTHROW( BPMNOS::Model(modelFile) );

  GIVEN( "x = 8, y = 15, z = 3*x + 5*y" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,X,8\n"
      "Process_1, Instance_1,Y,15\n";

    WHEN( "The expression is executed" ) {
      BPMNOS::StaticDataProvider dataProvider(modelFile,csv);
      auto& instances = dataProvider.getInstances();
      THEN( "The result is correct" ) {
        for ( auto& [id,instance] : instances ) {
          auto status = instance->process->extensionElements->represents<Status>();
          Values values;
          dataProvider.appendActualValues(instance.get(),instance->process,values);
          REQUIRE( values.size() == 3 );
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
  }
}

SCENARIO( "Generic expression", "[data][static][expression]" ) {
  const std::string modelFile = "Expression/genericExpression.bpmn";
  REQUIRE_NOTHROW( BPMNOS::Model(modelFile) );

  GIVEN( "x = 8, y = 15, z = 3*x + 5*y" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,X,8\n"
      "Process_1, Instance_1,Y,15\n";

    WHEN( "The expression is executed" ) {
      BPMNOS::StaticDataProvider dataProvider(modelFile,csv);
      auto& instances = dataProvider.getInstances();
      THEN( "The result is correct" ) {
        for ( auto& [id,instance] : instances ) {
          auto status = instance->process->extensionElements->represents<Status>();
          Values values;
          dataProvider.appendActualValues(instance.get(),instance->process,values);
          REQUIRE( values.size() == 3 );
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
  }
}
