# Model provider
@page bpmnos Model provider

The @ref BPMNOS::Model::Model class can be used to read a BPMN file with respective extension elements for optimisation and simulation. All elements of a BPMN process or collaboration diagram can be read, however, not all elements are considered by the @ref BPMNOS::Execution::Engine "execution engine". An overview over the supported BPMN elements and the extension for optimisation and simulation is provided in
- @subpage elements "BPMN elements" and
- @subpage extension "BPMN extension".

Below is a minimal example loading a BPMN model stored in the file `diagram.bpmn`.

```cpp
#include <bpmnos-model.h>
  
int main() {
  BPMNOS::Model::Model model("diagram.bpmn");
  return 0;
}

```
