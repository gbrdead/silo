# Silo Java multi-threading performace test

## Build

You need Apache Maven and a Java Development Kit (version 8 or newer). The Debian package names are ***maven*** and ***default-jdk***, respectively.  
Build with the following command:

`$ mvn clean package`

The resulting JAR is `target/silo-*.jar`.

## Running the reference test

`silo.jar` must be run from the top directory of the silo project. There is only one command-line parameter, specifying the implementation type:

| Implementation | Queue type | Non-blocking | Suffers from scheduler unfairness |
|---|---|---|---|
| syncless | queueless, with no synchronization overhead | yes | yes |
| concurrent | [ConcurrentLinkedQueue](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/util/concurrent/ConcurrentLinkedQueue.html) | mostly | no |
| blocking | [ArrayBlockingQueue](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/util/concurrent/ArrayBlockingQueue.html) | no | no |
| textbook | a simple blocking bounded queue using only Java SE (ArrayDeque, ReentrantLock and Condition) | no | no |

## Test results

| Implementation / CPU (hardware parallelism) | Intel Core i5-4210M (4) | Intel Core i5-10210U (8) | AMD Ryzen 7735HS (16) |
|---|---|---|---|
| syncless |  | 821 | 1770 |
| concurrent |  | 659 | 1951 |
| blocking |  | 553 | 1127 |
| textbook |  | 572 | 1068 |

General results:
- `concurrent` is the winner among the queues and scales with hardware parallelism better than syncless.

Some remarks: 
- The thread scheduler is very unfair. Thus the `syncless` implementation is very far from perfect. The most privileged thread finishes its job at less than 90% of the total job done. As a result, the CPUs are not fully utilized for a big amount of time and the average speed is negatively affected.
- For higher hardware parallelism, the blocking implementations cannot utilize the CPUs at 100%. That shows high lock contention. No such problem is observed in the C++ blocking implementations.

## TODO
- Measure the performance of older versions of the JVM to determine whether it has improved in the recent past. The tests so far have been performed with Java 21.
- Investigate the lock contention.