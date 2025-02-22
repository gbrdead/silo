# Silo C++ multi-threading performance test

## Build

### Quick build instructions for a Debian-based GNU/Linux distribution:

`# apt install g++ autoconf automake autoconf-archive make libboost-all-dev libtbb-dev`  
`$ ./autogen.sh`  
`$ CXXFLAGS="-O3" ./configure`  
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

`silo` must be run from the top directory of the silo project. There is only one command-line parameter, specifying the implementation type:

| Implementation | Queue type | Non-blocking | Suffers from scheduler unfairness |
|---|---|---|---|
| syncless | queueless, with no synchronization overhead | yes | yes |
| atomic | [atomic_queue](https://max0x7ba.github.io/atomic_queue/) | mostly | no |
| concurrent | [moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue) | mostly | no |
| lockfree | [Boost lockfree queue](https://www.boost.org/doc/libs/release/doc/html/lockfree.html) | mostly | no |
| textbook | a simple blocking bounded queue using only the standard C++ library (queue, mutex and condition_variable) | no | no |
| onetbb | [oneTBB concurrent_queue](https://oneapi-spec.uxlfoundation.org/specifications/oneapi/latest/elements/onetbb/source/containers/concurrent_queue_cls) | mostly (?) | no |
| onetbb_bounded | [oneTBB concurrent_bounded_queue](https://oneapi-spec.uxlfoundation.org/specifications/oneapi/latest/elements/onetbb/source/containers/concurrent_bounded_queue_cls) | mostly (?) | no |
| sync_bounded | [Boost synchronous bounded queue](https://www.boost.org/doc/libs/release/doc/html/thread/sds.html#thread.sds.synchronized_queues.ref.sync_bounded_queue_ref) | no | no (?) |

## Test results

| Implementation / CPU (hardware parallelism) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 7735HS (16) |
|---|---|---|---|
| syncless |  |  | 4736 |
| concurrent |  |  | 3388 |
| atomic |  |  | 3526 |
| lockfree |  |  | 3009 |
| textbook |  |  | 1694 |
| onetbb |  |  | 1217 |
| onetbb_bounded |  |  | 1068 |
| serial |  |  | 886 |

General results:
- `concurrent` and `atomic` have almost the same performance and can be considered as co-winners among the queues.

Some remarks: 
- The thread scheduler is very fair. Thus the syncless implementation is close to perfect. The most privileged thread finishes its job at more than 99% ot the total job done. As a result, the CPUs are not fully utilized for just a small amount of time and the average speed is not significantly affected.
- At first glance, `oneapi::tbb::concurrent_bounded_queue` should work like `org::voidland::concurrent::MostlyNonBlockingPortionQueue` - non-blocking most of the time, blocking only on hitting its bounds. But its performance is too low for this to be true.
- The average speed of the oneTBB queues is unexplainably low. Also, their performance is erratic - the speed varies wildly (no other implementation behaves in this way).
- `boost::sync_bounded_queue` is not tested because it is buggy. Sometimes it fails to wake up a producer despite that the queue becomes not full and this leads to a dead lock.

## TODO
- Measure the performance with Clang. The tests so far have been performed with GCC.
