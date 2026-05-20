#!/bin/bash
set -e -o pipefail

cd "$(dirname "${0}")"

TMP_FILE=$(mktemp)
for MIN_MEASUREMENT_COUNT in $(seq 1 ${1})
do
    for JAVA_VER in ${JAVA_VERSIONS}
    do
        for TEST in syncless concurrent serial textbook blocking
        do
            TEST_RUN="${HOST}-${JAVA_VER}-${TEST}"
            LOG_FILE="measurements/${TEST_RUN}.log"
            if [ -f "${LOG_FILE}" ] && [ $(wc -l < "${LOG_FILE}") -ge ${MIN_MEASUREMENT_COUNT} ]
               then
                continue
            fi
                    
            echo -n "Java ${TEST_RUN} ${MIN_MEASUREMENT_COUNT}: "
                    
            cd ..
            /usr/lib/jvm/java-${JAVA_VER}-openjdk-*/bin/java -jar ./java/target/silo-*.jar ${TEST} 2>&1 | tee "${TMP_FILE}"
            cd java
            
            CURRENT_TIME=$(date -Iminutes)
            echo -n "${CURRENT_TIME} " >> "${LOG_FILE}"
            cat "${TMP_FILE}" | tail -n 1 >> "${LOG_FILE}"
        done
    done
done
rm "${TMP_FILE}"
