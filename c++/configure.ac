AC_INIT([silo-cpp], [0.0.1], [gbr@voidland.org])
AM_INIT_AUTOMAKE([foreign no-dist subdir-objects])


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
if test "x${ax_cv_boost_regex}" = "xyes"
then
    LIBS="${BOOST_REGEX_LIB} ${LIBS}"
else
    AC_MSG_ERROR([boost::regex is required])
fi 

AX_BOOST_CHRONO
if test "x${ax_cv_boost_chrono}" = "xyes"
then
    LIBS="${BOOST_CHRONO_LIB} ${LIBS}"
else
    AC_MSG_ERROR([boost::chrono is required])
fi

AX_BOOST_THREAD
if test "x${ax_cv_boost_thread}" = "xyes"
then
    LIBS="${BOOST_THREAD_LIB} ${LIBS}"
else
    AC_MSG_ERROR([boost::thread is required])
fi


AC_LANG_PUSH([C++])

AC_MSG_CHECKING([for BlownQueue library])
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
        [[
            #include <voidland/blown_queue/BlownQueue.hpp>
        ]],
        [[
            voidland::concurrent::queue::createAtomicBlownQueue<int>(1);
            voidland::concurrent::queue::createConcurrentBlownQueue<int>(1);
            voidland::concurrent::queue::createLockfreeBlownQueue<int>(1);
            voidland::concurrent::queue::createMichaelScottBlownQueue<int>(1);
            voidland::concurrent::queue::createRamalheteBlownQueue<int>(1);
            voidland::concurrent::queue::createVyukovBlownQueue<int>(1);
        ]])],
    [
        AC_MSG_RESULT([yes])
    ],
    [
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([m4_joinall([m4_newline([])],
            [BlownQueue with all wrapped queue implementations enabled is required.],
            [Get it from https://github.com/gbrdead/blown_queue .])])
    ])


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
        AC_MSG_ERROR([m4_joinall([m4_newline([])],
            [oneTBB is required.],
            [Get it from https://uxlfoundation.github.io/oneTBB/ or install it using your package manager (the Debian package name is libtbb-dev).])])

    ])

AC_LANG_POP([C++])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
