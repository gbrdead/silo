# Silo Rust multi-threading performance test

## Description of the performance test

`unsafe` is not used.

## Build

You need either Cargo or Rustup. The Debian package names are ***cargo*** and ***rustup***.  
Build with the following command:

`$ cargo build --release`

The resulting executable is `target/release/silo`.

## Running the reference test

`silo` must be run from the top directory of the silo project. There is only one command-line parameter, specifying the implementation type:

| Implementation | Queue type | Non-blocking | Stable measurements |
|---|---|---|---|
| `concurrent` | [concurrent_queue](https://crates.io/crates/concurrent_queue) | mostly | yes |
| `async_mpmc` | [std::sync::mpsc::channel](https://doc.rust-lang.org/std/sync/mpsc/fn.channel.html) | mostly | yes |
| `textbook` | a simple blocking bounded queue using only the standard Rust library (VecDeque, Mutex and Condvar) | no | no |
| `textbook_pl` | like `textbook` but with Mutex and Condvar from [parking_lot](https://crates.io/crates/parking_lot) | no | no |
| `sync_mpmc` | [std::sync::mpsc::sync_channel](https://doc.rust-lang.org/std/sync/mpsc/fn.sync_channel.html) | no | yes |
| `syncless` | queueless, with no synchronization overhead | yes | yes |
| `serial` | single-threaded | N/A | yes |

## Test results

| Implementation / CPU (hardware parallelism) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 7735HS (16) |
|---|---|---|---|
| `concurrent` | 639 | 1118 | 2812 |
| `async_mpmc` | 614 | 1147 | 2939 |
| `textbook` | ~~334~~ | ~~639~~ | ~~725~~ |
| `textbook_pl` | ~~596~~ | ~~1109~~ | ~~352~~ |
| `sync_mpmc` | 637 | 1109 | 2564 |
| `syncless` | 719 | 1278 | 3762 |
| `serial` | 338 | 399 | 664 |

General results:
- `concurrent` and `async_mpmc` have almost the same performance and can be considered as co-winners among the queues.
- Do not use Rust if you expect high contention for mutexes.

Some remarks: 
- The thread scheduler is very fair. Thus the syncless implementation is close to perfect. The most privileged thread finishes its job at more than 99% ot the total job done.
- Rust has its own implementation of synchronization primitives (specifically, mutex and condition) that perform very badly under high contention. The underlying cause is that Condvar does not perform wait morphing, i.e. notifying a condition leads to a ["hurry up and wait"](https://en.cppreference.com/w/cpp/thread/condition_variable/notify_one) situation. Up until version 1.61 (inclusive) Rust used the pthreads library - the latter implements wait morphing and performs well under high contention (similar to C++). But the pthreads-backed implementation seems to be gone for good.
- The parking_lot crate behaves much worse under high contention. The test execution starts well but rather sooner than later the mutex' throughput suddenly drops to such a low level that the processing speed of `textbook_pl` goes below the one of `serial`. Eventually, it may recover, only to degrade again. The likelyhood of falling in this performance hole seems to go up with the number of CPUs and threads (i.e. the contention itself). The underlying cause is unknown.
