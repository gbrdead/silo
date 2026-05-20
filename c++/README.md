# Silo C++ performance test

## Build

These instructions have been tested on Debian GNU/Linux but they should be applicable with minor modifications on any modern UNIX OS.

1. Install a C++ compiler. Both GCC and Clang C++ compilers work. The package names are ***g++*** and ***clang++***, respectively.

2. Install [autoconf](https://www.gnu.org/software/autoconf/) and [automake](https://www.gnu.org/software/automake/). The package names are ***autoconf*** and ***automake***, respectively.

3. Install the [autoconf archive](https://www.gnu.org/software/autoconf-archive/). The package name is ***autoconf-archive***. In case you need to install it manually, make sure to put the macros in the directory printed by the command `aclocal --print-ac-dir`.

4. Install [Boost](https://www.boost.org/). The package name is ***libboost-all-dev***.

5. Install [oneAPI Threading Building Blocks](https://uxlfoundation.github.io/oneTBB/). The package name is ***libtbb-dev***.

6. Build and install [BlownQueue](https://github.com/gbrdead/blown_queue) for C++:

`$ git clone https://github.com/gbrdead/blown_queue`  
`$ cd blown_queue`  
`# ./.github/workflows/install_queues.sh /usr/local`  
`$ cd c++`  
`$ autoreconf -s -i`  
`$ ./configure`  
`$ make`  
`# make install`  

7. Build `silo` itself:

`$ cd silo/c++`  
`$ autoreconf -s -i`  
`$ CXXFLAGS="-O3" ./configure`  
`$ make`  

The resulting executable is named `silo`.

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

### Test results per implementation

| Implementation / CPU (hardware parallelism) | Allwinner A64 (4) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 3700X (16) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|---|---|
| `atomic` | 245 | 831 | 1311 | 3398 | 3437 | 3574|
| `ramalhete` | 238 | 825 | 1310 | 3192 | 3311 | 3428|
| `concurrent` | 245 | 829 | 1272 | 3089 | 3027 | 3378|
| `vyukov` | 248 | 838 | 1312 | 2946 | 3173 | 3253|
| `michael_scott` | 226 | 791 | 1262 | 2607 | 2857 | 2937|
| `lockfree` | 231 | 808 | 1292 | 2340 | 2818 | 2936|
| `nikolaev_bounded` | 228 | 811 | 1270 | 3100 | 3252 | 3256|
| `kirsch_1fifo` | 190 | 723 | 1147 | 1208 | 1494 | 1695|
| `kirsch_bounded_1fifo` | 214 | 796 | 1252 | 1990 | 2343 | 2265|
| `onetbb` | ~~138~~ | ~~418~~ | ~~659~~ | ~~842~~ | ~~755~~ | ~~706~~|
| `onetbb_bounded` | ~~127~~ | ~~386~~ | ~~636~~ | ~~1107~~ | ~~895~~ | ~~997~~|
| `textbook` | 225 | 661 | 806 | 1089 | 967 | 1125|
| `syncless` | 290 | 981 | 1600 | 4743 | 4206 | 4615|
| `serial` | 73 | 389 | 347 | 528 | 501 | 584|

- `atomic` is the winner among the queues.
- The thread scheduler is very fair. Thus the `syncless` implementation is close to perfect. The most privileged thread finishes its job at more than 99% ot the total job done.
- At first glance, `onetbb_bounded` should work like `blown_queue` - non-blocking most of the time, blocking only on hitting its bounds. But its performance is too low for this to be true.
- The average speeds of the oneTBB queues are inexplicably low. Also, their performance is erratic - the speed varies wildly.
- `nikolaev_queue` has a stopper bug. Frequently it fails to pop an element even when the queue is not empty and this leads to a deadlock. That is why it is not measured.
- `sync_bounded` has a stopper bug. Sometimes it fails to wake up a producer despite that the queue becomes not full and this leads to a deadlock. That is why it is not measured.
- `nikolaev_bounded` has a non-stopper bug. Sometimes it moves the data out of the portion even if the push fails because of a full queue. The portion is always copied as a workaround for this bug.

### Test results per compiler

| Implementation (compiler) / CPU (hardware parallelism) | Allwinner A64 (4) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 3700X (16) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|---|---|
| syncless (GCC) | 290 | 981 | 1600 | 4743 | 4206 | 4615 |
| syncless (Clang) | 238 | 844 | 1439 | 4211 | 3591 | 3878 |
||
| best_non_blocking (GCC) | 248 | 838 | 1312 | 3398 | 3437 | 3574 |
| best_non_blocking (Clang) | 209 | 742 | 1226 | 3219 | 2945 | 2841 |
||
| textbook (GCC) | 225 | 661 | 806 | 1089 | 967 | 1125 |
| textbook (Clang) | 196 | 638 | 761 | 1015 | 662 | 763 |
||
| serial (GCC) | 73 | 389 | 347 | 528 | 501 | 584 |
| serial (Clang) | 61 | 250 | 326 | 461 | 424 | 500 |

- Clang uses LLVM for code generation.
- GCC-generated code is about 10-15% more performant that Clang/LLVM-generated code - exactly the opposite of the Rust case.