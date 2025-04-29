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
| `concurrent` | [ConcurrentLinkedQueue](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/util/concurrent/ConcurrentLinkedQueue.html) | mostly (wrapped by `blown_queue`) | yes |
| `textbook` | a simple blocking bounded queue using only Java SE (ArrayDeque, ReentrantLock and Condition) | no | yes |
| `blocking` | [ArrayBlockingQueue](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/util/concurrent/ArrayBlockingQueue.html) | no | yes |
| `syncless` | queueless, with no synchronization overhead | yes | no |
| `serial` | single-threaded | N/A | yes |

## Test results

| Implementation / CPU (hardware parallelism) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 3700X (16) | AMD Ryzen 7735HS (16) |
|---|---|---|---|---|
| concurrent | 481 | 561 | 2201 | 1943 |
| textbook | ~~409~~ | ~~384~~ | ~~814~~ | ~~862~~ |
| blocking | 366 | 426 | 969 | 1249 |
| syncless | ~~510~~ | ~~630~~ | ~~2161~~ | ~~2113~~ |
| serial | 240 | 155 | 358 | 390 |

---  

AMD Ryzen 7735HS

| Implementation / JVM  version | 8 | 11 | 17 | 21 |
|---|---|---|---|---|
| concurrent | 2046 | 2051 | 2125 | 1943 |
| textbook | ~~922~~ | ~~897~~ | ~~1034~~ | ~~862~~ |
| blocking | 897 | 814 | 1159 | 1249 |
| syncless | ~~2186~~ | ~~2321~~ | ~~2073~~ | ~~2113~~ |
| serial | 395 | 363 | 372 | 390 |

General results:
- `concurrent` is the winner among the queues.
- There are no noticeable performance improvements in the JVM between version 8 and version 21.
- `blocking` (`ArrayBlockingQueue`) has been improved between JVM 11 and 17.

Some remarks: 
- The thread scheduler is very unfair (in a random manner). The most privileged thread finishes its job at less than 90% of the total job done. Thus the `syncless` implementation is very far from perfect and its measurements are very unstable. 
- `ReentrantLock` performs very badly under high contention, similar to the standard mutex in Rust and unlike C++.
