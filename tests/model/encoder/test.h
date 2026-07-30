SCENARIO( "Encoding of literals", "[model][encoder]" ) {
  GIVEN( "A text stating one literal" ) {

    WHEN( "The literal is a string" ) {
      InputEncoder encoder(R"("A")");
      THEN( "It is registered as a string and reported as one" ) {
        REQUIRE( encoder.type() == STRING );
        REQUIRE( encoder.text() == std::to_string( stringRegistry("A") ) );
      }
    }

    WHEN( "The literal is a collection of strings" ) {
      InputEncoder encoder(R"([ "A", "B", "C" ])");
      THEN( "It is reported as a collection holding strings" ) {
        REQUIRE( encoder.type() == COLLECTION );
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
        REQUIRE( collectionRegistry.memberType(index) == STRING );
        REQUIRE( BPMNOS::to_string(BPMNOS::number(index),COLLECTION) == R"([ "A", "B", "C" ])" );
      }
    }

    WHEN( "The literal is a collection of numbers" ) {
      InputEncoder encoder("[ 1, 2.5 ]");
      THEN( "Whole and fractional members agree in type and are written out as they were read" ) {
        REQUIRE( encoder.type() == COLLECTION );
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
        REQUIRE( collectionRegistry.memberType(index) == DECIMAL );
        REQUIRE( BPMNOS::to_string(BPMNOS::number(index),COLLECTION) == "[ 1, 2.5 ]" );
      }
    }

    WHEN( "The literal is a collection of truth values" ) {
      InputEncoder encoder("[ true, false ]");
      THEN( "It is reported as a collection holding truth values" ) {
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
        REQUIRE( collectionRegistry.memberType(index) == BOOLEAN );
        REQUIRE( BPMNOS::to_string(BPMNOS::number(index),COLLECTION) == "[ true, false ]" );
      }
    }

    WHEN( "The literal is a collection of collections" ) {
      InputEncoder encoder(R"([ [ "A", "B" ], [ 1 ] ])");
      THEN( "Each inner collection carries its own member type and is written out by it" ) {
        REQUIRE( encoder.type() == COLLECTION );
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
        REQUIRE( collectionRegistry.memberType(index) == COLLECTION );
        REQUIRE( BPMNOS::to_string(BPMNOS::number(index),COLLECTION) == R"([ [ "A", "B" ], [ 1 ] ])" );
      }
    }

    WHEN( "The literal holds a string with a comma and a bracket" ) {
      InputEncoder encoder(R"([ "A, [B]", "C" ])");
      THEN( "The string is text rather than structure" ) {
        auto index = (size_t)BPMNOS::stoi( encoder.text() );
        REQUIRE( collectionRegistry[index].size() == 2 );
        REQUIRE( BPMNOS::to_string(BPMNOS::number(index),COLLECTION) == R"([ "A, [B]", "C" ])" );
      }
    }

    WHEN( "A collection of the same numbers holds members of another type" ) {
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

    WHEN( "A collection is indexed" ) {
      InputEncoder encoder("x[1]");
      THEN( "The brackets are copied and no literal is registered" ) {
        REQUIRE( encoder.text() == "x[1]" );
        REQUIRE( !encoder.type().has_value() );
      }
    }

    WHEN( "An index is indexed" ) {
      InputEncoder encoder("x[ y[1] ]");
      THEN( "The brackets are copied" ) {
        REQUIRE( encoder.text() == "x[ y[1] ]" );
      }
    }

    WHEN( "A collection is stated after a membership operator" ) {
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

    WHEN( "A literal is assigned" ) {
      InputEncoder encoder(R"(x := [ "A" ])");
      THEN( "The literal is registered and the text states more than it" ) {
        REQUIRE( !encoder.type().has_value() );
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
      REQUIRE_THROWS( InputEncoder(R"([ 1, [ 2 ] ])") );          // a value and a collection
      REQUIRE_THROWS( InputEncoder(R"([ "A", "B" )") );           // unterminated collection
      REQUIRE_THROWS( InputEncoder(R"([ "A )") );                 // unterminated string
      REQUIRE_THROWS( InputEncoder("[ ]") );                      // collection without members
      REQUIRE_THROWS( InputEncoder("[ 1, ]") );                   // member without value
      REQUIRE_THROWS( InputEncoder("[ A ]") );                    // member that is no value
      REQUIRE_THROWS( InputEncoder("[ 1+2 ]") );                  // member that is no value
    }
  }
}
