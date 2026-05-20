#!/bin/bash
set -e -o pipefail

LANGUAGES="c++ rust java"

cd "$(dirname "${0}")"


############################## CPU start ##############################

while read HOST CPU_MODEL CPU_FREQ NO_TURBO BOOST JAVA_VERSIONS RUST_BACKENDS
do
    if lscpu | grep "Model name:" | grep --quiet ${CPU_MODEL}
    then
        break
    fi
    HOST=""
done <<< $(cat cpu.def | sed '/#.*$/d' | sed '/^[[:space:]]*$/d')
if [ -z "${HOST}" ]
then
    echo "Unknown CPU model."
    exit 1
fi
export HOST

sudo sh -c "echo on > /sys/devices/system/cpu/smt/control"
if [ $(lscpu | grep "Thread(s) per core:" | sed 's/^[^:]*: *//') -gt 1 ]
then
    SMP=true
else
    SMP=false
fi

sudo cpupower --cpu all frequency-set --governor performance --min "${CPU_FREQ}" --max "${CPU_FREQ}"

if [ ${NO_TURBO} != "N/A" ]
then
    sudo sh -c "echo ${NO_TURBO} > /sys/devices/system/cpu/intel_pstate/no_turbo"
fi

if [ ${BOOST} != "N/A" ]
then
    sudo sh -c "echo ${BOOST} > /sys/devices/system/cpu/cpufreq/boost"
fi

sudo sh -c "echo 0 > /proc/sys/kernel/randomize_va_space"


############################## CPU end ##############################


JAVA_VERSIONS=$(echo ${JAVA_VERSIONS} | sed 's/_/ /g')
RUST_BACKENDS=$(echo ${RUST_BACKENDS} | sed 's/_/ /g')
export JAVA_VERSIONS
export RUST_BACKENDS

for LANGUAGE in ${LANGUAGES}
do
    mkdir -p "${LANGUAGE}/measurements"
done

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

run()
{
    for LANGUAGE in ${LANGUAGES}
    do
        "./${LANGUAGE}/test.sh" ${MIN_MEASUREMENT_COUNT}
    done
}

for MIN_MEASUREMENT_COUNT in $(seq 1 1000000)
do
    if ${SMP}
    then
        sudo sh -c "echo on > /sys/devices/system/cpu/smt/control"
        HOST="${HOST}_smt" run
        sudo sh -c "echo off > /sys/devices/system/cpu/smt/control"
        HOST="${HOST}" run
    else
        run
    fi
done
