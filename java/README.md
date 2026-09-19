# Silo Java performance test

## Build

You need Apache Maven and a Java Development Kit (version 8 or newer). The Debian package names are ***maven*** and ***default-jdk***, respectively.

1. Build and install [BlownQueue](https://github.com/gbrdead/blown_queue) for Java:

`$ git clone https://github.com/gbrdead/blown_queue`  
`$ cd blown_queue/java`  
`$ mvn clean install`  

2. Build `silo` itself:

`$ cd silo/java`  
`$ mvn clean package`  

The resulting JAR is `target/silo-*.jar`.

## Running the reference test

`silo.jar` must be run from the top directory of the silo project. There is only one command-line parameter, specifying the implementation type:

| Implementation | Queue type | Non-blocking | Stable measurements |
|---|---|---|---|
| `concurrent` | [ConcurrentLinkedQueue](https://docs.oracle.com/en/java/javase/25/docs/api/java.base/java/util/concurrent/ConcurrentLinkedQueue.html) | mostly (wrapped by `blown_queue`) | yes* |
| `textbook` | a simple blocking bounded queue using only Java SE (ArrayDeque, ReentrantLock and Condition) | no | yes* |
| `blocking` | [ArrayBlockingQueue](https://docs.oracle.com/en/java/javase/25/docs/api/java.base/java/util/concurrent/ArrayBlockingQueue.html) | no | yes* |
| `syncless` | queueless, with no synchronization overhead | yes | no |
| `serial` | single-threaded | N/A | yes* |

\* by Java's relatively low standards

## Test results

### Test results per implementation

| Implementation / CPU (hardware parallelism) | Allwinner A64 (4) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|---|
| `concurrent` | 96 | 511 | 840 | 2036 | 2214|
| `textbook` | 111 | 352 | 474 | 923 | 996|
| `blocking` | 113 | 346 | 517 | 1200 | 1195|
| `syncless` | ~~148~~ | ~~535~~ | ~~917~~ | ~~2118~~ | ~~2608~~|
| `serial` | 38 | 258 | 226 | 336 | 400|

- `concurrent` is the winner among the queues (it is the only non-blocking queue anyway).
- `blocking` and `textbook` are very similar in their implementations and virtually identical when it comes to synchronization, yet the former is noticeably better than the latter at higher parallelism levels. The cause is unknown.
- The Java test measurements are much less stable than the ones of C++ and Rust. There are two likely causes: the garbage collector and the JIT compiler. Both are known to affect performance in a non-deterministic manner. 
- In addition to or as a consequence of the above, the thread scheduler is very unfair (in a random manner). In the `syncless` implementation, the most privileged thread finishes its job at less than 90% of the total job done. Thus this implementation is very far from perfect and its measurements are especially unstable. 

### Test results per JVM version

| Implementation (JVM) / CPU (hardware parallelism) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|
| concurrent (25) | 484 | 800 | 2036 | 2214 |
| concurrent (21) | 451 | 840 | 1863 | 1502 |
| concurrent (17) | 456 | 822 | 1917 | 2082 |
| concurrent (11) | 455 | 710 | 1417 | 1834 |
| concurrent (8) | 511 | 803 | 1937 | 2207 |
||
| textbook (25) | 352 | 465 | 923 | 996 |
| textbook (21) | 336 | 473 | 854 | 822 |
| textbook (17) | 342 | 474 | 810 | 952 |
| textbook (11) | 300 | 355 | 872 | 895 |
| textbook (8) | 351 | 439 | 841 | 924 |
||
| blocking (25) | 346 | 472 | 1200 | 1145 |
| blocking (21) | 336 | 517 | 1175 | 1195 |
| blocking (17) | 337 | 503 | 1173 | 1067 |
| blocking (11) | 305 | 358 | 873 | 928 |
| blocking (8) | 340 | 399 | 998 | 979 |
||
| syncless (25) | ~~520~~ | ~~855~~ | ~~1816~~ | ~~2032~~ |
| syncless (21) | ~~480~~ | ~~873~~ | ~~1697~~ | ~~1667~~ |
| syncless (17) | ~~486~~ | ~~917~~ | ~~1778~~ | ~~1927~~ |
| syncless (11) | ~~478~~ | ~~737~~ | ~~1812~~ | ~~1861~~ |
| syncless (8) | ~~535~~ | ~~836~~ | ~~2118~~ | ~~2608~~ |
||
| serial (25) | 250 | 226 | 319 | 377 |
| serial (21) | 236 | 223 | 327 | 389 |
| serial (17) | 237 | 217 | 315 | 374 |
| serial (11) | 228 | 187 | 307 | 373 |
| serial (8) | 258 | 210 | 336 | 400 |

- There aren't any clearly noticeable accross-the-board performance improvements in the JVM between version 8 and version 25.
