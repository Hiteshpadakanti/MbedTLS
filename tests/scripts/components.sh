# components.sh
#
# Copyright The Mbed TLS Contributors
# SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later

# This file contains the test components that are executed by all.sh

# The functions below are named as follows:
#  * component_XXX: independent components. They can be run in any order.
#      * component_check_XXX: quick tests that aren't worth parallelizing.
#      * component_build_XXX: build things but don't run them.
#      * component_test_XXX: build and test.
#      * component_release_XXX: tests that the CI should skip during PR testing.
#  * support_XXX: if support_XXX exists and returns false then
#    component_XXX is not run by default.

# Each component must start by invoking `msg` with a short informative message.
#
# Warning: due to the way bash detects errors, the failure of a command
# inside 'if' or '!' is not detected. Use the 'not' function instead of '!'.
#
# Each component is executed in a separate shell process. The component
# fails if any command in it returns a non-zero status.
#
# The framework in all.sh performs some cleanup tasks after each component.
# This means that components can assume that the working directory is in a
# cleaned-up state, and don't need to perform the cleanup themselves.
# * Run `make clean`.
# * Restore `include/mbedtls/mbedtls_config.h` from a backup made before running
#   the component.
# * Check out `Makefile`, `library/Makefile`, `programs/Makefile`,
#   `tests/Makefile` and `programs/fuzz/Makefile` from git.
#   This cleans up after an in-tree use of CMake.
#
# The tests are roughly in order from fastest to slowest. This doesn't
# have to be exact, but in general you should add slower tests towards
# the end and fast checks near the beginning.


################################################################
#### Build and test many configurations and targets
################################################################


skip_suites_without_constant_flow () {
    # Skip the test suites that don't have any constant-flow annotations.
    # This will need to be adjusted if we ever start declaring things as
    # secret from macros or functions inside tests/include or tests/src.
    SKIP_TEST_SUITES=$(
        git -C tests/suites grep -L TEST_CF_ 'test_suite_*.function' |
            sed 's/test_suite_//; s/\.function$//' |
            tr '\n' ,)
    export SKIP_TEST_SUITES
}

skip_all_except_given_suite () {
    # Skip all but the given test suite
    SKIP_TEST_SUITES=$(
        ls -1 tests/suites/test_suite_*.function |
        grep -v $1.function |
         sed 's/tests.suites.test_suite_//; s/\.function$//' |
        tr '\n' ,)
    export SKIP_TEST_SUITES
}

component_test_memsan_constant_flow () {
    # This tests both (1) accesses to undefined memory, and (2) branches or
    # memory access depending on secret values. To distinguish between those:
    # - unset MBEDTLS_TEST_CONSTANT_FLOW_MEMSAN - does the failure persist?
    # - or alternatively, change the build type to MemSanDbg, which enables
    # origin tracking and nicer stack traces (which are useful for debugging
    # anyway), and check if the origin was TEST_CF_SECRET() or something else.
    msg "build: cmake MSan (clang), full config minus MBEDTLS_USE_PSA_CRYPTO with constant flow testing"
    scripts/config.py full
    scripts/config.py set MBEDTLS_TEST_CONSTANT_FLOW_MEMSAN
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py unset MBEDTLS_AESNI_C # memsan doesn't grok asm
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=MemSan .
    make

    msg "test: main suites (full minus MBEDTLS_USE_PSA_CRYPTO, Msan + constant flow)"
    make test
}

component_test_memsan_constant_flow_psa () {
    # This tests both (1) accesses to undefined memory, and (2) branches or
    # memory access depending on secret values. To distinguish between those:
    # - unset MBEDTLS_TEST_CONSTANT_FLOW_MEMSAN - does the failure persist?
    # - or alternatively, change the build type to MemSanDbg, which enables
    # origin tracking and nicer stack traces (which are useful for debugging
    # anyway), and check if the origin was TEST_CF_SECRET() or something else.
    msg "build: cmake MSan (clang), full config with constant flow testing"
    scripts/config.py full
    scripts/config.py set MBEDTLS_TEST_CONSTANT_FLOW_MEMSAN
    scripts/config.py unset MBEDTLS_AESNI_C # memsan doesn't grok asm
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=MemSan .
    make

    msg "test: main suites (Msan + constant flow)"
    make test
}

component_release_test_valgrind_constant_flow () {
    # This tests both (1) everything that valgrind's memcheck usually checks
    # (heap buffer overflows, use of uninitialized memory, use-after-free,
    # etc.) and (2) branches or memory access depending on secret values,
    # which will be reported as uninitialized memory. To distinguish between
    # secret and actually uninitialized:
    # - unset MBEDTLS_TEST_CONSTANT_FLOW_VALGRIND - does the failure persist?
    # - or alternatively, build with debug info and manually run the offending
    # test suite with valgrind --track-origins=yes, then check if the origin
    # was TEST_CF_SECRET() or something else.
    msg "build: cmake release GCC, full config minus MBEDTLS_USE_PSA_CRYPTO with constant flow testing"
    scripts/config.py full
    scripts/config.py set MBEDTLS_TEST_CONSTANT_FLOW_VALGRIND
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    skip_suites_without_constant_flow
    cmake -D CMAKE_BUILD_TYPE:String=Release .
    make

    # this only shows a summary of the results (how many of each type)
    # details are left in Testing/<date>/DynamicAnalysis.xml
    msg "test: some suites (full minus MBEDTLS_USE_PSA_CRYPTO, valgrind + constant flow)"
    make memcheck

    # Test asm path in constant time module - by default, it will test the plain C
    # path under Valgrind or Memsan. Running only the constant_time tests is fast (<1s)
    msg "test: valgrind asm constant_time"
    scripts/config.py --force set MBEDTLS_TEST_CONSTANT_FLOW_ASM
    skip_all_except_given_suite test_suite_constant_time
    cmake -D CMAKE_BUILD_TYPE:String=Release .
    make clean
    make
    make memcheck
}

component_release_test_valgrind_constant_flow_psa () {
    # This tests both (1) everything that valgrind's memcheck usually checks
    # (heap buffer overflows, use of uninitialized memory, use-after-free,
    # etc.) and (2) branches or memory access depending on secret values,
    # which will be reported as uninitialized memory. To distinguish between
    # secret and actually uninitialized:
    # - unset MBEDTLS_TEST_CONSTANT_FLOW_VALGRIND - does the failure persist?
    # - or alternatively, build with debug info and manually run the offending
    # test suite with valgrind --track-origins=yes, then check if the origin
    # was TEST_CF_SECRET() or something else.
    msg "build: cmake release GCC, full config with constant flow testing"
    scripts/config.py full
    scripts/config.py set MBEDTLS_TEST_CONSTANT_FLOW_VALGRIND
    skip_suites_without_constant_flow
    cmake -D CMAKE_BUILD_TYPE:String=Release .
    make

    # this only shows a summary of the results (how many of each type)
    # details are left in Testing/<date>/DynamicAnalysis.xml
    msg "test: some suites (valgrind + constant flow)"
    make memcheck
}

component_test_tsan () {
    msg "build: TSan (clang)"
    scripts/config.py full
    scripts/config.py set MBEDTLS_THREADING_C
    scripts/config.py set MBEDTLS_THREADING_PTHREAD
    # Self-tests do not currently use multiple threads.
    scripts/config.py unset MBEDTLS_SELF_TEST

    # The deprecated MBEDTLS_PSA_CRYPTO_SE_C interface is not thread safe.
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_SE_C

    CC=clang cmake -D CMAKE_BUILD_TYPE:String=TSan .
    make

    msg "test: main suites (TSan)"
    make test
}

component_test_no_psa_crypto_full_cmake_asan() {
    # full minus MBEDTLS_PSA_CRYPTO_C: run the same set of tests as basic-build-test.sh
    msg "build: cmake, full config minus PSA crypto, ASan"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_C
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_CLIENT
    scripts/config.py unset MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py unset MBEDTLS_SSL_PROTO_TLS1_3
    scripts/config.py unset MBEDTLS_PSA_ITS_FILE_C
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_SE_C
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_STORAGE_C
    scripts/config.py unset MBEDTLS_LMS_C
    scripts/config.py unset MBEDTLS_LMS_PRIVATE
    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: main suites (full minus PSA crypto)"
    make test

    # Note: ssl-opt.sh has some test cases that depend on
    # MBEDTLS_ECP_RESTARTABLE && !MBEDTLS_USE_PSA_CRYPTO
    # This is the only component where those tests are not skipped.
    msg "test: ssl-opt.sh (full minus PSA crypto)"
    tests/ssl-opt.sh

    msg "test: compat.sh default (full minus PSA crypto)"
    tests/compat.sh

    msg "test: compat.sh NULL (full minus PSA crypto)"
    tests/compat.sh -f 'NULL'

    msg "test: compat.sh ARIA + ChachaPoly (full minus PSA crypto)"
    env OPENSSL="$OPENSSL_NEXT" tests/compat.sh -e '^$' -f 'ARIA\|CHACHA'
}

component_build_no_std_function () {
    # catch compile bugs in _uninit functions
    msg "build: full config with NO_STD_FUNCTION, make, gcc" # ~ 30s
    scripts/config.py full
    scripts/config.py set MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
    scripts/config.py unset MBEDTLS_ENTROPY_NV_SEED
    scripts/config.py unset MBEDTLS_PLATFORM_NV_SEED_ALT
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Check .
    make
}

component_build_no_ssl_srv () {
    msg "build: full config except SSL server, make, gcc" # ~ 30s
    scripts/config.py full
    scripts/config.py unset MBEDTLS_SSL_SRV_C
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -O1'
}

component_build_no_ssl_cli () {
    msg "build: full config except SSL client, make, gcc" # ~ 30s
    scripts/config.py full
    scripts/config.py unset MBEDTLS_SSL_CLI_C
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -O1'
}

component_build_no_sockets () {
    # Note, C99 compliance can also be tested with the sockets support disabled,
    # as that requires a POSIX platform (which isn't the same as C99).
    msg "build: full config except net_sockets.c, make, gcc -std=c99 -pedantic" # ~ 30s
    scripts/config.py full
    scripts/config.py unset MBEDTLS_NET_C # getaddrinfo() undeclared, etc.
    scripts/config.py set MBEDTLS_NO_PLATFORM_ENTROPY # uses syscall() on GNU/Linux
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -O1 -std=c99 -pedantic' lib
}

component_test_memory_buffer_allocator_backtrace () {
    msg "build: default config with memory buffer allocator and backtrace enabled"
    scripts/config.py set MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_PLATFORM_MEMORY
    scripts/config.py set MBEDTLS_MEMORY_BACKTRACE
    scripts/config.py set MBEDTLS_MEMORY_DEBUG
    cmake -DCMAKE_BUILD_TYPE:String=Release .
    make

    msg "test: MBEDTLS_MEMORY_BUFFER_ALLOC_C and MBEDTLS_MEMORY_BACKTRACE"
    make test
}

component_test_memory_buffer_allocator () {
    msg "build: default config with memory buffer allocator"
    scripts/config.py set MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_PLATFORM_MEMORY
    cmake -DCMAKE_BUILD_TYPE:String=Release .
    make

    msg "test: MBEDTLS_MEMORY_BUFFER_ALLOC_C"
    make test

    msg "test: ssl-opt.sh, MBEDTLS_MEMORY_BUFFER_ALLOC_C"
    # MBEDTLS_MEMORY_BUFFER_ALLOC is slow. Skip tests that tend to time out.
    tests/ssl-opt.sh -e '^DTLS proxy'
}

component_test_no_max_fragment_length () {
    # Run max fragment length tests with MFL disabled
    msg "build: default config except MFL extension (ASan build)" # ~ 30s
    scripts/config.py unset MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: ssl-opt.sh, MFL-related tests"
    tests/ssl-opt.sh -f "Max fragment length"
}

component_test_asan_remove_peer_certificate () {
    msg "build: default config with MBEDTLS_SSL_KEEP_PEER_CERTIFICATE disabled (ASan build)"
    scripts/config.py unset MBEDTLS_SSL_KEEP_PEER_CERTIFICATE
    scripts/config.py unset MBEDTLS_SSL_PROTO_TLS1_3
    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: !MBEDTLS_SSL_KEEP_PEER_CERTIFICATE"
    make test

    msg "test: ssl-opt.sh, !MBEDTLS_SSL_KEEP_PEER_CERTIFICATE"
    tests/ssl-opt.sh

    msg "test: compat.sh, !MBEDTLS_SSL_KEEP_PEER_CERTIFICATE"
    tests/compat.sh

    msg "test: context-info.sh, !MBEDTLS_SSL_KEEP_PEER_CERTIFICATE"
    tests/context-info.sh
}

component_test_no_max_fragment_length_small_ssl_out_content_len () {
    msg "build: no MFL extension, small SSL_OUT_CONTENT_LEN (ASan build)"
    scripts/config.py unset MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
    scripts/config.py set MBEDTLS_SSL_IN_CONTENT_LEN 16384
    scripts/config.py set MBEDTLS_SSL_OUT_CONTENT_LEN 4096
    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MFL tests (disabled MFL extension case) & large packet tests"
    tests/ssl-opt.sh -f "Max fragment length\|Large buffer"

    msg "test: context-info.sh (disabled MFL extension case)"
    tests/context-info.sh
}

component_test_variable_ssl_in_out_buffer_len () {
    msg "build: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH enabled (ASan build)"
    scripts/config.py set MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH
    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH enabled"
    make test

    msg "test: ssl-opt.sh, MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH enabled"
    tests/ssl-opt.sh

    msg "test: compat.sh, MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH enabled"
    tests/compat.sh
}

component_test_dtls_cid_legacy () {
    msg "build: MBEDTLS_SSL_DTLS_CONNECTION_ID (legacy) enabled (ASan build)"
    scripts/config.py set MBEDTLS_SSL_DTLS_CONNECTION_ID_COMPAT 1

    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MBEDTLS_SSL_DTLS_CONNECTION_ID (legacy)"
    make test

    msg "test: ssl-opt.sh, MBEDTLS_SSL_DTLS_CONNECTION_ID (legacy) enabled"
    tests/ssl-opt.sh

    msg "test: compat.sh, MBEDTLS_SSL_DTLS_CONNECTION_ID (legacy) enabled"
    tests/compat.sh
}

component_test_ssl_alloc_buffer_and_mfl () {
    msg "build: default config with memory buffer allocator and MFL extension"
    scripts/config.py set MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_PLATFORM_MEMORY
    scripts/config.py set MBEDTLS_MEMORY_DEBUG
    scripts/config.py set MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
    scripts/config.py set MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH
    cmake -DCMAKE_BUILD_TYPE:String=Release .
    make

    msg "test: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH, MBEDTLS_MEMORY_BUFFER_ALLOC_C, MBEDTLS_MEMORY_DEBUG and MBEDTLS_SSL_MAX_FRAGMENT_LENGTH"
    make test

    msg "test: MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH, MBEDTLS_MEMORY_BUFFER_ALLOC_C, MBEDTLS_MEMORY_DEBUG and MBEDTLS_SSL_MAX_FRAGMENT_LENGTH"
    tests/ssl-opt.sh -f "Handshake memory usage"
}

component_test_when_no_ciphersuites_have_mac () {
    msg "build: when no ciphersuites have MAC"
    scripts/config.py unset MBEDTLS_CIPHER_NULL_CIPHER
    scripts/config.py unset MBEDTLS_CIPHER_MODE_CBC
    scripts/config.py unset MBEDTLS_CMAC_C
    make

    msg "test: !MBEDTLS_SSL_SOME_MODES_USE_MAC"
    make test

    msg "test ssl-opt.sh: !MBEDTLS_SSL_SOME_MODES_USE_MAC"
    tests/ssl-opt.sh -f 'Default\|EtM' -e 'without EtM'
}

component_test_no_date_time () {
    msg "build: default config without MBEDTLS_HAVE_TIME_DATE"
    scripts/config.py unset MBEDTLS_HAVE_TIME_DATE
    cmake -D CMAKE_BUILD_TYPE:String=Check .
    make

    msg "test: !MBEDTLS_HAVE_TIME_DATE - main suites"
    make test
}

component_test_platform_calloc_macro () {
    msg "build: MBEDTLS_PLATFORM_{CALLOC/FREE}_MACRO enabled (ASan build)"
    scripts/config.py set MBEDTLS_PLATFORM_MEMORY
    scripts/config.py set MBEDTLS_PLATFORM_CALLOC_MACRO calloc
    scripts/config.py set MBEDTLS_PLATFORM_FREE_MACRO   free
    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MBEDTLS_PLATFORM_{CALLOC/FREE}_MACRO enabled (ASan build)"
    make test
}

component_test_malloc_0_null () {
    msg "build: malloc(0) returns NULL (ASan+UBSan build)"
    scripts/config.py full
    make CC=$ASAN_CC CFLAGS="'-DMBEDTLS_USER_CONFIG_FILE=\"$PWD/tests/configs/user-config-malloc-0-null.h\"' $ASAN_CFLAGS" LDFLAGS="$ASAN_CFLAGS"

    msg "test: malloc(0) returns NULL (ASan+UBSan build)"
    make test

    msg "selftest: malloc(0) returns NULL (ASan+UBSan build)"
    # Just the calloc selftest. "make test" ran the others as part of the
    # test suites.
    programs/test/selftest calloc

    msg "test ssl-opt.sh: malloc(0) returns NULL (ASan+UBSan build)"
    # Run a subset of the tests. The choice is a balance between coverage
    # and time (including time indirectly wasted due to flaky tests).
    # The current choice is to skip tests whose description includes
    # "proxy", which is an approximation of skipping tests that use the
    # UDP proxy, which tend to be slower and flakier.
    tests/ssl-opt.sh -e 'proxy'
}

support_test_aesni() {
    # Check that gcc targets x86_64 (we can build AESNI), and check for
    # AESNI support on the host (we can run AESNI).
    #
    # The name of this function is possibly slightly misleading, but needs to align
    # with the name of the corresponding test, component_test_aesni.
    #
    # In principle 32-bit x86 can support AESNI, but our implementation does not
    # support 32-bit x86, so we check for x86-64.
    # We can only grep /proc/cpuinfo on Linux, so this also checks for Linux
    (gcc -v 2>&1 | grep Target | grep -q x86_64) &&
        [[ "$HOSTTYPE" == "x86_64" && "$OSTYPE" == "linux-gnu" ]] &&
        (lscpu | grep -qw aes)
}

component_test_aesni () { # ~ 60s
    # This tests the two AESNI implementations (intrinsics and assembly), and also the plain C
    # fallback. It also tests the logic that is used to select which implementation(s) to build.
    #
    # This test does not require the host to have support for AESNI (if it doesn't, the run-time
    # AESNI detection will fallback to the plain C implementation, so the tests will instead
    # exercise the plain C impl).

    msg "build: default config with different AES implementations"
    scripts/config.py set MBEDTLS_AESNI_C
    scripts/config.py unset MBEDTLS_AES_USE_HARDWARE_ONLY
    scripts/config.py set MBEDTLS_HAVE_ASM

    # test the intrinsics implementation
    msg "AES tests, test intrinsics"
    make clean
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -mpclmul -msse2 -maes'
    # check that we built intrinsics - this should be used by default when supported by the compiler
    ./programs/test/selftest aes | grep "AESNI code" | grep -q "intrinsics"

    # test the asm implementation
    msg "AES tests, test assembly"
    make clean
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -mno-pclmul -mno-sse2 -mno-aes'
    # check that we built assembly - this should be built if the compiler does not support intrinsics
    ./programs/test/selftest aes | grep "AESNI code" | grep -q "assembly"

    # test the plain C implementation
    scripts/config.py unset MBEDTLS_AESNI_C
    scripts/config.py unset MBEDTLS_AES_USE_HARDWARE_ONLY
    msg "AES tests, plain C"
    make clean
    make CC=gcc CFLAGS='-O2 -Werror'
    # check that there is no AESNI code present
    ./programs/test/selftest aes | not grep -q "AESNI code"
    not grep -q "AES note: using AESNI" ./programs/test/selftest
    grep -q "AES note: built-in implementation." ./programs/test/selftest

    # test the intrinsics implementation
    scripts/config.py set MBEDTLS_AESNI_C
    scripts/config.py set MBEDTLS_AES_USE_HARDWARE_ONLY
    msg "AES tests, test AESNI only"
    make clean
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -mpclmul -msse2 -maes'
    ./programs/test/selftest aes | grep -q "AES note: using AESNI"
    ./programs/test/selftest aes | not grep -q "AES note: built-in implementation."
    grep -q "AES note: using AESNI" ./programs/test/selftest
    not grep -q "AES note: built-in implementation." ./programs/test/selftest
}

component_test_sha3_variations() {
    msg "sha3 loop unroll variations"

    # define minimal config sufficient to test SHA3
    cat > include/mbedtls/mbedtls_config.h << END
        #define MBEDTLS_SELF_TEST
        #define MBEDTLS_SHA3_C
END

    msg "all loops unrolled"
    make clean
    make -C tests test_suite_shax CFLAGS="-DMBEDTLS_SHA3_THETA_UNROLL=1 -DMBEDTLS_SHA3_PI_UNROLL=1 -DMBEDTLS_SHA3_CHI_UNROLL=1 -DMBEDTLS_SHA3_RHO_UNROLL=1"
    ./tests/test_suite_shax

    msg "all loops rolled up"
    make clean
    make -C tests test_suite_shax CFLAGS="-DMBEDTLS_SHA3_THETA_UNROLL=0 -DMBEDTLS_SHA3_PI_UNROLL=0 -DMBEDTLS_SHA3_CHI_UNROLL=0 -DMBEDTLS_SHA3_RHO_UNROLL=0"
    ./tests/test_suite_shax
}

support_test_aesni_m32() {
    support_test_m32_no_asm && (lscpu | grep -qw aes)
}

component_test_aesni_m32 () { # ~ 60s
    # This tests are duplicated from component_test_aesni for i386 target
    #
    # AESNI intrinsic code supports i386 and assembly code does not support it.

    msg "build: default config with different AES implementations"
    scripts/config.py set MBEDTLS_AESNI_C
    scripts/config.py set MBEDTLS_PADLOCK_C
    scripts/config.py unset MBEDTLS_AES_USE_HARDWARE_ONLY
    scripts/config.py set MBEDTLS_HAVE_ASM

    # test the intrinsics implementation with gcc
    msg "AES tests, test intrinsics (gcc)"
    make clean
    make CC=gcc CFLAGS='-m32 -Werror -Wall -Wextra' LDFLAGS='-m32'
    # check that we built intrinsics - this should be used by default when supported by the compiler
    ./programs/test/selftest aes | grep "AESNI code" | grep -q "intrinsics"
    grep -q "AES note: using AESNI" ./programs/test/selftest
    grep -q "AES note: built-in implementation." ./programs/test/selftest
    grep -q "AES note: using VIA Padlock" ./programs/test/selftest
    grep -q mbedtls_aesni_has_support ./programs/test/selftest

    scripts/config.py set MBEDTLS_AESNI_C
    scripts/config.py unset MBEDTLS_PADLOCK_C
    scripts/config.py set MBEDTLS_AES_USE_HARDWARE_ONLY
    msg "AES tests, test AESNI only"
    make clean
    make CC=gcc CFLAGS='-m32 -Werror -Wall -Wextra -mpclmul -msse2 -maes' LDFLAGS='-m32'
    ./programs/test/selftest aes | grep -q "AES note: using AESNI"
    ./programs/test/selftest aes | not grep -q "AES note: built-in implementation."
    grep -q "AES note: using AESNI" ./programs/test/selftest
    not grep -q "AES note: built-in implementation." ./programs/test/selftest
    not grep -q "AES note: using VIA Padlock" ./programs/test/selftest
    not grep -q mbedtls_aesni_has_support ./programs/test/selftest
}

support_test_aesni_m32_clang() {
    # clang >= 4 is required to build with target attributes
    support_test_aesni_m32 && [[ $(clang_version) -ge 4 ]]
}

component_test_aesni_m32_clang() {

    scripts/config.py set MBEDTLS_AESNI_C
    scripts/config.py set MBEDTLS_PADLOCK_C
    scripts/config.py unset MBEDTLS_AES_USE_HARDWARE_ONLY
    scripts/config.py set MBEDTLS_HAVE_ASM

    # test the intrinsics implementation with clang
    msg "AES tests, test intrinsics (clang)"
    make clean
    make CC=clang CFLAGS='-m32 -Werror -Wall -Wextra' LDFLAGS='-m32'
    # check that we built intrinsics - this should be used by default when supported by the compiler
    ./programs/test/selftest aes | grep "AESNI code" | grep -q "intrinsics"
    grep -q "AES note: using AESNI" ./programs/test/selftest
    grep -q "AES note: built-in implementation." ./programs/test/selftest
    grep -q "AES note: using VIA Padlock" ./programs/test/selftest
    grep -q mbedtls_aesni_has_support ./programs/test/selftest
}

# For timebeing, no aarch64 gcc available in CI and no arm64 CI node.
component_build_aes_aesce_armcc () {
    msg "Build: AESCE test on arm64 platform without plain C."
    scripts/config.py baremetal

    # armc[56] don't support SHA-512 intrinsics
    scripts/config.py unset MBEDTLS_SHA512_USE_A64_CRYPTO_IF_PRESENT

    # Stop armclang warning about feature detection for A64_CRYPTO.
    # With this enabled, the library does build correctly under armclang,
    # but in baremetal builds (as tested here), feature detection is
    # unavailable, and the user is notified via a #warning. So enabling
    # this feature would prevent us from building with -Werror on
    # armclang. Tracked in #7198.
    scripts/config.py unset MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_IF_PRESENT
    scripts/config.py set MBEDTLS_HAVE_ASM

    msg "AESCE, build with default configuration."
    scripts/config.py set MBEDTLS_AESCE_C
    scripts/config.py unset MBEDTLS_AES_USE_HARDWARE_ONLY
    armc6_build_test "-O1 --target=aarch64-arm-none-eabi -march=armv8-a+crypto"

    msg "AESCE, build AESCE only"
    scripts/config.py set MBEDTLS_AESCE_C
    scripts/config.py set MBEDTLS_AES_USE_HARDWARE_ONLY
    armc6_build_test "-O1 --target=aarch64-arm-none-eabi -march=armv8-a+crypto"
}

support_build_aes_armce() {
    # clang >= 11 is required to build with AES extensions
    [[ $(clang_version) -ge 11 ]]
}

component_build_aes_armce () {
    # Test variations of AES with Armv8 crypto extensions
    scripts/config.py set MBEDTLS_AESCE_C
    scripts/config.py set MBEDTLS_AES_USE_HARDWARE_ONLY

    msg "MBEDTLS_AES_USE_HARDWARE_ONLY, clang, aarch64"
    make -B library/aesce.o CC=clang CFLAGS="--target=aarch64-linux-gnu -march=armv8-a+crypto"

    msg "MBEDTLS_AES_USE_HARDWARE_ONLY, clang, arm"
    make -B library/aesce.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a72+crypto -marm"

    msg "MBEDTLS_AES_USE_HARDWARE_ONLY, clang, thumb"
    make -B library/aesce.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a32+crypto -mthumb"

    scripts/config.py unset MBEDTLS_AES_USE_HARDWARE_ONLY

    msg "no MBEDTLS_AES_USE_HARDWARE_ONLY, clang, aarch64"
    make -B library/aesce.o CC=clang CFLAGS="--target=aarch64-linux-gnu -march=armv8-a+crypto"

    msg "no MBEDTLS_AES_USE_HARDWARE_ONLY, clang, arm"
    make -B library/aesce.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a72+crypto -marm"

    msg "no MBEDTLS_AES_USE_HARDWARE_ONLY, clang, thumb"
    make -B library/aesce.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a32+crypto -mthumb"

    # test for presence of AES instructions
    scripts/config.py set MBEDTLS_AES_USE_HARDWARE_ONLY
    msg "clang, test A32 crypto instructions built"
    make -B library/aesce.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a72+crypto -marm -S"
    grep -E 'aes[0-9a-z]+.[0-9]\s*[qv]' library/aesce.o
    msg "clang, test T32 crypto instructions built"
    make -B library/aesce.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a32+crypto -mthumb -S"
    grep -E 'aes[0-9a-z]+.[0-9]\s*[qv]' library/aesce.o
    msg "clang, test aarch64 crypto instructions built"
    make -B library/aesce.o CC=clang CFLAGS="--target=aarch64-linux-gnu -march=armv8-a -S"
    grep -E 'aes[a-z]+\s*[qv]' library/aesce.o

    # test for absence of AES instructions
    scripts/config.py unset MBEDTLS_AES_USE_HARDWARE_ONLY
    scripts/config.py unset MBEDTLS_AESCE_C
    msg "clang, test A32 crypto instructions not built"
    make -B library/aesce.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a72+crypto -marm -S"
    not grep -E 'aes[0-9a-z]+.[0-9]\s*[qv]' library/aesce.o
    msg "clang, test T32 crypto instructions not built"
    make -B library/aesce.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a32+crypto -mthumb -S"
    not grep -E 'aes[0-9a-z]+.[0-9]\s*[qv]' library/aesce.o
    msg "clang, test aarch64 crypto instructions not built"
    make -B library/aesce.o CC=clang CFLAGS="--target=aarch64-linux-gnu -march=armv8-a -S"
    not grep -E 'aes[a-z]+\s*[qv]' library/aesce.o
}

support_build_sha_armce() {
    # clang >= 4 is required to build with SHA extensions
    [[ $(clang_version) -ge 4 ]]
}

component_build_sha_armce () {
    scripts/config.py unset MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_IF_PRESENT


    # Test variations of SHA256 Armv8 crypto extensions
    scripts/config.py set MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_ONLY
        msg "MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_ONLY clang, aarch64"
        make -B library/sha256.o CC=clang CFLAGS="--target=aarch64-linux-gnu -march=armv8-a"
        msg "MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_ONLY clang, arm"
        make -B library/sha256.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a72+crypto -marm"
    scripts/config.py unset MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_ONLY


    # test the deprecated form of the config option
    scripts/config.py set MBEDTLS_SHA256_USE_A64_CRYPTO_ONLY
        msg "MBEDTLS_SHA256_USE_A64_CRYPTO_ONLY clang, thumb"
        make -B library/sha256.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a32+crypto -mthumb"
    scripts/config.py unset MBEDTLS_SHA256_USE_A64_CRYPTO_ONLY

    scripts/config.py set MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_IF_PRESENT
        msg "MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_IF_PRESENT clang, aarch64"
        make -B library/sha256.o CC=clang CFLAGS="--target=aarch64-linux-gnu -march=armv8-a"
    scripts/config.py unset MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_IF_PRESENT


    # test the deprecated form of the config option
    scripts/config.py set MBEDTLS_SHA256_USE_A64_CRYPTO_IF_PRESENT
        msg "MBEDTLS_SHA256_USE_A64_CRYPTO_IF_PRESENT clang, arm"
        make -B library/sha256.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a72+crypto -marm -std=c99"
        msg "MBEDTLS_SHA256_USE_A64_CRYPTO_IF_PRESENT clang, thumb"
        make -B library/sha256.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a32+crypto -mthumb"
    scripts/config.py unset MBEDTLS_SHA256_USE_A64_CRYPTO_IF_PRESENT


    # examine the disassembly for presence of SHA instructions
    for opt in MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_ONLY MBEDTLS_SHA256_USE_ARMV8_A_CRYPTO_IF_PRESENT; do
        scripts/config.py set ${opt}
            msg "${opt} clang, test A32 crypto instructions built"
            make -B library/sha256.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a72+crypto -marm -S"
            grep -E 'sha256[a-z0-9]+.32\s+[qv]' library/sha256.o

            msg "${opt} clang, test T32 crypto instructions built"
            make -B library/sha256.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a32+crypto -mthumb -S"
            grep -E 'sha256[a-z0-9]+.32\s+[qv]' library/sha256.o

            msg "${opt} clang, test aarch64 crypto instructions built"
            make -B library/sha256.o CC=clang CFLAGS="--target=aarch64-linux-gnu -march=armv8-a -S"
            grep -E 'sha256[a-z0-9]+\s+[qv]' library/sha256.o
        scripts/config.py unset ${opt}
    done


    # examine the disassembly for absence of SHA instructions
    msg "clang, test A32 crypto instructions not built"
    make -B library/sha256.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a72+crypto -marm -S"
    not grep -E 'sha256[a-z0-9]+.32\s+[qv]' library/sha256.o

    msg "clang, test T32 crypto instructions not built"
    make -B library/sha256.o CC=clang CFLAGS="--target=arm-linux-gnueabihf -mcpu=cortex-a32+crypto -mthumb -S"
    not grep -E 'sha256[a-z0-9]+.32\s+[qv]' library/sha256.o

    msg "clang, test aarch64 crypto instructions not built"
    make -B library/sha256.o CC=clang CFLAGS="--target=aarch64-linux-gnu -march=armv8-a -S"
    not grep -E 'sha256[a-z0-9]+\s+[qv]' library/sha256.o
}

# For timebeing, no VIA Padlock platform available.
component_build_aes_via_padlock () {

    msg "AES:VIA PadLock, build with default configuration."
    scripts/config.py unset MBEDTLS_AESNI_C
    scripts/config.py set MBEDTLS_PADLOCK_C
    scripts/config.py unset MBEDTLS_AES_USE_HARDWARE_ONLY
    make CC=gcc CFLAGS="$ASAN_CFLAGS -m32" LDFLAGS="-m32 $ASAN_CFLAGS"
    grep -q mbedtls_padlock_has_support ./programs/test/selftest

}

support_build_aes_via_padlock_only () {
    ( [ "$MBEDTLS_TEST_PLATFORM" == "Linux-x86_64" ] || \
        [ "$MBEDTLS_TEST_PLATFORM" == "Linux-amd64" ] ) && \
    [ "`dpkg --print-foreign-architectures`" == "i386" ]
}

component_test_aes_only_128_bit_keys () {
    msg "build: default config + AES_ONLY_128_BIT_KEY_LENGTH"
    scripts/config.py set MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH
    scripts/config.py unset MBEDTLS_PADLOCK_C

    make CFLAGS='-O2 -Werror -Wall -Wextra'

    msg "test: default config + AES_ONLY_128_BIT_KEY_LENGTH"
    make test
}

component_test_no_ctr_drbg_aes_only_128_bit_keys () {
    msg "build: default config + AES_ONLY_128_BIT_KEY_LENGTH - CTR_DRBG_C"
    scripts/config.py set MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH
    scripts/config.py unset MBEDTLS_CTR_DRBG_C
    scripts/config.py unset MBEDTLS_PADLOCK_C

    make CC=clang CFLAGS='-Werror -Wall -Wextra'

    msg "test: default config + AES_ONLY_128_BIT_KEY_LENGTH - CTR_DRBG_C"
    make test
}

component_test_aes_only_128_bit_keys_have_builtins () {
    msg "build: default config + AES_ONLY_128_BIT_KEY_LENGTH - AESNI_C - AESCE_C"
    scripts/config.py set MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH
    scripts/config.py unset MBEDTLS_PADLOCK_C
    scripts/config.py unset MBEDTLS_AESNI_C
    scripts/config.py unset MBEDTLS_AESCE_C

    make CFLAGS='-O2 -Werror -Wall -Wextra'

    msg "test: default config + AES_ONLY_128_BIT_KEY_LENGTH - AESNI_C - AESCE_C"
    make test

    msg "selftest: default config + AES_ONLY_128_BIT_KEY_LENGTH - AESNI_C - AESCE_C"
    programs/test/selftest
}

component_test_gcm_largetable () {
    msg "build: default config + GCM_LARGE_TABLE - AESNI_C - AESCE_C"
    scripts/config.py set MBEDTLS_GCM_LARGE_TABLE
    scripts/config.py unset MBEDTLS_PADLOCK_C
    scripts/config.py unset MBEDTLS_AESNI_C
    scripts/config.py unset MBEDTLS_AESCE_C

    make CFLAGS='-O2 -Werror -Wall -Wextra'

    msg "test: default config - GCM_LARGE_TABLE - AESNI_C - AESCE_C"
    make test
}

component_test_aes_fewer_tables () {
    msg "build: default config with AES_FEWER_TABLES enabled"
    scripts/config.py set MBEDTLS_AES_FEWER_TABLES
    make CFLAGS='-O2 -Werror -Wall -Wextra'

    msg "test: AES_FEWER_TABLES"
    make test
}

component_test_aes_rom_tables () {
    msg "build: default config with AES_ROM_TABLES enabled"
    scripts/config.py set MBEDTLS_AES_ROM_TABLES
    make CFLAGS='-O2 -Werror -Wall -Wextra'

    msg "test: AES_ROM_TABLES"
    make test
}

component_test_aes_fewer_tables_and_rom_tables () {
    msg "build: default config with AES_ROM_TABLES and AES_FEWER_TABLES enabled"
    scripts/config.py set MBEDTLS_AES_FEWER_TABLES
    scripts/config.py set MBEDTLS_AES_ROM_TABLES
    make CFLAGS='-O2 -Werror -Wall -Wextra'

    msg "test: AES_FEWER_TABLES + AES_ROM_TABLES"
    make test
}

# helper for common_block_cipher_no_decrypt() which:
# - enable/disable the list of config options passed from -s/-u respectively.
# - build
# - test for tests_suite_xxx
# - selftest
#
# Usage: helper_block_cipher_no_decrypt_build_test
#        [-s set_opts] [-u unset_opts] [-c cflags] [-l ldflags] [option [...]]
# Options:  -s set_opts     the list of config options to enable
#           -u unset_opts   the list of config options to disable
#           -c cflags       the list of options passed to CFLAGS
#           -l ldflags      the list of options passed to LDFLAGS
helper_block_cipher_no_decrypt_build_test () {
    while [ $# -gt 0 ]; do
        case "$1" in
            -s)
                shift; local set_opts="$1";;
            -u)
                shift; local unset_opts="$1";;
            -c)
                shift; local cflags="-Werror -Wall -Wextra $1";;
            -l)
                shift; local ldflags="$1";;
        esac
        shift
    done
    set_opts="${set_opts:-}"
    unset_opts="${unset_opts:-}"
    cflags="${cflags:-}"
    ldflags="${ldflags:-}"

    [ -n "$set_opts" ] && echo "Enabling: $set_opts" && scripts/config.py set-all $set_opts
    [ -n "$unset_opts" ] && echo "Disabling: $unset_opts" && scripts/config.py unset-all $unset_opts

    msg "build: default config + BLOCK_CIPHER_NO_DECRYPT${set_opts:+ + $set_opts}${unset_opts:+ - $unset_opts} with $cflags${ldflags:+, $ldflags}"
    make clean
    make CFLAGS="-O2 $cflags" LDFLAGS="$ldflags"

    # Make sure we don't have mbedtls_xxx_setkey_dec in AES/ARIA/CAMELLIA
    not grep mbedtls_aes_setkey_dec library/aes.o
    not grep mbedtls_aria_setkey_dec library/aria.o
    not grep mbedtls_camellia_setkey_dec library/camellia.o
    # Make sure we don't have mbedtls_internal_aes_decrypt in AES
    not grep mbedtls_internal_aes_decrypt library/aes.o
    # Make sure we don't have mbedtls_aesni_inverse_key in AESNI
    not grep mbedtls_aesni_inverse_key library/aesni.o

    msg "test: default config + BLOCK_CIPHER_NO_DECRYPT${set_opts:+ + $set_opts}${unset_opts:+ - $unset_opts} with $cflags${ldflags:+, $ldflags}"
    make test

    msg "selftest: default config + BLOCK_CIPHER_NO_DECRYPT${set_opts:+ + $set_opts}${unset_opts:+ - $unset_opts} with $cflags${ldflags:+, $ldflags}"
    programs/test/selftest
}

# This is a common configuration function used in:
# - component_test_block_cipher_no_decrypt_aesni_legacy()
# - component_test_block_cipher_no_decrypt_aesni_use_psa()
# in order to test BLOCK_CIPHER_NO_DECRYPT with AESNI intrinsics,
# AESNI assembly and AES C implementation on x86_64 and with AESNI intrinsics
# on x86.
common_block_cipher_no_decrypt () {
    # test AESNI intrinsics
    helper_block_cipher_no_decrypt_build_test \
        -s "MBEDTLS_AESNI_C" \
        -c "-mpclmul -msse2 -maes"

    # test AESNI assembly
    helper_block_cipher_no_decrypt_build_test \
        -s "MBEDTLS_AESNI_C" \
        -c "-mno-pclmul -mno-sse2 -mno-aes"

    # test AES C implementation
    helper_block_cipher_no_decrypt_build_test \
        -u "MBEDTLS_AESNI_C"

    # test AESNI intrinsics for i386 target
    helper_block_cipher_no_decrypt_build_test \
        -s "MBEDTLS_AESNI_C" \
        -c "-m32 -mpclmul -msse2 -maes" \
        -l "-m32"
}

# This is a configuration function used in component_test_block_cipher_no_decrypt_xxx:
# usage: 0: no PSA crypto configuration
#        1: use PSA crypto configuration
config_block_cipher_no_decrypt () {
    use_psa=$1

    scripts/config.py set MBEDTLS_BLOCK_CIPHER_NO_DECRYPT
    scripts/config.py unset MBEDTLS_CIPHER_MODE_CBC
    scripts/config.py unset MBEDTLS_CIPHER_MODE_XTS
    scripts/config.py unset MBEDTLS_DES_C
    scripts/config.py unset MBEDTLS_NIST_KW_C

    if [ "$use_psa" -eq 1 ]; then
        # Enable support for cryptographic mechanisms through the PSA API.
        # Note: XTS, KW are not yet supported via the PSA API in Mbed TLS.
        scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
        scripts/config.py -f "$CRYPTO_CONFIG_H" unset PSA_WANT_ALG_CBC_NO_PADDING
        scripts/config.py -f "$CRYPTO_CONFIG_H" unset PSA_WANT_ALG_CBC_PKCS7
        scripts/config.py -f "$CRYPTO_CONFIG_H" unset PSA_WANT_ALG_ECB_NO_PADDING
        scripts/config.py -f "$CRYPTO_CONFIG_H" unset PSA_WANT_KEY_TYPE_DES
    fi
}

component_test_block_cipher_no_decrypt_aesni () {
    # This consistently causes an llvm crash on clang 3.8, so use gcc
    export CC=gcc
    config_block_cipher_no_decrypt 0
    common_block_cipher_no_decrypt
}

component_test_block_cipher_no_decrypt_aesni_use_psa () {
    # This consistently causes an llvm crash on clang 3.8, so use gcc
    export CC=gcc
    config_block_cipher_no_decrypt 1
    common_block_cipher_no_decrypt
}

component_test_block_cipher_no_decrypt_aesce_armcc () {
    scripts/config.py baremetal

    # armc[56] don't support SHA-512 intrinsics
    scripts/config.py unset MBEDTLS_SHA512_USE_A64_CRYPTO_IF_PRESENT

    # Stop armclang warning about feature detection for A64_CRYPTO.
    # With this enabled, the library does build correctly under armclang,
    # but in baremetal builds (as tested here), feature detection is
    # unavailable, and the user is notified via a #warning. So enabling
    # this feature would prevent us from building with -Werror on
    # armclang. Tracked in #7198.
    scripts/config.py unset MBEDTLS_SHA256_USE_A64_CRYPTO_IF_PRESENT
    scripts/config.py set MBEDTLS_HAVE_ASM

    config_block_cipher_no_decrypt 1

    # test AESCE baremetal build
    scripts/config.py set MBEDTLS_AESCE_C
    msg "build: default config + BLOCK_CIPHER_NO_DECRYPT with AESCE"
    armc6_build_test "-O1 --target=aarch64-arm-none-eabi -march=armv8-a+crypto -Werror -Wall -Wextra"

    # Make sure we don't have mbedtls_xxx_setkey_dec in AES/ARIA/CAMELLIA
    not grep mbedtls_aes_setkey_dec library/aes.o
    not grep mbedtls_aria_setkey_dec library/aria.o
    not grep mbedtls_camellia_setkey_dec library/camellia.o
    # Make sure we don't have mbedtls_internal_aes_decrypt in AES
    not grep mbedtls_internal_aes_decrypt library/aes.o
    # Make sure we don't have mbedtls_aesce_inverse_key and aesce_decrypt_block in AESCE
    not grep mbedtls_aesce_inverse_key library/aesce.o
    not grep aesce_decrypt_block library/aesce.o
}

component_test_ctr_drbg_aes_256_sha_256 () {
    msg "build: full + MBEDTLS_ENTROPY_FORCE_SHA256 (ASan build)"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_ENTROPY_FORCE_SHA256
    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: full + MBEDTLS_ENTROPY_FORCE_SHA256 (ASan build)"
    make test
}

component_test_ctr_drbg_aes_128_sha_512 () {
    msg "build: full + MBEDTLS_CTR_DRBG_USE_128_BIT_KEY (ASan build)"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_CTR_DRBG_USE_128_BIT_KEY
    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: full + MBEDTLS_CTR_DRBG_USE_128_BIT_KEY (ASan build)"
    make test
}

component_test_ctr_drbg_aes_128_sha_256 () {
    msg "build: full + MBEDTLS_CTR_DRBG_USE_128_BIT_KEY + MBEDTLS_ENTROPY_FORCE_SHA256 (ASan build)"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.py set MBEDTLS_CTR_DRBG_USE_128_BIT_KEY
    scripts/config.py set MBEDTLS_ENTROPY_FORCE_SHA256
    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: full + MBEDTLS_CTR_DRBG_USE_128_BIT_KEY + MBEDTLS_ENTROPY_FORCE_SHA256 (ASan build)"
    make test
}

component_test_se_default () {
    msg "build: default config + MBEDTLS_PSA_CRYPTO_SE_C"
    scripts/config.py set MBEDTLS_PSA_CRYPTO_SE_C
    make CC=clang CFLAGS="$ASAN_CFLAGS -Os" LDFLAGS="$ASAN_CFLAGS"

    msg "test: default config + MBEDTLS_PSA_CRYPTO_SE_C"
    make test
}

component_test_psa_crypto_drivers () {
    msg "build: full + test drivers dispatching to builtins"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_PSA_CRYPTO_CONFIG
    loc_cflags="$ASAN_CFLAGS -DPSA_CRYPTO_DRIVER_TEST_ALL"
    loc_cflags="${loc_cflags} '-DMBEDTLS_USER_CONFIG_FILE=\"../tests/configs/user-config-for-test.h\"'"
    loc_cflags="${loc_cflags} -I../tests/include -O2"

    make CC=$ASAN_CC CFLAGS="${loc_cflags}" LDFLAGS="$ASAN_CFLAGS"

    msg "test: full + test drivers dispatching to builtins"
    make test
}

test_build_opt () {
    info=$1 cc=$2; shift 2
    $cc --version
    for opt in "$@"; do
          msg "build/test: $cc $opt, $info" # ~ 30s
          make CC="$cc" CFLAGS="$opt -std=c99 -pedantic -Wall -Wextra -Werror"
          # We're confident enough in compilers to not run _all_ the tests,
          # but at least run the unit tests. In particular, runs with
          # optimizations use inline assembly whereas runs with -O0
          # skip inline assembly.
          make test # ~30s
          make clean
    done
}

# For FreeBSD we invoke the function by name so this condition is added
# to disable the existing test_clang_opt function for linux.
if [[ $(uname) != "Linux" ]]; then
    component_test_clang_opt () {
        scripts/config.py full
        test_build_opt 'full config' clang -O0 -Os -O2
    }
fi

component_test_clang_latest_opt () {
    scripts/config.py full
    test_build_opt 'full config' "$CLANG_LATEST" -O0 -Os -O2
}
support_test_clang_latest_opt () {
    type "$CLANG_LATEST" >/dev/null 2>/dev/null
}

component_test_clang_earliest_opt () {
    scripts/config.py full
    test_build_opt 'full config' "$CLANG_EARLIEST" -O0
}
support_test_clang_earliest_opt () {
    type "$CLANG_EARLIEST" >/dev/null 2>/dev/null
}

component_test_gcc_latest_opt () {
    scripts/config.py full
    test_build_opt 'full config' "$GCC_LATEST" -O0 -Os -O2
}
support_test_gcc_latest_opt () {
    type "$GCC_LATEST" >/dev/null 2>/dev/null
}

component_test_gcc_earliest_opt () {
    scripts/config.py full
    test_build_opt 'full config' "$GCC_EARLIEST" -O0
}
support_test_gcc_earliest_opt () {
    type "$GCC_EARLIEST" >/dev/null 2>/dev/null
}

component_build_mbedtls_config_file () {
    msg "build: make with MBEDTLS_CONFIG_FILE" # ~40s
    scripts/config.py -w full_config.h full
    echo '#error "MBEDTLS_CONFIG_FILE is not working"' >"$CONFIG_H"
    make CFLAGS="-I '$PWD' -DMBEDTLS_CONFIG_FILE='\"full_config.h\"'"
    # Make sure this feature is enabled. We'll disable it in the next phase.
    programs/test/query_compile_time_config MBEDTLS_NIST_KW_C
    make clean

    msg "build: make with MBEDTLS_CONFIG_FILE + MBEDTLS_USER_CONFIG_FILE"
    # In the user config, disable one feature (for simplicity, pick a feature
    # that nothing else depends on).
    echo '#undef MBEDTLS_NIST_KW_C' >user_config.h
    make CFLAGS="-I '$PWD' -DMBEDTLS_CONFIG_FILE='\"full_config.h\"' -DMBEDTLS_USER_CONFIG_FILE='\"user_config.h\"'"
    not programs/test/query_compile_time_config MBEDTLS_NIST_KW_C

    rm -f user_config.h full_config.h
}

component_build_psa_config_file () {
    msg "build: make with MBEDTLS_PSA_CRYPTO_CONFIG_FILE" # ~40s
    scripts/config.py set MBEDTLS_PSA_CRYPTO_CONFIG
    cp "$CRYPTO_CONFIG_H" psa_test_config.h
    echo '#error "MBEDTLS_PSA_CRYPTO_CONFIG_FILE is not working"' >"$CRYPTO_CONFIG_H"
    make CFLAGS="-I '$PWD' -DMBEDTLS_PSA_CRYPTO_CONFIG_FILE='\"psa_test_config.h\"'"
    # Make sure this feature is enabled. We'll disable it in the next phase.
    programs/test/query_compile_time_config MBEDTLS_CMAC_C
    make clean

    msg "build: make with MBEDTLS_PSA_CRYPTO_CONFIG_FILE + MBEDTLS_PSA_CRYPTO_USER_CONFIG_FILE" # ~40s
    # In the user config, disable one feature, which will reflect on the
    # mbedtls configuration so we can query it with query_compile_time_config.
    echo '#undef PSA_WANT_ALG_CMAC' >psa_user_config.h
    scripts/config.py unset MBEDTLS_CMAC_C
    make CFLAGS="-I '$PWD' -DMBEDTLS_PSA_CRYPTO_CONFIG_FILE='\"psa_test_config.h\"' -DMBEDTLS_PSA_CRYPTO_USER_CONFIG_FILE='\"psa_user_config.h\"'"
    not programs/test/query_compile_time_config MBEDTLS_CMAC_C

    rm -f psa_test_config.h psa_user_config.h
}

component_build_psa_alt_headers () {
    msg "build: make with PSA alt headers" # ~20s

    # Generate alternative versions of the substitutable headers with the
    # same content except different include guards.
    make -C tests include/alt-extra/psa/crypto_platform_alt.h include/alt-extra/psa/crypto_struct_alt.h

    # Build the library and some programs.
    # Don't build the fuzzers to avoid having to go through hoops to set
    # a correct include path for programs/fuzz/Makefile.
    make CFLAGS="-I ../tests/include/alt-extra -DMBEDTLS_PSA_CRYPTO_PLATFORM_FILE='\"psa/crypto_platform_alt.h\"' -DMBEDTLS_PSA_CRYPTO_STRUCT_FILE='\"psa/crypto_struct_alt.h\"'" lib
    make -C programs -o fuzz CFLAGS="-I ../tests/include/alt-extra -DMBEDTLS_PSA_CRYPTO_PLATFORM_FILE='\"psa/crypto_platform_alt.h\"' -DMBEDTLS_PSA_CRYPTO_STRUCT_FILE='\"psa/crypto_struct_alt.h\"'"

    # Check that we're getting the alternative include guards and not the
    # original include guards.
    programs/test/query_included_headers | grep -x PSA_CRYPTO_PLATFORM_ALT_H
    programs/test/query_included_headers | grep -x PSA_CRYPTO_STRUCT_ALT_H
    programs/test/query_included_headers | not grep -x PSA_CRYPTO_PLATFORM_H
    programs/test/query_included_headers | not grep -x PSA_CRYPTO_STRUCT_H
}

component_test_no_x509_info () {
    msg "build: full + MBEDTLS_X509_REMOVE_INFO" # ~ 10s
    scripts/config.pl full
    scripts/config.pl unset MBEDTLS_MEMORY_BACKTRACE # too slow for tests
    scripts/config.pl set MBEDTLS_X509_REMOVE_INFO
    make CFLAGS='-Werror -O2'

    msg "test: full + MBEDTLS_X509_REMOVE_INFO" # ~ 10s
    make test

    msg "test: ssl-opt.sh, full + MBEDTLS_X509_REMOVE_INFO" # ~ 1 min
    tests/ssl-opt.sh
}

component_test_tls12_only () {
    msg "build: default config without MBEDTLS_SSL_PROTO_TLS1_3, cmake, gcc, ASan"
    scripts/config.py unset MBEDTLS_SSL_PROTO_TLS1_3
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: main suites (inc. selftests) (ASan build)"
    make test

    msg "test: ssl-opt.sh (ASan build)"
    tests/ssl-opt.sh

    msg "test: compat.sh (ASan build)"
    tests/compat.sh
}

component_test_tls13_only () {
    msg "build: default config without MBEDTLS_SSL_PROTO_TLS1_2"
    scripts/config.py set MBEDTLS_SSL_EARLY_DATA
    scripts/config.py set MBEDTLS_SSL_RECORD_SIZE_LIMIT
    make CFLAGS="'-DMBEDTLS_USER_CONFIG_FILE=\"../tests/configs/tls13-only.h\"'"

    msg "test: TLS 1.3 only, all key exchange modes enabled"
    make test

    msg "ssl-opt.sh: TLS 1.3 only, all key exchange modes enabled"
    tests/ssl-opt.sh
}

component_test_tls13_only_psk () {
    msg "build: TLS 1.3 only from default, only PSK key exchange mode"
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED
    scripts/config.py unset MBEDTLS_ECDH_C
    scripts/config.py unset MBEDTLS_DHM_C
    scripts/config.py unset MBEDTLS_X509_CRT_PARSE_C
    scripts/config.py unset MBEDTLS_X509_RSASSA_PSS_SUPPORT
    scripts/config.py unset MBEDTLS_SSL_SERVER_NAME_INDICATION
    scripts/config.py unset MBEDTLS_ECDSA_C
    scripts/config.py unset MBEDTLS_PKCS1_V21
    scripts/config.py unset MBEDTLS_PKCS7_C
    scripts/config.py set   MBEDTLS_SSL_EARLY_DATA
    make CFLAGS="'-DMBEDTLS_USER_CONFIG_FILE=\"../tests/configs/tls13-only.h\"'"

    msg "test_suite_ssl: TLS 1.3 only, only PSK key exchange mode enabled"
    cd tests; ./test_suite_ssl; cd ..

    msg "ssl-opt.sh: TLS 1.3 only, only PSK key exchange mode enabled"
    tests/ssl-opt.sh
}

component_test_tls13_only_ephemeral () {
    msg "build: TLS 1.3 only from default, only ephemeral key exchange mode"
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED
    scripts/config.py unset MBEDTLS_SSL_EARLY_DATA
    make CFLAGS="'-DMBEDTLS_USER_CONFIG_FILE=\"../tests/configs/tls13-only.h\"'"

    msg "test_suite_ssl: TLS 1.3 only, only ephemeral key exchange mode"
    cd tests; ./test_suite_ssl; cd ..

    msg "ssl-opt.sh: TLS 1.3 only, only ephemeral key exchange mode"
    tests/ssl-opt.sh
}

component_test_tls13_only_ephemeral_ffdh () {
    msg "build: TLS 1.3 only from default, only ephemeral ffdh key exchange mode"
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED
    scripts/config.py unset MBEDTLS_SSL_EARLY_DATA
    scripts/config.py unset MBEDTLS_ECDH_C

    make CFLAGS="'-DMBEDTLS_USER_CONFIG_FILE=\"../tests/configs/tls13-only.h\"'"

    msg "test_suite_ssl: TLS 1.3 only, only ephemeral ffdh key exchange mode"
    cd tests; ./test_suite_ssl; cd ..

    msg "ssl-opt.sh: TLS 1.3 only, only ephemeral ffdh key exchange mode"
    tests/ssl-opt.sh
}

component_test_tls13_only_psk_ephemeral () {
    msg "build: TLS 1.3 only from default, only PSK ephemeral key exchange mode"
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED
    scripts/config.py unset MBEDTLS_X509_CRT_PARSE_C
    scripts/config.py unset MBEDTLS_X509_RSASSA_PSS_SUPPORT
    scripts/config.py unset MBEDTLS_SSL_SERVER_NAME_INDICATION
    scripts/config.py unset MBEDTLS_ECDSA_C
    scripts/config.py unset MBEDTLS_PKCS1_V21
    scripts/config.py unset MBEDTLS_PKCS7_C
    scripts/config.py set   MBEDTLS_SSL_EARLY_DATA
    make CFLAGS="'-DMBEDTLS_USER_CONFIG_FILE=\"../tests/configs/tls13-only.h\"'"

    msg "test_suite_ssl: TLS 1.3 only, only PSK ephemeral key exchange mode"
    cd tests; ./test_suite_ssl; cd ..

    msg "ssl-opt.sh: TLS 1.3 only, only PSK ephemeral key exchange mode"
    tests/ssl-opt.sh
}

component_test_tls13_only_psk_ephemeral_ffdh () {
    msg "build: TLS 1.3 only from default, only PSK ephemeral ffdh key exchange mode"
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED
    scripts/config.py unset MBEDTLS_X509_CRT_PARSE_C
    scripts/config.py unset MBEDTLS_X509_RSASSA_PSS_SUPPORT
    scripts/config.py unset MBEDTLS_SSL_SERVER_NAME_INDICATION
    scripts/config.py unset MBEDTLS_ECDSA_C
    scripts/config.py unset MBEDTLS_PKCS1_V21
    scripts/config.py unset MBEDTLS_PKCS7_C
    scripts/config.py set   MBEDTLS_SSL_EARLY_DATA
    scripts/config.py unset MBEDTLS_ECDH_C
    make CFLAGS="'-DMBEDTLS_USER_CONFIG_FILE=\"../tests/configs/tls13-only.h\"'"

    msg "test_suite_ssl: TLS 1.3 only, only PSK ephemeral ffdh key exchange mode"
    cd tests; ./test_suite_ssl; cd ..

    msg "ssl-opt.sh: TLS 1.3 only, only PSK ephemeral ffdh key exchange mode"
    tests/ssl-opt.sh
}

component_test_tls13_only_psk_all () {
    msg "build: TLS 1.3 only from default, without ephemeral key exchange mode"
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED
    scripts/config.py unset MBEDTLS_X509_CRT_PARSE_C
    scripts/config.py unset MBEDTLS_X509_RSASSA_PSS_SUPPORT
    scripts/config.py unset MBEDTLS_SSL_SERVER_NAME_INDICATION
    scripts/config.py unset MBEDTLS_ECDSA_C
    scripts/config.py unset MBEDTLS_PKCS1_V21
    scripts/config.py unset MBEDTLS_PKCS7_C
    scripts/config.py set   MBEDTLS_SSL_EARLY_DATA
    make CFLAGS="'-DMBEDTLS_USER_CONFIG_FILE=\"../tests/configs/tls13-only.h\"'"

    msg "test_suite_ssl: TLS 1.3 only, PSK and PSK ephemeral key exchange modes"
    cd tests; ./test_suite_ssl; cd ..

    msg "ssl-opt.sh: TLS 1.3 only, PSK and PSK ephemeral key exchange modes"
    tests/ssl-opt.sh
}

component_test_tls13_only_ephemeral_all () {
    msg "build: TLS 1.3 only from default, without PSK key exchange mode"
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED
    scripts/config.py set   MBEDTLS_SSL_EARLY_DATA
    make CFLAGS="'-DMBEDTLS_USER_CONFIG_FILE=\"../tests/configs/tls13-only.h\"'"

    msg "test_suite_ssl: TLS 1.3 only, ephemeral and PSK ephemeral key exchange modes"
    cd tests; ./test_suite_ssl; cd ..

    msg "ssl-opt.sh: TLS 1.3 only, ephemeral and PSK ephemeral key exchange modes"
    tests/ssl-opt.sh
}

component_test_tls13_no_padding () {
    msg "build: default config plus early data minus padding"
    scripts/config.py set MBEDTLS_SSL_CID_TLS1_3_PADDING_GRANULARITY 1
    scripts/config.py set MBEDTLS_SSL_EARLY_DATA
    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make
    msg "test: default config plus early data minus padding"
    make test
    msg "ssl-opt.sh (TLS 1.3 no padding)"
    tests/ssl-opt.sh
}

component_test_tls13_no_compatibility_mode () {
    msg "build: default config plus early data minus middlebox compatibility mode"
    scripts/config.py unset MBEDTLS_SSL_TLS1_3_COMPATIBILITY_MODE
    scripts/config.py set   MBEDTLS_SSL_EARLY_DATA
    CC=$ASAN_CC cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make
    msg "test: default config plus early data minus middlebox compatibility mode"
    make test
    msg "ssl-opt.sh (TLS 1.3 no compatibility mode)"
    tests/ssl-opt.sh
}

component_test_full_minus_session_tickets() {
    msg "build: full config without session tickets"
    scripts/config.py full
    scripts/config.py unset MBEDTLS_SSL_SESSION_TICKETS
    scripts/config.py unset MBEDTLS_SSL_EARLY_DATA
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make
    msg "test: full config without session tickets"
    make test
    msg "ssl-opt.sh (full config without session tickets)"
    tests/ssl-opt.sh
}

component_build_mingw () {
    msg "build: Windows cross build - mingw64, make (Link Library)" # ~ 30s
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror -Wall -Wextra -maes -msse2 -mpclmul' WINDOWS_BUILD=1 lib programs

    # note Make tests only builds the tests, but doesn't run them
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror -maes -msse2 -mpclmul' WINDOWS_BUILD=1 tests
    make WINDOWS_BUILD=1 clean

    msg "build: Windows cross build - mingw64, make (DLL)" # ~ 30s
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror -Wall -Wextra -maes -msse2 -mpclmul' WINDOWS_BUILD=1 SHARED=1 lib programs
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror -Wall -Wextra -maes -msse2 -mpclmul' WINDOWS_BUILD=1 SHARED=1 tests
    make WINDOWS_BUILD=1 clean

    msg "build: Windows cross build - mingw64, make (Library only, default config without MBEDTLS_AESNI_C)" # ~ 30s
    ./scripts/config.py unset MBEDTLS_AESNI_C #
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror -Wall -Wextra' WINDOWS_BUILD=1 lib
    make WINDOWS_BUILD=1 clean
}
support_build_mingw() {
    case $(i686-w64-mingw32-gcc -dumpversion 2>/dev/null) in
        [0-5]*|"") false;;
        *) true;;
    esac
}

component_test_memsan () {
    msg "build: MSan (clang)" # ~ 1 min 20s
    scripts/config.py unset MBEDTLS_AESNI_C # memsan doesn't grok asm
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=MemSan .
    make

    msg "test: main suites (MSan)" # ~ 10s
    make test

    msg "test: metatests (MSan)"
    tests/scripts/run-metatests.sh any msan

    msg "program demos (MSan)" # ~20s
    tests/scripts/run_demos.py

    msg "test: ssl-opt.sh (MSan)" # ~ 1 min
    tests/ssl-opt.sh

    # Optional part(s)

    if [ "$MEMORY" -gt 0 ]; then
        msg "test: compat.sh (MSan)" # ~ 6 min 20s
        tests/compat.sh
    fi
}

component_release_test_valgrind () {
    msg "build: Release (clang)"
    # default config, in particular without MBEDTLS_USE_PSA_CRYPTO
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=Release .
    make

    msg "test: main suites, Valgrind (default config)"
    make memcheck

    # Optional parts (slow; currently broken on OS X because programs don't
    # seem to receive signals under valgrind on OS X).
    # These optional parts don't run on the CI.
    if [ "$MEMORY" -gt 0 ]; then
        msg "test: ssl-opt.sh --memcheck (default config)"
        tests/ssl-opt.sh --memcheck
    fi

    if [ "$MEMORY" -gt 1 ]; then
        msg "test: compat.sh --memcheck (default config)"
        tests/compat.sh --memcheck
    fi

    if [ "$MEMORY" -gt 0 ]; then
        msg "test: context-info.sh --memcheck (default config)"
        tests/context-info.sh --memcheck
    fi
}

component_release_test_valgrind_psa () {
    msg "build: Release, full (clang)"
    # full config, in particular with MBEDTLS_USE_PSA_CRYPTO
    scripts/config.py full
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=Release .
    make

    msg "test: main suites, Valgrind (full config)"
    make memcheck
}

component_build_zeroize_checks () {
    msg "build: check for obviously wrong calls to mbedtls_platform_zeroize()"

    scripts/config.py full

    # Only compile - we're looking for sizeof-pointer-memaccess warnings
    make CFLAGS="'-DMBEDTLS_USER_CONFIG_FILE=\"../tests/configs/user-config-zeroize-memset.h\"' -DMBEDTLS_TEST_DEFINES_ZEROIZE -Werror -Wsizeof-pointer-memaccess"
}


component_test_zeroize () {
    # Test that the function mbedtls_platform_zeroize() is not optimized away by
    # different combinations of compilers and optimization flags by using an
    # auxiliary GDB script. Unfortunately, GDB does not return error values to the
    # system in all cases that the script fails, so we must manually search the
    # output to check whether the pass string is present and no failure strings
    # were printed.

    # Don't try to disable ASLR. We don't care about ASLR here. We do care
    # about a spurious message if Gdb tries and fails, so suppress that.
    gdb_disable_aslr=
    if [ -z "$(gdb -batch -nw -ex 'set disable-randomization off' 2>&1)" ]; then
        gdb_disable_aslr='set disable-randomization off'
    fi

    for optimization_flag in -O2 -O3 -Ofast -Os; do
        for compiler in clang gcc; do
            msg "test: $compiler $optimization_flag, mbedtls_platform_zeroize()"
            make programs CC="$compiler" DEBUG=1 CFLAGS="$optimization_flag"
            gdb -ex "$gdb_disable_aslr" -x tests/scripts/test_zeroize.gdb -nw -batch -nx 2>&1 | tee test_zeroize.log
            grep "The buffer was correctly zeroized" test_zeroize.log
            not grep -i "error" test_zeroize.log
            rm -f test_zeroize.log
            make clean
        done
    done
}

component_test_psa_compliance () {
    # The arch tests build with gcc, so require use of gcc here to link properly
    msg "build: make, default config (out-of-box), libmbedcrypto.a only"
    CC=gcc make -C library libmbedcrypto.a

    msg "unit test: test_psa_compliance.py"
    CC=gcc ./tests/scripts/test_psa_compliance.py
}

support_test_psa_compliance () {
    # psa-compliance-tests only supports CMake >= 3.10.0
    ver="$(cmake --version)"
    ver="${ver#cmake version }"
    ver_major="${ver%%.*}"

    ver="${ver#*.}"
    ver_minor="${ver%%.*}"

    [ "$ver_major" -eq 3 ] && [ "$ver_minor" -ge 10 ]
}
