# The ROme OpTimistic Simulator (ROOT-Sim) 3.0.0

[![Build Status](https://github.com/ROOT-Sim/core/workflows/ROOT-Sim%20core%20CI/badge.svg)](https://github.com/ROOT-Sim/core/actions)
[![codecov.io](https://codecov.io/gh/ROOT-Sim/branch/master/graphs/badge.svg)](https://codecov.io/gh/ROOT-Sim/core)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/7519f016f3d942b9b12c6ed03ae4ecf8)](https://www.codacy.com/gh/ROOT-Sim/core/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=ROOT-Sim/core&amp;utm_campaign=Badge_Grade)
[![doc coverage](https://img.shields.io/endpoint?url=https%3A%2F%2Froot-sim.github.io%2Fcore%2Fdocs%2Fcoverage%2Fmaster.json)](https://root-sim.github.io/core/docs/)
[![GitHub issues](https://img.shields.io/github/issues/ROOT-Sim/core)](https://github.com/ROOT-Sim/core/issues)
[![GitHub](https://img.shields.io/github/license/ROOT-Sim/core)](https://github.com/ROOT-Sim/core/blob/master/COPYING)

*Brought to you by the [High Performance and Dependable Computing Systems (HPDCS)](https://hpdcs.github.io/) Research Group*

----------------------------------------------------------------------------------------

The ROme OpTimistic Simulator is an x86-64 Open Source, distributed multithreaded parallel simulation library developed using C/POSIX technology. It transparently supports all the mechanisms associated with parallelization and distribution of workload across the nodes (e.g., mapping of simulation objects on different kernel instances) and optimistic synchronization (e.g., state recoverability).    
Distributed simulations rely on MPI3. In particular, global synchronization across the different nodes relies on asynchronous MPI primitives, for increased efficiency.

The programming model supported by ROOT-Sim allows the simulation model developer  to use a simple application-callback function named `ProcessEvent()` as the event handler, whose parameters determine which simulation object is currently taking control for processing its next event, and where the state of this object is located in memory.  An object is a data structure, whose state can be scattered on dynamically allocated memory chunks, hence the memory address passed to the callback locates a top level data structure implementing the object state-layout.

ROOT-Sim's development started as a research project late back in 1987, and is currently maintained by the High Performance and Dependable Computing Systems group, a joint research group between Sapienza, University of Rome and University of Rome "Tor Vergata".

## Dependencies

To build the project you need a C11 compiler such as GCC 8 or a later version, and the Meson build system.

## Building and installing

Run:

```bash
meson build -Dprefix={installdir}
cd build
ninja test
ninja install
```

where `{installdir}` is your preferred installation directory expressed as an absolute path.\
Alternatively you can skip the `-Dprefix` altogether: ROOT-Sim will be installed in your system directories.

## Compile and run a model

ROOT-Sim ships with two sample models in the `models` folder of the project: `pcs` and `phold`. For example, to compile the `pcs` model simply run:

```bash
cd models/pcs
{installdir}/bin/rootsim-cc *.c
```

If you installed ROOT-Sim system-wide the second command becomes simply:

```bash
rootsim-cc *.c -o model
```

To test the correctness of the model, it can be run sequentially, typing:

```bash
./model --serial --lp <number of required LPs> This allows to spot errors in the implementation more easily.
```

Then, to run it in parallel, type:
```bash
./model --wt <number of desired threads> --lp <number of required LPs>
```

To run in a distributed environment, you can use standard MPI commands, such as:

```bash
mpiexec -n 2 --hostfile hosts --map-by node ./model --wt 2 --lp 16
```

This command runs the simulation model on two nodes (`-n 2`) specified in the hosts file. Each node uses two concurrent threads (`--wt 2`). The simulation involves 16 total Logical Processes (`--lp 16`).

