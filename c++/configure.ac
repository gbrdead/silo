AC_INIT([silo-cpp], [0.0.1], [gbr@voidland.org])
AM_INIT_AUTOMAKE([foreign no-dist])
AC_CONFIG_HEADERS([config.h])


AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])

AX_PTHREAD
if test "x${ax_pthread_ok}" = "xyes"
then
    LIBS="${PTHREAD_LIBS} ${LIBS}"
    CFLAGS="${CFLAGS} ${PTHREAD_CFLAGS}"
    CXXFLAGS="${CXXFLAGS} ${PTHREAD_CFLAGS}"
else
    AC_MSG_ERROR([POSIX threads are required])
fi


AX_BOOST_BASE([1.56],
    [
        CPPFLAGS="${CPPFLAGS} ${BOOST_CPPFLAGS}"
        LDFLAGS="${LDFLAGS} ${BOOST_LDFLAGS}"
    ],
    [
        AC_MSG_ERROR([boost is required])
    ])

AX_BOOST_REGEX
if test "x$ax_cv_boost_regex" = "xyes"
then
    LIBS="${BOOST_REGEX_LIB} ${LIBS}"
else
    AC_MSG_ERROR([boost::regex is required])
fi 

AX_BOOST_CHRONO
if test "x$ax_cv_boost_chrono" = "xyes"
then
    LIBS="${BOOST_CHRONO_LIB} ${LIBS}"
else
    AC_MSG_ERROR([boost::chrono is required])
fi


AC_LANG_PUSH([C++])

AC_MSG_CHECKING([for oneAPI Threading Building Blocks (oneTBB) library])
LIBS="-ltbb ${LIBS}"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
        [[
            #include <oneapi/tbb/concurrent_queue.h>
        ]],
        [[
            oneapi::tbb::concurrent_queue<int> queue;
            oneapi::tbb::concurrent_bounded_queue<int> boundedQueue;
        ]])],
    [
        AC_MSG_RESULT([yes])
    ],
    [
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([oneTBB is required])
    ])

AC_LANG_POP([C++])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
