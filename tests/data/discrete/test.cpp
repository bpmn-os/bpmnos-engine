#include "prelude.h"
#include <limits>
#include <map>
#include <random>

namespace {

/// Resolves the names of an expression as the arrays of a map.
class ArrayResolver : public LIMEX::Resolver<double> {
public:
  ArrayResolver(const LIMEX::Expression<double>& expression, const std::map<std::string, std::vector<double>>& arrays) : expression(expression), arrays(arrays) {}
  double value(size_t, const std::vector<double>&) const override {
    throw std::runtime_error("no value");
  }
  LIMEX::View<double> array(size_t path, const std::vector<double>&) const override {
    return LIMEX::View<double>( arrays.at(expression.getPaths().at(path).name) );
  }
private:
  const LIMEX::Expression<double>& expression;
  const std::map<std::string, std::vector<double>>& arrays;
};

/// Evaluates the expression with the given handle, its names resolved as the given arrays.
double evaluate(const std::string& text, const LIMEX::Handle<double>& handle, const std::map<std::string, std::vector<double>>& arrays) {
  LIMEX::Expression<double> expression(text, handle);
  return expression.evaluate({}, ArrayResolver(expression, arrays));
}

} // namespace

TEST_CASE( "The discrete distribution aggregates an array of values and an array of probabilities", "[data][discrete]" ) {
  std::map<std::string, std::vector<double>> arrays = {
    { "values", { 10.0, 20.0, 30.0 } },
    { "probabilities", { 0.2, 0.5, 0.3 } },
    { "certain", { 0.0, 0.0, 1.0 } },
    { "short", { 0.5, 0.5 } },
    { "undefined", { 0.5, std::numeric_limits<double>::quiet_NaN(), 0.5 } }
  };

  SECTION( "The expected value is the value of highest probability" ) {
    LIMEX::Handle<double> handle;
    Execution::ExpectedValueFactory().registerFunctions(handle);
    REQUIRE( evaluate("discrete(values, probabilities)", handle, arrays) == 20.0 );
    REQUIRE( evaluate("discrete(values, certain)", handle, arrays) == 30.0 );
    REQUIRE_THROWS_WITH( evaluate("discrete(values, short)", handle, arrays), Catch::Matchers::ContainsSubstring("same size") );
    REQUIRE_THROWS_WITH( evaluate("discrete(values, undefined)", handle, arrays), Catch::Matchers::ContainsSubstring("must be defined") );
  }

  SECTION( "A value drawn is a value of the array, drawn with its probability" ) {
    LIMEX::Handle<double> handle;
    Execution::RandomDistributionFactory factory;
    factory.registerFunctions(handle);
    std::mt19937 rng(42);
    factory.setCurrentRng(&rng);
    for ( size_t i = 0; i < 20; i++ ) {
      auto value = evaluate("discrete(values, probabilities)", handle, arrays);
      REQUIRE( ( value == 10.0 || value == 20.0 || value == 30.0 ) );
      REQUIRE( evaluate("discrete(values, certain)", handle, arrays) == 30.0 );
    }
    REQUIRE_THROWS_WITH( evaluate("discrete(values, short)", handle, arrays), Catch::Matchers::ContainsSubstring("same size") );
    factory.setCurrentRng(nullptr);
  }

  SECTION( "The arrays are given as arrays" ) {
    LIMEX::Handle<double> handle;
    Execution::ExpectedValueFactory().registerFunctions(handle);
    REQUIRE_THROWS( LIMEX::Expression<double>("discrete(1, 2)", handle) );
  }
}
