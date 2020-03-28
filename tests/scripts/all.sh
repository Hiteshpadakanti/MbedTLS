#! /usr/bin/env bash

# all.sh
#
# This file is part of mbed TLS (https://tls.mbed.org)
#
# Copyright (c) 2014-2020, ARM Limited, All Rights Reserved



################################################################
#### Documentation
################################################################

# Purpose
# -------
#
# To run all tests possible or available on the platform.
#
# Notes for users
# ---------------
#
# Warning: the test is destructive. It includes various build modes and
# configurations, and can and will arbitrarily change the current CMake
# configuration. The following files must be committed into git:
#    * include/mbedtls/config.h
#    * Makefile, library/Makefile, programs/Makefile, tests/Makefile
# After running this script, the CMake cache will be lost and CMake
# will no longer be initialised.
#
# The script assumes the presence of a number of tools:
#   * Basic Unix tools (Windows users note: a Unix-style find must be before
#     the Windows find in the PATH)
#   * Perl
#   * GNU Make
#   * CMake
#   * GCC and Clang (recent enough for using ASan with gcc and MemSan with clang, or valgrind)
#   * arm-gcc and mingw-gcc
#   * ArmCC 5 and ArmCC 6, unless invoked with --no-armcc
#   * Yotta build dependencies, unless invoked with --no-yotta
#   * OpenSSL and GnuTLS command line tools, recent enough for the
#     interoperability tests. If they don't support SSLv3 then a legacy
#     version of these tools must be present as well (search for LEGACY
#     below).
# See the invocation of check_tools below for details.
#
# This script must be invoked from the toplevel directory of a git
# working copy of Mbed TLS.
#
# The behavior on an error depends on whether --keep-going (alias -k)
# is in effect.
#  * Without --keep-going: the script stops on the first error without
#    cleaning up. This lets you work in the configuration of the failing
#    component.
#  * With --keep-going: the script runs all requested components and
#    reports failures at the end. In particular the script always cleans
#    up on exit.
#
# Note that the output is not saved. You may want to run
#   script -c tests/scripts/all.sh
# or
#   tests/scripts/all.sh >all.log 2>&1
#
# Notes for maintainers
# ---------------------
#
# The bulk of the code is organized into functions that follow one of the
# following naming conventions:
#  * pre_XXX: things to do before running the tests, in order.
#  * component_XXX: independent components. They can be run in any order.
#      * component_check_XXX: quick tests that aren't worth parallelizing.
#      * component_build_XXX: build things but don't run them.
#      * component_test_XXX: build and test.
#  * support_XXX: if support_XXX exists and returns false then
#    component_XXX is not run by default.
#  * post_XXX: things to do after running the tests.
#  * other: miscellaneous support functions.
#
# Each component must start by invoking `msg` with a short informative message.
#
# Warning: due to the way bash detects errors, the failure of a command
# inside 'if' or '!' is not detected. Use the 'not' function instead of '!'.
#
# Each component is executed in a separate shell process. The component
# fails if any command in it returns a non-zero status.
#
# The framework performs some cleanup tasks after each component. This
# means that components can assume that the working directory is in a
# cleaned-up state, and don't need to perform the cleanup themselves.
# * Run `make clean`.
# * Restore `include/mbedtks/config.h` from a backup made before running
#   the component.
# * Check out `Makefile`, `library/Makefile`, `programs/Makefile` and
#   `tests/Makefile` from git. This cleans up after an in-tree use of
#   CMake.
#
# The tests are roughly in order from fastest to slowest. This doesn't
# have to be exact, but in general you should add slower tests towards
# the end and fast checks near the beginning.



################################################################
#### Initialization and command line parsing
################################################################

# Abort on errors (even on the left-hand side of a pipe).
# Treat uninitialised variables as errors.
set -e -o pipefail -u

pre_check_environment () {
    if [ -d library -a -d include -a -d tests ]; then :; else
        echo "Must be run from mbed TLS root" >&2
        exit 1
    fi
}

pre_initialize_variables () {
    CONFIG_H='include/mbedtls/config.h'
    CONFIG_BAK="$CONFIG_H.bak"

    MEMORY=0
    FORCE=0
    KEEP_GOING=0
    YOTTA=1

    # Default commands, can be overridden by the environment
    : ${OPENSSL:="openssl"}
    : ${OPENSSL_LEGACY:="$OPENSSL"}
    : ${GNUTLS_CLI:="gnutls-cli"}
    : ${GNUTLS_SERV:="gnutls-serv"}
    : ${GNUTLS_LEGACY_CLI:="$GNUTLS_CLI"}
    : ${GNUTLS_LEGACY_SERV:="$GNUTLS_SERV"}
    : ${OUT_OF_SOURCE_DIR:=./mbedtls_out_of_source_build}
    : ${ARMC5_BIN_DIR:=/usr/bin}
    : ${ARMC6_BIN_DIR:=/usr/bin}

    # if MAKEFLAGS is not set add the -j option to speed up invocations of make
    if [ -z "${MAKEFLAGS+set}" ]; then
        export MAKEFLAGS="-j"
    fi

    # CFLAGS and LDFLAGS for Asan builds that don't use CMake
    ASAN_CFLAGS='-Werror -Wall -Wextra -fsanitize=address,undefined -fno-sanitize-recover=all'

    # Gather the list of available components. These are the functions
    # defined in this script whose name starts with "component_".
    # Parse the script with sed. This way we get the functions in the order
    # they are defined.
    ALL_COMPONENTS=$(sed -n 's/^ *component_\([0-9A-Z_a-z]*\) *().*/\1/p' <"$0")

    # Exclude components that are not supported on this platform.
    SUPPORTED_COMPONENTS=
    for component in $ALL_COMPONENTS; do
        case $(type "support_$component" 2>&1) in
            *' function'*)
                if ! support_$component; then continue; fi;;
        esac
        SUPPORTED_COMPONENTS="$SUPPORTED_COMPONENTS $component"
    done
}

# Test whether the component $1 is included in the command line patterns.
is_component_included()
{
    set -f
    for pattern in $COMMAND_LINE_COMPONENTS; do
        set +f
        case ${1#component_} in $pattern) return 0;; esac
    done
    set +f
    return 1
}

usage()
{
    cat <<EOF
Usage: $0 [OPTION]... [COMPONENT]...
Run mbedtls release validation tests.
By default, run all tests. With one or more COMPONENT, run only those.
COMPONENT can be the name of a component or a shell wildcard pattern.

Examples:
  $0 "check_*"
    Run all sanity checks.
  $0 --no-armcc --except test_memsan
    Run everything except builds that require armcc and MemSan.

Special options:
  -h|--help             Print this help and exit.
  --list-all-components List all available test components and exit.
  --list-components     List components supported on this platform and exit.

General options:
  -f|--force            Force the tests to overwrite any modified files.
  -k|--keep-going       Run all tests and report errors at the end.
  -m|--memory           Additional optional memory tests.
     --armcc            Run ARM Compiler builds (on by default).
     --error-test       Error test mode: run a failing function in addition
                        to any specified component.
     --except           Exclude the COMPONENTs listed on the command line,
                        instead of running only those.
     --no-armcc         Skip ARM Compiler builds.
     --no-force         Refuse to overwrite modified files (default).
     --no-keep-going    Stop at the first error (default).
     --no-memory        No additional memory tests (default).
     --no-yotta         Skip yotta module build.
     --out-of-source-dir=<path>  Directory used for CMake out-of-source build tests.
     --random-seed      Use a random seed value for randomized tests (default).
  -r|--release-test     Run this script in release mode. This fixes the seed value to 1.
  -s|--seed             Integer seed value to use for this test run.
     --yotta            Build yotta module (on by default).

Tool path options:
     --armc5-bin-dir=<ARMC5_bin_dir_path>       ARM Compiler 5 bin directory.
     --armc6-bin-dir=<ARMC6_bin_dir_path>       ARM Compiler 6 bin directory.
     --gnutls-cli=<GnuTLS_cli_path>             GnuTLS client executable to use for most tests.
     --gnutls-serv=<GnuTLS_serv_path>           GnuTLS server executable to use for most tests.
     --gnutls-legacy-cli=<GnuTLS_cli_path>      GnuTLS client executable to use for legacy tests.
     --gnutls-legacy-serv=<GnuTLS_serv_path>    GnuTLS server executable to use for legacy tests.
     --openssl=<OpenSSL_path>                   OpenSSL executable to use for most tests.
     --openssl-legacy=<OpenSSL_path>            OpenSSL executable to use for legacy tests e.g. SSLv3.
EOF
}

# remove built files as well as the cmake cache/config
cleanup()
{
    command make clean

    # Remove CMake artefacts
    find . -name .git -prune -o -name yotta -prune -o \
           -iname CMakeFiles -exec rm -rf {} \+ -o \
           \( -iname cmake_install.cmake -o \
              -iname CTestTestfile.cmake -o \
              -iname CMakeCache.txt \) -exec rm {} \+
    # Recover files overwritten by in-tree CMake builds
    rm -f include/Makefile include/mbedtls/Makefile programs/*/Makefile
    git update-index --no-skip-worktree Makefile library/Makefile programs/Makefile tests/Makefile
    git checkout -- Makefile library/Makefile programs/Makefile tests/Makefile

    if [ -f "$CONFIG_BAK" ]; then
        mv "$CONFIG_BAK" "$CONFIG_H"
    fi
}

# Executed on exit. May be redefined depending on command line options.
final_report () {
    :
}

fatal_signal () {
    cleanup
    final_report $1
    trap - $1
    kill -$1 $$
}

trap 'fatal_signal HUP' HUP
trap 'fatal_signal INT' INT
trap 'fatal_signal TERM' TERM

msg()
{
    if [ -n "${current_component:-}" ]; then
        current_section="${current_component#component_}: $1"
    else
        current_section="$1"
    fi
    echo ""
    echo "******************************************************************"
    echo "* $current_section "
    printf "* "; date
    echo "******************************************************************"
}

armc6_build_test()
{
    FLAGS="$1"

    msg "build: ARM Compiler 6 ($FLAGS), make"
    ARM_TOOL_VARIANT="ult" CC="$ARMC6_CC" AR="$ARMC6_AR" CFLAGS="$FLAGS" \
                    WARNING_CFLAGS='-xc -std=c99' make lib
    make clean
}

err_msg()
{
    echo "$1" >&2
}

check_tools()
{
    for TOOL in "$@"; do
        if ! `type "$TOOL" >/dev/null 2>&1`; then
            err_msg "$TOOL not found!"
            exit 1
        fi
    done
}

pre_parse_command_line () {
    COMMAND_LINE_COMPONENTS=
    all_except=0
    error_test=0
    no_armcc=

    while [ $# -gt 0 ]; do
        case "$1" in
            --armcc) no_armcc=;;
            --armc5-bin-dir) shift; ARMC5_BIN_DIR="$1";;
            --armc6-bin-dir) shift; ARMC6_BIN_DIR="$1";;
            --error-test) error_test=$((error_test + 1));;
            --except) all_except=1;;
            --force|-f) FORCE=1;;
            --gnutls-cli) shift; GNUTLS_CLI="$1";;
            --gnutls-legacy-cli) shift; GNUTLS_LEGACY_CLI="$1";;
            --gnutls-legacy-serv) shift; GNUTLS_LEGACY_SERV="$1";;
            --gnutls-serv) shift; GNUTLS_SERV="$1";;
            --help|-h) usage; exit;;
            --keep-going|-k) KEEP_GOING=1;;
            --list-all-components) printf '%s\n' $ALL_COMPONENTS; exit;;
            --list-components) printf '%s\n' $SUPPORTED_COMPONENTS; exit;;
            --memory|-m) MEMORY=1;;
            --no-armcc) no_armcc=1;;
            --no-force) FORCE=0;;
            --no-keep-going) KEEP_GOING=0;;
            --no-memory) MEMORY=0;;
            --no-yotta) YOTTA=0;;
            --openssl) shift; OPENSSL="$1";;
            --openssl-legacy) shift; OPENSSL_LEGACY="$1";;
            --out-of-source-dir) shift; OUT_OF_SOURCE_DIR="$1";;
            --random-seed) unset SEED;;
            --release-test|-r) SEED=1;;
            --seed|-s) shift; SEED="$1";;
            --yotta) YOTTA=1;;
            -*)
                echo >&2 "Unknown option: $1"
                echo >&2 "Run $0 --help for usage."
                exit 120
                ;;
            *) COMMAND_LINE_COMPONENTS="$COMMAND_LINE_COMPONENTS $1";;
        esac
        shift
    done

    # With no list of components, run everything.
    if [ -z "$COMMAND_LINE_COMPONENTS" ]; then
        all_except=1
    fi

    # --no-armcc is a legacy option. The modern way is --except '*_armcc*'.
    # Ignore it if components are listed explicitly on the command line.
    if [ -n "$no_armcc" ] && [ $all_except -eq 1 ]; then
        COMMAND_LINE_COMPONENTS="$COMMAND_LINE_COMPONENTS *_armcc*"
        # --no-armcc also disables yotta.
        COMMAND_LINE_COMPONENTS="$COMMAND_LINE_COMPONENTS *_yotta*"
    fi

    if [ $all_except -eq 0 ]; then
        unsupported=0
        for component in $COMMAND_LINE_COMPONENTS; do
            case $component in
                *[*?\[]*) continue;;
            esac
            case " $SUPPORTED_COMPONENTS " in
                *" $component "*) :;;
                *)
                    echo >&2 "Component $component was explicitly requested, but is not known or not supported."
                    unsupported=$((unsupported + 1));;
            esac
        done
        if [ $unsupported -ne 0 ]; then
            exit 2
        fi
    fi

    # Build the list of components to run.
    RUN_COMPONENTS=
    for component in $SUPPORTED_COMPONENTS; do
        if is_component_included "$component"; [ $? -eq $all_except ]; then
            RUN_COMPONENTS="$RUN_COMPONENTS $component"
        fi
    done

    unset all_except
    unset no_armcc
}

pre_check_git () {
    if [ $FORCE -eq 1 ]; then
        rm -rf "$OUT_OF_SOURCE_DIR"
        if [ $YOTTA -eq 1 ]; then
            rm -rf yotta/module
        fi
        git checkout-index -f -q $CONFIG_H
        cleanup
    else

        if [ $YOTTA -ne 0 ] && [ -d yotta/module ]; then
            err_msg "Warning - there is an existing yotta module in the directory 'yotta/module'"
            echo "You can either delete your work and retry, or force the test to overwrite the"
            echo "test by rerunning the script as: $0 --force"
            exit 1
        fi

        if [ -d "$OUT_OF_SOURCE_DIR" ]; then
            echo "Warning - there is an existing directory at '$OUT_OF_SOURCE_DIR'" >&2
            echo "You can either delete this directory manually, or force the test by rerunning"
            echo "the script as: $0 --force --out-of-source-dir $OUT_OF_SOURCE_DIR"
            exit 1
        fi

        if ! git diff --quiet include/mbedtls/config.h; then
            err_msg "Warning - the configuration file 'include/mbedtls/config.h' has been edited. "
            echo "You can either delete or preserve your work, or force the test by rerunning the"
            echo "script as: $0 --force"
            exit 1
        fi
    fi
}

pre_setup_keep_going () {
    failure_count=0 # Number of failed components
    last_failure_status=0 # Last failure status in this component

    # See err_trap
    previous_failure_status=0
    previous_failed_command=
    previous_failure_funcall_depth=0
    unset report_failed_command

    start_red=
    end_color=
    if [ -t 1 ]; then
        case "${TERM:-}" in
            *color*|cygwin|linux|rxvt*|screen|[Eex]term*)
                start_red=$(printf '\033[31m')
                end_color=$(printf '\033[0m')
                ;;
        esac
    fi

    # Keep a summary of failures in a file. We'll print it out at the end.
    failure_summary_file=$PWD/all-sh-failures-$$.log
    : >"$failure_summary_file"

    # Whether it makes sense to keep a component going after the specified
    # command fails (test command) or not (configure or build).
    # This doesn't have to be 100% accurate: all failures are recorded anyway.
    can_keep_going_after_failure () {
        case "$1" in
            "msg "*) false;;
            *[!A-Za-z]"test"|*[!A-Za-z]"test"[!A-Za-z]*) true;;
            "tests/"*) true;;
            "grep "*|"! grep "*) true;;
            *) false;;
        esac
    }

    # This function runs if there is any error in a component.
    # It must either exit with a nonzero status, or set
    # last_failure_status to a nonzero value.
    err_trap () {
        # Save $? (status of the failing command). This must be the very
        # first thing, before $? is overridden.
        last_failure_status=$?
        failed_command=${report_failed_command-$BASH_COMMAND}

        if [[ $last_failure_status -eq $previous_failure_status &&
              "$failed_command" == "$previous_failed_command" &&
              ${#FUNCNAME[@]} == $((previous_failure_funcall_depth - 1)) ]]
        then
            # The same command failed twice in a row, but this time one level
            # less deep in the function call stack. This happens when the last
            # command of a function returns a nonzero status, and the function
            # returns that same status. Ignore the second failure.
            previous_failure_funcall_depth=${#FUNCNAME[@]}
            return
        fi
        previous_failure_status=$last_failure_status
        previous_failed_command=$failed_command
        previous_failure_funcall_depth=${#FUNCNAME[@]}

        text="$current_section: $failed_command -> $last_failure_status"
        echo "${start_red}^^^^$text^^^^${end_color}" >&2
        echo "$text" >>"$failure_summary_file"

        # If the command is fatal (configure or build command), stop this
        # component. Otherwise (test command) keep the component running
        # (run more tests from the same build).
        if ! can_keep_going_after_failure "$failed_command"; then
            exit $last_failure_status
        fi
    }

    final_report () {
        if [ $failure_count -gt 0 ]; then
            echo
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "${start_red}FAILED: $failure_count components${end_color}"
            cat "$failure_summary_file"
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        elif [ -z "${1-}" ]; then
            echo "SUCCESS :)"
        fi
        if [ -n "${1-}" ]; then
            echo "Killed by SIG$1."
        fi
        rm -f "$failure_summary_file"
        if [ $failure_count -gt 0 ]; then
            exit 1
        fi
    }
}

# '! true' does not trigger the ERR trap. Arrange to trigger it, with
# a reasonably informative error message (not just "$@").
not () {
    if "$@"; then
        report_failed_command="! $*"
        false
        unset report_failed_command
    fi
}


pre_print_configuration () {
    msg "info: $0 configuration"
    echo "MEMORY: $MEMORY"
    echo "FORCE: $FORCE"
    echo "SEED: ${SEED-"UNSET"}"
    echo "OPENSSL: $OPENSSL"
    echo "OPENSSL_LEGACY: $OPENSSL_LEGACY"
    echo "GNUTLS_CLI: $GNUTLS_CLI"
    echo "GNUTLS_SERV: $GNUTLS_SERV"
    echo "GNUTLS_LEGACY_CLI: $GNUTLS_LEGACY_CLI"
    echo "GNUTLS_LEGACY_SERV: $GNUTLS_LEGACY_SERV"
    echo "ARMC5_BIN_DIR: $ARMC5_BIN_DIR"
    echo "ARMC6_BIN_DIR: $ARMC6_BIN_DIR"
}

# Make sure the tools we need are available.
pre_check_tools () {
    # Build the list of variables to pass to output_env.sh.
    set env

    case " $RUN_COMPONENTS " in
        # Require OpenSSL and GnuTLS if running any tests (as opposed to
        # only doing builds). Not all tests run OpenSSL and GnuTLS, but this
        # is a good enough approximation in practice.
        *" test_"*)
            # To avoid setting OpenSSL and GnuTLS for each call to compat.sh
            # and ssl-opt.sh, we just export the variables they require.
            export OPENSSL_CMD="$OPENSSL"
            export GNUTLS_CLI="$GNUTLS_CLI"
            export GNUTLS_SERV="$GNUTLS_SERV"
            # Avoid passing --seed flag in every call to ssl-opt.sh
            if [ -n "${SEED-}" ]; then
                export SEED
            fi
            set "$@" OPENSSL="$OPENSSL" OPENSSL_LEGACY="$OPENSSL_LEGACY"
            set "$@" GNUTLS_CLI="$GNUTLS_CLI" GNUTLS_SERV="$GNUTLS_SERV"
            set "$@" GNUTLS_LEGACY_CLI="$GNUTLS_LEGACY_CLI"
            set "$@" GNUTLS_LEGACY_SERV="$GNUTLS_LEGACY_SERV"
            check_tools "$OPENSSL" "$OPENSSL_LEGACY" \
                        "$GNUTLS_CLI" "$GNUTLS_SERV" \
                        "$GNUTLS_LEGACY_CLI" "$GNUTLS_LEGACY_SERV"
            ;;
    esac

    case " $RUN_COMPONENTS " in
        *_doxygen[_\ ]*) check_tools "doxygen" "dot";;
    esac

    case " $RUN_COMPONENTS " in
        *_arm_none_eabi_gcc[_\ ]*) check_tools "arm-none-eabi-gcc";;
    esac

    case " $RUN_COMPONENTS " in
        *_mingw[_\ ]*) check_tools "i686-w64-mingw32-gcc";;
    esac

    case " $RUN_COMPONENTS " in
        *_armcc*|*_yotta*)
            ARMC5_CC="$ARMC5_BIN_DIR/armcc"
            ARMC5_AR="$ARMC5_BIN_DIR/armar"
            ARMC6_CC="$ARMC6_BIN_DIR/armclang"
            ARMC6_AR="$ARMC6_BIN_DIR/armar"
            check_tools "$ARMC5_CC" "$ARMC5_AR" "$ARMC6_CC" "$ARMC6_AR";;
    esac

    msg "info: output_env.sh"
    case $RUN_COMPONENTS in
        *_armcc*|*_yotta*)
            set "$@" ARMC5_CC="$ARMC5_CC" ARMC6_CC="$ARMC6_CC" RUN_ARMCC=1;;
        *) set "$@" RUN_ARMCC=0;;
    esac
    "$@" scripts/output_env.sh
}



################################################################
#### Basic checks
################################################################

#
# Test Suites to be executed
#
# The test ordering tries to optimize for the following criteria:
# 1. Catch possible problems early, by running first tests that run quickly
#    and/or are more likely to fail than others (eg I use Clang most of the
#    time, so start with a GCC build).
# 2. Minimize total running time, by avoiding useless rebuilds
#
# Indicative running times are given for reference.

component_check_recursion () {
    msg "test: recursion.pl" # < 1s
    tests/scripts/recursion.pl library/*.c
}

component_check_generated_files () {
    msg "test: freshness of generated source files" # < 1s
    tests/scripts/check-generated-files.sh
}

component_check_doxy_blocks () {
    msg "test: doxygen markup outside doxygen blocks" # < 1s
    tests/scripts/check-doxy-blocks.pl
}

component_check_files () {
    msg "test: check-files.py" # < 1s
    tests/scripts/check-files.py
}

component_check_names () {
    msg "test/build: declared and exported names" # < 3s
    tests/scripts/check-names.sh -v
}

component_check_doxygen_warnings () {
    msg "test: doxygen warnings" # ~ 3s
    tests/scripts/doxygen.sh
}


################################################################
#### Build and test many configurations and targets
################################################################

component_test_large_ecdsa_key_signature () {

    SMALL_MPI_MAX_SIZE=136 # Small enough to interfere with the EC signatures

    msg "build: cmake + MBEDTLS_MPI_MAX_SIZE=${SMALL_MPI_MAX_SIZE}, gcc, ASan" # ~ 1 min 50s
    scripts/config.pl set MBEDTLS_MPI_MAX_SIZE $SMALL_MPI_MAX_SIZE
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    INEVITABLY_PRESENT_FILE=Makefile
    SIGNATURE_FILE="${INEVITABLY_PRESENT_FILE}.sig" # Warning, this is rm -f'ed below

    msg "test: pk_sign secp521r1_prv.der for MBEDTLS_MPI_MAX_SIZE=${SMALL_MPI_MAX_SIZE} (ASan build)" # ~ 5s
    programs/pkey/pk_sign tests/data_files/secp521r1_prv.der $INEVITABLY_PRESENT_FILE
    rm -f $SIGNATURE_FILE
}

component_test_default_out_of_box () {
    msg "build: make, default config (out-of-box)" # ~1min
    make

    msg "test: main suites make, default config (out-of-box)" # ~10s
    make test

    msg "selftest: make, default config (out-of-box)" # ~10s
    programs/test/selftest
}

component_build_yotta () {
    # Note - use of yotta is deprecated, and yotta also requires armcc to be on the
    # path, and uses whatever version of armcc it finds there.
    msg "build: create and build yotta module" # ~ 30s
    tests/scripts/yotta-build.sh
}
support_build_yotta () {
    [ $YOTTA -ne 0 ]
}

component_test_default_cmake_gcc_asan () {
    msg "build: cmake, gcc, ASan" # ~ 1 min 50s
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "test: ssl-opt.sh (ASan build)" # ~ 1 min
    tests/ssl-opt.sh

    msg "test: compat.sh (ASan build)" # ~ 6 min
    tests/compat.sh
}

component_test_full_cmake_gcc_asan () {
    msg "build: full config, cmake, gcc, ASan"
    scripts/config.pl full
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: main suites (inc. selftests) (full config, ASan build)"
    make test

    msg "test: ssl-opt.sh (full config, ASan build)"
    tests/ssl-opt.sh

    msg "test: compat.sh (full config, ASan build)"
    tests/compat.sh
}

component_test_zlib_make() {
    msg "build: zlib enabled, make"
    scripts/config.pl set MBEDTLS_ZLIB_SUPPORT
    make ZLIB=1 CFLAGS='-Werror -O1'

    msg "test: main suites (zlib, make)"
    make test

    msg "test: ssl-opt.sh (zlib, make)"
    tests/ssl-opt.sh
}
support_test_zlib_make () {
    base=support_test_zlib_$$
    cat <<'EOF' > ${base}.c
#include "zlib.h"
int main(void) { return 0; }
EOF
    gcc -o ${base}.exe ${base}.c -lz 2>/dev/null
    ret=$?
    rm -f ${base}.*
    return $ret
}

component_test_zlib_cmake() {
    msg "build: zlib enabled, cmake"
    scripts/config.pl set MBEDTLS_ZLIB_SUPPORT
    cmake -D ENABLE_ZLIB_SUPPORT=On -D CMAKE_BUILD_TYPE:String=Check .
    make

    msg "test: main suites (zlib, cmake)"
    make test

    msg "test: ssl-opt.sh (zlib, cmake)"
    tests/ssl-opt.sh
}
support_test_zlib_cmake () {
    support_test_zlib_make "$@"
}

component_test_ref_configs () {
    msg "test/build: ref-configs (ASan build)" # ~ 6 min 20s
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    tests/scripts/test-ref-configs.pl
}

component_test_sslv3 () {
    msg "build: Default + SSLv3 (ASan build)" # ~ 6 min
    scripts/config.pl set MBEDTLS_SSL_PROTO_SSL3
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: SSLv3 - main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "build: SSLv3 - compat.sh (ASan build)" # ~ 6 min
    tests/compat.sh -m 'tls1 tls1_1 tls1_2 dtls1 dtls1_2'
    env OPENSSL_CMD="$OPENSSL_LEGACY" tests/compat.sh -m 'ssl3'

    msg "build: SSLv3 - ssl-opt.sh (ASan build)" # ~ 6 min
    tests/ssl-opt.sh
}

component_test_no_renegotiation () {
    msg "build: Default + !MBEDTLS_SSL_RENEGOTIATION (ASan build)" # ~ 6 min
    scripts/config.pl unset MBEDTLS_SSL_RENEGOTIATION
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: !MBEDTLS_SSL_RENEGOTIATION - main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "test: !MBEDTLS_SSL_RENEGOTIATION - ssl-opt.sh (ASan build)" # ~ 6 min
    tests/ssl-opt.sh
}

component_test_rsa_no_crt () {
    msg "build: Default + RSA_NO_CRT (ASan build)" # ~ 6 min
    scripts/config.pl set MBEDTLS_RSA_NO_CRT
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: RSA_NO_CRT - main suites (inc. selftests) (ASan build)" # ~ 50s
    make test

    msg "test: RSA_NO_CRT - RSA-related part of ssl-opt.sh (ASan build)" # ~ 5s
    tests/ssl-opt.sh -f RSA

    msg "test: RSA_NO_CRT - RSA-related part of compat.sh (ASan build)" # ~ 3 min
    tests/compat.sh -t RSA
}

component_test_full_cmake_clang () {
    msg "build: cmake, full config, clang" # ~ 50s
    scripts/config.pl full
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=Check -D ENABLE_TESTING=On .
    make

    msg "test: main suites (full config)" # ~ 5s
    make test

    msg "test: ssl-opt.sh default (full config)" # ~ 1s
    tests/ssl-opt.sh -f Default

    msg "test: compat.sh RC4, DES, 3DES & NULL (full config)" # ~ 2 min
    env OPENSSL_CMD="$OPENSSL_LEGACY" GNUTLS_CLI="$GNUTLS_LEGACY_CLI" GNUTLS_SERV="$GNUTLS_LEGACY_SERV" tests/compat.sh -e '^$' -f 'NULL\|DES\|RC4\|ARCFOUR'
}

component_build_deprecated () {
    msg "build: make, full config + DEPRECATED_WARNING, gcc -O" # ~ 30s
    scripts/config.pl full
    scripts/config.pl set MBEDTLS_DEPRECATED_WARNING
    # Build with -O -Wextra to catch a maximum of issues.
    make CC=gcc CFLAGS='-O -Werror -Wall -Wextra' lib programs
    make CC=gcc CFLAGS='-O -Werror -Wall -Wextra -Wno-unused-function' tests

    msg "build: make, full config + DEPRECATED_REMOVED, clang -O" # ~ 30s
    # No cleanup, just tweak the configuration and rebuild
    make clean
    scripts/config.pl unset MBEDTLS_DEPRECATED_WARNING
    scripts/config.pl set MBEDTLS_DEPRECATED_REMOVED
    # Build with -O -Wextra to catch a maximum of issues.
    make CC=clang CFLAGS='-O -Werror -Wall -Wextra' lib programs
    make CC=clang CFLAGS='-O -Werror -Wall -Wextra -Wno-unused-function' tests
}

component_test_depends_curves () {
    msg "test/build: curves.pl (gcc)" # ~ 4 min
    tests/scripts/curves.pl
}

component_test_depends_hashes () {
    msg "test/build: depends-hashes.pl (gcc)" # ~ 2 min
    tests/scripts/depends-hashes.pl
}

component_test_depends_pkalgs () {
    msg "test/build: depends-pkalgs.pl (gcc)" # ~ 2 min
    tests/scripts/depends-pkalgs.pl
}

component_build_key_exchanges () {
    msg "test/build: key-exchanges (gcc)" # ~ 1 min
    tests/scripts/key-exchanges.pl
}

component_build_default_make_gcc () {
    msg "build: Unix make, -Os (gcc)" # ~ 30s
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -Os'
}

component_test_no_platform () {
    # Full configuration build, without platform support, file IO and net sockets.
    # This should catch missing mbedtls_printf definitions, and by disabling file
    # IO, it should catch missing '#include <stdio.h>'
    msg "build: full config except platform/fsio/net, make, gcc, C99" # ~ 30s
    scripts/config.pl full
    scripts/config.pl unset MBEDTLS_PLATFORM_C
    scripts/config.pl unset MBEDTLS_NET_C
    scripts/config.pl unset MBEDTLS_PLATFORM_MEMORY
    scripts/config.pl unset MBEDTLS_PLATFORM_PRINTF_ALT
    scripts/config.pl unset MBEDTLS_PLATFORM_FPRINTF_ALT
    scripts/config.pl unset MBEDTLS_PLATFORM_SNPRINTF_ALT
    scripts/config.pl unset MBEDTLS_PLATFORM_TIME_ALT
    scripts/config.pl unset MBEDTLS_PLATFORM_EXIT_ALT
    scripts/config.pl unset MBEDTLS_ENTROPY_NV_SEED
    scripts/config.pl unset MBEDTLS_FS_IO
    # Note, _DEFAULT_SOURCE needs to be defined for platforms using glibc version >2.19,
    # to re-enable platform integration features otherwise disabled in C99 builds
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -std=c99 -pedantic -Os -D_DEFAULT_SOURCE' lib programs
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -Os' test
}

component_build_no_std_function () {
    # catch compile bugs in _uninit functions
    msg "build: full config with NO_STD_FUNCTION, make, gcc" # ~ 30s
    scripts/config.pl full
    scripts/config.pl set MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
    scripts/config.pl unset MBEDTLS_ENTROPY_NV_SEED
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -Os'
}

component_build_no_ssl_srv () {
    msg "build: full config except ssl_srv.c, make, gcc" # ~ 30s
    scripts/config.pl full
    scripts/config.pl unset MBEDTLS_SSL_SRV_C
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -O1'
}

component_build_no_ssl_cli () {
    msg "build: full config except ssl_cli.c, make, gcc" # ~ 30s
    scripts/config.pl full
    scripts/config.pl unset MBEDTLS_SSL_CLI_C
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -O1'
}

component_build_no_sockets () {
    # Note, C99 compliance can also be tested with the sockets support disabled,
    # as that requires a POSIX platform (which isn't the same as C99).
    msg "build: full config except net_sockets.c, make, gcc -std=c99 -pedantic" # ~ 30s
    scripts/config.pl full
    scripts/config.pl unset MBEDTLS_NET_C # getaddrinfo() undeclared, etc.
    scripts/config.pl set MBEDTLS_NO_PLATFORM_ENTROPY # uses syscall() on GNU/Linux
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -O1 -std=c99 -pedantic' lib
}

component_test_memory_buffer_allocator_backtrace () {
    msg "build: default config with memory buffer allocator and backtrace enabled"
    scripts/config.pl set MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.pl set MBEDTLS_PLATFORM_MEMORY
    scripts/config.pl set MBEDTLS_MEMORY_BACKTRACE
    scripts/config.pl set MBEDTLS_MEMORY_DEBUG
    CC=gcc cmake .
    make

    msg "test: MBEDTLS_MEMORY_BUFFER_ALLOC_C and MBEDTLS_MEMORY_BACKTRACE"
    make test
}

component_test_memory_buffer_allocator () {
    msg "build: default config with memory buffer allocator"
    scripts/config.pl set MBEDTLS_MEMORY_BUFFER_ALLOC_C
    scripts/config.pl set MBEDTLS_PLATFORM_MEMORY
    CC=gcc cmake .
    make

    msg "test: MBEDTLS_MEMORY_BUFFER_ALLOC_C"
    make test

    msg "test: ssl-opt.sh, MBEDTLS_MEMORY_BUFFER_ALLOC_C"
    # MBEDTLS_MEMORY_BUFFER_ALLOC is slow. Skip tests that tend to time out.
    tests/ssl-opt.sh -e '^DTLS proxy'
}

component_test_no_max_fragment_length () {
    msg "build: default config except MFL extension (ASan build)" # ~ 30s
    scripts/config.pl unset MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: ssl-opt.sh, MFL-related tests"
    tests/ssl-opt.sh -f "Max fragment length"
}

component_test_null_entropy () {
    msg "build: default config with  MBEDTLS_TEST_NULL_ENTROPY (ASan build)"
    scripts/config.pl set MBEDTLS_TEST_NULL_ENTROPY
    scripts/config.pl set MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
    scripts/config.pl set MBEDTLS_ENTROPY_C
    scripts/config.pl unset MBEDTLS_ENTROPY_NV_SEED
    scripts/config.pl unset MBEDTLS_ENTROPY_HARDWARE_ALT
    scripts/config.pl unset MBEDTLS_HAVEGE_C
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan -D UNSAFE_BUILD=ON .
    make

    msg "test: MBEDTLS_TEST_NULL_ENTROPY - main suites (inc. selftests) (ASan build)"
    make test
}

component_test_platform_calloc_macro () {
    msg "build: MBEDTLS_PLATFORM_{CALLOC/FREE}_MACRO enabled (ASan build)"
    scripts/config.pl set MBEDTLS_PLATFORM_MEMORY
    scripts/config.pl set MBEDTLS_PLATFORM_CALLOC_MACRO calloc
    scripts/config.pl set MBEDTLS_PLATFORM_FREE_MACRO   free
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MBEDTLS_PLATFORM_{CALLOC/FREE}_MACRO enabled (ASan build)"
    make test
}

component_test_malloc_0_null () {
    msg "build: malloc(0) returns NULL (ASan+UBSan build)"
    scripts/config.pl full
    scripts/config.pl unset MBEDTLS_MEMORY_BUFFER_ALLOC_C
    make CC=gcc CFLAGS="'-DMBEDTLS_CONFIG_FILE=\"$PWD/tests/configs/config-wrapper-malloc-0-null.h\"' -O $ASAN_CFLAGS" LDFLAGS="$ASAN_CFLAGS"

    msg "test: malloc(0) returns NULL (ASan+UBSan build)"
    make test

    msg "selftest: malloc(0) returns NULL (ASan+UBSan build)"
    # Just the calloc selftest. "make test" ran the others as part of the
    # test suites.
    programs/test/selftest calloc
}

component_test_make_shared () {
    msg "build/test: make shared" # ~ 40s
    make SHARED=1 all check
    ldd programs/util/strerror | grep libmbedcrypto
}

component_test_cmake_shared () {
    msg "build/test: cmake shared" # ~ 2min
    cmake -DUSE_SHARED_MBEDTLS_LIBRARY=On .
    make
    ldd programs/util/strerror | grep libmbedcrypto
    make test
}

test_build_opt () {
    info=$1 cc=$2; shift 2
    for opt in "$@"; do
          msg "build/test: $cc $opt, $info" # ~ 30s
          make CC="$cc" CFLAGS="$opt -Wall -Wextra -Werror"
          # We're confident enough in compilers to not run _all_ the tests,
          # but at least run the unit tests. In particular, runs with
          # optimizations use inline assembly whereas runs with -O0
          # skip inline assembly.
          make test # ~30s
          make clean
    done
}

component_test_clang_opt () {
    scripts/config.pl full
    test_build_opt 'full config' clang -O0 -Os -O2
}

component_test_gcc_opt () {
    scripts/config.pl full
    test_build_opt 'full config' gcc -O0 -Os -O2
}

component_build_mbedtls_config_file () {
    msg "build: make with MBEDTLS_CONFIG_FILE" # ~40s
    # Use the full config so as to catch a maximum of places where
    # the check of MBEDTLS_CONFIG_FILE might be missing.
    scripts/config.pl full
    sed 's!"check_config.h"!"mbedtls/check_config.h"!' <"$CONFIG_H" >full_config.h
    echo '#error "MBEDTLS_CONFIG_FILE is not working"' >"$CONFIG_H"
    make CFLAGS="-I '$PWD' -DMBEDTLS_CONFIG_FILE='\"full_config.h\"'"
    rm -f full_config.h
}

component_test_m32_o0 () {
    # Build once with -O0, to compile out the i386 specific inline assembly
    msg "build: i386, make, gcc -O0 (ASan build)" # ~ 30s
    scripts/config.pl full
    make CC=gcc CFLAGS="$ASAN_CFLAGS -m32 -O0" LDFLAGS="-m32 $ASAN_CFLAGS"

    msg "test: i386, make, gcc -O0 (ASan build)"
    make test
}
support_test_m32_o0 () {
    case $(uname -m) in
        *64*) true;;
        *) false;;
    esac
}

component_test_m32_o1 () {
    # Build again with -O1, to compile in the i386 specific inline assembly
    msg "build: i386, make, gcc -O1 (ASan build)" # ~ 30s
    scripts/config.pl full
    make CC=gcc CFLAGS="$ASAN_CFLAGS -m32 -O1" LDFLAGS="-m32 $ASAN_CFLAGS"

    msg "test: i386, make, gcc -O1 (ASan build)"
    make test

    msg "test ssl-opt.sh, i386, make, gcc-O1"
    tests/ssl-opt.sh
}
support_test_m32_o1 () {
    support_test_m32_o0 "$@"
}

component_test_mx32 () {
    msg "build: 64-bit ILP32, make, gcc" # ~ 30s
    scripts/config.pl full
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -mx32' LDFLAGS='-mx32'

    msg "test: 64-bit ILP32, make, gcc"
    make test
}
support_test_mx32 () {
    case $(uname -m) in
        amd64|x86_64) true;;
        *) false;;
    esac
}

component_test_min_mpi_window_size () {
    msg "build: Default + MBEDTLS_MPI_WINDOW_SIZE=1 (ASan build)" # ~ 10s
    scripts/config.pl set MBEDTLS_MPI_WINDOW_SIZE 1
    CC=gcc cmake -D CMAKE_BUILD_TYPE:String=Asan .
    make

    msg "test: MBEDTLS_MPI_WINDOW_SIZE=1 - main suites (inc. selftests) (ASan build)" # ~ 10s
    make test
}

component_test_have_int32 () {
    msg "build: gcc, force 32-bit bignum limbs"
    scripts/config.pl unset MBEDTLS_HAVE_ASM
    scripts/config.pl unset MBEDTLS_AESNI_C
    scripts/config.pl unset MBEDTLS_PADLOCK_C
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -DMBEDTLS_HAVE_INT32'

    msg "test: gcc, force 32-bit bignum limbs"
    make test
}

component_test_have_int64 () {
    msg "build: gcc, force 64-bit bignum limbs"
    scripts/config.pl unset MBEDTLS_HAVE_ASM
    scripts/config.pl unset MBEDTLS_AESNI_C
    scripts/config.pl unset MBEDTLS_PADLOCK_C
    make CC=gcc CFLAGS='-Werror -Wall -Wextra -DMBEDTLS_HAVE_INT64'

    msg "test: gcc, force 64-bit bignum limbs"
    make test
}

component_build_arm_none_eabi_gcc () {
    msg "build: arm-none-eabi-gcc, make" # ~ 10s
    scripts/config.pl baremetal
    make CC=arm-none-eabi-gcc AR=arm-none-eabi-ar LD=arm-none-eabi-ld CFLAGS='-Werror -Wall -Wextra' lib
}

component_build_arm_none_eabi_gcc_no_udbl_division () {
    msg "build: arm-none-eabi-gcc -DMBEDTLS_NO_UDBL_DIVISION, make" # ~ 10s
    scripts/config.pl baremetal
    scripts/config.pl set MBEDTLS_NO_UDBL_DIVISION
    make CC=arm-none-eabi-gcc AR=arm-none-eabi-ar LD=arm-none-eabi-ld CFLAGS='-Werror -Wall -Wextra' lib
    echo "Checking that software 64-bit division is not required"
    not grep __aeabi_uldiv library/*.o
}

component_build_armcc () {
    msg "build: ARM Compiler 5, make"
    scripts/config.pl baremetal

    make CC="$ARMC5_CC" AR="$ARMC5_AR" WARNING_CFLAGS='--strict --c99' lib
    make clean

    # ARM Compiler 6 - Target ARMv7-A
    armc6_build_test "--target=arm-arm-none-eabi -march=armv7-a"

    # ARM Compiler 6 - Target ARMv7-M
    armc6_build_test "--target=arm-arm-none-eabi -march=armv7-m"

    # ARM Compiler 6 - Target ARMv8-A - AArch32
    armc6_build_test "--target=arm-arm-none-eabi -march=armv8.2-a"

    # ARM Compiler 6 - Target ARMv8-M
    armc6_build_test "--target=arm-arm-none-eabi -march=armv8-m.main"

    # ARM Compiler 6 - Target ARMv8-A - AArch64
    armc6_build_test "--target=aarch64-arm-none-eabi -march=armv8.2-a"
}

component_build_ssl_hw_record_accel() {
    msg "build: default config with MBEDTLS_SSL_HW_RECORD_ACCEL enabled"
    scripts/config.pl set MBEDTLS_SSL_HW_RECORD_ACCEL
    make CFLAGS='-Werror -O1'
}

component_test_allow_sha1 () {
    msg "build: allow SHA1 in certificates by default"
    scripts/config.pl set MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES
    make CFLAGS='-Werror -Wall -Wextra'
    msg "test: allow SHA1 in certificates by default"
    make test
    tests/ssl-opt.sh -f SHA-1
}

component_build_mingw () {
    msg "build: Windows cross build - mingw64, make (Link Library)" # ~ 30s
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror -Wall -Wextra' WINDOWS_BUILD=1 lib programs

    # note Make tests only builds the tests, but doesn't run them
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror' WINDOWS_BUILD=1 tests
    make WINDOWS_BUILD=1 clean

    msg "build: Windows cross build - mingw64, make (DLL)" # ~ 30s
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror -Wall -Wextra' WINDOWS_BUILD=1 SHARED=1 lib programs
    make CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar LD=i686-w64-minggw32-ld CFLAGS='-Werror -Wall -Wextra' WINDOWS_BUILD=1 SHARED=1 tests
    make WINDOWS_BUILD=1 clean
}

component_test_memsan () {
    msg "build: MSan (clang)" # ~ 1 min 20s
    scripts/config.pl unset MBEDTLS_AESNI_C # memsan doesn't grok asm
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=MemSan .
    make

    msg "test: main suites (MSan)" # ~ 10s
    make test

    msg "test: ssl-opt.sh (MSan)" # ~ 1 min
    tests/ssl-opt.sh

    # Optional part(s)

    if [ "$MEMORY" -gt 0 ]; then
        msg "test: compat.sh (MSan)" # ~ 6 min 20s
        tests/compat.sh
    fi
}

component_test_valgrind () {
    msg "build: Release (clang)"
    CC=clang cmake -D CMAKE_BUILD_TYPE:String=Release .
    make

    msg "test: main suites valgrind (Release)"
    make memcheck

    # Optional parts (slow; currently broken on OS X because programs don't
    # seem to receive signals under valgrind on OS X).
    if [ "$MEMORY" -gt 0 ]; then
        msg "test: ssl-opt.sh --memcheck (Release)"
        tests/ssl-opt.sh --memcheck
    fi

    if [ "$MEMORY" -gt 1 ]; then
        msg "test: compat.sh --memcheck (Release)"
        tests/compat.sh --memcheck
    fi
}

component_test_cmake_out_of_source () {
    msg "build: cmake 'out-of-source' build"
    MBEDTLS_ROOT_DIR="$PWD"
    mkdir "$OUT_OF_SOURCE_DIR"
    cd "$OUT_OF_SOURCE_DIR"
    cmake "$MBEDTLS_ROOT_DIR"
    make

    msg "test: cmake 'out-of-source' build"
    make test
    # Test an SSL option that requires an auxiliary script in test/scripts/.
    # Also ensure that there are no error messages such as
    # "No such file or directory", which would indicate that some required
    # file is missing (ssl-opt.sh tolerates the absence of some files so
    # may exit with status 0 but emit errors).
    ./tests/ssl-opt.sh -f 'Fallback SCSV: beginning of list' 2>ssl-opt.err
    cat ssl-opt.err >&2
    # If ssl-opt.err is non-empty, record an error and keep going.
    [ ! -s ssl-opt.err ]
    rm ssl-opt.err
    cd "$MBEDTLS_ROOT_DIR"
    rm -rf "$OUT_OF_SOURCE_DIR"
}



################################################################
#### Termination
################################################################

post_report () {
    msg "Done, cleaning up"
    cleanup

    final_report
}



################################################################
#### Run all the things
################################################################

# Function invoked by --error-test to test error reporting.
pseudo_component_error_test () {
    msg "Testing error reporting $error_test"
    if [ $KEEP_GOING -ne 0 ]; then
        echo "Expect three failing commands."
    fi
    error_test='this should not be used since the component runs in a subshell'
    grep non_existent /dev/null
    not grep -q . "$0"
    make unknown_target
    false "this should not be executed"
}

# Run one component and clean up afterwards.
run_component () {
    # Back up the configuration in case the component modifies it.
    # The cleanup function will restore it.
    cp -p "$CONFIG_H" "$CONFIG_BAK"
    current_component="$1"

    if [ $KEEP_GOING -eq 1 ]; then
        set +e
    fi
    # Run the component in a subshell
    (
        if [ $KEEP_GOING -eq 1 ]; then
            # Keep "set -e" off, and run an ERR trap instead to record failures.
            set -E
            trap err_trap ERR
        fi
        "$@"
        if [ $KEEP_GOING -eq 1 ]; then
            trap - ERR
            exit $last_failure_status
        fi
    )
    component_status=$?
    if [ $KEEP_GOING -eq 1 ]; then
        set -e
        if [ $component_status -ne 0 ]; then
            failure_count=$((failure_count + 1))
        fi
    fi
    cleanup
}

# Preliminary setup
pre_check_environment
pre_initialize_variables
pre_parse_command_line "$@"

pre_check_git
build_status=0
if [ $KEEP_GOING -eq 1 ]; then
    pre_setup_keep_going
fi
pre_print_configuration
pre_check_tools
cleanup

# Run the requested tests.
while [ $error_test -gt 0 ]; do
    run_component pseudo_component_error_test
    error_test=$((error_test - 1))
done
for component in $RUN_COMPONENTS; do
    run_component "component_$component"
done

# We're done.
post_report
