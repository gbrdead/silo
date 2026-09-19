# Silo single-threaded performance test

Single-threaded performance is still important because of:
- algorithms that cannot be parallelized.
- legacy single-threaded software.

### Description of the performance test

The serial implementation uses a single thread (and thus a single CPU core). The same work is performed but in a serial manner. No synchronization is needed.

Unlike the other (multi-threaded) implementations, this one uses just a small amount of memory and is thus very unlikely to take advantage of the greater amount of cache memory in the newer CPUs.

| CPU / Language | C++ | Rust | Java |
|---|---|---|---|
| **Allwinner A64** | 73 | 59 | 38 |
| **Intel Core i5-4210M** | 413 | 317 | 261 |
| **Intel Core i5-10210U** | 362 | 288 | 227 |
| **AMD Ryzen 6800U** | 521 | 388 | 336 |
| **AMD Ryzen 7735HS** | 618 | 461 | 400 |

- Single-core CPU improvements are still happenning but very slowly.
- The CPU frequency still matters a bit.
- None of the above happens on a 1990-ies scale. Upgrading your CPU frequently is not worth it.
