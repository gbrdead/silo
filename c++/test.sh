#!/bin/bash
set -e -o pipefail

HOST=$(basename "${0}" _test.sh)

git pull
make -C "build.$(uname --machine)"

"$(dirname ${0})/../${HOST}_cpu.sh"

TMP_FILE=$(mktemp)
for MIN_MEASUREMENT_COUNT in $(seq 1 1000000)
do
	for TEST in syncless atomic concurrent ramalhete vyukov serial textbook lockfree michael_scott nikolaev_bounded kirsch_1fifo kirsch_bounded_1fifo onetbb onetbb_bounded
	do
                LOG_FILE="$(dirname ${0})/measurements/${HOST}-${TEST}.log"
                if [ -f "${LOG_FILE}" ] && [ $(wc -l < "${LOG_FILE}") -ge ${MIN_MEASUREMENT_COUNT} ]
                then
                        continue
                fi
                
                echo -n "${TEST}: "
                
                cd ..
                "./c++/build.$(uname --machine)/silo" ${TEST} 2>&1 | tee "${TMP_FILE}"
                cd c++
                
		CURRENT_TIME=$(date -Iminutes)
		echo -n "${CURRENT_TIME} " >> "${LOG_FILE}"
		cat "${TMP_FILE}" | tail -n 1 >> "${LOG_FILE}"
	done
done
