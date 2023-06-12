/**
 *  Constant-time functions
 *
 *  For readability, the static inline definitions are here, and
 *  constant_time_internal.h has only the declarations.
 *
 *  This results in duplicate declarations of the form:
 *      static inline void f() { ... }
 *      static inline void f();
 *  when constant_time_internal.h is included. This appears to behave
 *  exactly as if the declaration-without-definition was not present.
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef MBEDTLS_CONSTANT_TIME_IMPL_H
#define MBEDTLS_CONSTANT_TIME_IMPL_H

#include <stddef.h>

#include "common.h"

#if defined(MBEDTLS_BIGNUM_C)
#include "mbedtls/bignum.h"
#endif

/* constant_time_impl.h contains all the static inline implementations,
 * so that constant_time_internal.h is more readable.
 *
 * gcc generates warnings about duplicate declarations, so disable this
 * warning.
 */
#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wredundant-decls"
#endif

/* Disable asm under Memsan because it confuses Memsan and generates false errors */
#if defined(MBEDTLS_TEST_CONSTANT_FLOW_MEMSAN)
#define MBEDTLS_CT_NO_ASM
#elif defined(__has_feature)
#if __has_feature(memory_sanitizer)
#define MBEDTLS_CT_NO_ASM
#endif
#endif

/* armcc5 --gnu defines __GNUC__ but doesn't support GNU's extended asm */
#if defined(MBEDTLS_HAVE_ASM) && defined(__GNUC__) && (!defined(__ARMCC_VERSION) || \
    __ARMCC_VERSION >= 6000000) && !defined(MBEDTLS_CT_NO_ASM)
#define MBEDTLS_CT_ASM
#if (defined(__arm__) || defined(__thumb__) || defined(__thumb2__))
#define MBEDTLS_CT_ARM_ASM
#elif defined(__aarch64__)
#define MBEDTLS_CT_AARCH64_ASM
#endif
#endif

#define MBEDTLS_CT_SIZE (sizeof(mbedtls_ct_uint_t) * 8)


/* ============================================================================
 * Core const-time primitives
 */

/* Ensure that the compiler cannot know the value of x (i.e., cannot optimise
 * based on its value) after this function is called.
 *
 * If we are not using assembly, this will be fairly inefficient, so its use
 * should be minimised.
 */

#if !defined(MBEDTLS_CT_ASM)
extern volatile mbedtls_ct_uint_t mbedtls_ct_zero;
#endif

/**
 * \brief   Ensure that a value cannot be known at compile time.
 *
 * \param x        The value to hide from the compiler.
 * \return         The same value that was passed in, such that the compiler
 *                 cannot prove its value (even for calls of the form
 *                 x = mbedtls_ct_compiler_opaque(1), x will be unknown).
 *
 * \note           This is mainly used in constructing mbedtls_ct_condition_t
 *                 values and performing operations over them, to ensure that
 *                 there is no way for the compiler to ever know anything about
 *                 the value of an mbedtls_ct_condition_t.
 */
static inline mbedtls_ct_uint_t mbedtls_ct_compiler_opaque(mbedtls_ct_uint_t x)
{
#if defined(MBEDTLS_CT_ASM)
    asm volatile ("" : [x] "+r" (x) :);
    return x;
#else
    return x ^ mbedtls_ct_zero;
#endif
}

/* Convert a number into a condition in constant time. */
static inline mbedtls_ct_condition_t mbedtls_ct_bool(mbedtls_ct_uint_t x)
{
    /*
     * Define mask-generation code that, as far as possible, will not use branches or conditional instructions.
     *
     * For some platforms / type sizes, we define assembly to assure this.
     *
     * Otherwise, we define a plain C fallback which (in May 2023) does not get optimised into
     * conditional instructions or branches by trunk clang, gcc, or MSVC v19.
     */
#if defined(MBEDTLS_CT_AARCH64_ASM) && (defined(MBEDTLS_CT_SIZE_32) || defined(MBEDTLS_CT_SIZE_64))
    mbedtls_ct_uint_t s;
    asm volatile ("neg %x[s], %x[x]                     \n\t"
                  "orr %x[x], %x[s], %x[x]              \n\t"
                  "asr %x[x], %x[x], 63"
                  :
                  [s] "=&r" (s),
                  [x] "+&r" (x)
                  :
                  :
                  );
    return (mbedtls_ct_condition_t) x;
#elif defined(MBEDTLS_CT_ARM_ASM) && defined(MBEDTLS_CT_SIZE_32)
    uint32_t s;
    /*
     * Selecting unified syntax is needed for gcc, and harmless on clang.
     *
     * This is needed because on Thumb 1, condition flags are always set, so
     * e.g. "negs" is supported but "neg" is not (on Thumb 2, both exist).
     *
     * Under Thumb 1 unified syntax, only the "negs" form is accepted, and
     * under divided syntax, only the "neg" form is accepted. clang only
     * supports unified syntax.
     *
     * On Thumb 2 and Arm, both compilers are happy with the "s" suffix,
     * although we don't actually care about setting the flags.
     *
     * For gcc, restore divided syntax afterwards - otherwise old versions of gcc
     * seem to apply unified syntax globally, which breaks other asm code.
     */
#if !defined(__clang__)
#define RESTORE_ASM_SYNTAX  ".syntax divided             \n\t"
#else
#define RESTORE_ASM_SYNTAX
#endif

    asm volatile (".syntax unified                       \n\t"
                  "negs %[s], %[x]                       \n\t"
                  "orrs %[x], %[x], %[s]                 \n\t"
                  "asrs %[x], %[x], #31                  \n\t"
                  RESTORE_ASM_SYNTAX
                  :
                  [s] "=&l" (s),
                  [x] "+&l" (x)
                  :
                  :
                  "cc" /* clobbers flag bits */
                  );
    return (mbedtls_ct_condition_t) x;
#else
    const mbedtls_ct_uint_t xo = mbedtls_ct_compiler_opaque(x);
#if defined(_MSC_VER)
    /* MSVC has a warning about unary minus on unsigned, but this is
     * well-defined and precisely what we want to do here */
#pragma warning( push )
#pragma warning( disable : 4146 )
#endif
    return (mbedtls_ct_condition_t) (((mbedtls_ct_int_t) ((-xo) | -(xo >> 1))) >>
                                     (MBEDTLS_CT_SIZE - 1));
#if defined(_MSC_VER)
#pragma warning( pop )
#endif
#endif
}

static inline mbedtls_ct_uint_t mbedtls_ct_if(mbedtls_ct_condition_t condition,
                                              mbedtls_ct_uint_t if1,
                                              mbedtls_ct_uint_t if0)
{
#if defined(MBEDTLS_CT_AARCH64_ASM) && (defined(MBEDTLS_CT_SIZE_32) || defined(MBEDTLS_CT_SIZE_64))
    asm volatile ("and %x[if1], %x[if1], %x[condition]       \n\t"
                  "mvn %x[condition], %x[condition]          \n\t"
                  "and %x[condition], %x[condition], %x[if0] \n\t"
                  "orr %x[condition], %x[if1], %x[condition]"
                  :
                  [condition] "+&r" (condition),
                  [if1] "+&r" (if1)
                  :
                  [if0] "r" (if0)
                  :
                  );
    return (mbedtls_ct_uint_t) condition;
#elif defined(MBEDTLS_CT_ARM_ASM) && defined(MBEDTLS_CT_SIZE_32)
    asm volatile (".syntax unified                           \n\t"
                  "ands %[if1], %[if1], %[condition]         \n\t"
                  "mvns %[condition], %[condition]           \n\t"
                  "ands %[condition], %[condition], %[if0]   \n\t"
                  "orrs %[condition], %[if1], %[condition]   \n\t"
                  RESTORE_ASM_SYNTAX
                  :
                  [condition] "+&l" (condition),
                  [if1] "+&l" (if1)
                  :
                  [if0] "l" (if0)
                  :
                  "cc"
                  );
    return (mbedtls_ct_uint_t) condition;
#else
    mbedtls_ct_condition_t not_cond =
        (mbedtls_ct_condition_t) (~mbedtls_ct_compiler_opaque(condition));
    return (mbedtls_ct_uint_t) ((condition & if1) | (not_cond & if0));
#endif
}

static inline mbedtls_ct_condition_t mbedtls_ct_uint_lt(mbedtls_ct_uint_t x, mbedtls_ct_uint_t y)
{
#if defined(MBEDTLS_CT_AARCH64_ASM) && (defined(MBEDTLS_CT_SIZE_32) || defined(MBEDTLS_CT_SIZE_64))
    uint64_t s1, s2;
    asm volatile ("eor     %x[s1], %x[y], %x[x]          \n\t"
                  "sub     %x[s2], %x[x], %x[y]          \n\t"
                  "bic     %x[s2], %x[s2], %[s1]         \n\t"
                  "and     %x[s1], %x[s1], %x[y]         \n\t"
                  "orr     %x[s1], %x[s2], %x[s1]        \n\t"
                  "asr     %x[x], %x[s1], 63"
                  : [s1] "=&r" (s1), [s2] "=&r" (s2), [x] "+r" (x)
                  : [y] "r" (y)
                  :
                  );
    return (mbedtls_ct_condition_t) x;
#elif defined(MBEDTLS_CT_ARM_ASM) && defined(MBEDTLS_CT_SIZE_32)
    uint32_t s1;
    asm volatile (
        ".syntax unified                    \n\t"
#if defined(__thumb__) && !defined(__thumb2__)
        "movs     %[s1], %[x]               \n\t"
        "eors     %[s1], %[s1], %[y]        \n\t"
#else
        "eors     %[s1], %[x], %[y]         \n\t"
#endif
        "subs    %[x], %[x], %[y]           \n\t"
        "bics    %[x], %[x], %[s1]          \n\t"
        "ands    %[y], %[s1], %[y]          \n\t"
        "orrs    %[x], %[x], %[y]           \n\t"
        "asrs    %[x], %[x], #31            \n\t"
        RESTORE_ASM_SYNTAX
        : [s1] "=&l" (s1), [x] "+&l" (x),  [y] "+&l" (y)
        :
        :
        "cc"
        );
    return (mbedtls_ct_condition_t) x;
#else
    /* Ensure that the compiler cannot optimise the following operations over x and y,
     * even if it knows the value of x and y.
     */
    const mbedtls_ct_uint_t xo = mbedtls_ct_compiler_opaque(x);
    const mbedtls_ct_uint_t yo = mbedtls_ct_compiler_opaque(y);
    /*
     * Check if the most significant bits (MSB) of the operands are different.
     * cond is true iff the MSBs differ.
     */
    mbedtls_ct_condition_t cond = mbedtls_ct_bool((xo ^ yo) >> (MBEDTLS_CT_SIZE - 1));

    /*
     * If the MSB are the same then the difference x-y will be negative (and
     * have its MSB set to 1 during conversion to unsigned) if and only if x<y.
     *
     * If the MSB are different, then the operand with the MSB of 1 is the
     * bigger. (That is if y has MSB of 1, then x<y is true and it is false if
     * the MSB of y is 0.)
     */

    // Select either y, or x - y
    mbedtls_ct_uint_t ret = mbedtls_ct_if(cond, yo, (mbedtls_ct_uint_t) (xo - yo));

    // Extract only the MSB of ret
    ret = ret >> (MBEDTLS_CT_SIZE - 1);

    // Convert to a condition (i.e., all bits set iff non-zero)
    return mbedtls_ct_bool(ret);
#endif
}

static inline mbedtls_ct_condition_t mbedtls_ct_uint_ne(mbedtls_ct_uint_t x, mbedtls_ct_uint_t y)
{
    /* diff = 0 if x == y, non-zero otherwise */
    const mbedtls_ct_uint_t diff = mbedtls_ct_compiler_opaque(x) ^ mbedtls_ct_compiler_opaque(y);

    /* all ones if x != y, 0 otherwise */
    return mbedtls_ct_bool(diff);
}

static inline unsigned char mbedtls_ct_uchar_in_range_if(unsigned char low,
                                                         unsigned char high,
                                                         unsigned char c,
                                                         unsigned char t)
{
    const unsigned char co = (const unsigned char) mbedtls_ct_compiler_opaque(c);
    const unsigned char to = (const unsigned char) mbedtls_ct_compiler_opaque(t);

    /* low_mask is: 0 if low <= c, 0x...ff if low > c */
    unsigned low_mask = ((unsigned) co - low) >> 8;
    /* high_mask is: 0 if c <= high, 0x...ff if c > high */
    unsigned high_mask = ((unsigned) high - co) >> 8;

    return (unsigned char) (~(low_mask | high_mask)) & to;
}


/* ============================================================================
 * Everything below here is trivial wrapper functions
 */

static inline size_t mbedtls_ct_size_if(mbedtls_ct_condition_t condition,
                                        size_t if1,
                                        size_t if0)
{
    return (size_t) mbedtls_ct_if(condition, (mbedtls_ct_uint_t) if1, (mbedtls_ct_uint_t) if0);
}

static inline unsigned mbedtls_ct_uint_if(mbedtls_ct_condition_t condition,
                                          unsigned if1,
                                          unsigned if0)
{
    return (unsigned) mbedtls_ct_if(condition, (mbedtls_ct_uint_t) if1, (mbedtls_ct_uint_t) if0);
}

#if defined(MBEDTLS_BIGNUM_C)

static inline mbedtls_mpi_uint mbedtls_ct_mpi_uint_if(mbedtls_ct_condition_t condition,
                                                      mbedtls_mpi_uint if1,
                                                      mbedtls_mpi_uint if0)
{
    return (mbedtls_mpi_uint) mbedtls_ct_if(condition,
                                            (mbedtls_ct_uint_t) if1,
                                            (mbedtls_ct_uint_t) if0);
}

#endif

static inline size_t mbedtls_ct_size_if_else_0(mbedtls_ct_condition_t condition, size_t if1)
{
    return (size_t) (condition & if1);
}

static inline unsigned mbedtls_ct_uint_if_else_0(mbedtls_ct_condition_t condition, unsigned if1)
{
    return (unsigned) (condition & if1);
}

#if defined(MBEDTLS_BIGNUM_C)

static inline mbedtls_mpi_uint mbedtls_ct_mpi_uint_if_else_0(mbedtls_ct_condition_t condition,
                                                             mbedtls_mpi_uint if1)
{
    return (mbedtls_mpi_uint) (condition & if1);
}

#endif /* MBEDTLS_BIGNUM_C */

static inline mbedtls_ct_condition_t mbedtls_ct_uint_eq(mbedtls_ct_uint_t x,
                                                        mbedtls_ct_uint_t y)
{
    return ~mbedtls_ct_uint_ne(x, y);
}

static inline mbedtls_ct_condition_t mbedtls_ct_uint_gt(mbedtls_ct_uint_t x,
                                                        mbedtls_ct_uint_t y)
{
    return mbedtls_ct_uint_lt(y, x);
}

static inline mbedtls_ct_condition_t mbedtls_ct_uint_ge(mbedtls_ct_uint_t x,
                                                        mbedtls_ct_uint_t y)
{
    return ~mbedtls_ct_uint_lt(x, y);
}

static inline mbedtls_ct_condition_t mbedtls_ct_uint_le(mbedtls_ct_uint_t x,
                                                        mbedtls_ct_uint_t y)
{
    return ~mbedtls_ct_uint_gt(x, y);
}

static inline mbedtls_ct_condition_t mbedtls_ct_bool_xor(mbedtls_ct_condition_t x,
                                                         mbedtls_ct_condition_t y)
{
    return (mbedtls_ct_condition_t) (x ^ y);
}

static inline mbedtls_ct_condition_t mbedtls_ct_bool_and(mbedtls_ct_condition_t x,
                                                         mbedtls_ct_condition_t y)
{
    return (mbedtls_ct_condition_t) (x & y);
}

static inline mbedtls_ct_condition_t mbedtls_ct_bool_or(mbedtls_ct_condition_t x,
                                                        mbedtls_ct_condition_t y)
{
    return (mbedtls_ct_condition_t) (x | y);
}

static inline mbedtls_ct_condition_t mbedtls_ct_bool_not(mbedtls_ct_condition_t x)
{
    return (mbedtls_ct_condition_t) (~x);
}

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

#endif /* MBEDTLS_CONSTANT_TIME_IMPL_H */