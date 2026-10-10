# Model provider
@page bpmnos Model provider

The model provider reads a BPMN model, on which a data provider builds the
@ref BPMNOS::Execution::Scenario "scenario" that the @ref BPMNOS::Execution::Engine "execution engine" runs on.

## BPMN model

The @ref BPMNOS::Model::Model class can be used to read a BPMN file with respective extension elements for optimisation and simulation. All elements of a BPMN process or collaboration diagram can be read, however, not all elements are considered by the @ref BPMNOS::Execution::Engine "execution engine". An overview over the supported BPMN elements and the extension for optimisation and simulation is provided in
- @subpage elements "BPMN elements" and
- @subpage extension "BPMN extension".

Below is a minimal example loading a BPMN model stored in the file `diagram.bpmn`. A model declares its
inputs by the file name of their source, so the folders those files are to be found in are given alongside
it.

```cpp
#include <bpmnos-model.h>
  
int main() {
  BPMNOS::Model::Model model("diagram.bpmn", { "inputs", "shared/inputs" });
  return 0;
}

```

Alternatively the content of each input is supplied directly, keyed by the source file name it is declared
under, together with an already parsed model. @ref BPMNOS::Model::Model::getInputSources "getInputSources"
states which names a model expects.

## Scenario

A @ref BPMNOS::Execution::Scenario "scenario" holds what a run on a model needs to know of its environment: the process instances and the values of their attributes, and the times at which these become known. It is created by a @ref BPMNOS::Execution::DataProvider "data provider" from instance data supplied before the run, and the data provider enqueues the events the scenario gives rise to during the run, as described in the documentation of the @ref engine "execution engine". Four data providers are available, differing in what is known when, and are described below. The data may give no values to the attributes of an @ref BPMN::EventSubProcess "event subprocess" or of a compensation activity: the engine creates such scopes itself and gives them the values the model assigns, and a data provider rejects a row naming such a node in its INITIALIZATION column or, for the stochastic data provider, its READY column. The data may give no values to the attributes of a @ref BPMNOS::Model::Guidance "guidance" either, which take only the values the model assigns to them.

## Static data provider

The @ref BPMNOS::Execution::StaticDataProvider "static data provider" can be used in situations where all instances and all initialization values of attributes are either known or undefined.

Below is a minimal example creating a scenario containing instances of a BPMN model.
```cpp
#include <bpmnos-execution.h>

int main() {
  auto model = std::make_shared<const BPMNOS::Model::Model>("diagram.bpmn");
  auto dataProvider = std::make_shared<BPMNOS::Execution::StaticDataProvider>(model, "scenario.csv");
  auto scenario = dataProvider->createScenario();
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

### Objects

An object, i.e. an attribute whose type is an array or has fields, is given a literal, `facilities := [ { cost := 10, flags := [ true ] }, { cost := 20, flags := [ false ] } ]`, by an instance, or by the model as its initial value, which every instance then shares. A row whose initialization has no `:=` is a size declaration: it names no instance, names the node declaring the object, or no node for a global object, and states the sizes of the object for every instance as a path in which every number in brackets is the size of the next dimension and `[]` leaves a dimension as it is.

```plaintext
INSTANCE_ID; NODE_ID; INITIALIZATION
; ; location := [3, 1]
; FacilityProcess; facilities[6].flags[3]
; ; grid[10][]
Instance_1; FacilityProcess; facilities := [ { cost := 10, flags := [ true ] }, { cost := 20, flags := [ false ] } ]
```

Size declarations for the same object add up, so that `facilities[6]` and `facilities[].flags[3]` together equal `facilities[6].flags[3]`; a size contradicting the type or another declaration is an error. A dimension whose size is fixed, by the type or a size declaration, pads a shorter value with undefined values, and a longer value is an error; an open dimension takes the length of the value. The elements of an array in a literal are uniform, having the same fields and the same lengths; a field the value lacks is undefined in every element, an array field the value lacks having no elements until a value is first assigned to it, and numbers are converted to the types of the fields. An object no instance data gives a value takes the model's initial value or undefined values in the sizes fixed, an open dimension having no elements until an assignment gives it its length. The value of an object is known from the start: it takes no disclosure, ready or completion value, and the objects of event subprocesses, compensation activities and instances started by a trigger take the model's initial value or undefined values.

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

An expression of the instance data may read an object given a value in an earlier row, in the sizes the size declarations fix, or initialised by the model, e.g. `budget := sum(location)` following `location := [3, 1]`; an object without such a value is an error.

Values provided for `string` attributes must be quoted, values provided for `boolean` attributes must be `true` or `false`, and values provided for arrays and objects are literals, as described for objects above.

A literal in square brackets is an array, `[ 1, 2, 3 ]`, and a literal in braces is a value with fields, `{ name := "Depot", position := [ 0, 1.5 ] }`; both may be nested to any depth, as in `[ { cost := 10, flags := [ true, false ] }, { cost := 20, flags := [ false, false ] } ]`. The members of an array are uniform: they agree in type and, being arrays or values with fields themselves, in the sizes of their arrays and in their fields, so that `[ [ 1, 2 ], [ 3 ] ]` is refused. Every literal is registered once as a constant object, which is stored flat and shared by every attribute holding it, and two equal literals are the same constant object. Numbers in a literal are decimals.

Alternatively, the instance data may be provided by a string as shown in below example.

```cpp
#include <bpmnos-execution.h>
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

  auto model = std::make_shared<const BPMNOS::Model::Model>("examples/bin_packing_problem/Bin_packing_problem.bpmn");
  auto dataProvider = std::make_shared<BPMNOS::Execution::StaticDataProvider>(model, csv);
  auto scenario = dataProvider->createScenario();
}
```

## Dynamic data provider

The @ref BPMNOS::Execution::DynamicDataProvider "dynamic data provider" supports scenarios where attribute values may be disclosed at different points in time. This is useful for modeling situations with uncertain or gradually revealed information.

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
#include <bpmnos-execution.h>

int main() {
  auto model = std::make_shared<const BPMNOS::Model::Model>("diagram.bpmn");
  auto dataProvider = std::make_shared<BPMNOS::Execution::DynamicDataProvider>(model, "scenario.csv");
  auto scenario = dataProvider->createScenario();
}
```

## Stochastic data provider

The @ref BPMNOS::Execution::StochasticDataProvider "stochastic data provider" extends dynamic scenarios with support for random functions, stochastic arrival initialization, and stochastic task completion.

### CSV Format

The stochastic data provider uses a CSV file with up to six columns:

```plaintext
INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION
```

- **INSTANCE_ID**: The instance identifier (string). Leave empty for global attributes.
- **NODE_ID**: The BPMN node ID (process, activity, subprocess, etc.). Leave empty for global attributes.
- **INITIALIZATION**: An assignment expression in the format `attribute := expression`. Evaluated at parse time. May contain random functions and can reference any attributes previously parse-time evaluated in earlier CSV rows.
- **DISCLOSURE**: The time at which this attribute value becomes known. Leave empty for immediate disclosure.
- **READY**: Expression evaluated when a token arrives at an activity (Task, SubProcess, CallActivity). Evaluated at runtime with full context (status and data, the data including the global attributes).
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

**Limitation - Event Subprocesses and Compensation Activities**:
CSV-provided INITIALIZATION and READY expressions for an event subprocess or a compensation activity are rejected. Use model expressions for the attributes of such scopes, which are evaluated when an event subprocess is triggered or a compensation activity is entered.

### Reproducibility

Each (instance, node) pair has its own random number generator seeded from the scenario seed combined with the instance ID and node ID hash. This ensures:
- Reproducible results given the same seed
- Independence between different instances/nodes
- Different values for loop iterations (RNG advances)

### Usage

```cpp
#include <bpmnos-execution.h>

int main() {
  unsigned int seed = 42;
  auto model = std::make_shared<const BPMNOS::Model::Model>("diagram.bpmn");
  auto dataProvider = std::make_shared<BPMNOS::Execution::StochasticDataProvider>(model, "scenario.csv", seed);
  auto scenario = dataProvider->createScenario();
}
```

### Downward Compatibility

The stochastic data provider accepts every table whose header consists of the first three or more of its six columns in their order, so that a table of the static or the dynamic data provider is read as well, and a column that is missing gives no value to any row.

## Expected value data provider

The @ref BPMNOS::Execution::ExpectedValueDataProvider "expected value data provider" accepts dynamic and stochastic CSV formats but uses expected values instead of random sampling. All data is disclosed at time 0, regardless of the DISCLOSURE column values.

### CSV Format

The expected value data provider accepts every table whose header consists of the first three or more of the following columns in their order:

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
#include <bpmnos-execution.h>

int main() {
  auto model = std::make_shared<const BPMNOS::Model::Model>("diagram.bpmn");
  auto dataProvider = std::make_shared<BPMNOS::Execution::ExpectedValueDataProvider>(model, "scenario.csv");
  auto scenario = dataProvider->createScenario();
}
```
