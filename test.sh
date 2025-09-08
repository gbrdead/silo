#!/bin/bash
set -e -o pipefail

LANGUAGES="java rust c++"

export HOST=$(basename "${0}" _test.sh)
cd "$(dirname "${0}")"

"./cpu.sh"

CPU_DEFINITION=$(cat "cpu.def" | grep "^${HOST}[[:blank:]]")
read HOST_ CPU_MODEL_ CPU_COUNT_ CPU_FREQ_ NO_TURBO_ BOOST_ JAVA_VERSIONS <<< "${CPU_DEFINITION}"
JAVA_VERSIONS=$(echo ${JAVA_VERSIONS} | sed 's/_/ /g')
export JAVA_VERSIONS

for LANGUAGE in ${LANGUAGES}
do
    mkdir -p "${LANGUAGE}/measurements"
done

git pull
for COMPILER in gcc clang
do
    (cd c++ && make -C "build.$(uname --machine).${COMPILER}")
done
(cd rust && cargo build --release --target-dir "target.$(uname --machine)")
(cd java && mvn package)

for MIN_MEASUREMENT_COUNT in $(seq 1 1000000)
do
    for LANGUAGE in ${LANGUAGES}
    do
        "./${LANGUAGE}/test.sh" ${MIN_MEASUREMENT_COUNT}
    done
done
