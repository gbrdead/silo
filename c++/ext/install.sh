#!/bin/sh

set -e

PREFIX=${PREFIX:-/usr/local}
mkdir -p "${PREFIX}/include"

echo Installing atomic_queue...
(cd atomic_queue && git pull && cp -f -r include/atomic_queue "${PREFIX}/include")
echo Done installing atomic_queue.
echo

echo Installing moodycamel::ConcurrentQueue...
(cd concurrentqueue && git pull && cmake -DCMAKE_INSTALL_PREFIX:PATH="${PREFIX}" . && make install)
echo Done installing moodycamel::ConcurrentQueue.
echo

echo Installing xenium...
(cd xenium && git pull && cp -f -r xenium "${PREFIX}/include")
echo Done installing xenium.
echo
