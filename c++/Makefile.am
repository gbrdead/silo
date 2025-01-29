AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = -I$(srcdir)/ext -I$(srcdir)/ext/atomic_queue/include


bin_PROGRAMS = silo

silo_SOURCES = src/silo.cpp \
	src/WordsTrie.hpp src/WordsTrie.cpp \
	src/MPMC_PortionQueue.hpp src/BlockingPortionQueue.hpp src/NonBlockingPortionQueue.hpp \
	src/TurningGrille.hpp src/TurningGrille.cpp 
