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
