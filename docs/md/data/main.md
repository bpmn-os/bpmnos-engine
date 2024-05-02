# Data provider
@page data Data provider

A @ref BPMNOS::Model::DataProvider "data provider" is responsible for creating @ref BPMNOS::Model::Scenario "scenarios" providing access to all known or anticpated process instances and all known or anticpated attribute values.

## Static data provider

The @ref BPMNOS::Model::StaticDataProvider "static data provider" can be used in situations where all instances and all initialization values of attributes are either known or undefined.

Below is a minimal example creating a scenario containing instances of a BPMN model.
```cpp
#include <bpmnos-model.h>
  
int main() {
  BPMNOS::Model::StaticDataProvider dataProvider("diagram.bpmn","scenario.csv");
  auto scenario = dataProvider.createScenario();
}
```

The model is stored in the file `diagram.bpmn` and the data is provided in the file `scenario.csv`. Below is an example showing the structure of the data file.

```plaintext
PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE
MachineProcess;Machine1;Jobs;2
MachineProcess;Machine2;Jobs;3
MachineProcess;Machine3;Jobs;3
OrderProcess;Order1;Machines;["Machine1","Machine2","Machine3"]
OrderProcess;Order1;Durations;[3,2,2]
OrderProcess;Order2;Machines;["Machine1","Machine3","Machine2"]
OrderProcess;Order2;Durations;[2,1,4]
OrderProcess;Order3;Machines;["Machine2","Machine3"]
OrderProcess;Order3;Durations;[4,3]
```

The data file must begin with a header `PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE`. Then each of the subsequent lines contains attribute values for a specific process instance.
Values provided for `string` attributes must be quoted, values provided for `boolean` attributes must be `true` or `false`,  and values provided for `collection` attributes must be embraced in square brackets. 

Alternatively, the instance data may be provided by a string as shown in below example.

```cpp
#include <bpmnos-model.h>
#include <string>
  
int main() {
  std::string csv =
    "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
    ";;Items;3\n" // value of global attribute is provided without process and instance id 
    ";;Bins;3\n"  // value of global attribute is provided without process and instance id
    "BinProcess;Bin1;Capacity;40\n"
    "BinProcess;Bin2;Capacity;40\n"
    "BinProcess;Bin3;Capacity;40\n"
    "ItemProcess;Item1;Size;20\n"
    "ItemProcess;Item2;Size;15\n"
    "ItemProcess;Item3;Size;22\n"
  ;

  BPMNOS::Model::StaticDataProvider dataProvider("examples/bin_packing_problem/Guided_bin_packing_problem.bpmn",csv);
  auto scenario = dataProvider.createScenario();
}
```



## Dynamic data provider
@bug The implementation of the @ref BPMNOS::Model::DynamicDataProvider is not yet completed.

## Stochastic data provider
@note Currently, there is no implementation for a stochastic data provider.

## Real-life monitor

@note Currently, there is no implementation for a real-life monitor.
