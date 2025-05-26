#!/bin/bash
set -e -o pipefail

CPU_DEFINITION=$(cat "cpu.def" | grep "^${HOST}[[:blank:]]")
read HOST_ CPU_MODEL CPU_COUNT CPU_FREQ NO_TURBO BOOST JAVA_VERSIONS_ <<< "${CPU_DEFINITION}"

if ! lscpu | grep "Model name:" | grep --quiet ${CPU_MODEL}
then
	echo "Unexpected CPU model."
	exit 1
fi
	
if [ $(cat /proc/cpuinfo | grep '^processor\([[:space:]]\|:\)' | wc -l) -ne ${CPU_COUNT} ]
then
	echo "Unexpected CPU count."
	exit 1
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
