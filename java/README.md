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

| Implementation / CPU (hardware parallelism) | Allwinner A64 (4) | Broadcom BCM2711 (4) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|---|---|
| `concurrent` | 96 | 447 | 511 | 840 | 2036 | 2155 |
| `textbook` | 111 | 321 | 352 | 474 | 923 | 1094 |
| `blocking` | 113 | 355 | 346 | 517 | 1200 | 1200 |
| `syncless` | 148 | 528 | 535 | 917 | 2118 | 2472 |
| `serial` | 38 | 137 | 258 | 226 | 336 | 400 |

- `concurrent` is the winner among the queues (it is the only non-blocking queue anyway).
- `blocking` and `textbook` are very similar in their implementations and virtually identical when it comes to synchronization, yet the former is noticeably better than the latter at higher parallelism levels. The cause is unknown.
- The Java test measurements are much less stable than the ones of C++ and Rust. There are two likely causes: the garbage collector and the JIT compiler. Both are known to affect performance in a non-deterministic manner. 
- In addition to or as a consequence of the above, the thread scheduler is very unfair (in a random manner). In the `syncless` implementation, the most privileged thread finishes its job at less than 90% of the total job done. Thus this implementation is very far from perfect and its measurements are especially unstable. 

### Test results per JVM version

| Implementation (JVM) / CPU (hardware parallelism) | Broadcom BCM2711 (4) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|---|
| concurrent (25) | 438 | 484 | 800 | 2036 | 1965 |
| concurrent (21) | 392 | 451 | 840 | 1863 | 1752 |
| concurrent (17) | 401 | 456 | 822 | 1917 | 2094 |
| concurrent (11) | 383 | 455 | 710 | 1748 | 1703 |
| concurrent (8) | 447 | 511 | 803 | 1937 | 2155 |
||
| textbook (25) | 321 | 364 | 465 | 923 | 1094 |
| textbook (21) | 302 | 353 | 473 | 854 | 908 |
| textbook (17) | 277 | 375 | 474 | 810 | 1024 |
| textbook (11) | 227 | 318 | 355 | 872 | 943 |
| textbook (8) | 268 | 366 | 439 | 841 | 1031 |
||
| blocking (25) | 328 | 371 | 472 | 1200 | 1200 |
| blocking (21) | 355 | 356 | 517 | 1175 | 1185 |
| blocking (17) | 321 | 349 | 503 | 1173 | 1162 |
| blocking (11) | 231 | 312 | 358 | 873 | 852 |
| blocking (8) | 268 | 370 | 399 | 998 | 1007 |
||
| syncless (25) | 515 | 520 | 861 | 1816 | 1883 |
| syncless (21) | 469 | 480 | 873 | 1697 | 1754 |
| syncless (17) | 464 | 486 | 917 | 1778 | 1897 |
| syncless (11) | 452 | 478 | 737 | 1812 | 1938 |
| syncless (8) | 528 | 535 | 836 | 2118 | 2472 |
||
| serial (25) | 134 | 250 | 227 | 319 | 376 |
| serial (21) | 121 | 238 | 223 | 327 | 385 |
| serial (17) | 122 | 237 | 217 | 317 | 371 |
| serial (11) | 118 | 229 | 188 | 309 | 369 |
| serial (8) | 137 | 261 | 211 | 336 | 400 |

- There is a sharp drop in performance between Java 8 and 11. The underlying cause is unknown but it could be different default (as used by the tests) garbage collector settings.
- After Java 11 the performance improves gradually.
