/**
 * \file aesni.h
 *
 * \brief AES-NI for hardware AES acceleration on some Intel processors
 *
 * \warning These functions are only for internal use by other library
 *          functions; you must not call them directly.
 */
/*
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
#ifndef MBEDTLS_AESNI_H
#define MBEDTLS_AESNI_H

#include "mbedtls/build_info.h"

#include "mbedtls/aes.h"
#include "runtime_internal.h"

#if defined(MBEDTLS_AESNI_HAVE_CODE)

/* Can we do AESNI with intrinsics?
 * (Only implemented with certain compilers, only for certain targets.)
 */
#undef MBEDTLS_AESNI_HAVE_INTRINSICS
#if defined(_MSC_VER)
/* Visual Studio supports AESNI intrinsics since VS 2008 SP1. We only support
 * VS 2013 and up for other reasons anyway, so no need to check the version. */
#define MBEDTLS_AESNI_HAVE_INTRINSICS
#endif
/* GCC-like compilers: currently, we only support intrinsics if the requisite
 * target flag is enabled when building the library (e.g. `gcc -mpclmul -msse2`
 * or `clang -maes -mpclmul`). */
#if defined(__GNUC__) && defined(__AES__) && defined(__PCLMUL__)
#define MBEDTLS_AESNI_HAVE_INTRINSICS
#endif

/* Choose the implementation of AESNI, if one is available.
 *
 * Favor the intrinsics-based implementation if it's available, for better
 * maintainability.
 * Performance is about the same (see #7380).
 * In the long run, we will likely remove the assembly implementation. */
#if !defined(MBEDTLS_AESNI_HAVE_INTRINSICS)
/* Can we do AESNI with inline assembly?
 * (Only implemented with gas syntax, only for 64-bit.)
 */
#if !(defined(MBEDTLS_HAVE_ASM) && defined(MBEDTLS_ARCH_IS_X64))
#if defined(__GNUC__)
#   error "Must use `-mpclmul -msse2 -maes` for MBEDTLS_AESNI_C"
#else
#error "MBEDTLS_AESNI_C defined, but neither intrinsics nor assembly available"
#endif
#endif
#endif /* !MBEDTLS_AESNI_HAVE_INTRINSICS */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MBEDTLS_RUNTIME_HAVE_CODE) && defined(MBEDTLS_AES_RUNTIME_HAVE_CODE)

extern signed char mbedtls_aesni_aes_has_support_result;

/**
 * \brief          Internal function to detect AESNI AES in CPUs.
 *
 * \return         1 if CPU has support for the feature, 0 otherwise
 */
int mbedtls_aesni_aes_has_support(void);

#define MBEDTLS_AESNI_AES_HAS_SUPPORT() \
    (mbedtls_aesni_aes_has_support_result == -1 ? \
     mbedtls_aesni_aes_has_support() : mbedtls_aesni_aes_has_support_result)
#if defined(MBEDTLS_GCM_C)

extern signed char mbedtls_aesni_clmul_has_support_result;

/**
 * \brief          Internal function to detect CLMUL in CPUs.
 *
 * \return         1 if CPU has support for the feature, 0 otherwise
 */
int mbedtls_aesni_clmul_has_support(void);

#define MBEDTLS_AESNI_CLMUL_HAS_SUPPORT() \
    (mbedtls_aesni_clmul_has_support_result == -1 ? \
     mbedtls_aesni_clmul_has_support() : mbedtls_aesni_clmul_has_support_result)
#endif


#else /* MBEDTLS_RUNTIME_HAVE_CODE && MBEDTLS_AES_RUNTIME_HAVE_CODE */

#define MBEDTLS_AESNI_AES_HAS_SUPPORT() 1

#if defined(MBEDTLS_GCM_C)
#define MBEDTLS_AESNI_CLMUL_HAS_SUPPORT() 1
#endif

#endif /* !(MBEDTLS_RUNTIME_HAVE_CODE && MBEDTLS_AES_RUNTIME_HAVE_CODE) */

/**
 * \brief          Internal AES-NI AES-ECB block encryption and decryption
 *
 * \note           This function is only for internal use by other library
 *                 functions; you must not call it directly.
 *
 * \param ctx      AES context
 * \param mode     MBEDTLS_AES_ENCRYPT or MBEDTLS_AES_DECRYPT
 * \param input    16-byte input block
 * \param output   16-byte output block
 *
 * \return         0 on success (cannot fail)
 */
int mbedtls_aesni_crypt_ecb(mbedtls_aes_context *ctx,
                            int mode,
                            const unsigned char input[16],
                            unsigned char output[16]);

/**
 * \brief          Internal GCM multiplication: c = a * b in GF(2^128)
 *
 * \note           This function is only for internal use by other library
 *                 functions; you must not call it directly.
 *
 * \param c        Result
 * \param a        First operand
 * \param b        Second operand
 *
 * \note           Both operands and result are bit strings interpreted as
 *                 elements of GF(2^128) as per the GCM spec.
 */
void mbedtls_aesni_gcm_mult(unsigned char c[16],
                            const unsigned char a[16],
                            const unsigned char b[16]);

/**
 * \brief           Internal round key inversion. This function computes
 *                  decryption round keys from the encryption round keys.
 *
 * \note            This function is only for internal use by other library
 *                  functions; you must not call it directly.
 *
 * \param invkey    Round keys for the equivalent inverse cipher
 * \param fwdkey    Original round keys (for encryption)
 * \param nr        Number of rounds (that is, number of round keys minus one)
 */
void mbedtls_aesni_inverse_key(unsigned char *invkey,
                               const unsigned char *fwdkey,
                               int nr);

/**
 * \brief           Internal key expansion for encryption
 *
 * \note            This function is only for internal use by other library
 *                  functions; you must not call it directly.
 *
 * \param rk        Destination buffer where the round keys are written
 * \param key       Encryption key
 * \param bits      Key size in bits (must be 128, 192 or 256)
 *
 * \return          0 if successful, or MBEDTLS_ERR_AES_INVALID_KEY_LENGTH
 */
int mbedtls_aesni_setkey_enc(unsigned char *rk,
                             const unsigned char *key,
                             size_t bits);

#ifdef __cplusplus
}
#endif

#endif  /* MBEDTLS_AESNI_HAVE_CODE */

#endif /* MBEDTLS_AESNI_H */
