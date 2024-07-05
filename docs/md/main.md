# Introduction
@mainpage Introduction

## About

BPMN-OS is a framework for optimisation and simulation of business processes. It is based on [BPMN 2.0](http://www.omg.org/spec/BPMN/2.0/) and uses the built-in extension mechanism to provide data relevant for optimisation and simulation.

![Architecture](images/Architecture.svg)

The framework is composed of the components illustrated above:

- **Model provider**: The model provider reads a @ref BPMNOS::Model::Model "BPMN model" containing extension elements required for optimisation and simulation.
- **Data provider**: The data provider creates instances of the processes with respective instance data. It creates a @ref BPMNOS::Model::Scenario "scenario" that can be run by the execution engine.
- **Execution engine**: The execution engine maintains a @ref BPMNOS::Execution::SystemState "system state" containing @ref BPMNOS::Execution::StateMachine "state machines" for each BPMN element with a @ref BPMN::Scope "scope" and all @ref BPMNOS::Execution::Token "tokens" within the scope. It automatically advances all tokens as far as possible and waits for the dispatch of @ref BPMNOS::Execution::Event "events" and @ref BPMNOS::Execution::Decisions "decisions" made by the controller.
- **Controller**: The controller is responsible for making all necessary @ref BPMNOS::Execution::Decisions "decisions" during process execution.
- **Observer**: Observers can connect to the execution engine to monitor changes in the execution.



## Installation

You can obtain BPMN-OS from  https://github.com/bpmn-os/bpmnos-engine.

### Dependencies

BPMN-OS requires Xerces-C++ 3.2.x., `bpmn++`, and  `schematic++`.

On Ubuntu Linux Xerces can be installed by
```sh
sudo apt install libxerces-c-dev
```

You can obtain `bpmn++` from https://github.com/bpmn-os/bpmnpp and `schematic++` from https://github.com/rajgoel/schematicpp. Please follow build and install instructions provided by these dependencies.

## Build BPMN-OS

The library is built like a typical CMake project.

 A normal build will look something like this (output omitted):

```sh
mkdir build
cd build
cmake ..
make
cd ..
 ```

You can use multiple processors to reduce build times
```sh
make -j[n]
 ```
where `[n]` is the number of processors to be used.

### Compiler optimizations

The `NDEBUG` flag can be used to turn on or off compiler optimizations.

To turn on compiler optimizations use
```
cmake -DNDEBUG=ON ..
```

To turn off compiler optimizations use
```
cmake -DNDEBUG=OFF ..
```

This creates a single header file `lib/bpmnos-model.h` and a library `lib/libbpmnos-model.a`. Moreover, an executable `bpmnos` using these libraries is created in the `bin` folder. An example showing how to use this executable is given in the [example folder](example).

### Tests

To run tests use
```sh
make tests
```

### Documentation

To create the documentation run
```sh
make docs
```
Doxygen is required to build the documentation.

### Code profiles

To create code profiles for selected examples use
```sh
make profiles
```

Compiler optimizations must be turned off to profile the code. After creating the code profiles, re-create the documentation to include profiling graphs.


## License

<p xmlns:cc="http://creativecommons.org/ns#" xmlns:dct="http://purl.org/dc/terms/"><span property="dct:title">BPMN-OS</span> by <span property="cc:attributionName">Asvin Goel</span> is licensed under <a href="https://creativecommons.org/licenses/by-nc-nd/4.0/?ref=chooser-v1" target="_blank" rel="license noopener noreferrer" style="display:inline-block;">CC BY-NC-ND 4.0<img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/cc.svg?ref=chooser-v1" alt=""><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/by.svg?ref=chooser-v1" alt=""><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/nc.svg?ref=chooser-v1" alt=""><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/nd.svg?ref=chooser-v1" alt=""></a></p>

Other license models may be granted by the author.

## Author

Asvin Goel
