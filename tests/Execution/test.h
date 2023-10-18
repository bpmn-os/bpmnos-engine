SCENARIO( "Trivial executable process", "[execution]" ) {
  const std::string modelFile = "Execution/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    WHEN( "The engine is started" ) {
      THEN( "The engine completes without error" ) {
        REQUIRE_NOTHROW( engine.run(scenario.get()) );
      }
    }
  }
}
