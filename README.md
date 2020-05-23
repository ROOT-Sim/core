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
