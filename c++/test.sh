#!/bin/bash
set -e -o pipefail

cd "$(dirname "${0}")"

TMP_FILE=$(mktemp)
for MIN_MEASUREMENT_COUNT in $(seq 1 ${1})
do
    for COMPILER in gcc clang
    do
        for TEST in syncless atomic concurrent ramalhete vyukov serial textbook lockfree michael_scott nikolaev_bounded kirsch_1fifo kirsch_bounded_1fifo onetbb onetbb_bounded
        do
            TEST_RUN="${HOST}-${COMPILER}-${TEST}"
            LOG_FILE="measurements/${TEST_RUN}.log"
            if [ -f "${LOG_FILE}" ] && [ $(wc -l < "${LOG_FILE}") -ge ${MIN_MEASUREMENT_COUNT} ]
            then
                continue
            fi
                
            echo -n "C++ ${TEST_RUN} ${MIN_MEASUREMENT_COUNT}: "
                
            cd ..
            "./c++/build.$(uname --machine).${COMPILER}/silo" ${TEST} 2>&1 | tee "${TMP_FILE}"
            cd c++
                
            CURRENT_TIME=$(date -Iminutes)
            echo -n "${CURRENT_TIME} " >> "${LOG_FILE}"
            cat "${TMP_FILE}" | tail -n 1 >> "${LOG_FILE}"
        done
    done
done
rm "${TMP_FILE}"
