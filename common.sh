assertNotThrottled()
{
    if sudo vcgencmd get_throttled &> /dev/null
    then
        THROTTLED=$(sudo vcgencmd get_throttled)
        if [ "${THROTTLED}" != "throttled=0x0" ]
        then
            echo
            echo "The CPU has been throttled; restart the host and retry."
            exit 1
        fi
    fi
}

resetTimeSpentStats()
{
    for POLICY in /sys/devices/system/cpu/cpufreq/policy*
    do
        if [ ! -d ${POLICY}/stats ]
        then
            continue
        fi
        sudo sh -c "echo 1 > ${POLICY}/stats/reset"
    done
}

assertTimeSpentStats()
{
    for POLICY in /sys/devices/system/cpu/cpufreq/policy*
    do
        if [ ! -d ${POLICY}/stats ]
        then
            continue
        fi
        TIME_SPENT_IN_WRONG_STATE=$( \
            cat ${POLICY}/stats/time_in_state | grep -v "^${CPU_FREQ} " | cut -f 2 -d " " | \
            awk '{ sum += $1 } END { print sum + 0 }')
        if [ ${TIME_SPENT_IN_WRONG_STATE} -ne 0 ]
        then
            echo
            echo "The CPU has been throttled during the test; retrying."
            return 1
        fi
    done
    return 0
}
