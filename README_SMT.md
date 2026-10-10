# Silo SMT performance test

Simultaneous multi-threading (a.k.a. hyper-threading by Intel) is a technology that lets a single CPU core run two (or more) hardware threads. They are represented as two logical CPUs to the software. The two processes running on these two hardware threads are isolated from one another logically but they can run simultaneously only when they don't need the same part of the CPU. For example, they cannot both transfer data from/to memory at the same time.

| CPU / Implementation (SMT) | `syncless` (on) | `syncless` (off) | | `best mostly non-blocking` (on) | `best mostly non-blocking` (off) | | `textbook (blocking)` (on) | `textbook (blocking)` (off) | | `serial` (on) | `serial` (off) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| C++ (GCC) |
| **Intel Core i5-4210M** | 993 | 806 | | 853 | 692 | | 661 | 687 | | 413 | 394 |
| **Intel Core i5-10210U** | 1558 | 1360 | | 1300 | 1017 | | 587 | 974 | | 362 | 361 |
| **AMD Ryzen 6800U** | 4193 | 3460 | | 3368 | 2611 | | 830 | 1082 | | 521 | 519 |
| **AMD Ryzen 7735HS** | 4709 | 4023 | | 3439 | 2659 | | 1034 | 1422 | | 616 | 618 |
| Rust |
| **Intel Core i5-4210M** | 829 | 600 | | 722 | 549 | | 357 | 373 | | 317 | 295 |
| **Intel Core i5-10210U** | 1316 | 1126 | | 1192 | 920 | | 453 | 676 | | 288 | 286 |
| **AMD Ryzen 6800U** | 3290 | 2703 | | 2647 | 2206 | | 421 | 415 | | 383 | 388 |
| **AMD Ryzen 7735HS** | 3623 | 3162 | | 2773 | 2417 | | 535 | 696 | | 460 | 461 |
| Java |
| **Intel Core i5-4210M** | 520 | 475 | | 484 | 419 | | 352 | 364 | | 250 | 250 |
| **Intel Core i5-10210U** | 855 | 861 | | 800 | 745 | | 465 | 459 | | 226 | 227 |
| **AMD Ryzen 6800U** | 1816 | 1730 | | 2036 | 1704 | | 923 | 637 | | 319 | 319 |
| **AMD Ryzen 7735HS** | 1837 | 1883 | | 1965 | 1800 | | 1094 | 806 | | 376 | 376 |

- SMT gives an improvement of 15-35% for lock-free algorithms.
- SMT may lower the performance of blocking algorithms.
- SMT does not affect the performance of single-threaded algorithms.

Conclusion: Turning SMT on is usually worth it.
