include(${DIR}/tests.cmake)

list(
  APPEND
  LIBC_TESTS
  3rdparty/musl/libc-test/src/functional/crypt.c
  3rdparty/musl/libc-test/src/functional/pthread_cond.c
  3rdparty/musl/libc-test/src/functional/sem_init.c
  3rdparty/musl/libc-test/src/functional/strftime.c
  3rdparty/musl/libc-test/src/regression/dn_expand-empty.c
  3rdparty/musl/libc-test/src/regression/dn_expand-ptr-0.c
  3rdparty/musl/libc-test/src/regression/getpwnam_r-crash.c
  3rdparty/musl/libc-test/src/regression/getpwnam_r-errno.c
  3rdparty/musl/libc-test/src/regression/pthread_condattr_setclock.c
  3rdparty/musl/libc-test/src/regression/pthread_once-deadlock.c
  3rdparty/musl/libc-test/src/regression/pthread_rwlock-ebusy.c
  3rdparty/musl/libc-test/src/regression/syscall-sign-extend.c)
