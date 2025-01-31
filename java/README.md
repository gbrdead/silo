# Silo Java reference test

## Build

You need Apache Maven and a Java Development Kit (version 8 or newer). The Debian package names are ***maven*** and ***default-jdk***, respectively.  
Build with the following command:

`$ mvn clean package`

The resulting JAR is `target/silo-*.jar`.

## Running the reference test

`silo.jar` must be run from the top directory of the silo project. There is only one command-line parameter:

| Parameter value | Queue implementation | Non-blocking |
|---|---|---|
| concurrent | [ConcurrentLinkedQueue](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/util/concurrent/ConcurrentLinkedQueue.html) | mostly |
| blocking | [ArrayBlockingQueue](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/util/concurrent/ArrayBlockingQueue.html) | no |
| textbook | a simple blocking bounded queue using ArrayDeque, ReentrantLock and Condition | no |
| perfect | queueless, with no synchronization overhead | yes |
