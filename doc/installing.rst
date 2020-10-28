Installing
===================================

Getting the sources
-------------------

You can obtain the source code for this software directly from the 
`official Github repository <https://github.com/Piccions/NeuRome>`_.

Runtime dependencies
--------------------
NeuRome only has a runtime dependency on a C17 compliant compiler. 
The generated executables are self-contained: they should run without problems 
on machines with same OS and architecture as the compiling one.

Build dependencies
------------------

- a C17 compatible compiler
- `Meson <https://mesonbuild.com>`_
- `Python 3 <https://www.python.org>`_
- `Ninja <https://ninja-build.org>`_

If you want to enable the incremental checkpointing feature (this may dramatically 
improve performance on sparsely writing models) you also need:

- `Clang 9 <https://clang.llvm.org>`_ or greater
- `LLVM 9 <https://llvm.org/>`_ or greater

In addition, if you wish to run simulations on a computing cluster you need:

- a MPI 3 compliant compiler and runtime environment such as 
  `OpenMPI <https://www.open-mpi.org>`_ or `MPICH <https://www.mpich.org>`_

Building and installing
-----------------------

From the root folder of the project run:

.. code-block:: shell

   meson build -Dprefix={installdir}
   cd build
   ninja test
   ninja install

where ``{installdir}`` is your preferred installation directory expressed as an 
absolute path. Alternatively you can skip the ``-Dprefix`` option altogether: 
NeuRome will be installed in your system directories.
