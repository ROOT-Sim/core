# The ROme OpTimistic Simulator (ROOT-Sim) 3.0.0

[![Build Status](https://travis-ci.org/HPDCS/ROOT-Sim.svg?branch=master)](https://travis-ci.org/HPDCS/ROOT-Sim)
[![codecov.io](https://codecov.io/gh/HPDCS/ROOT-Sim/branch/master/graphs/badge.svg)](http://codecov.io/github/HPDCS/ROOT-Sim)
[![coverage](https://img.shields.io/endpoint?url=https%3A%2F%2Fhpdcs.github.io%2FROOT-Sim%2Fdocs%2Fcoverage%2Fmaster.json)](https://hpdcs.github.io/ROOT-Sim//documentation.html)
[![GitHub issues](https://img.shields.io/github/issues/HPDCS/ROOT-Sim)](https://github.com/HPDCS/ROOT-Sim/issues)
[![GitHub](https://img.shields.io/github/license/HPDCS/ROOT-Sim)](https://github.com/HPDCS/ROOT-Sim/blob/master/COPYING)


*Brought to you by the [High Performance and Dependable Computing Systems (HPDCS)](https://hpdcs.github.io/) Research Group*

----------------------------------------------------------------------------------------

The ROme OpTimistic Simulator is an x86-64 Open Source, distributed multithreaded parallel
simulation library developed using C/POSIX technology. It transparently supports all the
mechanisms associated with parallelization and distribution of workload across the nodes
(e.g., mapping of simulation objects on different kernel instances) and
optimistic synchronization (e.g., state recoverability).    
Distributed simulations rely on MPI3. In particular, global synchronization across
the different nodes relies on asynchronous MPI primitives, for increased efficiency.

The programming model supported by ROOT-Sim allows the simulation model developer 
to use a simple application-callback function named `ProcessEvent()` as the event handler,
whose parameters determine which simulation object is currently taking control for
processing its next event, and where the state of this object is located in memory. 
An object is a data structure, whose state can be scattered on dynamically allocated
memory chunks, hence the memory address passed to the callback locates a top level
data structure implementing the object state-layout.

ROOT-Sim's development started as a research project late back in 1987, and is currently
maintained by the High Performance and Dependable Computing Systems group, a joint
research group between Sapienza, University of Rome and University of Rome "Tor Vergata".

## Installation Notes

ROOT-Sim uses autotools to provide an installation workflow which is
common for all supported platforms. This repository does not provide
already-generated installation scripts (while released tarballs do),
rather we provide the convenience `autogen.sh` script which should
build everything on the target machine. Using autotools, `autoconf`,
`automake` and `libtoolize` are required to let `autogen.sh` generate
the correct `configure` script.

Briefly, the shell commands `./configure; make; make install` should
configure, build, and install this package.
Also, you can also type `make uninstall` to remove the installed files.

By default, `make install` installs the package's commands under
`/usr/local/bin`, include files under `/usr/local/include`, etc.  You
can specify an installation prefix other than `/usr/local` by giving
`configure` the option `--prefix=PREFIX`, where `PREFIX` must be an
absolute path name.

ROOT-Sim uses many `gcc` extensions, so the currently supported
compiler is only `gcc`.

For full configuration notes, and a discussion on the subsystems
which this simulation library offers, please refer to the 
[wiki](https://github.com/HPDCS/ROOT-Sim/wiki).


## Usage Notes

When running `make install`, the `rootsim-cc` compiler is added to the path.

To compile a simulaton model, simply `cd` into the project's directory
and type `rootsim-cc *.c -o model`. This will create the `model`
executable, which is the model code already linked with the ROOT-sim library.
`rootsim-cc` ultimately relies on `gcc`, so any flag supported by
`gcc` can be passed to `rootsim-cc`.

To test the correctness of the model, it can be run sequentially, typing    
`./model --sequential --lp <number of required LPs>`
This allows to spot errors in the implementation more easily.

Then, to run it in parallel, type    
`./model --wt <number of desired threads> --lp <number of required LPs>`

To run in a distributed environment, you can use standard MPI commands,
such as:    
`mpiexec -n 2 --hostfile hosts --map-by node ./model --wt 2 --lp 16`

This command runs the simulation model on two nodes (`-n 2`) specified in the
`hosts` file. Each node uses two concurrent threads (`--wt 2`). The simulation
involves 16 total Logical Processes (`--lp 16`).



<p align="center"><img width=12.5% src="../develop/doc/static/logo.png"></p>
<p align="center"><img width=30.0% src="../develop/doc/static/title.png"></p>


[![Build Status](https://travis-ci.com/Piccions/NeuRome.svg?token=Zuy1LvLCcAsJuLmKKi1V&branch=master)](https://travis-ci.com/Piccions/NeuRome) [![codecov](https://codecov.io/gh/Piccions/NeuRome/branch/master/graph/badge.svg?token=OZ61T1RTUS)](https://codecov.io/gh/Piccions/NeuRome) [![License](https://img.shields.io/badge/license-GPL3-blue.svg)](https://opensource.org/licenses/MIT)

## Overview


NeuRome is a discrete event simulation runtime written in C17.

## Dependencies

To build the project you need a C17 compliant compiler such as GCC 8 or a later version and the Meson build system.

## Building and installing

Run:

	meson build -Dprefix={installdir}
	cd build
	ninja test
	ninja install

where `{installdir}` is your preferred installation directory expressed as an absolute path.\
Alternatively you can skip the `-Dprefix` altogether: NeuRome will be installed in your system directories.

## Compile a model

NeuRome ships with two sample models in the `models` folder of the project: pcs and phold.\
For example, to compile the pcs model simply run:

	cd models/pcs
	{installdir}/bin/neurome-cc *.c
	
If you installed Neurome system-wide the second command becomes simply:

	neurome-cc *.c
	
This will create two files named `model_serial` and `model_parallel` which are respectively the single core and multithreaded version of the model.
