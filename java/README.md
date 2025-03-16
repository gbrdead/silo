# Silo Java multi-threading performance test

## Build

You need Apache Maven and a Java Development Kit (version 8 or newer). The Debian package names are ***maven*** and ***default-jdk***, respectively.  
Build with the following command:

`$ mvn clean package`

The resulting JAR is `target/silo-*.jar`.

## Running the reference test

`silo.jar` must be run from the top directory of the silo project. There is only one command-line parameter, specifying the implementation type:

| Implementation | Queue type | Non-blocking | Stable measurements |
|---|---|---|---|
| `concurrent` | [ConcurrentLinkedQueue](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/util/concurrent/ConcurrentLinkedQueue.html) | mostly | yes |
| `textbook` | a simple blocking bounded queue using only Java SE (ArrayDeque, ReentrantLock and Condition) | no | yes |
| `blocking` | [ArrayBlockingQueue](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/util/concurrent/ArrayBlockingQueue.html) | no | yes |
| `syncless` | queueless, with no synchronization overhead | yes | no |
| `serial` | single-threaded | N/A | yes |

## Test results

| Implementation / CPU (hardware parallelism) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 7735HS (16) |
|---|---|---|---|
| `concurrent` | 402 | 855 | 1671 |
| `textbook` | 352 | 655 | 863 |
| `blocking` | 352 | 689 | 1224 |
| `syncless` | ~~433~~ | ~~945~~ | ~~2067~~ |
| `serial` | 240 | 273 | 541 |

---  

Intel Core i5-10210U

| Implementation / JVM  version | 8 | 11 | 17 | 21 |
|---|---|---|---|---|
| `concurrent` | 2021 | 2012 | 1867 | 1671 |
| `textbook` | 960 | 942 | 862 | 863 |
| `blocking` | 994 | 977 | 1106 | 1224 |
| `syncless` | ~~2319~~ | ~~2267~~ | ~~1786~~ | ~~2067~~ |
| `serial` | 547 | 514 | 518 | 541 |

General results:
- `concurrent` is the winner among the queues.
- There are no noticeable performance improvements in the JVM between version 8 and version 21.

Some remarks: 
- The thread scheduler is very unfair (in a random manner). The most privileged thread finishes its job at less than 90% of the total job done. Thus the `syncless` implementation is very far from perfect and its measurements are very unstable. 
- `ReentrantLock` performs very badly under high contention, similar to the standard mutex in Rust and unlike C++.
