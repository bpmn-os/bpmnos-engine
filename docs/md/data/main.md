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
INSTANCE_ID; NODE_ID; INITIALIZATION
```

- **INSTANCE_ID**: The instance identifier (string). Leave empty for global attributes.
- **NODE_ID**: The BPMN node ID (process, activity, subprocess, etc.). Leave empty for global attributes.
- **INITIALIZATION**: An assignment expression in the format `attribute := expression`.

### Example

```plaintext
INSTANCE_ID; NODE_ID; INITIALIZATION
; ; items := 3
; ; bins := 3
Instance_1; BinProcess; timestamp := 0
Instance_1; BinProcess; capacity := 40
Instance_2; BinProcess; timestamp := 0
Instance_2; BinProcess; capacity := 40
Instance_3; BinProcess; timestamp := 0
Instance_3; BinProcess; capacity := 40
Instance_4; ItemProcess; timestamp := 0
Instance_4; ItemProcess; size := 20
Instance_5; ItemProcess; timestamp := 0
Instance_5; ItemProcess; size := 15
Instance_6; ItemProcess; timestamp := 0
Instance_6; ItemProcess; size := 22
```

The first row for each instance must reference the process node. Subsequent rows may reference other nodes (activities, subprocesses) within that process.

### Global Attributes

Global attributes are specified with empty INSTANCE_ID and NODE_ID:

```plaintext
INSTANCE_ID; NODE_ID; INITIALIZATION
; ; globalParam := 100
```

### Expressions

Initialization expressions can include arithmetic operations and references to previously evaluated attributes:

```plaintext
INSTANCE_ID; NODE_ID; INITIALIZATION
; ; baseTime := 10
; ; processingRate := 2
Instance_1; Process_1; timestamp := 0
Instance_1; Process_1; duration := baseTime + processingRate * 5
Instance_1; Process_1; endTime := timestamp + duration
```

**Note:** INITIALIZATION expressions can reference any attributes previously parse-time evaluated in earlier CSV rows (globals or instance attributes). Referenced attributes must be available in the node's attributeRegistry. Row order determines which attributes are available for reference.

Values provided for `string` attributes must be quoted, values provided for `boolean` attributes must be `true` or `false`, and values provided for `collection` attributes must be embraced in square brackets.

Alternatively, the instance data may be provided by a string as shown in below example.

```cpp
#include <bpmnos-model.h>
#include <string>

int main() {
  std::string csv =
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
    "; ; items := 3\n"
    "; ; bins := 3\n"
    "Instance_1; BinProcess; timestamp := 0\n"
    "Instance_1; BinProcess; capacity := 40\n"
    "Instance_2; BinProcess; timestamp := 0\n"
    "Instance_2; BinProcess; capacity := 40\n"
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
INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE
```

- **INSTANCE_ID**: The instance identifier (string). Leave empty for global attributes.
- **NODE_ID**: The BPMN node ID (process, activity, subprocess, etc.). Leave empty for global attributes.
- **INITIALIZATION**: An assignment expression in the format `attribute := expression`.
- **DISCLOSURE**: The time at which this attribute value becomes known (constant expression). Leave empty for immediate disclosure (time 0).

### Example

```plaintext
INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE
; ; maxTime := 100;
; ; baseDuration := 3;
Instance_1; Process_1; timestamp := 0;
Instance_1; Process_1; priority := 5; 10
Instance_1; Activity_1; duration := baseDuration + 2; 15
Instance_2; Process_1; timestamp := 5;
Instance_2; Process_1; priority := 3; 20
```

### Disclosure Rules

1. **Effective disclosure time**: The effective disclosure time for a node's data is the maximum of:
   - The node's own disclosure time
   - The parent scope's effective disclosure time

2. **Process instantiation**: A process instance is not instantiated until all of its process-level data is disclosed. If a process has `timestamp := 5` but process data has disclosure time 10, the instance will be instantiated at time 10 (not 5), and the timestamp status attribute will be updated accordingly.

3. **Parse-time evaluation**: Initialization expressions are evaluated at parse time (not disclosure time). The computed value is stored and revealed when the disclosure time is reached. This ensures deterministic scenario construction.

4. **Ordering requirement**: Rows must be ordered such that parent scope disclosures appear before child scope disclosures. For example, process attributes must be disclosed before subprocess attributes for the same instance.

5. **Parse-time evaluated attributes**: INITIALIZATION and DISCLOSURE expressions can reference any attributes previously parse-time evaluated in earlier CSV rows (globals or instance attributes). DISCLOSURE is evaluated after INITIALIZATION in the same row, so it can also reference the just-initialized attribute. Referenced attributes must be in the node's attributeRegistry.

### Global Attributes

Global attributes are specified with empty INSTANCE_ID and NODE_ID:

```plaintext
INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE
; ; globalParam := 100;
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

The @ref BPMNOS::Model::StochasticDataProvider "stochastic data provider" extends dynamic scenarios with support for random functions, stochastic arrival initialization, and stochastic task completion.

### CSV Format

The stochastic data provider uses a CSV file with up to six columns:

```plaintext
INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION
```

- **INSTANCE_ID**: The instance identifier (string). Leave empty for global attributes.
- **NODE_ID**: The BPMN node ID (process, activity, subprocess, etc.). Leave empty for global attributes.
- **INITIALIZATION**: An assignment expression in the format `attribute := expression`. Evaluated at parse time. May contain random functions and can reference any attributes previously parse-time evaluated in earlier CSV rows.
- **DISCLOSURE**: The time at which this attribute value becomes known. Leave empty for immediate disclosure.
- **READY**: Expression evaluated when a token arrives at an activity (Task, SubProcess, CallActivity). Evaluated at runtime with full context (status, data, globals).
- **COMPLETION**: Expression evaluated when a task completes. Only valid for Task nodes (not SendTask, ReceiveTask, DecisionTask). Evaluated at runtime with full context.

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
INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION
; ; basePriority := 5; ; ;
Instance_1; Process_1; timestamp := 0; ; ;
Instance_1; Process_1; priority := uniform(1, basePriority * 2); 5; ;
Instance_1; Activity_1; duration := normal(10,2); ; ; timestamp := timestamp + duration
Instance_1; SubProcess_1; ; ; localVar := parentValue * 2;
```

### Expression Evaluation

| Column | When Evaluated | Context Available |
|--------|----------------|-------------------|
| INITIALIZATION | Parse time | Previously evaluated attributes (same instance) |
| DISCLOSURE | Parse time | Previously evaluated attributes + INITIALIZATION from same row |
| READY | Runtime (token arrival) | Status, Data, Globals |
| COMPLETION | Runtime (task completion) | Status, Data, Globals |

**INITIALIZATION** expressions are evaluated at parse time. They can reference any attributes previously parse-time evaluated in earlier rows (globals or instance attributes).

**DISCLOSURE** expressions are evaluated at parse time, after INITIALIZATION in the same row. They can reference the same attributes as INITIALIZATION, plus the attribute just initialized in the same row.

**Row order matters**: Attributes initialized in earlier CSV rows become available for reference in later rows. Instance 1's attributes cannot be referenced from Instance 2's expressions.

**READY** expressions are evaluated when a token arrives at an activity (enters ARRIVED or CREATED state). They have access to the parent scope's status and data attributes, plus global attributes. This is useful for initializing activity-local attributes based on runtime state.

**COMPLETION** expressions are evaluated when a task enters BUSY state. They have full access to status, data, and global attributes.

### Attribute Initialization and Modification

**Initialization (mutually exclusive with model expressions)**:
An attribute's initial value can come from:
- INITIALIZATION expression (parse-time)
- Model expression (defined in BPMN)

If both attempt to initialize the same attribute, an error is thrown.

**Runtime Modification**:
- READY expressions can override INITIALIZATION values at runtime
- READY expressions cannot override model expressions
- COMPLETION expressions can modify any status attribute, including those with model expressions (model expressions run at ready time, COMPLETION runs later)

Both READY and COMPLETION modifications are local to the activity/task scope and do not affect parent scope values.

**Limitation - Event Subprocesses**:
CSV-provided READY and COMPLETION expressions for event subprocesses are disallowed unless they produce deterministic (equal) values for all triggerings. Use model expressions for event subprocess behavior that may vary between triggerings.

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

StochasticDataProvider is downward compatible with the following CSV formats:
- 3-column (Static): INSTANCE_ID; NODE_ID; INITIALIZATION
- 4-column (Dynamic): + DISCLOSURE
- 6-column (Stochastic): + READY; COMPLETION

Note: There is no 5-column format. Use 6 columns with empty READY or COMPLETION as needed.

## Expected value data provider

The @ref BPMNOS::Model::ExpectedValueDataProvider "expected value data provider" accepts dynamic and stochastic CSV formats but uses expected values instead of random sampling. All data is disclosed at time 0, regardless of the DISCLOSURE column values.

### CSV Format

The expected value data provider accepts CSV files with 3, 4, or 6 columns:

```plaintext
INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION
```

### Behavior

- **DISCLOSURE**: Ignored. All values are disclosed at time 0.
- **READY**: Ignored. Not applicable for expected value computation.
- **COMPLETION**: Ignored. Operators can be used to compute expected values during execution.
- **Random functions**: Return expected values instead of sampling.

### Expected Values

| Function | Expected Value |
|----------|----------------|
| `uniform(a, b)` | (a + b) / 2 |
| `uniform_int(a, b)` | (a + b) / 2 |
| `normal(mean, stddev)` | mean |
| `exponential(rate)` | 1 / rate |
| `poisson(mean)` | mean |
| `bernoulli(p)` | p |
| `binomial(n, p)` | n * p |
| `gamma(shape, scale)` | shape * scale |
| `lognormal(logscale, shape)` | exp(logscale + shape² / 2) |
| `geometric(p)` | (1 - p) / p |

### Example

```plaintext
INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION
Instance_1; Process_1; timestamp := 0; ; ;
Instance_1; Activity_1; duration := uniform(8, 12); ; ;
```

With expected values, `duration` will be `(8 + 12) / 2 = 10`.

### Usage

```cpp
#include <bpmnos-model.h>

int main() {
  BPMNOS::Model::ExpectedValueDataProvider dataProvider("diagram.bpmn", "scenario.csv");
  auto scenario = dataProvider.createScenario();
}
```

## Real-life monitor

@note Currently, there is no implementation for a real-life monitor.
