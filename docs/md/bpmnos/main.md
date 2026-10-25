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

## Objects and inputs

Besides scalar attributes of type `boolean`, `integer`, `decimal` and `string`, an attribute may be an object, an
array or a value with fields, whose `type` states its shape, `integer[3]`, `decimal[][]` or
`{ cost: decimal, flags: boolean[] }[]`. Objects are global, data or status attributes like scalars; they are
given by literals in the model and the instance data, read and written by expressions through access paths such
as `facilities[i].cost`, aggregated, resized, shared until written, merged at joins, and carried by messages and
signals. Data that is only read during a run is given as an input of a data store instead, a lookup table, a
matrix or a JSON object, which every expression reads by its name and none writes. The @ref extension
"BPMN extension" describes both in full, and the instance data below states how objects are given and sized.

The log of a run, as a @ref BPMNOS::Execution::Recorder "recorder" keeps it, shows the attributes of a token's
`status`, `data` and `globals` in the order of their declaration, outer scopes first, scalars and objects alike,
and renders an object as JSON: a dimension as an array, fields as an object, a string as text and an undefined
value as `null`, so that `facilities` appears as `[ { "cost": 10, "flags": [ true, null ] } ]`. The objects of
message and signal content are rendered alike.

## Scenario

A @ref BPMNOS::Execution::Scenario "scenario" holds what a run on a model needs to know of its environment: the process instances and the values of their attributes, and the times at which these become known. It is created by a @ref BPMNOS::Execution::DataProvider "data provider" from instance data supplied before the run, and the data provider enqueues the events the scenario gives rise to during the run, as described in the documentation of the @ref engine "execution engine". Four data providers are available, differing in what is known when, and are described below. The data may give no values to the attributes of an @ref BPMN::EventSubProcess "event subprocess": the engine creates such scopes itself and gives them the values the model assigns when it creates them, and a data provider rejects a row naming such a node in its INITIALIZATION column or, for the stochastic data provider, its READY column. A compensation activity takes its values as any activity does, except that it becomes ready when compensation is triggered, so that the data may give it neither a disclosure of its own nor a ready value of its timestamp. The data may give no values to the attributes of a @ref BPMNOS::Model::Guidance "guidance" either, which take only the values the model assigns to them.

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

An object, i.e. an attribute whose type is an array or has fields, is given a literal, `facilities := [ { cost := 10.0, flags := [ true ] }, { cost := 20.0, flags := [ false ] } ]`, by an instance, or by the model as its initial value, which every instance then shares. A row whose initialization has no `:=` is a size declaration: it names no instance, names the node declaring the object, or no node for a global object, and states the sizes of the object for every instance as a path in which every number in brackets is the size of the next dimension and `[]` leaves a dimension as it is.

```plaintext
INSTANCE_ID; NODE_ID; INITIALIZATION
; ; location := [3, 1]
; FacilityProcess; facilities[6].flags[3]
; ; grid[10][]
Instance_1; FacilityProcess; facilities := [ { cost := 10, flags := [ true ] }, { cost := 20, flags := [ false ] } ]
```

Size declarations for the same object add up, so that `facilities[6]` and `facilities[].flags[3]` together equal `facilities[6].flags[3]`; a size contradicting the type or another declaration is an error. A dimension whose size is fixed, by the type or a size declaration, pads a shorter value with undefined values, and a longer value is an error; an open dimension takes the length of the value. The elements of an array in a literal are uniform, having the same fields and the same lengths; a field the value lacks is undefined in every element, an array field the value lacks having no elements until a value is first assigned to it, and the values must have the value types the type of the object states, an array or object never being converted from one value type to another. An object no instance data gives a value takes the model's initial value or undefined values in the sizes fixed, an open dimension having no elements until an assignment gives it its length. An object is initialised as a whole: an initialization names the object, never an element or a field of it. The objects of event subprocesses and instances started by a trigger take the model's initial value or undefined values.

The content object of a message, error or escalation, named after it, is declared in the same way at the node throwing it, `; SendRequestTask; request.route[5]`, as message, error and escalation names need not be unique across the model; the throwing node starts the content its operators write from undefined values in the sizes declared. A signal is the same signal wherever its name is thrown or raised, so the sizes of its content are declared by its name and no node, `; ; cancellation.route[5]`, and apply to every node throwing it and to a signal raised by the environment, whose content a data provider gives by @ref BPMNOS::Execution::DataProvider::getDefaultContent "getDefaultContent"; a declaration at a signal event is refused. A data provider gives the effective schema of an object or of the content of a node, which is its type with the sizes declared, by @ref BPMNOS::Execution::DataProvider::getSchema "getSchema"; without size declarations, the effective schema is the type. A catching node declares nothing and receives the content as it is thrown.

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

A literal in square brackets is an array, `[ 1, 2, 3 ]`, and a literal in braces is a value with fields, `{ name := "Depot", position := [ 0.0, 1.5 ] }`; both may be nested to any depth, as in `[ { cost := 10.0, flags := [ true, false ] }, { cost := 20.0, flags := [ false, false ] } ]`. The members of an array are uniform: they agree in type and, being arrays or values with fields themselves, in the sizes of their arrays and in their fields, so that `[ [ 1, 2 ], [ 3 ] ]` is refused. Every literal is registered once as a constant object, which is stored flat and shared by every attribute holding it, and two equal literals are the same constant object. A member of a literal has the value type stated by how it is written: `true` and `false` are booleans, a quoted text is a string, a number written with a decimal point, `1.0` or `-2.5`, is a decimal, and a number written without one, `1` or `-2`, is an integer; any other form of a number, such as `1e3` or `.5`, is refused. Hence `[ 1, 2.5 ]` is refused and is written `[ 1.0, 2.5 ]`, and an array of decimals is written `[ 1.0, 2.0 ]` rather than `[ 1, 2 ]`.

Where the instance data give an object its value, the literal may be an object initializer, a literal whose members may be expressions, `depot := { x := uniform(0, 10), y := 2 }`. A member that is neither a quoted string, a truth value, a number nor a literal is an expression, reaching to the next comma or closing bracket or brace outside every parenthesis, bracket, brace and quote, so that `uniform(0, 10)` is one member. Each expression is evaluated at the node of the row as an initialization is, and its value is converted to the type the object has at the position of the member, whereas every other member follows the rules of literals. An expression may read only what is initialised before its row: the global attributes and objects, the inputs, and the attributes and objects of the same instance given by earlier rows, so that `site := { x := depot.x + 1 }` reads the object of an earlier row. It does not read another member of the same initializer, the object being given its value only when all its members are evaluated: in `{ x := 1, y := x }`, the name `x` is an attribute of the node, not the member. The static and the dynamic data provider evaluate an initializer once, when the instance data are read, the expected value data provider with the expected values of the random functions, and the stochastic data provider for each scenario, in the order of the rows; an initializer of a global object is evaluated once, when the instance data are read. The dynamic and the stochastic data provider disclose the object with its row, as they disclose a value.

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
- **READY**: An assignment applied when a token becomes ready at an activity (Task, SubProcess, CallActivity), over the initial values of the scope instance. It assigns a status attribute or a data attribute the scope owns, an object among them, or an element or field of such an object. One assigning the timestamp must be the first of its node and is evaluated when the token arrives, as described below.
- **COMPLETION**: Expression evaluated when a task completes. Only valid for Task nodes (not SendTask, ReceiveTask, DecisionTask). It assigns a status attribute, a status object, or an element or field of a status object. Evaluated at runtime with full context.

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
| READY | Runtime (ready event; the timestamp at token arrival) | Status, Data, Globals |
| COMPLETION | Runtime (task completion) | Status, Data, Globals |

**INITIALIZATION** expressions are evaluated at parse time. They can reference any attributes previously parse-time evaluated in earlier rows (globals or instance attributes).

**DISCLOSURE** expressions are evaluated at parse time, after INITIALIZATION in the same row. They can reference the same attributes as INITIALIZATION, plus the attribute just initialized in the same row.

**Row order matters**: Attributes initialized in earlier CSV rows become available for reference in later rows. Instance 1's attributes cannot be referenced from Instance 2's expressions. An INITIALIZATION or DISCLOSURE referring to an attribute or object that neither the global values, the initial objects of the model, nor an earlier row of the instance gives a value is refused when the instance data are read, and one that yields no value when it is sampled is refused, as for the static data provider.

**READY** expressions are evaluated when the ready event of a token at an activity becomes due, after the initial values of the scope instance have been built, and are evaluated over the status of the token, the data of the enclosing instances and the globals, and the attributes of the scope with their initial values. A ready expression may overwrite any value of the status and of the data the scope owns, one the model initialises included, so that a value the model expects may be replaced by a sampled one. It may assign an object, or an element or field of one, as an operator does, `route[2] := 7` or `plan.cost := uniform(1, 3)`; an object shared with another token is copied before it is written, so that a field of an inherited status object changes only the token's own copy. Inherited data and global attributes and objects are never assigned. A ready expression assigning the timestamp determines when the token becomes ready, and is therefore evaluated when the token arrives at the activity (enters the state ARRIVED or CREATED); it must be the first ready expression of its node and may read only what the scope inherits, the status the token holds, the data of the enclosing instances and the globals, never an attribute of the scope, which does not exist before the ready event. A change of the inherited data or of a global after the arrival of the token is therefore ignored by it. A token awaiting its ready event when a system state is installed has its ready timestamp evaluated anew at the installation, and evaluated again up to `maxResamplingTries` times while it precedes the time of the state, after which it is set to that time.

**COMPLETION** expressions are evaluated when a task enters BUSY state. They have full access to status, data, and global attributes, and assign the status only: a status attribute, a status object, or an element or field of a status object.

### Attribute Initialization and Modification

The values of a scope instance are built when the instance comes into being: when the ready event of a token at the scope becomes due, the ready event of a compensation activity becoming due when compensation is triggered, and for an event subprocess or an instance started by a trigger, which the engine creates itself, when it is created, a compensation event subprocess being created when the subprocess it compensates completes, with the status and the data of the subprocess at its completion. The status begins with the status the token holds, whose timestamp is set to the time at which the instance comes into being, and the instance of a process or of a non-interrupting event subprocess is given its identity. The attributes the scope declares are then given their initial values one at a time in the order of their declaration, the status attributes first, then the data attributes, then the objects. An attribute the model initialises takes the value of its initial expression, evaluated at that moment; any other attribute takes the value the INITIALIZATION column gives it, or remains undefined. An attribute is initialised either by the model or by the instance data, never by both, and a row giving a value to an attribute the model initialises is rejected. An initial expression thus reads everything the scope inherits, the timestamp, the instance and every attribute declared before it, and nothing declared after it. The values of a loop activity and of the instances of a multi-instance activity are built once, when the token at the activity becomes ready; what is to differ from one iteration or instance to another is stated by operators of the start event of a subprocess or by the operators of a task.

The READY expressions are applied over the initial values when the ready event becomes due and may overwrite any value of the status and of the data the scope owns, as described above, and the COMPLETION expressions may modify any status attribute when the task completes. Neither changes the values of an enclosing scope's data.

The data may give no values to the attributes of an event subprocess, and a row naming such a node in its INITIALIZATION or READY column is rejected; the attributes of such scopes take the values the model assigns when the engine creates them. Every compensation of a compensation activity within an instance takes the same values the instance data give, as the iterations of a loop activity and the instances of a multi-instance activity do. An event subprocess is created when the scope containing it receives its token and again after every trigger of a non-interrupting one, so that the weighted data it is created with enter the objective whether it is triggered or not: a non-interrupting event subprocess triggered k times counts the initial weighted data of k + 1 instances, and a data attribute given its value by an operator of its start event rather than by an initial expression counts only when the event subprocess is triggered.

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
