# Silo Rust multi-threading performace test

## Description of the performance test

`unsafe` is not used.

## Build

You need either Cargo or Rustup. The Debian package names are ***cargo*** and ***rustup***.  
Build with the following command:

`$ cargo build --release`

The resulting executable is `target/release/silo`.

## Running the reference test

`silo` must be run from the top directory of the silo project. There is only one command-line parameter, specifying the implementation type:

| Implementation | Queue type | Non-blocking | Suffers from scheduler unfairness |
|---|---|---|---|
| `concurrent` | [concurrent_queue](https://crates.io/crates/concurrent_queue) | mostly | no |
| `textbook` | a simple blocking bounded queue using only the standard Rust library (VecDeque, Mutex and Condvar) | no | no |
| `textbook_pl` | like `textbook` but with Mutex and Condvar from [parking_lot](https://crates.io/crates/parking_lot) | no | no |
| `syncless` | queueless, with no synchronization overhead | yes | yes |
| `serial` | single-threaded | N/A | N/A |

## Test results

| Implementation / CPU (hardware parallelism) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 7735HS (16) |
|---|---|---|---|
| `concurrent` |  |  |  |
| `textbook` |  |  |  |
| `textbook_pl` |  |  |  |
| `syncless` |  |  |  |
| `serial` |  |  |  |

General results:
- Rust is noticeably slower than C++.
- Do not use Rust if you expect to have high contention for a specific mutex.

Some remarks: 
- The thread scheduler is very fair. Thus the syncless implementation is close to perfect. The most privileged thread finishes its job at more than 99% ot the total job done. As a result, the CPUs are not fully utilized for just a small amount of time and the average speed is not significantly affected.
- Rust has its own implementation of synchronization primitives (specifically, mutex and condition) that perform very badly under high contention. The underlying cause is that notifying a condition leads to a ["hurry up and wait"](https://en.cppreference.com/w/cpp/thread/condition_variable/notify_one) situation. Up until version 1.61 (inclusive) Rust used the pthreads library - it avoids "hurry up and wait" and performs very well under high contention but this implementation seems to be gone for good.
- The parking_lot crate at first glance performs better under high contention but it has the tendency to occasionally throw its hands up. After such an event, the processing speed of `textbook_pl` goes below the one of `serial`. The underlying cause is unknown.

## TODO
- Make both a blocking and an asynchronous implementation based on [std::sync::mpmc](https://doc.rust-lang.org/std/sync/mpmc/index.html), as soon as it gets released.
