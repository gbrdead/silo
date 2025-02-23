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
- The standard library mutex does not perform well under high contention.

Some remarks: 
- The thread scheduler is very fair. Thus the syncless implementation is close to perfect. The most privileged thread finishes its job at more than 99% ot the total job done. As a result, the CPUs are not fully utilized for just a small amount of time and the average speed is not significantly affected.
- Rust has its own mutex implementation that performs very badly under high contention. Up until version 1.61 (inclusive) Rust used the pthread mutex - it performs much better but seems to be gone for good. The parking_lot mutex is almost as good as the pthread one.

## TODO
- Make both a blocking and an asynchronous implementation based on [std::sync::mpmc](https://doc.rust-lang.org/std/sync/mpmc/index.html), as soon as it gets released.
