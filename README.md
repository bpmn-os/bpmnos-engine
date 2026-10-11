# BPMN-OS
## BPMN for optimization and simulation

- **Download:** https://github.com/bpmn-os/bpmnos-engine
- **Documentation:** https://bpmn-os.github.io/bpmnos-engine

## Requirements

A C++23 compiler, GCC 15.2 or Clang 18.1.3 or later, CMake 3.26.4 or later, and git.

Every other dependency, namely bpmn++, schematic++, Xerces-C++ 3.2.x, cnl, limex, nlohmann/json and Catch2,
is fetched automatically unless it is installed already.

## Build

This project has three preset configurations for different purposes that are created in folders `build/release`, `build/debug`, and `build/profile`. Presets are configured the first time they are needed or when running `make configure`.


| Preset | Folder | Compiled with | Used for |
| --- | --- | --- | --- |
| `release` | `build/release` | `-O3 -DNDEBUG`, assertions off | building, documentation, installing |
| `debug` | `build/debug` | unoptimised, assertions live, address/undefined/leak sanitizers | development, tests |
| `profile` | `build/profile` | `-O3` with `-pg` | profiles |

You can build the libraries and `bpmnos-greedy` by

```sh
make # (release)
```
or
```sh
make dev # (debug)
```
with the preset indicated in parentheses.

The [examples folder](examples) holds a BPMN-OS model and its instance data for selected models, together with the commands for running a simulation using a greedy controller.

## Tests

To (build and) run the test suite, use
```sh
make tests # (debug)
```

Once the tests are built (and run) with `make tests`, you can use
`./build/debug/run_tests "[selected_tag]"` to run selected tests carrying the given Catch2 tag.

## Profiling

To profile the code for selected example use cases, run
```sh
make profiles # (profile)
```

Each example is executed by the instrumented `bpmnos-greedy` and its profile is written to `build/profile`
as a `.dot` and an `.svg`. 

> [!NOTE]
> Profiling requires `gprof`, which comes with `binutils`, and the graphs additionally require `gprof2dot` and `graphviz`. Run `pipx install gprof2dot` and `sudo apt install graphviz` to install these.

## Documentation

To generate docs in `build/release/docs/html` folder, run

```sh
make docs # (release)
```

> [!NOTE]
> Building the documentation requires `doxygen` and `graphviz`. Run `sudo apt install doxygen graphviz` to install these. Moreover, [`bpmnosdoc`](https://github.com/bpmn-os/bpmnos-js) is required to include BPMN-OS model documentation.

## Installation

To install, run
```sh
make # (release)
sudo make install
```
to copy
```
bin/bpmnos-greedy
include/bpmnos-model.h
include/bpmnos-execution.h
include/limex.h
include/cnl/
include/nlohmann/
lib/libbpmnos-model.a
lib/libbpmnos-execution.a
lib/cmake/bpmnos/
```
into `/usr/local`.

Alternatively, run
```sh
cmake --install build/release --prefix <target>
```
to install these files into the `<target>` folder.

> [!NOTE]
> Only a release build can be installed. Installing `build/debug` or `build/profile` is refused with an error.

A consumer can resolve the libraries with

```cmake
find_package(bpmnos REQUIRED)
target_link_libraries(mytarget PRIVATE bpmnos::execution bpmnos::model)
```

Nothing further is needed when bpmnos was installed into `/usr/local`. For a custom target folder, add
`-DCMAKE_PREFIX_PATH=<target>` when configuring the consumer.

A minimum version can be required, for instance `find_package(bpmnos 0.1.0 REQUIRED)`, which accepts any
later `0.1.x` and refuses `0.2.0`.

### Uninstall

Remove the installed files, where `<target>` is `/usr/local` or the folder given when installing.

```sh
rm <target>/bin/bpmnos-greedy
rm <target>/include/bpmnos-model.h <target>/include/bpmnos-execution.h
rm <target>/include/limex.h
rm -rf <target>/include/cnl <target>/include/nlohmann
rm <target>/lib/libbpmnos-model.a <target>/lib/libbpmnos-execution.a
rm -rf <target>/lib/cmake/bpmnos
```

The file `build/release/install_manifest.txt` lists the installed files.

> [!WARNING]
> Only remove these files if you are sure that no other application requires them.
