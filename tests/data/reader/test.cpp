#include "prelude.h"

SCENARIO( "Reading instance data", "[data][reader]" ) {
  const std::string modelFile = "examples/bin_packing_problem/Bin_packing_problem.bpmn";
  Model::Model model(modelFile);
  const std::vector<std::string> columns = { "INSTANCE_ID", "NODE_ID", "INITIALIZATION" };
  const std::vector<std::string> allColumns = { "INSTANCE_ID", "NODE_ID", "INITIALIZATION", "DISCLOSURE", "READY", "COMPLETION" };

  GIVEN( "A table with global values and two instances" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; bins := 2\n"
      "Bin1; BinProcess; capacity := 40.0\n"
      "Item1; ItemProcess; size := 20.0\n"
    ;

    WHEN( "The table is read with the columns it names" ) {
      Execution::InstanceDataReader reader(&model, csv, columns);

      THEN( "The rows are resolved in their order" ) {
        REQUIRE( reader.rows.size() == 3 );
        REQUIRE( reader.rows[0].node == nullptr );
        REQUIRE( reader.rows[1].node->id == "BinProcess" );
        REQUIRE( reader.rows[2].node->id == "ItemProcess" );
        REQUIRE( reader.processes.size() == 2 );
      }
      THEN( "Global values and values of an instance are evaluated" ) {
        reader.evaluateGlobal(reader.rows[0].initialization, model.limexHandle);
        REQUIRE( reader.globals.size() == 1 );
        REQUIRE( reader.globals.begin()->second == 2 );

        auto& row = reader.rows[2];
        auto [attribute, expression] = reader.lookupAttribute(row.node, row.initialization);
        REQUIRE( attribute->name == "size" );
        auto value = reader.evaluate(row.instanceId, row.node, expression, attribute->type, model.limexHandle);
        REQUIRE( value == 20 );
        reader.setValue(row.instanceId, attribute, value);
        REQUIRE( reader.evaluate(row.instanceId, row.node, "size * 2", DECIMAL, model.limexHandle) == 40 );
      }
      THEN( "An expression referring to an attribute without a value is rejected" ) {
        REQUIRE_THROWS( reader.evaluate(reader.rows[2].instanceId, reader.rows[2].node, "size * 2", DECIMAL, model.limexHandle) );
      }
      THEN( "The default values of the instance and the timestamp are added" ) {
        std::unordered_map<const Model::Attribute*, BPMNOS::number> values;
        reader.addDefaultValues(reader.rows[2].instanceId, values);
        REQUIRE( values.size() == 2 );
        auto extensionElements = reader.processes.at(reader.rows[2].instanceId)->extensionElements->as<Model::ExtensionElements>();
        REQUIRE( values.at(extensionElements->attributes[Model::ExtensionElements::Index::Timestamp].get()) == 0 );
        REQUIRE( values.at(extensionElements->data[Model::ExtensionElements::Index::Instance].get()) == BPMNOS::number(reader.rows[2].instanceId) );
      }
    }

    WHEN( "The table is read by a data provider reading further columns" ) {
      Execution::InstanceDataReader reader(&model, csv, allColumns);

      THEN( "The columns the table lacks give no input" ) {
        REQUIRE( reader.rows[1].cells.size() == 3 );
        REQUIRE( reader.rows[1].cells[0].empty() );
        REQUIRE( reader.rows[1].cells[2].empty() );
      }
    }
  }

  GIVEN( "A table with a column the data provider does not read" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Bin1; BinProcess; capacity := 40.0; 5\n"
    ;
    THEN( "The reader rejects it, and accepts it from a data provider reading the column" ) {
      REQUIRE_THROWS( Execution::InstanceDataReader(&model, csv, columns) );
      Execution::InstanceDataReader reader(&model, csv, allColumns);
      REQUIRE( reader.rows[0].cells[0] == "5.000000" );
    }
  }

  GIVEN( "A table naming its columns differently" ) {
    std::string csv =
      "INSTANCE; NODE_ID; INITIALIZATION\n"
      "Bin1; BinProcess; capacity := 40.0\n"
    ;
    THEN( "The reader rejects it" ) {
      REQUIRE_THROWS( Execution::InstanceDataReader(&model, csv, columns) );
    }
  }

  GIVEN( "A table whose first row of an instance names no process" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Item1; RequestActivity;\n"
    ;
    THEN( "The reader rejects it" ) {
      REQUIRE_THROWS( Execution::InstanceDataReader(&model, csv, columns) );
    }
  }

  GIVEN( "A table with a global value in a further column" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "; ; bins := 2; 5\n"
    ;
    THEN( "The reader rejects it" ) {
      REQUIRE_THROWS( Execution::InstanceDataReader(&model, csv, allColumns) );
    }
  }

  GIVEN( "A value for an attribute of a guidance" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Bin1; BinProcess;\n"
      "Bin1; CatchRequestMessage; fill_rate := 0.5\n"
    ;
    THEN( "The lookup of the attribute rejects it" ) {
      Execution::InstanceDataReader reader(&model, csv, columns);
      REQUIRE_THROWS_WITH( reader.lookupAttribute(reader.rows[1].node, reader.rows[1].initialization), Catch::Matchers::ContainsSubstring("guidance") );
    }
  }

  GIVEN( "A value for an attribute of an event subprocess" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Bin1; BinProcess;\n"
      "Bin1; EventSubProcess; item := 1\n"
    ;
    THEN( "The lookup of the attribute rejects it" ) {
      Execution::InstanceDataReader reader(&model, csv, columns);
      REQUIRE_THROWS( reader.lookupAttribute(reader.rows[1].node, reader.rows[1].initialization) );
    }
  }

  GIVEN( "A value for an attribute of a compensation activity" ) {
    Model::Model compensationModel("tests/execution/compensationactivity/Compensation_task.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; CompensationActivity_1; timestamp := 1\n"
    ;
    THEN( "The lookup of the attribute rejects it" ) {
      Execution::InstanceDataReader reader(&compensationModel, csv, columns);
      REQUIRE_THROWS( reader.lookupAttribute(reader.rows[1].node, reader.rows[1].initialization) );
    }
  }

  GIVEN( "A value for a global attribute the model assigns" ) {
    Model::Model processModel("tests/execution/process/Empty_executable_process.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; objective := 1\n"
    ;
    THEN( "The evaluation of the global value rejects it" ) {
      Execution::InstanceDataReader reader(&processModel, csv, columns);
      REQUIRE_THROWS( reader.evaluateGlobal(reader.rows[0].initialization, processModel.limexHandle) );
    }
  }
}
