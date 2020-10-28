AC_PREREQ([2.67])
LT_PREREQ([2.2])

AC_INIT([ROOT-Sim],[2.0.0],[rootsim@googlegroups.com],[rootsim],[https://github.com/HPDCS/ROOT-Sim/])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE([-Wno-portability 1.10.1 -Wall foreign subdir-objects])

AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Debugging symbols or not?
AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug], [Enable debugging of the platform]))

# Run code coverage?
AC_ARG_ENABLE([coverage],
    AS_HELP_STRING([--enable-coverage], [Enable code coverage]))

<<<<<<< HEAD
AM_CONDITIONAL([DEBUG], [test x$enable_debug = xyes])

# Setup compilation flags
<<<<<<< HEAD
CFLAGS_COMMON="-std=c11 -fno-omit-frame-pointer -ftree-vectorize -ffloat-store -fno-common -fstrict-aliasing -fgnu89-inline -rdynamic -lm -D_FORTIFY_SOURCE"
=======
CFLAGS_COMMON="-std=c11 -fvisibility=hidden -fno-omit-frame-pointer -fno-common -fstrict-aliasing -fgnu89-inline -U_FORTIFY_SOURCE -D_GNU_SOURCE"
>>>>>>> origin/incremental
CFLAGS_WARNINGS="-Wall -Wextra -Wmisleading-indentation -Wcast-align -Wpointer-arith -Wstrict-overflow=5 -Wstrict-prototypes -Winline -Wundef -Wnested-externs -Wshadow -Wunreachable-code -Wfloat-equal -Wredundant-decls -Wold-style-definition -Wno-switch-enum"
#CFLAGS_WARNINGS="$CFLAGS_WARNINGS -Wconversion"

# Flag to enable debugging the platform
AS_IF([test x$enable_debug = xyes],
      [CFLAGS="-g3 -ggdb3 -O0 $CFLAGS_COMMON $CFLAGS_WARNINGS"],
      [CFLAGS="-O3 $CFLAGS_COMMON"]
      AC_DEFINE([NDEBUG]))

# Flag to enable code coverage
AS_IF([test x$enable_coverage = xyes],
      [CFLAGS="-coverage $CFLAGS"])
=======
if test "x$enable_debug" != "xyes"
then
	AC_DEFINE([NDEBUG])
fi


AC_ARG_ENABLE([profile],
    AS_HELP_STRING([--enable-profile], [Enable profiling the platform]))
>>>>>>> origin/reverse

# Extra check flags
AC_ARG_ENABLE(extra-checks,
AS_HELP_STRING([--enable-extra-checks],[Enable extra checks on models (kills performance!)]),
[enable_extra_checks=$enableval],[enable_extra_checks=no])
AS_IF([test "x$enable_extra_checks" = "xyes"], [
	AC_DEFINE([EXTRA_CHECKS])
])


# Code profiling flag
AC_ARG_ENABLE([profile],
    AS_HELP_STRING([--enable-profile], [Enable profiling the platform]))
AS_IF([test "x$enable_profile" = "xyes"], [
	CFLAGS="-pg $CFLAGS"
])


#----------------------------------------------------------------------------
# Doxygen Documentation
#
# If doxygen is installed on the box, generate a commodity Makefile in the
# doc subfolder to generate the documentation. This must be called explicitly,
# as it is separated from the main Makefile.
#----------------------------------------------------------------------------
AC_CHECK_PROGS([DOXYGEN], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/doxy.conf])])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/Makefile])])

#----------------------------------------------------------------------------
# Check for languages and compilation support
#----------------------------------------------------------------------------
AC_LANG([C])
AC_PROG_CC([gcc])
AC_PROG_CXX([clang++])
AC_PROG_CC_C_O
AC_PROG_CXXCPP
AC_PROG_CXX_C_O

AC_SUBST([LLVMFLAGS],[$(llvm-config --cppflags)])

#----------------------------------------------------------------------------
# Check for LLVM development headers FIXME
#----------------------------------------------------------------------------
# AC_LANG_PUSH([C++])
# AC_CHECK_HEADER([llvm/Pass.h], [ac_llvm_headers=yes], [ac_llvm_headers=no])
# if test "x$ac_llvm_headers" = "xno"
# then
#	 AC_MSG_ERROR([Cannot find LLVM development headers. Please install them.])
# fi
# AC_LANG_POP()

#----------------------------------------------------------------------------
# Check for an MPI environment
#----------------------------------------------------------------------------
AC_ARG_ENABLE([mpi], AS_HELP_STRING([--enable-mpi], [compile with MPI support. Default: no]))


# Look if we can find a suitable MPI compiler
AS_IF([test x"$enable_mpi" = xyes],[
	AC_CHECK_TOOLS([MPICC], [mpicc mpixlc_r mpixlc hcc mpxlc_r mpxlc sxmpicc mpifcc mpgcc mpcc cmpicc], [none])
	
	AS_IF([test x"$MPICC" = xnone],[with_mpi=no],[with_mpi=yes])
])

# We don't simply use MPI, rather a recent version which supports
# newer asynchronous primitives, e.g. MPI_Ireduce_scatter_block.
# We test here for the availability of such primitives
AS_IF([test x"$with_mpi" = xyes],[
	# Base C compiler, we will "inject" it into mpicc, later 
	BASE_CC="$CC"
	# Use our MPICC compiler
	CC="$MPICC"

	AC_MSG_CHECKING([if MPI version has asynchronous primitives])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
		#include <stdlib.h>
		#include <mpi.h>
	]], [[
		MPI_Ireduce_scatter_block(NULL, NULL, 1, MPI_INT, MPI_SUM, 1, NULL);
		MPI_Iallreduce(NULL, NULL, 1, MPI_INT, MPI_MIN, 1, NULL);
	]])], [
		AC_MSG_RESULT([yes])
		AC_DEFINE([HAVE_MPI])
	], [
		AC_MSG_FAILURE([Asynchronous MPI primitives are not available. Please update your MPI version.])
	])
])

#----------------------------------------------------------------------------
<<<<<<< HEAD
# Check for languages and compilation support
#----------------------------------------------------------------------------
AC_LANG([C])
AC_PROG_CC([gcc])
AX_CHECK_COMPILE_FLAG([-std=c11], [ac_c11=yes], [ac_c11=no])
AS_IF([test x$ac_c11 != xyes],[
	AC_MSG_FAILURE([No. ROOT-Sim requires a compiler speaking the C11 standard.])
])


#----------------------------------------------------------------------------
# Check for GCC version
#----------------------------------------------------------------------------
AC_USE_SYSTEM_EXTENSIONS
AX_COMPILER_VERSION
GCC_VERSION=$ax_cv_c_compiler_version
GCC_VERSION_MAJOR=$(echo $GCC_VERSION | cut -d'.' -f1)
GCC_VERSION_MINOR=$(echo $GCC_VERSION | cut -d'.' -f2)
GCC_VERSION_PATCH=$(echo $GCC_VERSION | cut -d'.' -f3)
AS_IF([test $GCC_VERSION_MAJOR -lt 4 -a $GCC_VERSION_MINOR -lt 9],[
	AC_MSG_FAILURE([At least version 4.9.0 is required.])
])

# GCC7 complains about switch cases fallthrough. We use this in some highly-
# optimized portions of the code. The -Wimplicit-fallthrough=3 compile-time
# flag can prevent the compiler from emitting warnings in all fallthrough
# cases where a "fallthrough" comment has been places (in a way compliant with
# the [[fallthrough]] C14 tag which we don't support here.
# Yet, previous version of the compiler fail badly if we simply use that
# flag. We check here for the compiler version, and if we're of GCC >= 7.0 we
# set the flag, only in debug mode.
AS_IF([test $GCC_VERSION_MAJOR -ge 7 -a "x$enable_debug" == "xyes"],[
	CFLAGS="$CFLAGS -Wimplicit-fallthrough=3"
])

#----------------------------------------------------------------------------
# Check for programs.
=======
# Checks for programs.
>>>>>>> origin/incremental
#----------------------------------------------------------------------------
AC_PROG_MKDIR_P
AM_PROG_AR
# Silence warning: ar: 'u' modifier ignored since 'D' is the default
AC_SUBST(AR_FLAGS, [cr])
AC_SUBST(ARFLAGS, [cr])
AM_PROG_AS
<<<<<<< HEAD
AC_PROG_AWK
LT_INIT([dlopen shared])
AC_ENABLE_SHARED
AC_DISABLE_STATIC


#----------------------------------------------------------------------------
# Check for objdump. TODO: we need to flag the compilation of globvars!
#----------------------------------------------------------------------------
AC_CHECK_PROG(OBJDUMP, objdump, objdump)

if test -z "$OBJDUMP"; then
  AC_MSG_ERROR([Cannot find objdump in your path; please set the OBJDUMP environment variable!])
fi

=======
AC_CHECK_PROGS([HIJACKER], [hijacker])
LT_INIT
>>>>>>> origin/reverse



#----------------------------------------------------------------------------
# Check for architecture
#----------------------------------------------------------------------------
#
# We rely on assembly modules, so we can support only the following architectures.
# Perform a test here, and halt if an unsupported architecture is found.

AC_CANONICAL_HOST

AC_MSG_CHECKING([for a supported CPU])

case "${host_cpu}" in
	x86_64)
		AC_MSG_RESULT([yes, ${host_cpu}])
		;;
	*)
		AC_MSG_RESULT([no, ${host_cpu}])
		AC_MSG_ERROR([Unsupported host architecture. Currently ROOT-Sim supports only x86_64 systems.])
		;;
esac




#----------------------------------------------------------------------------
# Checks for C header files and libraries
#----------------------------------------------------------------------------

# Check for POSIX thread support
AX_PTHREAD([], [AC_MSG_ERROR([PTHREAD support not found])])

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

AC_SUBST([LIBS])
AC_SUBST([CFLAGS])
AC_SUBST([CC])
AC_SUBST([CXX])



#----------------------------------------------------------------------------
# Check for Operating System
#----------------------------------------------------------------------------
#
# This must go after CPU check, as it relies on AC_CANONICAL_HOST
#

AC_MSG_CHECKING([for supported host Operating System])

case $host_os in
  linux*)
        AC_MSG_RESULT([yes, ${host_os}])
        AC_DEFINE([OS_LINUX])
        ac_on_linux=yes
	;;
  cygwin*)
        AC_MSG_RESULT([yes, ${host_os}])
        AC_DEFINE([OS_WINDOWS])
        AC_DEFINE([HAVE_CYGWIN])
	;;
  mingw*)
        AC_MSG_RESULT([yes, ${host_os}])
        AC_DEFINE([OS_WINDOWS])
	;;
    *)
        #Default Case
        AC_MSG_RESULT([no, ${host_os}])
        AC_MSG_ERROR([Your platform is not currently supported])
        ;;
esac

<<<<<<< HEAD
#----------------------------------------------------------------------------
# COMPILE TIME OPTIONS
#
# From here on, we enable/disable compile time switches for the simulator
#----------------------------------------------------------------------------


# LP binding support
AC_ARG_ENABLE([rebinding],
AS_HELP_STRING([--disable-rebinding], [Disable LPs Rebinding (Enabled by default)]))

AS_IF([test "x$enable_rebinding" != "xno"], [
        AC_DEFINE([HAVE_LP_REBINDING])
        ac_have_lp_rebinding=yes
])

# approximated rollback support
AC_ARG_ENABLE(approximated-rollback,
AS_HELP_STRING([--enable-approximated-rollback],[Enable approximated rollback support)]),
[enable_approximated_rollback=$enableval],[enable_approximated_rollback=no])
AS_IF([test "x$enable_approximated_rollback" = "xyes"], [
	AC_DEFINE([HAVE_APPROXIMATED_ROLLBACK])
])

<<<<<<< HEAD
=======
#------------------------------------------------------------
# Power management support
#------------------------------------------------------------
ac_power_management=no
if test "x$ac_on_linux" = "xyes"
then
	ac_power_management=yes
	AC_CHECK_FILE("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor",, [ac_power_management=no])
	AC_CHECK_FILE("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies",, [ac_power_management=no])
	AC_CHECK_FILE("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed",, [ac_power_management=no])
	AC_CHECK_FILE("/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj",, [ac_power_management=no])

	AS_IF([test "x$ac_power_management" != "xno"], [
		AC_DEFINE([HAVE_POWER_MANAGEMENT])
		ac_have_power_management=yes
	])
fi
>>>>>>> origin/energy
=======
#------------------------------------------------------------
# Check for availability of Intel performance counters
#------------------------------------------------------------
AC_MSG_CHECKING([if we can use Intel PMU])

AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#define BIT(nr)			(1UL << (nr))
#define BIT_ULL(nr)		(1ULL << (nr))

#define cpuid(_a) \
	a = (_a); \
	__asm__ __volatile__("cpuid" \
			: "=a" (a), "=b" (b), "=c" (c), "=d" (d) \
			: "0" (a), "2" (c))
]], [[
  unsigned a = 0, b = 0, c = 0, d = 0;

  cpuid(0); // Check vendor
  if (!(b == 0x756e6547 && c == 0x6c65746e && d == 0x49656e69))
    return -1;

  cpuid(0xa); // Architectural performance monitoring version ID
  if(!((a & 0xFF) > 1 && ((a >> 8) & 0XFF) > 0))
    return -1;

  cpuid(0x1); // Debug Store (DS) support
  if(!(!!(d & BIT(21)) && !!(c & BIT(15))))
    return -1;

  return 0;
]])], [
AC_DEFINE([HAVE_PMU])
AC_MSG_RESULT([yes])
ac_pmu="(PMU available)"
ac_pmu_cc="xyes"
ac_iss="Enabled (Hardware based)"
], [
AC_MSG_RESULT([no])
ac_pmu=""
ac_pmu_cc="xno"
])

>>>>>>> origin/incremental

#------------------------------------------------------------
# Linux specific subsystems.
# On other architectures all these checks are just skipped
#------------------------------------------------------------
ac_preemption=no
ac_have_cross_state=no
ac_kernel_headers=no
ac_disable_modules=yes
ac_system_map=no

if test "x$ac_on_linux" = "xyes"
then
<<<<<<< HEAD
=======

	# If our libtimestretch library is installed, then we can use Preemptive Time Warp
	AC_CHECK_LIB(timestretch, ts_start, [libtimestretch=yes], [libtimestretch=no])
	if test "x$libtimestretch" = "xyes" -a "x$enable_ult" != "xno"
	then
			#
			# We have found everything that we need, but the user can disable the subsystem
			#

		AC_ARG_ENABLE([preemption],
		AS_HELP_STRING([--disable-preemption], [Disable Preemptive Time Warp)]))

		AS_IF([test "x$enable_preemption" != "xno"], [
			AC_DEFINE([HAVE_PREEMPTION])
			ac_have_preemption=yes
			CFLAGS="$CFLAGS -ltimestretch"
		])
	fi

	# If we are on a NUMA system, try to use the NUMA subsystem
	AC_CHECK_LIB(numa, numa_num_possible_nodes, [numa_cv_libnuma=yes], [numa_cv_libnuma=no])
	AC_CHECK_HEADER(numa.h, [numa_cv_numa_h=yes], [numa_cv_numa_h=no])
	AC_CHECK_PROG([numactl],[numactl],[yes],[no])

	if test "x$numa_cv_libnuma" = "xyes" -a "x$numa_cv_numa_h" = "xyes" -a "x$numactl" = "xyes" -a "x$ac_have_parallel_allocator" = "xyes"
	then
			#
			# We have found everything that we need, but the user can disable the subsystem
			#

		AC_ARG_ENABLE([numa],
		AS_HELP_STRING([--disable-numa], [Disable NUMA subsystem on NUMA machines]))

		AS_IF([test "x$enable_numa" != "xno"], [
			AC_DEFINE([HAVE_NUMA])
			ac_have_numa=yes
			CFLAGS="$CFLAGS -lnuma"
		])
	fi

	# Power Management Subsystem Configuration
	ac_power_management=yes
	AC_CHECK_FILE("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor",, [ac_power_management=no])
	AC_CHECK_FILE("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies",, [ac_power_management=no])
	AC_CHECK_FILE("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed",, [ac_power_management=no])
	AC_CHECK_FILE("/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj",, [ac_power_management=no])

	if test "x$ac_power_management" = "xyes"
	then
		#
		# We have found everything that we need, but the user can disable the subsystem
		#

		AC_ARG_ENABLE([pmanagement],
		AS_HELP_STRING([--disable-pmanagement], [Disable the Power Management Subsystem]))

		AS_IF([test "x$enable_pmanagement" != "xno"], [
			AC_DEFINE([HAVE_POWER_MANAGEMENT])
			ac_have_power_management=yes
		])
	fi



>>>>>>> origin/power
	# Test for linux headers, to compile kernel modules
	ac_kernel_headers=no
	AC_CHECK_HEADERS([linux/ioctl.h],
		[ac_kernel_headers=yes])

	if test "x$ac_kernel_headers" = "xyes"
	then
		# Configure kernel module paths
		AC_SUBST([with_kernel], [`uname -r`])
		AC_SUBST([with_kernel_mod], [/lib/modules/$with_kernel/extra])
		AC_SUBST([KERNEL_SRC], [/lib/modules/$with_kernel/build])
		AC_SUBST([KERNEL_MOD], [$with_kernel_mod])
	fi

	# We might want to disable all kernel modules explicitly. This is useful, for example,
	# when installing the simulator in a per-user space, in a system where the modules
	# were already installed. Using the --disable-modules won't compile nor try to
	# install any module.
	AC_ARG_ENABLE([modules],
		AS_HELP_STRING([--enable-modules], [Enable compilation/installation of kernel modules]))

	AS_IF([test "x$enable_modules" = "xyes"], [
		ac_disable_modules=no
		ac_kernel_headers=yes
	])
fi # Closes che global check for Linux availability

# This instructs the Makefile to build kernel modules only when possible
AM_CONDITIONAL([NO_KERNEL_MODULES], [test "x$ac_disable_modules" = "xyes"])


# Workaround to have MPI use clang for compilation
AS_IF([test x"$with_mpi" = xyes],[
	CC="OMPI_CC=$BASE_CC MPICH_CC=$BASE_CC $CC"
])
<<<<<<< HEAD
AC_SUBST([COMPILER], [$CC])
=======



# If we are on a NUMA system, try to use the NUMA subsystem
AC_CHECK_LIB(numa, numa_num_possible_nodes, [numa_cv_libnuma=yes], [numa_cv_libnuma=no])
AC_CHECK_HEADER(numa.h, [numa_cv_numa_h=yes], [numa_cv_numa_h=no])
AC_CHECK_PROG([numactl],[numactl],[yes],[no])
ac_have_numa=no

if test "x$numa_cv_libnuma" = "xyes" -a "x$numa_cv_numa_h" = "xyes" -a "x$numactl" = "xyes" -a "x$ac_have_parallel_allocator" = "xyes"
then
        #
        # We have found everything that we need, but the user can disable the subsystem
        #

	AC_ARG_ENABLE([numa],
	AS_HELP_STRING([--disable-numa], [Disable NUMA subsystem on NUMA machines (enabled by default)]))

	AS_IF([test "x$enable_numa" != "xno"], [
		AC_DEFINE([HAVE_NUMA])
		ac_have_numa=yes
		CFLAGS="$CFLAGS -lnuma"
	])
fi


# If requested, enable reverse computation
if test "x$HIJACKER" = "xhijacker"
then
	AC_ARG_ENABLE([reverse],
	AS_HELP_STRING([--enable-reverse], [Enable reverse computation]))

	AS_IF([test "x$enable_reverse" = "xyes"], [
	        AC_DEFINE([HAVE_REVERSE])
	        ac_have_reverse=yes
	])
fi



>>>>>>> origin/reverse

#----------------------------------------------------------------------------
# Configure rootsim-cc.
# The compiler script is dynamically built in order to create ad compile
# time linker scripts which will be used to produce the final version
# of the executable. We compute here the sizes of those cat'd scripts
# in order to make the compiler behave consistently with the last version
# of the linker scripts provided in the installation.
# Also, we must tell the compiler if ISS is available or not
#----------------------------------------------------------------------------
AC_MSG_CHECKING([compiler and linker scripts sizes])
AC_SUBST([cc_script_lines], [$(wc -l scripts/rootsim-cc.in | sed 's,\ .*,,g')])
<<<<<<< HEAD
AC_SUBST([cc_lddata1_lines], [$(wc -l scripts/ld-data1 | sed 's,\ .*,,g')])
AC_SUBST([cc_lddata2_lines], [$(wc -l scripts/ld-data2 | sed 's,\ .*,,g')])
AC_SUBST([cc_ldfinal_lines], [$(wc -l scripts/ld-final | sed 's,\ .*,,g')])
AC_SUBST([iss_support], ["x$ac_gcc_headers"])
AC_SUBST([pmu_support], ["$ac_pmu_cc"])
AC_MSG_RESULT([ok])

=======
AC_SUBST([cc_hijacker_conf_lines], [$(wc -l scripts/hijacker-conf.xml | sed 's,\ .*,,g')])
AC_MSG_RESULT([ok])

# Additionally, test whether we have to call hijacker from rootsim-cc
AM_CONDITIONAL([USE_HIJACKER], [test "x$enable_reverse" = "xyes"])


>>>>>>> origin/reverse
# Generate Makefiles for all these subfolders
AC_CONFIG_FILES([Makefile
		 man/Makefile
		 scripts/Makefile
		 src/arch/x86/linux/Makefile])
AC_OUTPUT



#----------------------------------------------------------------------------
# Generate a summary of the configuration adopted on the current machine.
# At this step, we give as well some information on how to enable/disable
# autoconfigured stuff.
#----------------------------------------------------------------------------

# Compose the debugging message
if test "x$enable_debug" = "xyes"
then
	enable_debug="Enabled"
else
	enable_debug="Disabled (use --enable-debug if wanted)"
fi

# Compose the message regarding MPI
if test x"$enable_mpi" == xyes
then
	if test x"$with_mpi" == xyes
	then
		enable_mpi="Enabled (compiler: $MPICC)"
	else
		enable_mpi="Disabled (no MPI compiler found)"
	fi
else
	enable_mpi="Disabled (enable with --enable-mpi)"
fi


<<<<<<< HEAD
# Compose the message regarding LP rebinding
if test "x$ac_have_lp_rebinding" = "xyes"
then
	ac_lp_rebinding="Enabled (By default, use --disable-rebinding if not wanted)"
else
	ac_lp_rebinding="Disabled (manually excluded)"
fi


# Compose the message regarding approximated rollback
if test "x$enable_approximated_rollback" = "xyes"
then
	ac_approximated_rollback="Enabled"
else
	ac_approximated_rollback="Disabled (By default, use --enable-approximated-rollback if wanted)"
fi


=======
>>>>>>> origin/energy
# Compose the message regarding Preemptive Time Warp
if test "x$ac_have_preemption" = "xyes"
then
	ac_preemption="Enabled (by default with libtimestretch, use --disable-preemption if not wanted)"
else
	ac_preemption="Disabled"
	if test "x$libtimestretch" = "xyes"
	then
		ac_preemption="${ac_preemption} by the user"
	else
		ac_preemption="${ac_preemption} (libtimestretch not found)"
	fi
fi


# Compose the message regarding ECS
ac_have_cross_state="no"
if test "x$ac_have_cross_state" = "xyes"
then
	ac_ecs="Enabled"
else
	ac_ecs="Disabled"
	if test "x$ac_on_linux" = "xyes"
	then
		if test "x$ac_disable_modules" = "xno"
		then
			if test "x$ac_kernel_headers" = "xno"
			then
				ac_ecs="${ac_ecs} (Linux Headers not found)"
			fi

			if test "x$ac_system_map" = "xno"
			then
				ac_ecs="${ac_ecs} (System.map not present)"
			fi
		else
			ac_ecs="${ac_ecs} (Requires Linux Kernel Modules)"
		fi
	else
		ac_ecs="${ac_ecs} (requires linux)"
	fi
fi

# Compose the message regarding Power Management
if test "x$ac_have_power_management" = "xyes"
then
	ac_power="Enabled"
else
	if test "x$enable_pmanagement" = "xno"
	then
		ac_power="Disabled by the user"
	else
		ac_power="Disabled (kernel files not found)"
	fi
fi

# Compose the message regarding reverse computing
if test "x$ac_have_reverse" = "xyes"
then
	ac_reverse="Enabled"
else
	ac_reverse="Disabled"
	if test "x$enable_reverse" = "xyes"
	then
		ac_reverse="${ac_reverse} (hijacker not found on the system)"
	else
		ac_reverse="${ac_reverse} (use --enable-reverse to activate)"
	fi
fi



# Compose message to tell whether modules are disabled
ac_modules="Enabled"
if test "x$ac_disable_modules" = "xyes"
then
	ac_modules="Disabled"
	if test "x$ac_on_linux" = "xyes"
	then
		ac_modules="${ac_modules} (enable with --enable-modules)"
	else
		ac_modules="${ac_modules} on a non-Linux system"
	fi
fi


# Compose the message regarding Power Management
if test "x$ac_have_power_management" = "xyes"
then
	ac_power="Enabled"
else
	ac_power="Governor information not found"
fi

# This is the line to reintroduce the ECS and modules messages
#Event Cross State......... : ${ac_ecs}
#Linux Kernel Modules...... : ${ac_modules}
cat<<EOF


   * ROOT-Sim Configuration on Current Machine *

CPU Architecture.......... : ${host_cpu} ${ac_pmu} (C: ${CC}, C++: ${CXX})
Operating System.......... : ${host_os}
Debugging Support......... : ${enable_debug}
MPI....................... : ${enable_mpi}
Power Management.......... : ${ac_power}
LP Preemption Support..... : ${ac_preemption}
<<<<<<< HEAD
LP Rebinding.............. : ${ac_lp_rebinding}
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
Approximated Rollback..... : ${ac_approximated_rollback}
=======
>>>>>>> origin/energy

=======
=======
Power Management.......... : ${ac_power}
>>>>>>> origin/power
Linux Kernel Modules...... : ${ac_modules}
Incremental State Saving.. : ${ac_iss}
>>>>>>> origin/incremental
=======
Reverse Computing......... : ${ac_reverse}

>>>>>>> origin/reverse

EOF
