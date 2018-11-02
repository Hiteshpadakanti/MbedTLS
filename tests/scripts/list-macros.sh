#!/bin/sh

set -eu

if [ -d include/mbedtls ]; then :; else
    echo "$0: must be run from root" >&2
    exit 1
fi

HEADERS=$( ls include/mbedtls/*.h crypto/include/mbedtls/*.h | egrep -v 'compat-1\.3\.h' )
HEADERS="$HEADERS
$( ls library/*.h )"

sed -n -e 's/.*#define \([a-zA-Z0-9_]*\).*/\1/p' $HEADERS \
    | egrep -v '^(asm|inline|EMIT|_CRT_SECURE_NO_DEPRECATE)$|^MULADDC_' \
    | sort -u > macros

wc -l macros
