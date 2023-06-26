# BPMN-OS
## BPMN for optimization and simulation

## Dependencies

bpmnos requires Xerces-C++ 3.2.x. and bpmn++.

On Ubuntu Linux Xerces can be installed by
```sh
sudo apt install libxerces-c-dev
```

You can obtain `bpmn++` from https://github.com/bpmn-os/bpmnpp.

Furthermore, `schematic++` must be available for the build process. You can obtain `schematic++` from https://github.com/rajgoel/schematicpp.

## Build the library

The library is built like a typical CMake project. A normal build will look something like this (output omitted):

```sh
 ~/bpmnos$ mkdir build
 ~/bpmnos$ cd build
 ~/bpmnos/build$ cmake ..
 ~/bpmnos/build$ make
 ```

This creates a single header file `lib/bpmnos-model.h` and a library `lib/libbpmnos-model.a`.

## Example

An example using the library can be found in `example` folder.

After successful installation of the library, you can build the example by

```sh
cd example
g++ -std=c++20 main.cpp -L../parser/lib -lbpmn++ -lbpmnos-model -lxerces-c -o bpmnos
```

You can run the example by
```sh
cd example
./bpmnos diagram.bpmn
```
