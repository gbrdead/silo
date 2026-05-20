# Silo single-threaded performance test

Single-threaded performance is still important because of:
- algorithms that cannot be parallelized.
- legacy single-threaded software.

### Description of the performance test

The serial implementation uses a single thread. The same work is performed but in a serial manner. No synchronization is needed.

Unlike the other (multi-threaded) implementations, this one uses just a small amount of memory and is thus very unlikely to take advantage of the greater amount of cache memory in the newer CPUs.

| CPU name | Frequency | L1 cache | L2 cache | L3 cache | Year |
|---|---|---|---|---|---|
| Allwinner A64 | 1.15 GHz | 256 KB | 512 KB | 0 | 2015 |
| Intel Core i5-4210M | 2.6 GHz | 128 KB | 512 KB | 3 MB | 2014 |
| Intel Core i5-10210U | 2.4 GHz | 256 KB | 1 MB | 6 MB | 2019 |
| AMD Ryzen 7 3700X | 3.6 GHz | 512 KB | 4 MB | 32 MB | 2019 |
| AMD Ryzen 7 6800U | 2.7 GHz | 512 KB | 4 MB | 16 MB | 2022 |
| AMD Ryzen 7 7735HS | 3.2 GHz | 512 KB | 4 MB | 16 MB | 2023 |

| CPU / Language | C++ | Rust | Java |
|---|---|---|---|
| **Allwinner A64** | 73 | 59 | 38 |
| **Intel Core i5-4210M** | 389 | 301 | 250 |
| **Intel Core i5-10210U** | 347 | 297 | 228 |
| **AMD Ryzen 3700X** | 589 | 510 | 411 |
| **AMD Ryzen 6800U** | 886 | 709 | 524 |
| **AMD Ryzen 7735HS** | 847 | 682 | 541 |

- Single-core CPU improvements are still happenning (compare AMD Ryzen 7 3700X with the newer Ryzens).
- The CPU frequency still matters a bit (compare the two Intel Core i5's).
