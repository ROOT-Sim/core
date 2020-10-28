ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I$(top_srcdir)/src 
AM_MAKEFLAGS = -s
AUTOMAKE_OPTIONS = subdir-objects

librootsim_include_HEADERS = ROOT-Sim.h
librootsim_includedir = $(includedir)
lib_LIBRARIES = librootsim.a libwrapperl.a libdymelor.a

librootsim_a_SOURCES =	main.c \
			lib/numerical.c \
			lib/parseparam.c \
			lib/topology.c \
			arch/memusage.c \
			arch/thread.c \
			arch/ult.c \
			arch/x86.c \
			datatypes/array.c \
			datatypes/list.c \
			datatypes/calqueue.c \
<<<<<<< HEAD
<<<<<<< HEAD
=======
			datatypes/msgchannel.c \
			datatypes/slab.c \
			datatypes/treiber.c \
			datatypes/heap.c\
>>>>>>> origin/power
			mm/state.c \
			mm/globvars.c \
=======
			datatypes/slab.c \
			mm/state.c \
			mm/reverse.c \
>>>>>>> origin/reverse
			queues/queues.c \
			queues/xxhash.c \
			core/init.c \
			core/core.c \
<<<<<<< HEAD
=======
			core/power.c \
			scheduler/binding.c \
			scheduler/control.c \
			scheduler/preempt.c \
			scheduler/preempt_callback.S \
			scheduler/process.c \
>>>>>>> origin/power
			scheduler/stf.c \
			scheduler/scheduler.c \
			scheduler/control.c \
			serial/serial.c \
			statistics/statistics.c \
			gvt/gvt.c \
			gvt/fossil.c \
			gvt/cancelback.c \
			gvt/ccgs.c \
			gvt/models.c \
			communication/window.c \
			communication/communication.c

libwrapperl_a_SOURCES = lib-wrapper/stdlib.c \
			lib-wrapper/math.c

libdymelor_a_SOURCES = 	mm/checkpoints.c \
			mm/dymelor.c \
			mm/lp-alloc.c \
			mm/malloc.c
