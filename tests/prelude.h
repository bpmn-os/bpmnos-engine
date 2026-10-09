#pragma once

// Prelude of every test file. Each test file is a translation unit of its own, and this header, which is
// precompiled, gives each of them Catch2 and the engine.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
// Provided transitively by bpmnos-execution.h, which is self-contained; named here to
// state what the tests use, and commented out so that it stays a check on that.
//#include <bpmn++.h>
//#include <bpmnos-model.h>
#include <bpmnos-execution.h>

using namespace BPMNOS;
