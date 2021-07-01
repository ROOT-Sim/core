# Download and Installation Guide

The ROOT-Sim core can be downloaded cloning the official repository, or downloading a prepackaged tarball containing the distributed source code. To compile the library, a C11 compiler is required, such as GCC 8 or a later version, and the [Meson build system](https://mesonbuild.com/).

If you decide to clone the repository, there are two main branches of interest: the `master` branch contains the latest release version, while the `develop` branch contains the latest development version. At the time of this writing, the latest stable version is 3.0.0.

To clone the repository, which is hosted on GitHub, you can issue the following command:

```
git checkout https://github.com/ROOT-Sim/core
```
## Building and installing

Run:

```bash
meson build -Dprefix={installdir}
cd build
ninja test
ninja install
```

where `{installdir}` is your preferred installation directory expressed as an absolute path.\
Alternatively you can skip the `-Dprefix` altogether: NeuRome will be installed in your system directories.

## Compile a model

ROOT-Sim ships with two sample models in the `models` folder of the project: `pcs` and `phold`. For example, to compile the `pcs` model simply run:

```bash
cd models/pcs
{installdir}/bin/neurome-cc *.c
```

If you installed ROOT-Sim system-wide the second command becomes simply:

```bash
rootsim-cc *.c
```

This will create two files named `model_serial` and `model_parallel` which are respectively the single core and multithreaded version of the model.

## Dependencies

These are the requirements to build ROOT-Sim. Note that if you exclude specific subsystems, you can ignore the corresponding dependency.

| Dependency | Minimum Version |
|:-----------|:---------------:|
| gcc        | 8.0.0           |
| OpenMPI(*) | 3.0.1           |
| MPICH(*)   | 3.2.0           |

(*) Only one between OpenMPI and MPICH are required.