#!/bin/bash
set -e -o pipefail

LANGUAGES="c++ rust java"

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

for MIN_MEASUREMENT_COUNT in $(seq 1 1000000)
do
	for LANGUAGE in ${LANGUAGES}
	do
		"./${LANGUAGE}/test.sh" ${MIN_MEASUREMENT_COUNT}
	done
done

