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

| Implementation / CPU (hardware parallelism) | Allwinner A64 (4) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|---|
| `atomic` | 245 | 853 | 1326 | 3368 | 3439|
| `ramalhete` | 238 | 818 | 1298 | 3270 | 3398|
| `concurrent` | 245 | 837 | 1327 | 3256 | 3223|
| `vyukov` | 248 | 846 | 1276 | 3161 | 3257|
| `michael_scott` | 226 | 808 | 1241 | 2955 | 3026|
| `lockfree` | 231 | 824 | 1285 | 2830 | 2878|
| `nikolaev_bounded` | 228 | 800 | 1280 | 3234 | 3406|
| `kirsch_1fifo` | 190 | 744 | 1149 | 1466 | 1594|
| `kirsch_bounded_1fifo` | 214 | 797 | 1276 | 2227 | 2242|
| `onetbb` | ~~138~~ | ~~323~~ | ~~525~~ | ~~851~~ | ~~1047~~|
| `onetbb_bounded` | ~~127~~ | ~~314~~ | ~~465~~ | ~~829~~ | ~~916~~|
| `textbook` | 225 | 661 | 730 | 830 | 1034|
| `syncless` | 290 | 993 | 1578 | 4193 | 4709|
| `serial` | 73 | 413 | 362 | 521 | 616|

- `atomic` is the winner among the queues.
- The thread scheduler is very fair. Thus the `syncless` implementation is close to perfect. The most privileged thread finishes its job at more than 99% ot the total job done.
- At first glance, `onetbb_bounded` should work like `blown_queue` - non-blocking most of the time, blocking only on hitting its bounds. But its performance is too low for this to be true.
- The average speeds of the oneTBB queues are inexplicably low. Also, their performance is erratic - the speed varies wildly.
- `nikolaev_queue` has a stopper bug. Frequently it fails to pop an element even when the queue is not empty and this leads to a deadlock. That is why it is not measured.
- `sync_bounded` has a stopper bug. Sometimes it fails to wake up a producer despite that the queue becomes not full and this leads to a deadlock. That is why it is not measured.
- `nikolaev_bounded` has a non-stopper bug. Sometimes it moves the data out of the portion even if the push fails because of a full queue. The portion is always copied as a workaround for this bug.

### Test results per compiler

| Implementation (compiler) / CPU (hardware parallelism) | Allwinner A64 (4) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|---|
| syncless (GCC) | 290 | 993 | 1558 | 4193 | 4709 |
| syncless (Clang) | 238 | 867 | 1578 | 3442 | 3640 |
||
| best_non_blocking (GCC) | 248 | 853 | 1300 | 3368 | 3439 |
| best_non_blocking (Clang) | 209 | 757 | 1327 | 2697 | 2593 |
||
| textbook (GCC) | 225 | 661 | 587 | 830 | 1034 |
| textbook (Clang) | 196 | 627 | 730 | 652 | 686 |
||
| serial (GCC) | 73 | 413 | 362 | 521 | 616 |
| serial (Clang) | 61 | 259 | 348 | 418 | 489 |

- Clang uses LLVM for code generation.
- GCC-generated code is noticeably more performant that Clang/LLVM-generated code - exactly the opposite of the Rust case.