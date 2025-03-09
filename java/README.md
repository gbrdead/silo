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
| `concurrent` |  | 841 | 1621 |
| `textbook` |  | 666 | 886 |
| `blocking` |  | 694 | 1233 |
| `syncless` |  | ~~937~~ | ~~2092~~ |
| `serial` |  | 261 | 542 |

---  

| Implementation / JVM  version | 8 | 11 | 17 | 21 |
|---|---|---|---|---|
| `concurrent` | 1902 | 1766 | 1673 | 1621 |
| `textbook` | 866 | 909 | 797 | 886 |
| `blocking` | 912 | 788 | 1044 | 1233 |
| `syncless` | 2129 | 2216 | 1796 | 2092 |
| `serial` | 544 | 510 | 519 | 542 |

General results:
- `concurrent` is the winner among the queues.
- There are no noticeable performance improvements in the JVM between version 8 and version 21.

Some remarks: 
- The Java measurements are in general less stable than with C++ or Rust, probably because of the non-deterministic nature of the JIT compiler and the garbage collector. Thus, the standard for deeming an implementation stable for Java is lower.
- The thread scheduler is very unfair. Thus the `syncless` implementation is very far from perfect. The most privileged thread finishes its job at less than 90% of the total job done. As a result, the CPUs are not fully utilized for a big amount of time and the average speed is negatively affected.
- The Java mutex performs very badly under high contention, similar to the standard mutex in Rust and unlike C++.
