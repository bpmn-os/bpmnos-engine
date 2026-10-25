#include "prelude.h"

SCENARIO( "Encoding of literals", "[model][encoder]" ) {
  GIVEN( "A text stating one literal" ) {

    WHEN( "The literal is a string" ) {
      InputEncoder encoder(R"("A")");
      THEN( "It is registered as a string and is no object" ) {
        REQUIRE( !encoder.getObjectIndex().has_value() );
        REQUIRE( encoder.getEncodedInput() == std::to_string( stringRegistry("A") ) );
      }
    }

    WHEN( "The literal is an array of strings" ) {
      InputEncoder encoder(R"([ "A", "B", "C" ])");
      THEN( "It is reported as an array holding strings" ) {
        REQUIRE( encoder.getObjectIndex().has_value() );
        auto index = (size_t)BPMNOS::stoi( encoder.getEncodedInput() );
        REQUIRE( objectRegistry[index]->layout->stringify() == "string[3]" );
        REQUIRE( objectRegistry.schema(index).stringify() == "string[]" );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == R"([ "A", "B", "C" ])" );
      }
    }

    WHEN( "The literal is an array of numbers written with a decimal point" ) {
      InputEncoder encoder("[ 1.0, 2.5 ]");
      THEN( "The members are decimals" ) {
        REQUIRE( encoder.getObjectIndex().has_value() );
        auto index = (size_t)BPMNOS::stoi( encoder.getEncodedInput() );
        REQUIRE( objectRegistry[index]->layout->stringify() == "decimal[2]" );
        REQUIRE( objectRegistry.schema(index).stringify() == "decimal[]" );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == "[ 1.0, 2.5 ]" );
      }
    }

    WHEN( "The literal is an array of numbers written without a decimal point" ) {
      InputEncoder encoder("[ 1, -2 ]");
      THEN( "The members are integers" ) {
        REQUIRE( encoder.getObjectIndex().has_value() );
        auto index = (size_t)BPMNOS::stoi( encoder.getEncodedInput() );
        REQUIRE( objectRegistry[index]->layout->stringify() == "integer[2]" );
        REQUIRE( objectRegistry.schema(index).stringify() == "integer[]" );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == "[ 1, -2 ]" );
      }
    }

    WHEN( "The literal is an array of truth values" ) {
      InputEncoder encoder("[ true, false ]");
      THEN( "It is reported as an array holding truth values" ) {
        auto index = (size_t)BPMNOS::stoi( encoder.getEncodedInput() );
        REQUIRE( objectRegistry[index]->layout->stringify() == "boolean[2]" );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == "[ true, false ]" );
      }
    }

    WHEN( "The literal is an array of arrays" ) {
      InputEncoder encoder(R"([ [ "A", "B" ], [ "C", "D" ], [ "E", "F" ] ])");
      THEN( "It is stored flat with its dimensions in index order and written out as it was read" ) {
        REQUIRE( encoder.getObjectIndex().has_value() );
        auto index = (size_t)BPMNOS::stoi( encoder.getEncodedInput() );
        REQUIRE( objectRegistry[index]->layout->stringify() == "string[3][2]" );
        REQUIRE( objectRegistry.schema(index).stringify() == "string[][]" );
        REQUIRE( objectRegistry[index]->values.size() == 6 );
        REQUIRE( objectRegistry[index]->values[3] == BPMNOS::number(stringRegistry("D")) );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == R"([ [ "A", "B" ], [ "C", "D" ], [ "E", "F" ] ])" );
      }
    }

    WHEN( "The literal has fields" ) {
      InputEncoder encoder(R"({ name := "Depot", position := [ 0.0, 1.5 ] })");
      THEN( "The fields are stored one after the other in their order" ) {
        REQUIRE( encoder.getObjectIndex().has_value() );
        auto index = (size_t)BPMNOS::stoi( encoder.getEncodedInput() );
        REQUIRE( objectRegistry[index]->layout->stringify() == "{ name: string, position: decimal[2] }" );
        REQUIRE( objectRegistry.schema(index).stringify() == "{ name: string, position: decimal[] }" );
        REQUIRE( objectRegistry[index]->values.size() == 3 );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == R"({ name := "Depot", position := [ 0.0, 1.5 ] })" );
      }
    }

    WHEN( "The literal is an array of values with fields" ) {
      InputEncoder encoder("[ { cost := 10.0, flags := [ true, false ] }, { cost := 20.0, flags := [ false, false ] } ]");
      THEN( "Each element is a stride of values apart" ) {
        auto index = (size_t)BPMNOS::stoi( encoder.getEncodedInput() );
        auto& object = *objectRegistry[index];
        REQUIRE( object.layout->stringify() == "{ cost: decimal, flags: boolean[2] }[2]" );
        REQUIRE( std::get<BPMNOS::ArrayLayout>(*object.layout).layout->size == 3 );
        REQUIRE( object.values[3] == BPMNOS::number(20) );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == "[ { cost := 10.0, flags := [ true, false ] }, { cost := 20.0, flags := [ false, false ] } ]" );
      }
    }

    WHEN( "The same literal is stated twice" ) {
      THEN( "It is registered once" ) {
        REQUIRE( InputEncoder("[ 1, 2, 3 ]").getEncodedInput() == InputEncoder("[1,2,3]").getEncodedInput() );
        REQUIRE( InputEncoder("{ x := 1 }").getEncodedInput() == InputEncoder("{x:=1}").getEncodedInput() );
      }
    }

    WHEN( "The literal holds a string with a comma and a bracket" ) {
      InputEncoder encoder(R"([ "A, [B]", "C" ])");
      THEN( "The string is text rather than structure" ) {
        auto index = (size_t)BPMNOS::stoi( encoder.getEncodedInput() );
        REQUIRE( objectRegistry[index]->values.size() == 2 );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == R"([ "A, [B]", "C" ])" );
      }
    }

    WHEN( "An array of the same numbers holds members of another type" ) {
      InputEncoder numbers("[ 1, 2 ]");
      InputEncoder strings(R"([ "A", "B" ])");
      THEN( "The two are registered separately" ) {
        auto stringIndices = std::vector<double>{
          (double)stringRegistry("A"),
          (double)stringRegistry("B")
        };
        InputEncoder identical("[ " + BPMNOS::to_string(stringIndices[0]) + ", " + BPMNOS::to_string(stringIndices[1]) + " ]");
        REQUIRE( identical.getEncodedInput() != strings.getEncodedInput() );
        REQUIRE( numbers.getEncodedInput() != strings.getEncodedInput() );
      }
    }
  }

  GIVEN( "A text stating more than a literal" ) {

    WHEN( "An array is indexed" ) {
      InputEncoder encoder("x[1]");
      THEN( "The brackets are copied and no literal is registered" ) {
        REQUIRE( encoder.getEncodedInput() == "x[1]" );
        REQUIRE( !encoder.getObjectIndex().has_value() );
      }
    }

    WHEN( "An index is indexed" ) {
      InputEncoder encoder("x[ y[1] ]");
      THEN( "The brackets are copied" ) {
        REQUIRE( encoder.getEncodedInput() == "x[ y[1] ]" );
      }
    }

    WHEN( "A set is stated after a membership operator" ) {
      THEN( "The brackets are copied, whether the operator is a name or a symbol" ) {
        REQUIRE( InputEncoder("x in [1,2,3]").getEncodedInput() == "x in [1,2,3]" );
        REQUIRE( InputEncoder("x ∈ [1,2,3]").getEncodedInput() == "x ∈ [1,2,3]" );
        REQUIRE( InputEncoder("x not in [1,2,3]").getEncodedInput() == "x not in [1,2,3]" );
        REQUIRE( InputEncoder("x ∉ [1,2,3]").getEncodedInput() == "x ∉ [1,2,3]" );
      }
      AND_THEN( "A string among the members is registered where it stands" ) {
        REQUIRE( InputEncoder(R"(x in ["A"])").getEncodedInput() == "x in [" + std::to_string( stringRegistry("A") ) + "]" );
      }
    }

    WHEN( "Braces state a set or the body of an aggregation" ) {
      THEN( "They are copied" ) {
        REQUIRE( InputEncoder("x in {1,2}").getEncodedInput() == "x in {1,2}" );
        REQUIRE( InputEncoder("sum{ i | i in 1..3 }").getEncodedInput() == "sum{ i | i in 1..3 }" );
        REQUIRE( InputEncoder("max{ a, b }").getEncodedInput() == "max{ a, b }" );
      }
    }

    WHEN( "A literal is assigned" ) {
      InputEncoder encoder(R"(x := [ "A" ])");
      THEN( "The literal is registered and the text states more than it" ) {
        REQUIRE( !encoder.getObjectIndex().has_value() );
        REQUIRE( encoder.getEncodedInput() != R"(x := [ "A" ])" );
      }
    }

    WHEN( "The text is a line of an instance file" ) {
      InputEncoder encoder(R"(Instance_1; Process_1; x := [ "A", "B" ])");
      THEN( "The delimiters outside the literal are kept" ) {
        REQUIRE( encoder.getEncodedInput().starts_with("Instance_1; Process_1; x := ") );
        REQUIRE( !encoder.getEncodedInput().contains("[") );
      }
    }
  }

  GIVEN( "A text that cannot be read" ) {
    THEN( "It is refused" ) {
      REQUIRE_THROWS( InputEncoder(R"([ "A", 1 ])") );            // members of different type
      REQUIRE_THROWS( InputEncoder("[ 1, 2.5 ]") );               // an integer and a decimal
      REQUIRE_THROWS( InputEncoder("[ 1e3 ]") );                  // a number in another form
      REQUIRE_THROWS( InputEncoder("[ .5 ]") );                   // a number in another form
      REQUIRE_THROWS( InputEncoder("[ 1. ]") );                   // a number in another form
      REQUIRE_THROWS( InputEncoder(R"([ 1, [ 2 ] ])") );          // a value and an array
      REQUIRE_THROWS( InputEncoder(R"([ [ "A", "B" ], [ "C" ] ])") ); // arrays of different size
      REQUIRE_THROWS( InputEncoder(R"([ [ "A" ], [ 1 ] ])") );    // arrays of different type
      REQUIRE_THROWS( InputEncoder("[ { x := 1 }, { y := 1 } ]") ); // values with different fields
      REQUIRE_THROWS( InputEncoder("{ x := 1, x := 2 }") );       // a field stated twice
      REQUIRE_THROWS( InputEncoder("{ x := 1") );                 // unterminated value with fields
      REQUIRE_THROWS( InputEncoder(R"([ "A", "B" )") );           // unterminated array
      REQUIRE_THROWS( InputEncoder(R"([ "A )") );                 // unterminated string
      REQUIRE_THROWS( InputEncoder("[ ]") );                      // array without members
      REQUIRE_THROWS( InputEncoder("[ 1, ]") );                   // member without value
      REQUIRE_THROWS( InputEncoder("[ A ]") );                    // member that is no value
      REQUIRE_THROWS( InputEncoder("[ 1+2 ]") );                  // member that is no value
    }
  }
}

SCENARIO( "Object initializers", "[model][encoder]" ) {
  GIVEN( "A line admitting object initializers" ) {
    std::string line = R"(Instance_1; Process_1; depot := { x := uniform(1, 3), name := "Depot;A" }; [ 1, 2 ])";
    InputEncoder encoder(line, true);

    THEN( "A literal with an expression member is copied unchanged and a constant literal is registered" ) {
      REQUIRE( encoder.getEncodedInput().contains(R"({ x := uniform(1, 3), name := "Depot;A" })") );
      REQUIRE( encoder.getEncodedLiterals().size() == 2 );
      REQUIRE( encoder.getEncodedLiterals()[0].type == InputEncoder::EncodedLiteral::Type::INITIALIZER );
      REQUIRE( encoder.getEncodedLiterals()[1].type == InputEncoder::EncodedLiteral::Type::OBJECT );
    }
  }

  GIVEN( "An object initializer" ) {
    ObjectInitializer initializer(R"({ x := uniform(1, 3), name := "Depot", flags := [ true, flip() ] })");

    THEN( "Its expression members are found with their paths in the order in which they are written" ) {
      REQUIRE( initializer.members().size() == 2 );
      REQUIRE( initializer.members()[0].path == ObjectInitializer::Path{ "x" } );
      REQUIRE( initializer.members()[0].expression == "uniform(1, 3)" );
      REQUIRE( initializer.members()[1].path == ObjectInitializer::Path{ "flags", "" } );
    }
    THEN( "It is instantiated with the values of its expression members" ) {
      auto [object, schema] = initializer.instantiate({ { 2.5, DECIMAL }, { 0, BOOLEAN } });
      REQUIRE( BPMNOS::to_string(*object) == R"({ x := 2.5, name := "Depot", flags := [ true, false ] })" );
      REQUIRE( schema.stringify() == "{ x: decimal, name: string, flags: boolean[] }" );
    }
  }

  GIVEN( "A text without object initializers admitted" ) {
    THEN( "A literal with an expression member is refused" ) {
      REQUIRE_THROWS( InputEncoder("[ uniform(1, 3) ]") );
    }
  }
}

TEST_CASE( "Write values as they are read", "[model][encoder]" ) {
  REQUIRE( BPMNOS::to_string(BPMNOS::number(1), DECIMAL) == "1.0" );
  REQUIRE( BPMNOS::to_string(BPMNOS::number(2.5), DECIMAL) == "2.5" );
  REQUIRE( BPMNOS::to_string(BPMNOS::number(-3), DECIMAL) == "-3.0" );
  REQUIRE( BPMNOS::to_string(BPMNOS::number(2), INTEGER) == "2" );
  // a decimal written is read back as a decimal
  auto index = InputEncoder("[ " + BPMNOS::to_string(BPMNOS::number(4), DECIMAL) + " ]").getObjectIndex().value();
  REQUIRE( objectRegistry.schema(index).stringify() == "decimal[]" );
}
