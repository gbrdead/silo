# Silo SMT performance test

Simultaneous multi-threading (a.k.a. hyper-threading by Intel) is a technology that lets a single CPU core run two (or more) hardware threads. They are represented as two logical CPUs to the software. The two processes running on these two hardware threads are isolated from one another logically but they can run simultaneously only when they don't need the same part of the CPU. For example, they cannot both transfer data from/to memory at the same time.

| CPU / Implementation (SMT) | `syncless` (on) | `syncless` (off) | | `best mostly non-blocking` (on) | `best mostly non-blocking` (off) | | `textbook (blocking)` (on) | `textbook (blocking)` (off) | | `serial` (on) | `serial` (off) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| C++ (GCC) |
| **Intel Core i5-4210M** | 981 | 720 | | 838 | 673 | | 661 | 658 | | 389 | 343 |
| **Intel Core i5-10210U** | 1600 | 1373 | | 1312 | 1020 | | 806 | 944 | | 347 | 299 |
| **AMD Ryzen 3700X** | 4743 | 3561 | | 3398 | 2390 | | 1089 | 1215 | | 528 | 589 |
| **AMD Ryzen 6800U** | 4206 | 3748 | | 3437 | 2765 | | 967 | 1280 | | 501 | 886 |
| **AMD Ryzen 7735HS** | 4615 | 3773 | | 3574 | 2680 | | 1125 | 1222 | | 584 | 847 |
| Rust |
| **Intel Core i5-4210M** | 838 | 574 | | 549 | 566 | | 377 | 337 | | 289 | 301 |
| **Intel Core i5-10210U** | 1145 | 1099 | | 1086 | 909 | | 615 | 734 | | 287 | 297 |
| **AMD Ryzen 3700X** | 3154 | N/A | | 2688 | 2113 | | 761 | N/A | | 510 | N/A |
| **AMD Ryzen 6800U** | 3167 | 3037 | | 2846 | 2430 | | 417 | 555 | | 709 | 668 |
| **AMD Ryzen 7735HS** | 3115 | 3215 | | 2233 | 2456 | | 750 | 896 | | 476 | 682 |
| Java |
| **Intel Core i5-4210M** | ~~534~~ | ~~494~~ | | 474 | 444 | | 356 | 401 | | 250 | 249 |
| **Intel Core i5-10210U** | ~~846~~ | ~~859~~ | | 811 | 765 | | 402 | 436 | | 223 | 224 |
| **AMD Ryzen 3700X** | ~~1999~~ | ~~1991~~ | | 2127 | 2014 | | 853 | 699 | | 358 | 411 |
| **AMD Ryzen 6800U** | ~~2012~~ | ~~2026~~ | | 2216 | 1904 | | 925 | 752 | | 320 | 490 |
| **AMD Ryzen 7735HS** | ~~2114~~ | ~~1979~~ | | 2252 | 1874 | | 991 | 876 | | 380 | 521 |

- SMT gives an improvement of 15-35% for lock-free algorithms.
- SMT may lower the performance of blocking algorithms.
- SMT lowers the performance of single-threaded algorithms.

Conclusion: Unless running a legacy single CPU operating system, turning SMT on is worth it.
