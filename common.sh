assertNotThrottled()
{
    if sudo which vcgencmd &> /dev/null
    then
        THROTTLED=$(sudo vcgencmd get_throttled)
        if [ "${THROTTLED}" != "throttled=0x0" ]
        then
            echo
            echo "The CPU has been throttled."
            exit 1
        fi
    fi
}
