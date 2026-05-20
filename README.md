# Silo - a multi-threading performance test, comparing C++, Rust and Java

This project compares the multi-threading performance of the following runtimes:

- [C++/native](c++/README.md)
- [Rust/native](rust/README.md)
- [Java/JVM](java/README.md)

**The test program employs typical operations - integer arithmetic, heavy RAM I/O (optimized for cache coherence to the best of every compiler's ability) and for most of the tests - synchronization primitives.** Peripheral I/O is not employed because its latency would swamp the differences between the benchmarked languages and runtimes.

### Description of the performance test

The test program implements the [multiple producer - multiple consumer pattern](https://en.wikipedia.org/wiki/Producer%E2%80%93consumer_problem) for a real and useful workload. The task solved by the test is brute-forcing the key for an encrypted message.

The cipher used to encrypt the message is known as [turning grille](https://www.dcode.fr/turning-grille-cipher). Each portion is a grille (key). The producers have to generate all the possible grilles. Consuming a portion is applying the grille to the encrypted message and counting the natural language words that can be found in the decrypted message. Words are counted using a trie data structure - the execution time does not depend on the size of the word list. As a result, consuming a portion is just a few times slower than producing it, i.e. the CPU resources used by a consumer are comparable to those used by a producer.

### Portion queue requirements

The portion queue must:
- be thread-safe
- be bounded and blocking on hitting the bounds (empty or full)

The bounded and blocking queue must:
- block a producer if the size of the queue has reached its maximum size
- block a consumer if the queue is empty

There are two possible mechanisms for ensuring the thread-safety of a data structure:
- non-blocking
- blocking

Non-blocking queues have a much higher throughput than blocking ones.  
But the portion queue must be blocking.  
But the portion queue must be blocking only on hitting its bounds. Most of the time it does not have to block.

`org::voidland::concurrent::queue::MPMC_PortionQueue` is an interface for a portion queue that can be used in a multiple producer - multiple consumer scenario.  
`org::voidland::concurrent::queue::BlownQueue` is an implementation of a portion queue that wraps a non-blocking queue. It blocks only when necesssary, i.e. most of the time it has the throughput of the internal non-blocking queue.  
`org::voidland::concurrent::queue::TextbookPortionQueue` is a blocking implementation of a portion queue. It blocks on a single mutex and uses two conditions for signaling between the consumers and the producers. In the test, the mutex works under heavy contention.


### Notes on the solution of the actual task

Let N be the hardware parallelism (number of CPUs * number of hardware threads per CPU).

`BlownQueue` still occasionally blocks. N producers and N consumers do not utilize all of the available CPU time.

`org::voidland::concurrent::turning_grille::TurningGrilleCrackerProducerConsumer` implements the producer-consumer pattern using a fixed number of producers and an adaptive number of consumers. If some part of the CPUs idle occasionally then the number of the consumers will be increased. If the contention of the threads becomes too high, their number will be decreased. `TurningGrilleCrackerProducerConsumer` is used for both the blocking and the mostly non-blocking portion queues.

`org::voidland::concurrent::TurningGrilleCrackerSyncless` solves the same task without employing the producer-consumer pattern and without needing any synchronization. It divides the portions to N equal packages, each applied by a separate thread (acting as both a producer and a consumer). If the thread scheduler is perfectly fair then this solution will be perfect. If the thread scheduler is fairly unfair then the more privileged threads will be done earlier and leave some of the CPUs unused until the less privileged threads finish their jobs.  
Notice that the syncless implementation is synchronization-free only as far as software goes. The CPUs still synchronize their access to RAM and the higher level (common) caches. The hardware threads within a CPU are even more contentious with each other.

### Notes on the measurements

The measurements are performed on a mostly free of other processes Debian GNU/Linux installation. The scenario's running time is long enough (1-3 hours), so that temporary variations in the speed due to external factors are eliminated.  
The speed is reported in thousands of grilles (i.e. portions consumed) per second.  
Running a reference test with the environment variable `VERBOSE` set to `true` will print status info every 0.1% and will print likely candidates for the decrypted message. By default, verbosity is off in order not to influence the measurement.

#### Comparisons

- The `textbook` and `syncless` implementations in the different languages are 100% equivalent and can be used for inter-runtime comparisons.
- The best mostly non-blocking implementations can also be used for inter-runtime comparisons.
- The `serial` implementations can be used for single-threaded (i.e. single-core) comparisons.

### Unstable measurements

Some of the implementations for a given language/runtime do not yield stable measurements, i.e. the results vary too much. The possible reasons are:
- A synchronization primitive (mutex) abruptly lowering its throughput at arbitrary moments.
- Unfair scheduler for the `syncless` implementation - with such a scheduler the more privileged threads finish too early and then the CPUs are not fully utilized for a random amount of time until the less privileged threads finish their work.
- Non-deterministic characteristics of the runtime (e.g. the JIT compiler and the garbage collector of the JVM). In general, even when deemed stable, Java's measurements are noticeably less stable than C++' and Rust's ones.
- High contention on a single mutex. The `textbook` implementations' measurements are inherently unstable, with higher parallelism leading to higher variations. They are deemed sufficiently stable, though.

Implementations with very unstable measurements should not be trusted too much for comparisons. Such results will be shown ~~striken through~~.

### Test results

Test hardware

| CPU name | Architecture | Frequency | Cores | Hardware threads (N) | L1 cache | L2 cache | L3 cache | Year | Computer type |
|---|---|---|---|---|---|---|---|---|---|
| Allwinner A64 | aarch64 | 1.15 GHz | 4 | 4 | 256 KB | 512 KB | 0 | 2015 | single-board |
| Intel Core i5-4210M | x86-64 | 2.6 GHz | 2 | 4 | 128 KB | 512 KB | 3 MB | 2014 | mid-range laptop |
| Intel Core i5-10210U | x86-64 | 2.4 GHz | 4 | 8 | 256 KB | 1 MB | 6 MB | 2019 | low mid-range laptop |
| AMD Ryzen 7 3700X | x86-64 | 3.6 GHz | 8 | 16 | 512 KB | 4 MB | 32 MB | 2019 | high mid-range desktop |
| AMD Ryzen 7 6800U | x86-64 | 2.7 GHz | 8 | 16 | 512 KB | 4 MB | 16 MB | 2022 | mid-range laptop |
| AMD Ryzen 7 7735HS | x86-64 | 3.2 GHz | 8 | 16 | 512 KB | 4 MB | 16 MB | 2023 | high mid-range laptop |

All the CPUs are set to run constantly at their specified frequency for the duration of the test. Boosting the frequency is disabled for the sake of stable measurements.

#### Test results per CPU

| Language / Implementation | `syncless` | `best mostly non-blocking` | `textbook (blocking)` | `serial` |
|---|---|---|---|---|
| Allwinner A64 |
| **C++** | 290 | 248 | 225 | 73 |
| **Rust** | 238 | 208 | 195 | 59 |
| **Java** | ~~148~~ | 96 | 111 | 38 |
| Intel Core i5-4210M |
| **C++** | 981 | 838 | 661 | 389 |
| **Rust** | 838 | 566 | 377 | 301 |
| **Java** | ~~534~~ | 482 | 426 | 250 |
| Intel Core i5-10210U |
| **C++** | 1600 | 1312 | 944 | 347 |
| **Rust** | 1145 | 1086 | 734 | 297 |
| **Java** | ~~933~~ | 869 | 580 | 228 |
| AMD Ryzen 3700X |
| **C++** | 4743 | 3398 | 1215 | 589 |
| **Rust** | 3154 | 2688 | 761 | 510 |
| **Java** | ~~1999~~ | 2127 | 853 | 411 |
| AMD Ryzen 6800U |
| **C++** | 4206 | 3437 | 1488 | 886 |
| **Rust** | 3167 | 2846 | 555 | 709 |
| **Java** | ~~2683~~ | 2268 | 993 | 524 |
| AMD Ryzen 7735HS |
| **C++** | 4615 | 3574 | 1291 | 847 |
| **Rust** | 3215 | 2456 | 896 | 682 |
| **Java** | ~~2424~~ | 2252 | 1034 | 541 |

- C++ is 15-25% more performant than Rust.
- C++ is about 50% more performant than Java.
- Non-blocking queues perform better than blocking ones, even at low hardware parallelism.
- Non-blocking queues scale better than blocking ones with hardware parallelism.
- The more the CPUs, the less viable is a blocking algorithm (especially with Rust).

Conclusions:
- If you want an algorithm to take advantage of new CPUs it better be parallelized.
- Use non-blocking synchronization to scale better with higher hardware parallelism.
- If an algorithm consists mostly of in-memory computations (i.e. no I/O) then it is definitely worth implementing it in C++ instead of Java and even Rust.
- Rust does not live up to its hype, as its performance is noticeably lower than that of C++. Notice that the test implementations do not use Rust's unsafe features. While they may help with the performance a bit, their use would void Rust's primary advantage over C++ (memory safety).

Language/runtime-specific results:
- [C++ results and conclusions](c++/README.md)
- [Rust results and conclusions](rust/README.md)
- [Java results and conclusions](java/README.md)

CPU feature-specific results:
- [Single core results and conclusions](README_serial.md)
- [Simultaneous multi-threading results and conclusions](README_SMT.md)