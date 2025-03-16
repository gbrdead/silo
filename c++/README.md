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

| Implementation | Queue type | Non-blocking | Stable measurements |
|---|---|---|---|
| `concurrent` | [moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue) | mostly | yes |
| `atomic` | [atomic_queue](https://max0x7ba.github.io/atomic_queue/) | mostly | yes |
| `lockfree` | [Boost lockfree queue](https://www.boost.org/doc/libs/release/doc/html/lockfree.html) | mostly | yes |
| `onetbb` | [oneTBB concurrent_queue](https://oneapi-spec.uxlfoundation.org/specifications/oneapi/latest/elements/onetbb/source/containers/concurrent_queue_cls) | mostly (?) | no |
| `onetbb_bounded` | [oneTBB concurrent_bounded_queue](https://oneapi-spec.uxlfoundation.org/specifications/oneapi/latest/elements/onetbb/source/containers/concurrent_bounded_queue_cls) | mostly (?) | no |
| `textbook` | a simple blocking bounded queue using only the standard C++ library (queue, mutex and condition_variable) | no | yes |
| `sync_bounded` | [Boost synchronous bounded queue](https://www.boost.org/doc/libs/release/doc/html/thread/sds.html#thread.sds.synchronized_queues.ref.sync_bounded_queue_ref) | no | no |
| `syncless` | queueless, with no synchronization overhead | yes | yes |
| `serial` | single-threaded | N/A | yes |

## Test results

| Implementation / CPU (hardware parallelism) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 7735HS (16) |
|---|---|---|---|
| concurrent | 730 | 1368 | 3447 |
| atomic | 741 | 1409 | 3551 |
| lockfree | 715 | 1321 | 2833 |
| textbook | 684 | 1110 | 1769 |
| syncless | 863 | 1661 | 4682 |
| serial | 393 | 534 | 854 |
| onetbb | ~~444~~ | ~~670~~ | ~~936~~ |
| onetbb_bounded | ~~331~~ | ~~605~~ | ~~741~~ |

General results:
- `concurrent` and `atomic` have almost the same performance and can be considered as co-winners among the queues.

Some remarks: 
- The thread scheduler is very fair. Thus the syncless implementation is close to perfect. The most privileged thread finishes its job at more than 99% ot the total job done.
- At first glance, `oneapi::tbb::concurrent_bounded_queue` should work like `org::voidland::concurrent::MostlyNonBlockingPortionQueue` - non-blocking most of the time, blocking only on hitting its bounds. But its performance is too low for this to be true.
- The average speed of the oneTBB queues is inexplicably low. Also, their performance is erratic - the speed varies wildly.
- `boost::sync_bounded_queue` is buggy. Sometimes it fails to wake up a producer despite that the queue becomes not full and this leads to a deadlock. That is why it is not measured.

## TODO
- Measure the performance with Clang. The tests so far have been performed with GCC.
- Find and report the bug in `boost::sync_bounded_queue`.
