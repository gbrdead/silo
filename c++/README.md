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
| `atomic` | [atomic_queue](https://max0x7ba.github.io/atomic_queue/) | mostly (wrapped by `blown_queue`) | yes |
| `concurrent` | [moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue) | mostly (wrapped by `blown_queue`) | yes |
| `lockfree` | [Boost lock-free queue](https://www.boost.org/doc/libs/release/doc/html/doxygen/classboost_1_1lockfree_1_1queue.html) | mostly (wrapped by `blown_queue`) | yes |
| `michael_scott` | [xenium michael_scott_queue](https://mpoeter.github.io/xenium/classxenium_1_1michael__scott__queue.html) | mostly (wrapped by `blown_queue`) | yes |
| `ramalhete` | [xenium ramalhete_queue](https://mpoeter.github.io/xenium/classxenium_1_1ramalhete__queue.html) | mostly (wrapped by `blown_queue`) | yes |
| `vyukov` | [xenium vyukov_bounded_queue](https://mpoeter.github.io/xenium/structxenium_1_1vyukov__bounded__queue.html) | mostly (wrapped by `blown_queue`) | yes |
| `kirsch_1fifo` | [xenium kirsch_kfifo_queue](https://mpoeter.github.io/xenium/classxenium_1_1kirsch__kfifo__queue.html) with k=1 | mostly (wrapped by `blown_queue`) | yes |
| `kirsch_bounded_1fifo` | [xenium kirsch_bounded_kfifo_queue](https://mpoeter.github.io/xenium/classxenium_1_1kirsch__bounded__kfifo__queue.html) with k=1 | mostly (wrapped by `blown_queue`) | yes |
| `nikolaev` | [xenium nikolaev_queue](https://mpoeter.github.io/xenium/classxenium_1_1nikolaev__queue.html) | mostly (wrapped by `blown_queue`) | N/A |
| `nikolaev_bounded` | [xenium nikolaev_bounded_queue](https://mpoeter.github.io/xenium/classxenium_1_1nikolaev__bounded__queue.html) | mostly (wrapped by `blown_queue`) | yes |
| `onetbb` | [oneTBB concurrent_queue](https://oneapi-spec.uxlfoundation.org/specifications/oneapi/latest/elements/onetbb/source/containers/concurrent_queue_cls) | mostly (wrapped by `blown_queue`) | no |
| `onetbb_bounded` | [oneTBB concurrent_bounded_queue](https://oneapi-spec.uxlfoundation.org/specifications/oneapi/latest/elements/onetbb/source/containers/concurrent_bounded_queue_cls) | mostly (on its own) | no |
| `textbook` | a simple blocking bounded queue using only the standard C++ library (queue, mutex and condition_variable) | no | yes |
| `sync_bounded` | [Boost synchronous bounded queue](https://www.boost.org/doc/libs/release/doc/html/thread/sds.html#thread.sds.synchronized_queues.ref.sync_bounded_queue_ref) | no | N/A |
| `syncless` | queueless, with no synchronization overhead | yes | yes |
| `serial` | single-threaded | N/A | yes |

## Test results

| Implementation / CPU (hardware parallelism) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 3700X (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|
| atomic | 831 | 1308 | 3367 | 3574 |
| ramalhete | 825 | 1309 | 3080 | 3428 |
| concurrent | 829 | 1264 | 2998 | 3378 |
| vyukov | 832 | 1310 | 2918 | 3253 |
| michael_scott | 791 | 1265 | 2567 | 2969 |
| lockfree | 808 | 1295 | 2312 | 2938 |
| nikolaev_bounded | 811 | 1264 | 2823 | 3289 |
| kirsch_1fifo | 723 | 1150 | 1214 | 1698 |
| kirsch_bounded_1fifo | 796 | 1254 | 1504 | 1965 |
| onetbb | ~~418~~ | ~~653~~ | ~~823~~ | ~~719~~ |
| onetbb_bounded | ~~386~~ | ~~548~~ | ~~1407~~ | ~~998~~ |
| textbook | 661 | 821 | 1094 | 1222 |
| syncless | 981 | 1597 | 4689 | 4615 |
| serial | 389 | 347 | 520 | 584 |

General results:
- `atomic` is the winner among the queues.

Some remarks: 
- The thread scheduler is very fair. Thus the syncless implementation is close to perfect. The most privileged thread finishes its job at more than 99% ot the total job done.
- At first glance, `onetbb_bounded` should work like `blown_queue` - non-blocking most of the time, blocking only on hitting its bounds. But its performance is too low compared 
- The average speed of the oneTBB queues is inexplicably low. Also, their performance is erratic - the speed varies wildly.
- `nikolaev_queue` has a stopper bug. Frequently it fails to pop an element even when the queue is not empty and this leads to a deadlock. That is why it is not measured.
- `sync_bounded` has a stopper bug. Sometimes it fails to wake up a producer despite that the queue becomes not full and this leads to a deadlock. That is why it is not measured.
- `nikolaev_bounded` has a non-stopper bug. Sometimes it moves the data out of the portion even if the push fails (because of a full queue). The portion is always copied as a workaround for this bug. 

## TODO
- Measure the performance with Clang. The tests so far have been performed with GCC.
