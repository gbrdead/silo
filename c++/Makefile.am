AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = -I$(srcdir)/ext -I$(srcdir)/ext/atomic_queue/include -I$(srcdir)/ext/xenium


bin_PROGRAMS = silo

silo_SOURCES = src/silo.cpp \
	src/WordsTrie.hpp src/WordsTrie.cpp \
	src/MPMC_PortionQueue.hpp src/MostlyNonBlockingPortionQueue.hpp src/BlockingPortionQueue.hpp src/NonBlockingQueue.hpp \
	src/Grille.hpp src/TurningGrilleCracker.hpp src/TurningGrilleCracker.cpp 
