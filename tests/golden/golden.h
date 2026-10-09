#pragma once

// Golden master of the global state machine refactor, to be removed once the refactor is complete. A run
// compares its objective and its entire log with those the engine produced before the refactor, so that a
// change in behaviour shows also in entries no test asserts on. The expected values are written by the
// first run in which they are absent, which must be a run of the engine before the refactor, and are kept
// in tests/golden/expected/, which is not under version control.

#include <filesystem>
#include <fstream>
#include <string>

inline void goldenMaster(const std::string& name, const Execution::Recorder& recorder, const Execution::Engine& engine) {
  auto path = std::filesystem::path("tests/golden/expected") / (name + ".json");
  nlohmann::ordered_json actual = {
    { "objective", (double)engine.getSystemState()->getObjective() },
    { "log", recorder.log }
  };

  if ( !std::filesystem::exists(path) ) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path) << actual.dump(1) << std::endl;
    WARN( "Golden master written: " << path.string() );
    return;
  }

  std::ifstream stream(path);
  auto expected = nlohmann::ordered_json::parse(stream);
  INFO( "Golden master: " << path.string() );
  // the dumps are compared, which are what is stored: a value written as null (NaN, infinity) compares equal
  // to itself, and a value changing between an integer and a floating-point number is a difference
  REQUIRE( actual["objective"].dump() == expected["objective"].dump() );
  // the objective is held by the system state and no longer by a global attribute named objective, which the
  // golden masters written before show with the running objective; the attribute is left out on both sides
  auto withoutObjectiveAttribute = [](nlohmann::ordered_json log) {
    for ( auto& entry : log ) {
      if ( entry.is_object() && entry.contains("globals") && entry["globals"].is_object() ) {
        entry["globals"].erase("objective");
      }
    }
    return log;
  };
  REQUIRE( withoutObjectiveAttribute(actual["log"]).dump() == withoutObjectiveAttribute(expected["log"]).dump() );
}
