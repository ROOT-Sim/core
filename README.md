# ROOT-Sim core 3.0.0

_Brought to you by the [High Performance Computing & Simulation (HPCS)](https://hpdcs.github.io/)
Research Group_

[![Build Status](https://github.com/ROOT-Sim/core/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/ROOT-Sim/core/actions)
[![codecov](https://codecov.io/gh/ROOT-Sim/core/branch/master/graph/badge.svg)](https://codecov.io/gh/ROOT-Sim/core)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/7519f016f3d942b9b12c6ed03ae4ecf8)](https://www.codacy.com/gh/ROOT-Sim/core/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=ROOT-Sim/core&amp;utm_campaign=Badge_Grade)
[![doc coverage](https://img.shields.io/endpoint?url=https%3A%2F%2Froot-sim.github.io%2Fcore%2Fdocs%2Fmaster.json)](https://root-sim.github.io/core/docs/)
[![GitHub issues](https://img.shields.io/github/issues/ROOT-Sim/core)](https://github.com/ROOT-Sim/core/issues)
[![GitHub](https://img.shields.io/github/license/ROOT-Sim/core)](https://github.com/ROOT-Sim/core/blob/master/LICENSES/GPL-3.0-only.txt)
[![REUSE status](https://api.reuse.software/badge/github.com/ROOT-Sim/core)](https://api.reuse.software/info/github.com/ROOT-Sim/core)

----------------------------------------------------------------------------------------

## The ROme OpTimistic Simulator

The ROme OpTimistic Simulator is an open-source, distributed and parallel simulation framework developed using C11.
It transparently supports all the mechanisms associated with parallelization and distribution of workload across the
nodes (e.g., mapping of simulation objects on different kernel instances) and optimistic synchronization (e.g., state
recoverability). Distributed simulations rely on MPI3. In particular, global synchronization across the various nodes
relies on asynchronous MPI primitives for increased efficiency.

The programming model supported by ROOT-Sim allows the simulation model developer to use a simple application-callback
function named `ProcessEvent()` as the event handler. Its parameters determine which simulation object is currently
taking control for processing its next event and where the state of this object is located in memory. An object is a
data structure whose state can be scattered on dynamically allocated memory chunks. Hence, the memory address passed to
the callback identifies a top-level data structure implementing the object state layout.

ROOT-Sim's development started as a research project late back in 1987 and is currently maintained by the High
Performance and Dependable Computing Systems group, a research group of the University of Rome "Tor Vergata".

## ROOT-Sim Core

This repository keeps the sources of the ROOT-Sim core: this is the lowest-level library that implements the most
significant part of the simulation algorithms used in the simulation framework.

The core can be built and used as a stand-alone low-level library writing C code, or it can be used within other
projects, such as [cROOT-Sim](https://github.com/ROOT-Sim/cROOT-Sim), i.e. the C/C++ packaging of the simulation
library.

## Dependencies and platforms

The core successfully compiles on x86 and ARM architectures, using either GCC or Clang compilers, on Linux, Windows,
and macOS.
A compiler supporting the C11 standard is required, such as GCC 8 or later. MSVC on Windows does not correctly
implement the full C11 standard (e.g., `stdatomic.h` is not provided, or has a minimal implementation for lock-free
objects only in latest versions) and cannot be therefore used to build the project.
Windows users are encouraged to use clang.

MPI is a mandatory dependency of the project, used to support simulations runs on distributed systems.
The core is continuously tested against the following MPI implementations:

* OpenMPI
* MPICH
* Intel MPI

Any of the three is required to build the project. A full MPI3 implementation supporting multithreading is necessary.

## Building

To build the project, run:

```bash
mkdir build
cd build
cmake -DCMAKE_C_COMPILER=gcc ..
make
```

You can specify a different compiler using the `-DCMAKE_C_COMPILER=` flag.
To run the test suite (which includes a correctness test), run in the `build` folder:

```bash
ctest
```

## Compiling and running a model

An implementation of test models can be located in `test\integration`.
The tests can be compiled using the standard `mpicc` compiler, linking against `librscore` and launching
locally, or using `mpiexec` to run on multiple nodes.

Some example models are available in the [models](https://github.com/ROOT-Sim/models) repository.
