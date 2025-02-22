# Silo Rust multi-threading performace test

## Description of the performance test

`unsafe` is not used.

## Build

You only need Cargo - it will bring the Rust compiler as a dependency. The Debian package name is ***cargo***.  
Build with the following command:

`$ cargo build --release`

The resulting executable is `target/release/silo`.

## Running the reference test

`silo` must be run from the top directory of the silo project. There is only one command-line parameter, specifying the implementation type:

| Implementation | Queue type | Non-blocking | Suffers from scheduler unfairness |
|---|---|---|---|
| syncless | queueless, with no synchronization overhead | yes | yes |
| concurrent | [concurrent_queue](https://docs.rs/concurrent-queue/latest/concurrent_queue/) | mostly | no |
| textbook | a simple blocking bounded queue using only the standard Rust library (VecDeque, Mutex and Condvar) | no | no |

## Test results

| Implementation / CPU (hardware parallelism) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 7735HS (16) |
|---|---|---|---|
| syncless | 718 | 1424 | 3967 |
| concurrent | 675 | 1272 | 2998 |
| textbook |  | 654 | 743 |
| serial | 331 | 376 | 700 |

General results:
- Rust is noticeably slower than C++.

Some remarks: 
- The thread scheduler is very fair. Thus the syncless implementation is close to perfect. The most privileged thread finishes its job at more than 99% ot the total job done. As a result, the CPUs are not fully utilized for just a small amount of time and the average speed is not significantly affected.
- For higher hardware parallelism, the blocking implementation cannot utilize the CPUs at 100%. That shows high lock contention. No such problem is observed in the C++ blocking implementations.

## TODO
- Make both a blocking and an asynchronous implementation based on [std::sync::mpmc](https://doc.rust-lang.org/std/sync/mpmc/index.html), as soon as it gets released.
- Investigate the lock contention.