# Silo - a multi-threading performance test, comparing C++, Rust and Java

This project compares the multi-threading performance of the following runtimes:

- [C++/native](c++/README.md)
- [Rust/native](rust/README.md)
- [Java/JVM](java/README.md)

The test results and conclusions are at the bottom.  
***Note: the measurements are still in progress. The presented results are not final.***


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
`org::voidland::concurrent::queue::MostlyNonBlockingPortionQueue` is an implementation of a portion queue that wraps a non-blocking queue. It blocks only when necesssary, i.e. most of the time it has the throughput of the internal non-blocking queue.  
`org::voidland::concurrent::queue::TextbookPortionQueue` is a blocking implementation of a portion queue. It blocks on a single mutex and uses two conditions for signaling between the consumers and the producers. In the test, the mutex works under heavy contention.


### Notes on the solution of the actual task

Let N be the hardware parallelism (number of CPUs * number of hardware threads per CPU).

`MostlyNonBlockingPortionQueue` still occasionally blocks. N producers and N consumers do not utilize all of the available CPU time.

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
| AMD Ryzen 3700X | x86-64 | 3.6 GHz | 8 | 16 | 512 KB | 4 MB | 32 MB | 2019 | high mid-range desktop |
| AMD Ryzen 7735HS | x86-64 | 3.2 GHz | 8 | 16 | 512 KB | 4 MB | 16 MB | 2023 | high mid-range laptop |

All the CPUs are set to run constantly at their specified frequency for the duration of the test. Boosting the frequency is disabled for the sake of stable measurements.  

---

Intel Core i5-4210M

| runtime / test scenario | `syncless` | `best mostly non-blocking` | `textbook (blocking)` |
|---|---|---|---|
| **C++/native** | 981 | 832 | 661 |
| **Rust/native** | 838 | 723 | 377 |
| **Java/JVM** | ~~510~~ | 482 | 411 |

Intel Core i5-10210U

| runtime / test scenario | `syncless` | `best mostly non-blocking` | `textbook (blocking)` |
|---|---|---|---|
| **C++/native** | 1597 | 1310 |  821 |
| **Rust/native** | 1196 | 1067 | 621 |
| **Java/JVM** | ~~934~~ | 847 | 614 |

AMD Ryzen 3700X

| runtime / scenario implementation | `syncless` | `best mostly non-blocking` | `textbook (blocking)` |
|---|---|---|---|
| **C++/native** | 4689 | 3367 | 1094 |
| **Rust/native** | 4247 | 3008 | 700 |
| **Java/JVM** | ~~2161~~ | 2201 | 814 |

AMD Ryzen 7735HS

| runtime / scenario implementation | `syncless` | `best mostly non-blocking` | `textbook (blocking)` |
|---|---|---|---|
| **C++/native** | 4615 | 3574 | 1222 |
| **Rust/native** | 3936 | 2943 | 752 |
| **Java/JVM** | ~~1983~~ | 2119 | 882 |

`serial` (single-threaded)

| CPU / runtime | C++ | Rust | Java |
|---|---|---|---|
| Intel Core i5-4210M | 389 | 329 | 240 |
| Intel Core i5-10210U | 347 | 281 | 227 |
| AMD Ryzen 3700X | 520 | 449 | 358 |
| AMD Ryzen 7735HS | 584 | 476 | 390 |

---

General results:
- C++ is 15-25% more performant than Rust.
- C++ is about 2 times more performant than Java.
- Non-blocking queues perform better than blocking ones, even at low hardware parallelism.
- Non-blocking queues scale better than blocking ones with hardware parallelism.
- The more the CPUs, the less viable is a blocking algorithm, especially with Rust and Java.
- Single-core CPU improvements are still happenning, although not on a 1990-ies scale.
- The CPU frequency still matters.

Conclusions:
- If you want an algorithm to take advantage of new CPUs it better be parallelized.
- Use non-blocking synchronization to scale better with higher hardware parallelism.
- If an algorithm consists mostly of in-memory computations (i.e. no I/O) then it is definitely worth implementing it in C++ instead of Java and even Rust.

Language/runtime-specific results:
- [C++ results and conslusions](c++/README.md)
- [Rust results and conslusions](rust/README.md)
- [Java results and conslusions](java/README.md)
