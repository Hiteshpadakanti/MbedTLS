/**
 * \file des.h
 *
 * \brief DES block cipher
 *
 * \warning   DES is considered a weak cipher and its use constitutes a
 *            security risk. We recommend considering stronger ciphers
 *            instead.
 */
/*
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
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
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 *
 */
#ifndef MBEDTLS_DES_H
#define MBEDTLS_DES_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <stddef.h>
#include <stdint.h>

#define MBEDTLS_DES_ENCRYPT     1
#define MBEDTLS_DES_DECRYPT     0

#define MBEDTLS_ERR_DES_INVALID_INPUT_LENGTH              -0x0032  /**< The data input has an invalid length. */

/* MBEDTLS_ERR_DES_HW_ACCEL_FAILED is deprecated and should not be used. */
#define MBEDTLS_ERR_DES_HW_ACCEL_FAILED                   -0x0033  /**< DES hardware accelerator failed.      */
#define MBEDTLS_ERR_DES_BAD_INPUT_DATA                    -0x005D  /**< Invalid input data.                   */

#define MBEDTLS_DES_KEY_SIZE    8

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MBEDTLS_DES_ALT)
// Regular implementation
//

/**
 * \brief          DES context structure
 *
 * \warning        DES is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 */
typedef struct mbedtls_des_context
{
    uint32_t sk[32];            /*!<  DES subkeys       */
}
mbedtls_des_context;

/**
 * \brief          Triple-DES context structure
 */
typedef struct mbedtls_des3_context
{
    uint32_t sk[96];            /*!<  3DES subkeys      */
}
mbedtls_des3_context;

#else  /* MBEDTLS_DES_ALT */
#include "des_alt.h"
#endif /* MBEDTLS_DES_ALT */

/**
 * \brief          Initialize a DES context.
 *
 * \param ctx      The DES context to be initialized.
 *                 Must not be \c NULL.
 *
 * \warning        DES is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 */
void mbedtls_des_init( mbedtls_des_context *ctx );

/**
 * \brief          Clear a DES context.
 *
 * \param ctx      The DES context to be cleared. May be \c NULL,
 *                 in which case this function is a no-op.
 *
 * \warning        DES is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 */
void mbedtls_des_free( mbedtls_des_context *ctx );

/**
 * \brief          Initialize a Triple-DES context.
 *
 * \param ctx      The DES3 context to be initialized.
 *                 Must not be \c NULL.
 */
void mbedtls_des3_init( mbedtls_des3_context *ctx );

/**
 * \brief          Clear a Triple-DES context
 *
 * \param ctx      The DES3 context to be cleared. May be \c NULL,
 *                 in which case this function is a no-op.
 */
void mbedtls_des3_free( mbedtls_des3_context *ctx );

/**
 * \brief          Set the key parity on the given key to odd.
 *
 *                 DES keys are 56 bits long, but each byte is padded with
 *                 a parity bit to allow verification.
 *
 * \param key      The secret key.
 *                 Must be a R/W buffer of length 8 Bytes.
 *
 * \warning        DES is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 */
void mbedtls_des_key_set_parity( unsigned char key[MBEDTLS_DES_KEY_SIZE] );

/**
 * \brief          Check that the key parity on the given key is odd.
 *
 *                 DES keys are 56 bits long, but each byte is padded with
 *                 a parity bit to allow verification.
 *
 * \param key      The secret key.
 *                 Must be a readable buffer of length 8 Bytes.
 *
 * \return         \c 0 if the parity was ok.
 * \return         \c 1 if the parity was not correct.
 *
 * \warning        DES is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 */
int mbedtls_des_key_check_key_parity( const unsigned char key[MBEDTLS_DES_KEY_SIZE] );

/**
 * \brief          Check that key is not a weak or semi-weak DES key
 *
 * \param key      The secret key.
 *                 Must be a readable buffer of length 8 Bytes.
 *
 * \return         \c 0 if no weakness was found in the key.
 * \return         \c 1 if a weak key was identified.
 *
 * \warning        DES is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 */
int mbedtls_des_key_check_weak( const unsigned char key[MBEDTLS_DES_KEY_SIZE] );

/**
 * \brief          Perform a DES key schedule (56-bit, encryption).
 *
 * \param ctx      The DES context to use. Must be initialized.
 * \param key      The secret key.
 *                 Must be a readable buffer of length 8 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 *
 * \warning        DES is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 */
int mbedtls_des_setkey_enc( mbedtls_des_context *ctx,
                            const unsigned char key[MBEDTLS_DES_KEY_SIZE] );

/**
 * \brief          Perform a DES key schedule (56-bit, decryption).
 *
 * \param ctx      The DES context to use. Must be initialized.
 * \param key      The secret key.
 *                 Must be a readable buffer of length 8 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 *
 * \warning        DES is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 */
int mbedtls_des_setkey_dec( mbedtls_des_context *ctx,
                            const unsigned char key[MBEDTLS_DES_KEY_SIZE] );

/**
 * \brief          Perform a Triple-DES key schedule (112-bit, encryption).
 *
 * \param ctx      The 3DES context to use. Must be initialized.
 * \param key      The secret key.
 *                 Must be a readable buffer of length 16 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 *
 */
int mbedtls_des3_set2key_enc( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 2] );

/**
 * \brief          Perform a Triple-DES key schedule (112-bit, decryption).
 *
 * \param ctx      The 3DES context to use. Must be initialized.
 * \param key      The secret key.
 *                 Must be a readable buffer of length 16 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 *
 */
int mbedtls_des3_set2key_dec( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 2] );

/**
 * \brief          Perform a Triple-DES key schedule (168-bit, encryption).
 *
 * \param ctx      The 3DES context to use. Must be initialized.
 * \param key      The secret key.
 *                 Must be a readable buffer of length 24 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 *
 */
int mbedtls_des3_set3key_enc( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 3] );

/**
 * \brief          Perform a Triple-DES key schedule (168-bit, decryption).
 *
 * \param ctx      The 3DES context to use. Must be initialized.
 * \param key      The secret key.
 *                 Must be a readable buffer of length 24 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 *
 */
int mbedtls_des3_set3key_dec( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 3] );

/**
 * \brief          Perform a DES-ECB block encryption/decryption.
 *
 * \param ctx      The DES context to use. Must be initialized.
 * \param input    The input block.
 *                 Must be a readable buffer of length 8 Bytes.
 * \param output   The output block.
 *                 Must be a writable buffer of length 8 Bytes.
 *
 * \return         \c 0 if successful.
 * \return         A negative error code on failure.
 *
 * \warning        DES is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 */
int mbedtls_des_crypt_ecb( mbedtls_des_context *ctx,
                    const unsigned char input[8],
                    unsigned char output[8] );

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/**
 * \brief          Perform a DES-CBC buffer encryption/decryption.
 *
 * \note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * \param ctx      The DES context to use. Must be initialized.
 * \param mode     The mode to use:
 *                 #MBEDTLS_DES_ENCRYPT for encryption, or
 *                 #MBEDTLS_DES_DECRYPT for decryption.
 * \param length   The length of the input data \p input.
 * \param iv       The initialization vector (updated after use).
 *                 Must point to a R/W buffer of length 8 Bytes.
 * \param input    The buffer holding the input data.
 *                 Must be a readable buffer of length \p length Bytes.
 * \param output   The buffer to hold the output data.
 *                 Must be a writable buffer of length \p length Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 *
 * \warning        DES is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 */
int mbedtls_des_crypt_cbc( mbedtls_des_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[8],
                    const unsigned char *input,
                    unsigned char *output );
#endif /* MBEDTLS_CIPHER_MODE_CBC */

/**
 * \brief          Perform a 3DES-ECB block encryption/decryption.
 *
 * \param ctx      The 3DES context to use. Must be initialized.
 * \param input    The input block.
 *                 Must be a readable buffer of length 8 Bytes.
 * \param output   The output block.
 *                 Must be a writable buffer of length 8 Bytes.
 *
 * \return         \c 0 on success.
 * \return         A negative error code on failure.
 */
int mbedtls_des3_crypt_ecb( mbedtls_des3_context *ctx,
                     const unsigned char input[8],
                     unsigned char output[8] );

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/**
 * \brief          Perform a 3DES-CBC buffer encryption/decryption.
 *
 * \note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * \param ctx      The 3DES context to use. Must be initialized.
 * \param mode     The mode of operation:
 *                 #MBEDTLS_DES_ENCRYPT for encryption, or
 *                 #MBEDTLS_DES_DECRYPT for decryption.
 * \param length   The length of the input data \p input.
 * \param iv       The initialization vector (updated after use).
 *                 Must be a R/W buffer of length 8 Bytes.
 * \param input    The buffer holding the input data.
 *                 Must be a readable buffer of length \p length Bytes.
 * \param output   The buffer to hold the output data.
 *                 Must be a writable buffer of length \p length Bytes.
 *
 * \return         \c 0 if successful.
 * \return         A negative error code on failure.
 */
int mbedtls_des3_crypt_cbc( mbedtls_des3_context *ctx,
                     int mode,
                     size_t length,
                     unsigned char iv[8],
                     const unsigned char *input,
                     unsigned char *output );
#endif /* MBEDTLS_CIPHER_MODE_CBC */

/**
 * \brief          Internal function for key expansion.
 *                 (Only exposed to allow overriding it,
 *                 see MBEDTLS_DES_SETKEY_ALT)
 *
 * \param SK       Round keys
 * \param key      Base key
 *
 * \warning        DES is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 */
void mbedtls_des_setkey( uint32_t SK[32],
                         const unsigned char key[MBEDTLS_DES_KEY_SIZE] );

/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int mbedtls_des_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif /* des.h */
