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

| Implementation / CPU (hardware parallelism) | Allwinner A64 (4) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 3700X (16) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|---|---|
| `concurrent` | 96 | 482 | 869 | 2127 | 2268 | 2252|
| `textbook` | 111 | 411 | 580 | 853 | 931 | 991|
| `blocking` | 113 | 380 | 636 | 920 | 1212 | 1206|
| `syncless` | ~~148~~ | ~~534~~ | ~~933~~ | ~~1999~~ | ~~2683~~ | ~~2424~~|
| `serial` | 38 | 250 | 228 | 358 | 336 | 393|

- `concurrent` is the winner among the queues (it is the only non-blocking queue anyway).
- `blocking` and `textbook` are very similar in their implementations and virtually identical when it comes to synchronization, yet the former is noticeably better than the latter at higher parallelism levels. The cause is unknown.
- The Java test measurements are much less stable than the ones of C++ and Rust. There are two likely causes: the garbage collector and the JIT compiler. Both are known to affect performance in a non-deterministic manner. 
- In addition to or as a consequence of the above, the thread scheduler is very unfair (in a random manner). In the `syncless` implementation, the most privileged thread finishes its job at less than 90% of the total job done. Thus this implementation is very far from perfect and its measurements are especially unstable. 

### Test results per JVM version

| Implementation (JVM) / CPU (hardware parallelism) | AMD Ryzen 6800U (16) | AMD Ryzen 7735HS (16) |
|---|---|---|
| concurrent (25) | 2216 | 2252 |
| concurrent (21) | 2170 | 2077 |
| concurrent (17) | 2268 | 2205 |
| concurrent (11) | 1521 | 2151 |
| concurrent (8) | 2155 | 2050 |
||
| textbook (25) | 925 | 991 |
| textbook (21) | 914 | 889 |
| textbook (17) | 931 | 978 |
| textbook (11) | 777 | 911 |
| textbook (8) | 903 | 951 |
||
| blocking (25) | 1124 | 1205 |
| blocking (21) | 1212 | 1206 |
| blocking (17) | 1089 | 1168 |
| blocking (11) | 810 | 813 |
| blocking (8) | 875 | 904 |
||
| syncless (25) | ~~2012~~ | ~~2114~~ |
| syncless (21) | ~~1991~~ | ~~1961~~ |
| syncless (17) | ~~2079~~ | ~~2028~~ |
| syncless (11) | ~~2046~~ | ~~2339~~ |
| syncless (8) | ~~2683~~ | ~~2424~~ |
||
| serial (25) | 320 | 380 |
| serial (21) | 333 | 391 |
| serial (17) | 314 | 373 |
| serial (11) | 307 | 366 |
| serial (8) | 336 | 393 |

- There aren't any clearly noticeable accross-the-board performance improvements in the JVM between version 8 and version 25.
- `blocking` (`ArrayBlockingQueue`) looks like improved between JVM 11 and 17. But there are no differences in the source and no other implementation shows a clear improvement at this threshold. So this is very likely an artefact of the measurements' instability.
