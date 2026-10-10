#include "prelude.h"

SCENARIO( "Encoding of literals", "[model][encoder]" ) {
  GIVEN( "A text stating one literal" ) {

    WHEN( "The literal is a string" ) {
      InputEncoder encoder(R"("A")");
      THEN( "It is registered as a string and is no object" ) {
        REQUIRE( !encoder.object().has_value() );
        REQUIRE( encoder.text() == std::to_string( stringRegistry("A") ) );
      }
    }

    WHEN( "The literal is an array of strings" ) {
      InputEncoder encoder(R"([ "A", "B", "C" ])");
      THEN( "It is reported as an array holding strings" ) {
        REQUIRE( encoder.object().has_value() );
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
        REQUIRE( objectRegistry[index]->layout->stringify() == "string[3]" );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == R"([ "A", "B", "C" ])" );
      }
    }

    WHEN( "The literal is an array of numbers" ) {
      InputEncoder encoder("[ 1, 2.5 ]");
      THEN( "Whole and fractional members agree in type and are written out as they were read" ) {
        REQUIRE( encoder.object().has_value() );
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
        REQUIRE( objectRegistry[index]->layout->stringify() == "decimal[2]" );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == "[ 1, 2.5 ]" );
      }
    }

    WHEN( "The literal is an array of truth values" ) {
      InputEncoder encoder("[ true, false ]");
      THEN( "It is reported as an array holding truth values" ) {
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
        REQUIRE( objectRegistry[index]->layout->stringify() == "boolean[2]" );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == "[ true, false ]" );
      }
    }

    WHEN( "The literal is an array of arrays" ) {
      InputEncoder encoder(R"([ [ "A", "B" ], [ "C", "D" ], [ "E", "F" ] ])");
      THEN( "It is stored flat with its dimensions in index order and written out as it was read" ) {
        REQUIRE( encoder.object().has_value() );
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
        REQUIRE( objectRegistry[index]->layout->stringify() == "string[3][2]" );
        REQUIRE( objectRegistry[index]->values.size() == 6 );
        REQUIRE( objectRegistry[index]->values[3] == BPMNOS::number(stringRegistry("D")) );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == R"([ [ "A", "B" ], [ "C", "D" ], [ "E", "F" ] ])" );
      }
    }

    WHEN( "The literal has fields" ) {
      InputEncoder encoder(R"({ name := "Depot", position := [ 0, 1.5 ] })");
      THEN( "The fields are stored one after the other in their order" ) {
        REQUIRE( encoder.object().has_value() );
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
        REQUIRE( objectRegistry[index]->layout->stringify() == "{ name: string, position: decimal[2] }" );
        REQUIRE( objectRegistry[index]->values.size() == 3 );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == R"({ name := "Depot", position := [ 0, 1.5 ] })" );
      }
    }

    WHEN( "The literal is an array of values with fields" ) {
      InputEncoder encoder("[ { cost := 10, flags := [ true, false ] }, { cost := 20, flags := [ false, false ] } ]");
      THEN( "Each element is a stride of values apart" ) {
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
        auto& object = *objectRegistry[index];
        REQUIRE( object.layout->stringify() == "{ cost: decimal, flags: boolean[2] }[2]" );
        REQUIRE( object.layout->stride == 3 );
        REQUIRE( object.values[3] == BPMNOS::number(20) );
        REQUIRE( BPMNOS::to_string(*objectRegistry[index]) == "[ { cost := 10, flags := [ true, false ] }, { cost := 20, flags := [ false, false ] } ]" );
      }
    }

    WHEN( "The same literal is stated twice" ) {
      THEN( "It is registered once" ) {
        REQUIRE( InputEncoder("[ 1, 2, 3 ]").text() == InputEncoder("[1,2,3]").text() );
        REQUIRE( InputEncoder("{ x := 1 }").text() == InputEncoder("{x:=1}").text() );
      }
    }

    WHEN( "The literal holds a string with a comma and a bracket" ) {
      InputEncoder encoder(R"([ "A, [B]", "C" ])");
      THEN( "The string is text rather than structure" ) {
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
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
        REQUIRE( identical.text() != strings.text() );
        REQUIRE( numbers.text() != strings.text() );
      }
    }
  }

  GIVEN( "A text stating more than a literal" ) {

    WHEN( "An array is indexed" ) {
      InputEncoder encoder("x[1]");
      THEN( "The brackets are copied and no literal is registered" ) {
        REQUIRE( encoder.text() == "x[1]" );
        REQUIRE( !encoder.object().has_value() );
      }
    }

    WHEN( "An index is indexed" ) {
      InputEncoder encoder("x[ y[1] ]");
      THEN( "The brackets are copied" ) {
        REQUIRE( encoder.text() == "x[ y[1] ]" );
      }
    }

    WHEN( "A set is stated after a membership operator" ) {
      THEN( "The brackets are copied, whether the operator is a name or a symbol" ) {
        REQUIRE( InputEncoder("x in [1,2,3]").text() == "x in [1,2,3]" );
        REQUIRE( InputEncoder("x ∈ [1,2,3]").text() == "x ∈ [1,2,3]" );
        REQUIRE( InputEncoder("x not in [1,2,3]").text() == "x not in [1,2,3]" );
        REQUIRE( InputEncoder("x ∉ [1,2,3]").text() == "x ∉ [1,2,3]" );
      }
      AND_THEN( "A string among the members is registered where it stands" ) {
        REQUIRE( InputEncoder(R"(x in ["A"])").text() == "x in [" + std::to_string( stringRegistry("A") ) + "]" );
      }
    }

    WHEN( "Braces state a set or the body of an aggregation" ) {
      THEN( "They are copied" ) {
        REQUIRE( InputEncoder("x in {1,2}").text() == "x in {1,2}" );
        REQUIRE( InputEncoder("sum{ i | i in 1..3 }").text() == "sum{ i | i in 1..3 }" );
        REQUIRE( InputEncoder("max{ a, b }").text() == "max{ a, b }" );
      }
    }

    WHEN( "A literal is assigned" ) {
      InputEncoder encoder(R"(x := [ "A" ])");
      THEN( "The literal is registered and the text states more than it" ) {
        REQUIRE( !encoder.object().has_value() );
        REQUIRE( encoder.text() != R"(x := [ "A" ])" );
      }
    }

    WHEN( "The text is a line of an instance file" ) {
      InputEncoder encoder(R"(Instance_1; Process_1; x := [ "A", "B" ])");
      THEN( "The delimiters outside the literal are kept" ) {
        REQUIRE( encoder.text().starts_with("Instance_1; Process_1; x := ") );
        REQUIRE( !encoder.text().contains("[") );
      }
    }
  }

  GIVEN( "A text that cannot be read" ) {
    THEN( "It is refused" ) {
      REQUIRE_THROWS( InputEncoder(R"([ "A", 1 ])") );            // members of different type
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
