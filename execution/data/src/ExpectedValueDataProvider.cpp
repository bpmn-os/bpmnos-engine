#include "ExpectedValueDataProvider.h"
#include "model/utility/src/ExpectedValueFactory.h"

using namespace BPMNOS::Execution;

ExpectedValueDataProvider::ExpectedValueDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, unsigned int clockTickDuration)
  : StaticDataProvider(std::move(model), clockTickDuration)
{
  // the expressions are evaluated with the lookup tables of the model and the expected values of the
  // random functions
  LIMEX::Handle<double> handle;
  for ( auto& lookupTable : sharedModel->lookupTables ) {
    auto table = lookupTable.get();
    handle.addFunction(table->name, [table](const std::vector<double>& args) {
      return table->at(args);
    });
  }
  BPMNOS::ExpectedValueFactory().registerFunctions(handle);

  readInstances(instanceFileOrString, { "INSTANCE_ID", "NODE_ID", "INITIALIZATION", "DISCLOSURE", "READY", "COMPLETION" }, handle);
}
