#!/bin/bash
set -e -o pipefail

LANGUAGES="java rust c++"

export HOST=$(basename "${0}" _test.sh)
cd "$(dirname "${0}")"

"./cpu.sh"

CPU_DEFINITION=$(cat "cpu.def" | grep "^${HOST}[[:blank:]]")
read HOST_ CPU_MODEL_ CPU_COUNT_ CPU_FREQ_ NO_TURBO_ BOOST_ JAVA_VERSIONS RUST_BACKENDS <<< "${CPU_DEFINITION}"
JAVA_VERSIONS=$(echo ${JAVA_VERSIONS} | sed 's/_/ /g')
RUST_BACKENDS=$(echo ${RUST_BACKENDS} | sed 's/_/ /g')
export JAVA_VERSIONS
export RUST_BACKENDS

for LANGUAGE in ${LANGUAGES}
do
    mkdir -p "${LANGUAGE}/measurements"
done

git pull
for COMPILER in gcc clang
do
    (cd c++ && make -C "build.$(uname --machine).${COMPILER}")
done
(cd rust && cargo build --profile release_default --target-dir "target.$(uname --machine)")
if [ -d ~/rustc_codegen_gcc ]
then
	(cd rust && CHANNEL=release ~/rustc_codegen_gcc/y.sh cargo build --profile release_gcc --target-dir "target.$(uname --machine)")
fi
(cd java && mvn package)

for MIN_MEASUREMENT_COUNT in $(seq 1 1000000)
do
    for LANGUAGE in ${LANGUAGES}
    do
        "./${LANGUAGE}/test.sh" ${MIN_MEASUREMENT_COUNT}
    done
done
