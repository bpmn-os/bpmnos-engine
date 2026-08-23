# Examples

Each folder holds a BPMN-OS model of an optimisation problem together with the CSV files it is executed on.
The instance data is `instance.csv`, and any further CSV file is a lookup table the model reads.

The examples are run with `bpmnos-greedy`, which `make` places in `build/release/bin`. From the project
folder, a model without lookup tables is executed by

```sh
./build/release/bin/bpmnos-greedy \
  --model examples/knapsack_problem/Knapsack_problem.bpmn \
  --data examples/knapsack_problem/instance.csv \
  --verbose
```

and a model with lookup tables by naming the folder they are found in.

```sh
./build/release/bin/bpmnos-greedy \
  --model examples/travelling_salesperson_problem/Travelling_salesperson_problem.bpmn \
  --data examples/travelling_salesperson_problem/instance.csv \
  --folders examples/travelling_salesperson_problem \
  --verbose
```

Execution is stochastic and guided by default, so a repeated run need not give the same result. Add
`--provider static` for a deterministic run, or `--seed <number>` to repeat a stochastic one. Run
`bpmnos-greedy` without arguments for the full list of options.
