#!/bin/bash
set -e -o pipefail

HOST=$(basename "${0}" _test.sh)

git pull
cargo build --release --target-dir "target.$(uname --machine)"

"$(dirname ${0})/../${HOST}_cpu.sh"

TMP_FILE=$(mktemp)
for MIN_MEASUREMENT_COUNT in $(seq 1 1000000)
do
	for TEST in syncless concurrent async_mpmc serial textbook sync_mpmc textbook_pl
	do
		LOG_FILE="$(dirname ${0})/measurements/${HOST}-${TEST}.log"
		if [ -f "${LOG_FILE}" ] && [ $(wc -l < "${LOG_FILE}") -ge ${MIN_MEASUREMENT_COUNT} ]
		then
			continue
		fi
		
		echo -n "${TEST}: "
		
		cd ..
		"./rust/target.$(uname --machine)/release/silo" ${TEST} 2>&1 | tee "${TMP_FILE}"
		cd rust
		
		CURRENT_TIME=$(date -Iminutes)
		echo -n "${CURRENT_TIME} " >> "${LOG_FILE}"
		cat "${TMP_FILE}" | tail -n 1 >> "${LOG_FILE}"
	done
done
