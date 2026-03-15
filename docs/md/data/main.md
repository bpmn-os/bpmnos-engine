# Data provider
@page data Data provider

A @ref BPMNOS::Model::DataProvider "data provider" is responsible for creating @ref BPMNOS::Model::Scenario "scenarios" providing access to all known or anticipated process instances and all known or anticipated attribute values.

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

### CSV Format

The static data provider uses a CSV file with three columns:

```plaintext
INSTANCE_ID, NODE_ID, INITIALIZATION
```

- **INSTANCE_ID**: The instance identifier (string). Leave empty for global attributes.
- **NODE_ID**: The BPMN node ID (process, activity, subprocess, etc.). Leave empty for global attributes.
- **INITIALIZATION**: An assignment expression in the format `attribute := expression`.

### Example

```plaintext
INSTANCE_ID, NODE_ID, INITIALIZATION
, , items := 3
, , bins := 3
Instance_1, BinProcess, timestamp := 0
Instance_1, BinProcess, capacity := 40
Instance_2, BinProcess, timestamp := 0
Instance_2, BinProcess, capacity := 40
Instance_3, BinProcess, timestamp := 0
Instance_3, BinProcess, capacity := 40
Instance_4, ItemProcess, timestamp := 0
Instance_4, ItemProcess, size := 20
Instance_5, ItemProcess, timestamp := 0
Instance_5, ItemProcess, size := 15
Instance_6, ItemProcess, timestamp := 0
Instance_6, ItemProcess, size := 22
```

The first row for each instance must reference the process node. Subsequent rows may reference other nodes (activities, subprocesses) within that process.

### Global Attributes

Global attributes are specified with empty INSTANCE_ID and NODE_ID:

```plaintext
INSTANCE_ID, NODE_ID, INITIALIZATION
, , globalParam := 100
```

### Expressions

Initialization expressions can include arithmetic operations and references to other attributes:

```plaintext
Instance_1, Process_1, duration := baseTime + processingRate * quantity
```

Values provided for `string` attributes must be quoted, values provided for `boolean` attributes must be `true` or `false`, and values provided for `collection` attributes must be embraced in square brackets.

Alternatively, the instance data may be provided by a string as shown in below example.

```cpp
#include <bpmnos-model.h>
#include <string>

int main() {
  std::string csv =
    "INSTANCE_ID, NODE_ID, INITIALIZATION\n"
    ", , items := 3\n"
    ", , bins := 3\n"
    "Instance_1, BinProcess, timestamp := 0\n"
    "Instance_1, BinProcess, capacity := 40\n"
    "Instance_2, BinProcess, timestamp := 0\n"
    "Instance_2, BinProcess, capacity := 40\n"
  ;

  BPMNOS::Model::StaticDataProvider dataProvider("examples/bin_packing_problem/Guided_bin_packing_problem.bpmn",csv);
  auto scenario = dataProvider.createScenario();
}
```

## Dynamic data provider

The @ref BPMNOS::Model::DynamicDataProvider "dynamic data provider" supports scenarios where attribute values may be disclosed at different points in time. This is useful for modeling situations with uncertain or gradually revealed information.

### CSV Format

The dynamic data provider uses a CSV file with four columns:

```plaintext
INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE
```

- **INSTANCE_ID**: The instance identifier (string). Leave empty for global attributes.
- **NODE_ID**: The BPMN node ID (process, activity, subprocess, etc.). Leave empty for global attributes.
- **INITIALIZATION**: An assignment expression in the format `attribute := expression`.
- **DISCLOSURE**: The time at which this attribute value becomes known (constant expression). Leave empty for immediate disclosure (time 0).

### Example

```plaintext
INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE
, , maxTime := 100,
Instance_1, Process_1, timestamp := 0,
Instance_1, Process,_1 priority := 5, 10
Instance_1, Activity_1, duration := 3 + priority, 15
Instance_2, Process_1, timestamp := 5,
Instance_2, Process_1, priority := 3, 20
```

### Disclosure Rules

1. **Effective disclosure time**: The effective disclosure time for a node's data is the maximum of:
   - The node's own disclosure time
   - The parent scope's effective disclosure time

2. **Process instantiation**: A process instance is not instantiated until all of its process-level data is disclosed. If a process has `timestamp := 5` but process data has disclosure time 10, the instance will be instantiated at time 10 (not 5), and the timestamp status attribute will be updated accordingly.

3. **Deferred evaluation**: Initialization expressions are compiled at parse time but evaluated at disclosure time. This allows expressions to reference attributes that are disclosed earlier.

4. **Ordering requirement**: Rows must be ordered such that parent scope disclosures appear before child scope disclosures. For example, process attributes must be disclosed before subprocess attributes for the same instance.

### Global Attributes

Global attributes are specified with empty INSTANCE_ID and NODE_ID:

```plaintext
INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE
, , globalParam := 100,
```

Global attributes must not have a disclosure time (must be immediately available).

### Usage

```cpp
#include <bpmnos-model.h>

int main() {
  BPMNOS::Model::DynamicDataProvider dataProvider("diagram.bpmn", "scenario.csv");
  auto scenario = dataProvider.createScenario();

  // During simulation, call revealData() to evaluate deferred initializations
  // scenario->revealData(currentTime);
}
```

## Stochastic data provider

The @ref BPMNOS::Model::StochasticDataProvider "stochastic data provider" extends dynamic scenarios with support for random functions and stochastic task completion.

### CSV Format

The stochastic data provider uses a CSV file with up to five columns:

```plaintext
INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE, COMPLETION
```

- **INSTANCE_ID**: The instance identifier (string). Leave empty for global attributes.
- **NODE_ID**: The BPMN node ID (process, activity, subprocess, etc.). Leave empty for global attributes.
- **INITIALIZATION**: An assignment expression in the format `attribute := expression`. May contain random functions.
- **DISCLOSURE**: The time at which this attribute value becomes known. Leave empty for immediate disclosure.
- **COMPLETION**: Expression evaluated when a task completes. Only valid for Task nodes (not SendTask, ReceiveTask, DecisionTask).

### Random Functions

The following random functions can be used in expressions:

| Function | Parameters | Description |
|----------|------------|-------------|
| `uniform(min, max)` | min, max | Uniform real distribution |
| `uniform_int(min, max)` | min, max | Uniform integer distribution |
| `normal(mean, stddev)` | mean, stddev | Normal/Gaussian distribution |
| `exponential(rate)` | rate (λ) | Exponential distribution |
| `poisson(mean)` | mean (λ) | Poisson distribution |
| `bernoulli(p)` | probability | Bernoulli (0 or 1) |
| `binomial(n, p)` | trials, probability | Binomial distribution |
| `gamma(shape, scale)` | α, β | Gamma distribution |
| `lognormal(logscale, shape)` | m, s | Log-normal distribution |
| `geometric(p)` | probability | Geometric distribution |

### Example

```plaintext
INSTANCE_ID, NODE_ID, INITIALIZATION, DISCLOSURE, COMPLETION
Instance_1, Process_1, timestamp := 0, ,
Instance_1, Process_1, priority := uniform(1,10), 5,
Instance_1, Activity_1, duration := normal(10,2), , timestamp := timestamp + duration
```

### Reproducibility

Each (instance, node) pair has its own random number generator seeded from the scenario seed combined with the instance ID and node ID hash. This ensures:
- Reproducible results given the same seed
- Independence between different instances/nodes
- Different values for loop iterations (RNG advances)

### Usage

```cpp
#include <bpmnos-model.h>

int main() {
  unsigned int seed = 42;
  BPMNOS::Model::StochasticDataProvider dataProvider("diagram.bpmn", "scenario.csv", seed);
  auto scenario = dataProvider.createScenario();
}
```

### Downward Compatibility

StochasticDataProvider is downward compatible with all CSV formats:
- 3-column (Static): INSTANCE_ID, NODE_ID, INITIALIZATION
- 4-column (Dynamic): + DISCLOSURE
- 5-column (Stochastic): + COMPLETION

## Real-life monitor

@note Currently, there is no implementation for a real-life monitor.
