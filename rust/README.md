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
| `async_mpmc` | [std::sync::mpmc::channel](https://doc.rust-lang.org/std/sync/mpmc/fn.channel.html) | mostly (wrapped by `blown_queue`) | yes |
| `concurrent` | [concurrent_queue](https://crates.io/crates/concurrent_queue) | mostly (wrapped by `blown_queue`) | yes |
| `textbook` | a simple blocking bounded queue using only the standard Rust library (VecDeque, Mutex and Condvar) | no | yes |
| `textbook_pl` | like `textbook` but with Mutex and Condvar from [parking_lot](https://crates.io/crates/parking_lot) | no | no |
| `sync_mpmc` | [std::sync::mpmc::sync_channel](https://doc.rust-lang.org/std/sync/mpmc/fn.sync_channel.html) | no | yes |
| `syncless` | queueless, with no synchronization overhead | yes | yes |
| `serial` | single-threaded | N/A | yes |

## Test results

### Test results per implementation

| Implementation / CPU (hardware parallelism) | Allwinner A64 (4) | Broadcom BCM2711 (4) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|---|---|
| `async_mpmc` | 206 | 501 | 722 | 1144 | 2647 | 2675 |
| `concurrent` | 208 | 510 | 657 | 1192 | 2609 | 2773 |
| `textbook` | 195 | 180 | 357 | 453 | 421 | 535 |
| `textbook_pl` | 201 | 485 | 780 | 1221 | 1855 | 1067 |
| `sync_mpmc` | 207 | 493 | 752 | 1144 | 2339 | 2261 |
| `syncless` | 238 | 661 | 829 | 1316 | 3290 | 3623 |
| `serial` | 59 | 170 | 317 | 288 | 383 | 460 |

- `async_mpmc` is the winner among the queues, with `concurrent` a close second.
- The thread scheduler is very fair. Thus the syncless implementation is close to perfect. The most privileged thread finishes its job at more than 99% of the total job done.
- Do not use Rust if you expect high contention on mutexes and conditions. Rust has its own implementation of synchronization primitives that perform very badly under high contention. The underlying cause is that Condvar does not implement wait morphing, i.e. notifying a condition leads to a ["hurry up and wait"](https://en.cppreference.com/w/cpp/thread/condition_variable/notify_one) situation. Up until version 1.61 (inclusive) Rust used the pthreads library - the latter implements wait morphing and performs much better under high contention (similar to C++). But the pthreads-backed implementation of Condvar seems to be gone for good.
- The parking_lot crate behaves much worse than the standard synchronization primitives under high contention. The test execution starts well but rather sooner than later the mutex' throughput suddenly drops to such a low level that the processing speed of `textbook_pl` goes below the one of `serial`. Eventually, it may recover, only to degrade again. The likelyhood of falling in this performance hole seems to go up with the number of CPUs and threads (i.e. the contention itself). The underlying cause is unknown.

### Test results per compiler backend

| Implementation (backend) / CPU (hardware parallelism) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|
| syncless (default) | 829 | 1316 | 3290 | 3623 |
| syncless (GCC) | 563 | 961 | 2428 | 2843 |
||
| best_non_blocking (default) | 722 | 1192 | 2647 | 2773 |
| best_non_blocking (GCC) | 497 | 839 | 1995 | 1964 |
||
| textbook (default) | 357 | 453 | 421 | 535 |
| textbook (GCC) | 287 | 365 | 373 | 410 |
||
| serial (default) | 317 | 288 | 383 | 460 |
| serial (GCC) | 248 | 213 | 303 | 354 |

- The default backend is LLVM - the same as used by Clang.
- The GCC backend is experimental and not officially supported.
- LLVM-generated code is noticeably more performant than GCC-generated code - exactly the opposite of the C++ case.
