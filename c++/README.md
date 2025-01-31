# Silo C++ reference test

## Build

### Quick build instructions for a Debian-based GNU/Linux distribution:

`# apt install g++ autoconf automake autoconf-archive make libboost-all-dev libtbb-dev`  
`$ ./autogen.sh`  
`$ ./configure`  
`$ make`  

### Detailed build instructions

These instructions have been tested on Debian GNU/Linux but they should be applicable with minor modifications on any modern UNIX OS.

1. Install a C++ compiler. Both GCC and Clang C++ compilers work. The package names are ***g++*** and ***clang++***, respectively.

2. Install [autoconf](https://www.gnu.org/software/autoconf/) and [automake](https://www.gnu.org/software/automake/). The package names are ***autoconf*** and ***automake***, respectively.

3. Install the [autoconf archive](https://www.gnu.org/software/autoconf-archive/). The package name is ***autoconf-archive***. In case you need to install it manually, make sure to put the macros in the directory printed by the command `aclocal --print-ac-dir`.

4. Install [Boost](https://www.boost.org/). The package name is ***libboost-all-dev***.

5. Install [oneAPI Threading Building Blocks](https://uxlfoundation.github.io/oneTBB/). The package name is ***libtbb-dev***.

6. Run `autogen.sh` in the source directory.

7. Run `configure` from the build directory. You can use the source directory as a build directory, too. Using a separate initially empty directory is recommended.

8. Run `make`. The resulting executable is named `silo`.

## Running the reference test

`silo` must be run from the top directory of the silo project. There is only one command-line parameter:

| Parameter value | Queue implementation | Non-blocking |
|---|---|---|
| concurrent | [moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue) | mostly |
| atomic | [atomic_queue](https://max0x7ba.github.io/atomic_queue/) | mostly |
| lockfree | [Boost lockfree queue](https://www.boost.org/doc/libs/release/doc/html/lockfree.html) | mostly |
| textbook | a simple blocking bounded queue using only the standard C++ library (queue, mutex and condition_variable) | no |
| onetbb | [oneTBB concurrent_queue](https://oneapi-spec.uxlfoundation.org/specifications/oneapi/latest/elements/onetbb/source/containers/concurrent_queue_cls) | mostly |
| onetbb_bounded | [oneTBB concurrent_bounded_queue](https://oneapi-spec.uxlfoundation.org/specifications/oneapi/latest/elements/onetbb/source/containers/concurrent_bounded_queue_cls) | no |
| perfect | queueless, with no synchronization overhead | yes |
