/*
 *  Version feature information
 *
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
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_VERSION_C)

#include "mbedtls/version.h"

#include <string.h>

static const char *features[] = {
#if defined(MBEDTLS_VERSION_FEATURES)
#if defined(MBEDTLS_HAVE_ASM)
    "MBEDTLS_HAVE_ASM",
#endif /* MBEDTLS_HAVE_ASM */
#if defined(MBEDTLS_NO_UDBL_DIVISION)
    "MBEDTLS_NO_UDBL_DIVISION",
#endif /* MBEDTLS_NO_UDBL_DIVISION */
#if defined(MBEDTLS_NO_64BIT_MULTIPLICATION)
    "MBEDTLS_NO_64BIT_MULTIPLICATION",
#endif /* MBEDTLS_NO_64BIT_MULTIPLICATION */
#if defined(MBEDTLS_HAVE_SSE2)
    "MBEDTLS_HAVE_SSE2",
#endif /* MBEDTLS_HAVE_SSE2 */
#if defined(MBEDTLS_HAVE_TIME)
    "MBEDTLS_HAVE_TIME",
#endif /* MBEDTLS_HAVE_TIME */
#if defined(MBEDTLS_HAVE_TIME_DATE)
    "MBEDTLS_HAVE_TIME_DATE",
#endif /* MBEDTLS_HAVE_TIME_DATE */
#if defined(MBEDTLS_PLATFORM_MEMORY)
    "MBEDTLS_PLATFORM_MEMORY",
#endif /* MBEDTLS_PLATFORM_MEMORY */
#if defined(MBEDTLS_PLATFORM_NO_STD_FUNCTIONS)
    "MBEDTLS_PLATFORM_NO_STD_FUNCTIONS",
#endif /* MBEDTLS_PLATFORM_NO_STD_FUNCTIONS */
#if defined(MBEDTLS_PLATFORM_EXIT_ALT)
    "MBEDTLS_PLATFORM_EXIT_ALT",
#endif /* MBEDTLS_PLATFORM_EXIT_ALT */
#if defined(MBEDTLS_PLATFORM_TIME_ALT)
    "MBEDTLS_PLATFORM_TIME_ALT",
#endif /* MBEDTLS_PLATFORM_TIME_ALT */
#if defined(MBEDTLS_PLATFORM_FPRINTF_ALT)
    "MBEDTLS_PLATFORM_FPRINTF_ALT",
#endif /* MBEDTLS_PLATFORM_FPRINTF_ALT */
#if defined(MBEDTLS_PLATFORM_PRINTF_ALT)
    "MBEDTLS_PLATFORM_PRINTF_ALT",
#endif /* MBEDTLS_PLATFORM_PRINTF_ALT */
#if defined(MBEDTLS_PLATFORM_SNPRINTF_ALT)
    "MBEDTLS_PLATFORM_SNPRINTF_ALT",
#endif /* MBEDTLS_PLATFORM_SNPRINTF_ALT */
#if defined(MBEDTLS_PLATFORM_NV_SEED_ALT)
    "MBEDTLS_PLATFORM_NV_SEED_ALT",
#endif /* MBEDTLS_PLATFORM_NV_SEED_ALT */
#if defined(MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT)
    "MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT",
#endif /* MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT */
#if defined(MBEDTLS_DEPRECATED_WARNING)
    "MBEDTLS_DEPRECATED_WARNING",
#endif /* MBEDTLS_DEPRECATED_WARNING */
#if defined(MBEDTLS_DEPRECATED_REMOVED)
    "MBEDTLS_DEPRECATED_REMOVED",
#endif /* MBEDTLS_DEPRECATED_REMOVED */
#if defined(MBEDTLS_CHECK_PARAMS)
    "MBEDTLS_CHECK_PARAMS",
#endif /* MBEDTLS_CHECK_PARAMS */
#if defined(MBEDTLS_CHECK_PARAMS_ASSERT)
    "MBEDTLS_CHECK_PARAMS_ASSERT",
#endif /* MBEDTLS_CHECK_PARAMS_ASSERT */
#if defined(MBEDTLS_TIMING_ALT)
    "MBEDTLS_TIMING_ALT",
#endif /* MBEDTLS_TIMING_ALT */
#if defined(MBEDTLS_AES_ALT)
    "MBEDTLS_AES_ALT",
#endif /* MBEDTLS_AES_ALT */
#if defined(MBEDTLS_ARC4_ALT)
    "MBEDTLS_ARC4_ALT",
#endif /* MBEDTLS_ARC4_ALT */
#if defined(MBEDTLS_ARIA_ALT)
    "MBEDTLS_ARIA_ALT",
#endif /* MBEDTLS_ARIA_ALT */
#if defined(MBEDTLS_BLOWFISH_ALT)
    "MBEDTLS_BLOWFISH_ALT",
#endif /* MBEDTLS_BLOWFISH_ALT */
#if defined(MBEDTLS_CAMELLIA_ALT)
    "MBEDTLS_CAMELLIA_ALT",
#endif /* MBEDTLS_CAMELLIA_ALT */
#if defined(MBEDTLS_CCM_ALT)
    "MBEDTLS_CCM_ALT",
#endif /* MBEDTLS_CCM_ALT */
#if defined(MBEDTLS_CHACHA20_ALT)
    "MBEDTLS_CHACHA20_ALT",
#endif /* MBEDTLS_CHACHA20_ALT */
#if defined(MBEDTLS_CHACHAPOLY_ALT)
    "MBEDTLS_CHACHAPOLY_ALT",
#endif /* MBEDTLS_CHACHAPOLY_ALT */
#if defined(MBEDTLS_CMAC_ALT)
    "MBEDTLS_CMAC_ALT",
#endif /* MBEDTLS_CMAC_ALT */
#if defined(MBEDTLS_DES_ALT)
    "MBEDTLS_DES_ALT",
#endif /* MBEDTLS_DES_ALT */
#if defined(MBEDTLS_DHM_ALT)
    "MBEDTLS_DHM_ALT",
#endif /* MBEDTLS_DHM_ALT */
#if defined(MBEDTLS_ECJPAKE_ALT)
    "MBEDTLS_ECJPAKE_ALT",
#endif /* MBEDTLS_ECJPAKE_ALT */
#if defined(MBEDTLS_GCM_ALT)
    "MBEDTLS_GCM_ALT",
#endif /* MBEDTLS_GCM_ALT */
#if defined(MBEDTLS_NIST_KW_ALT)
    "MBEDTLS_NIST_KW_ALT",
#endif /* MBEDTLS_NIST_KW_ALT */
#if defined(MBEDTLS_MD2_ALT)
    "MBEDTLS_MD2_ALT",
#endif /* MBEDTLS_MD2_ALT */
#if defined(MBEDTLS_MD4_ALT)
    "MBEDTLS_MD4_ALT",
#endif /* MBEDTLS_MD4_ALT */
#if defined(MBEDTLS_MD5_ALT)
    "MBEDTLS_MD5_ALT",
#endif /* MBEDTLS_MD5_ALT */
#if defined(MBEDTLS_POLY1305_ALT)
    "MBEDTLS_POLY1305_ALT",
#endif /* MBEDTLS_POLY1305_ALT */
#if defined(MBEDTLS_RIPEMD160_ALT)
    "MBEDTLS_RIPEMD160_ALT",
#endif /* MBEDTLS_RIPEMD160_ALT */
#if defined(MBEDTLS_RSA_ALT)
    "MBEDTLS_RSA_ALT",
#endif /* MBEDTLS_RSA_ALT */
#if defined(MBEDTLS_SHA1_ALT)
    "MBEDTLS_SHA1_ALT",
#endif /* MBEDTLS_SHA1_ALT */
#if defined(MBEDTLS_SHA256_ALT)
    "MBEDTLS_SHA256_ALT",
#endif /* MBEDTLS_SHA256_ALT */
#if defined(MBEDTLS_SHA512_ALT)
    "MBEDTLS_SHA512_ALT",
#endif /* MBEDTLS_SHA512_ALT */
#if defined(MBEDTLS_XTEA_ALT)
    "MBEDTLS_XTEA_ALT",
#endif /* MBEDTLS_XTEA_ALT */
#if defined(MBEDTLS_ECP_ALT)
    "MBEDTLS_ECP_ALT",
#endif /* MBEDTLS_ECP_ALT */
#if defined(MBEDTLS_MD2_PROCESS_ALT)
    "MBEDTLS_MD2_PROCESS_ALT",
#endif /* MBEDTLS_MD2_PROCESS_ALT */
#if defined(MBEDTLS_MD4_PROCESS_ALT)
    "MBEDTLS_MD4_PROCESS_ALT",
#endif /* MBEDTLS_MD4_PROCESS_ALT */
#if defined(MBEDTLS_MD5_PROCESS_ALT)
    "MBEDTLS_MD5_PROCESS_ALT",
#endif /* MBEDTLS_MD5_PROCESS_ALT */
#if defined(MBEDTLS_RIPEMD160_PROCESS_ALT)
    "MBEDTLS_RIPEMD160_PROCESS_ALT",
#endif /* MBEDTLS_RIPEMD160_PROCESS_ALT */
#if defined(MBEDTLS_SHA1_PROCESS_ALT)
    "MBEDTLS_SHA1_PROCESS_ALT",
#endif /* MBEDTLS_SHA1_PROCESS_ALT */
#if defined(MBEDTLS_SHA256_PROCESS_ALT)
    "MBEDTLS_SHA256_PROCESS_ALT",
#endif /* MBEDTLS_SHA256_PROCESS_ALT */
#if defined(MBEDTLS_SHA512_PROCESS_ALT)
    "MBEDTLS_SHA512_PROCESS_ALT",
#endif /* MBEDTLS_SHA512_PROCESS_ALT */
#if defined(MBEDTLS_DES_SETKEY_ALT)
    "MBEDTLS_DES_SETKEY_ALT",
#endif /* MBEDTLS_DES_SETKEY_ALT */
#if defined(MBEDTLS_DES_CRYPT_ECB_ALT)
    "MBEDTLS_DES_CRYPT_ECB_ALT",
#endif /* MBEDTLS_DES_CRYPT_ECB_ALT */
#if defined(MBEDTLS_DES3_CRYPT_ECB_ALT)
    "MBEDTLS_DES3_CRYPT_ECB_ALT",
#endif /* MBEDTLS_DES3_CRYPT_ECB_ALT */
#if defined(MBEDTLS_AES_SETKEY_ENC_ALT)
    "MBEDTLS_AES_SETKEY_ENC_ALT",
#endif /* MBEDTLS_AES_SETKEY_ENC_ALT */
#if defined(MBEDTLS_AES_SETKEY_DEC_ALT)
    "MBEDTLS_AES_SETKEY_DEC_ALT",
#endif /* MBEDTLS_AES_SETKEY_DEC_ALT */
#if defined(MBEDTLS_AES_ENCRYPT_ALT)
    "MBEDTLS_AES_ENCRYPT_ALT",
#endif /* MBEDTLS_AES_ENCRYPT_ALT */
#if defined(MBEDTLS_AES_DECRYPT_ALT)
    "MBEDTLS_AES_DECRYPT_ALT",
#endif /* MBEDTLS_AES_DECRYPT_ALT */
#if defined(MBEDTLS_ECDH_GEN_PUBLIC_ALT)
    "MBEDTLS_ECDH_GEN_PUBLIC_ALT",
#endif /* MBEDTLS_ECDH_GEN_PUBLIC_ALT */
#if defined(MBEDTLS_ECDH_COMPUTE_SHARED_ALT)
    "MBEDTLS_ECDH_COMPUTE_SHARED_ALT",
#endif /* MBEDTLS_ECDH_COMPUTE_SHARED_ALT */
#if defined(MBEDTLS_ECDSA_VERIFY_ALT)
    "MBEDTLS_ECDSA_VERIFY_ALT",
#endif /* MBEDTLS_ECDSA_VERIFY_ALT */
#if defined(MBEDTLS_ECDSA_SIGN_ALT)
    "MBEDTLS_ECDSA_SIGN_ALT",
#endif /* MBEDTLS_ECDSA_SIGN_ALT */
#if defined(MBEDTLS_ECDSA_GENKEY_ALT)
    "MBEDTLS_ECDSA_GENKEY_ALT",
#endif /* MBEDTLS_ECDSA_GENKEY_ALT */
#if defined(MBEDTLS_ECP_INTERNAL_ALT)
    "MBEDTLS_ECP_INTERNAL_ALT",
#endif /* MBEDTLS_ECP_INTERNAL_ALT */
#if defined(MBEDTLS_ECP_RANDOMIZE_JAC_ALT)
    "MBEDTLS_ECP_RANDOMIZE_JAC_ALT",
#endif /* MBEDTLS_ECP_RANDOMIZE_JAC_ALT */
#if defined(MBEDTLS_ECP_ADD_MIXED_ALT)
    "MBEDTLS_ECP_ADD_MIXED_ALT",
#endif /* MBEDTLS_ECP_ADD_MIXED_ALT */
#if defined(MBEDTLS_ECP_DOUBLE_JAC_ALT)
    "MBEDTLS_ECP_DOUBLE_JAC_ALT",
#endif /* MBEDTLS_ECP_DOUBLE_JAC_ALT */
#if defined(MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT)
    "MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT",
#endif /* MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT */
#if defined(MBEDTLS_ECP_NORMALIZE_JAC_ALT)
    "MBEDTLS_ECP_NORMALIZE_JAC_ALT",
#endif /* MBEDTLS_ECP_NORMALIZE_JAC_ALT */
#if defined(MBEDTLS_ECP_DOUBLE_ADD_MXZ_ALT)
    "MBEDTLS_ECP_DOUBLE_ADD_MXZ_ALT",
#endif /* MBEDTLS_ECP_DOUBLE_ADD_MXZ_ALT */
#if defined(MBEDTLS_ECP_RANDOMIZE_MXZ_ALT)
    "MBEDTLS_ECP_RANDOMIZE_MXZ_ALT",
#endif /* MBEDTLS_ECP_RANDOMIZE_MXZ_ALT */
#if defined(MBEDTLS_ECP_NORMALIZE_MXZ_ALT)
    "MBEDTLS_ECP_NORMALIZE_MXZ_ALT",
#endif /* MBEDTLS_ECP_NORMALIZE_MXZ_ALT */
#if defined(MBEDTLS_TEST_NULL_ENTROPY)
    "MBEDTLS_TEST_NULL_ENTROPY",
#endif /* MBEDTLS_TEST_NULL_ENTROPY */
#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT)
    "MBEDTLS_ENTROPY_HARDWARE_ALT",
#endif /* MBEDTLS_ENTROPY_HARDWARE_ALT */
#if defined(MBEDTLS_AES_ROM_TABLES)
    "MBEDTLS_AES_ROM_TABLES",
#endif /* MBEDTLS_AES_ROM_TABLES */
#if defined(MBEDTLS_AES_FEWER_TABLES)
    "MBEDTLS_AES_FEWER_TABLES",
#endif /* MBEDTLS_AES_FEWER_TABLES */
#if defined(MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH)
    "MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH",
#endif /* MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH */
#if defined(MBEDTLS_AES_ONLY_ENCRYPT)
    "MBEDTLS_AES_ONLY_ENCRYPT",
#endif /* MBEDTLS_AES_ONLY_ENCRYPT */
#if defined(MBEDTLS_CAMELLIA_SMALL_MEMORY)
    "MBEDTLS_CAMELLIA_SMALL_MEMORY",
#endif /* MBEDTLS_CAMELLIA_SMALL_MEMORY */
#if defined(MBEDTLS_CIPHER_MODE_CBC)
    "MBEDTLS_CIPHER_MODE_CBC",
#endif /* MBEDTLS_CIPHER_MODE_CBC */
#if defined(MBEDTLS_CIPHER_MODE_CFB)
    "MBEDTLS_CIPHER_MODE_CFB",
#endif /* MBEDTLS_CIPHER_MODE_CFB */
#if defined(MBEDTLS_CIPHER_MODE_CTR)
    "MBEDTLS_CIPHER_MODE_CTR",
#endif /* MBEDTLS_CIPHER_MODE_CTR */
#if defined(MBEDTLS_CIPHER_MODE_OFB)
    "MBEDTLS_CIPHER_MODE_OFB",
#endif /* MBEDTLS_CIPHER_MODE_OFB */
#if defined(MBEDTLS_CIPHER_MODE_XTS)
    "MBEDTLS_CIPHER_MODE_XTS",
#endif /* MBEDTLS_CIPHER_MODE_XTS */
#if defined(MBEDTLS_CIPHER_NULL_CIPHER)
    "MBEDTLS_CIPHER_NULL_CIPHER",
#endif /* MBEDTLS_CIPHER_NULL_CIPHER */
#if defined(MBEDTLS_CIPHER_PADDING_PKCS7)
    "MBEDTLS_CIPHER_PADDING_PKCS7",
#endif /* MBEDTLS_CIPHER_PADDING_PKCS7 */
#if defined(MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS)
    "MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS",
#endif /* MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS */
#if defined(MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN)
    "MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN",
#endif /* MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN */
#if defined(MBEDTLS_CIPHER_PADDING_ZEROS)
    "MBEDTLS_CIPHER_PADDING_ZEROS",
#endif /* MBEDTLS_CIPHER_PADDING_ZEROS */
#if defined(MBEDTLS_ENABLE_WEAK_CIPHERSUITES)
    "MBEDTLS_ENABLE_WEAK_CIPHERSUITES",
#endif /* MBEDTLS_ENABLE_WEAK_CIPHERSUITES */
#if defined(MBEDTLS_REMOVE_ARC4_CIPHERSUITES)
    "MBEDTLS_REMOVE_ARC4_CIPHERSUITES",
#endif /* MBEDTLS_REMOVE_ARC4_CIPHERSUITES */
#if defined(MBEDTLS_REMOVE_3DES_CIPHERSUITES)
    "MBEDTLS_REMOVE_3DES_CIPHERSUITES",
#endif /* MBEDTLS_REMOVE_3DES_CIPHERSUITES */
#if defined(MBEDTLS_ECP_DP_SECP192R1_ENABLED)
    "MBEDTLS_ECP_DP_SECP192R1_ENABLED",
#endif /* MBEDTLS_ECP_DP_SECP192R1_ENABLED */
#if defined(MBEDTLS_ECP_DP_SECP224R1_ENABLED)
    "MBEDTLS_ECP_DP_SECP224R1_ENABLED",
#endif /* MBEDTLS_ECP_DP_SECP224R1_ENABLED */
#if defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED)
    "MBEDTLS_ECP_DP_SECP256R1_ENABLED",
#endif /* MBEDTLS_ECP_DP_SECP256R1_ENABLED */
#if defined(MBEDTLS_ECP_DP_SECP384R1_ENABLED)
    "MBEDTLS_ECP_DP_SECP384R1_ENABLED",
#endif /* MBEDTLS_ECP_DP_SECP384R1_ENABLED */
#if defined(MBEDTLS_ECP_DP_SECP521R1_ENABLED)
    "MBEDTLS_ECP_DP_SECP521R1_ENABLED",
#endif /* MBEDTLS_ECP_DP_SECP521R1_ENABLED */
#if defined(MBEDTLS_ECP_DP_SECP192K1_ENABLED)
    "MBEDTLS_ECP_DP_SECP192K1_ENABLED",
#endif /* MBEDTLS_ECP_DP_SECP192K1_ENABLED */
#if defined(MBEDTLS_ECP_DP_SECP224K1_ENABLED)
    "MBEDTLS_ECP_DP_SECP224K1_ENABLED",
#endif /* MBEDTLS_ECP_DP_SECP224K1_ENABLED */
#if defined(MBEDTLS_ECP_DP_SECP256K1_ENABLED)
    "MBEDTLS_ECP_DP_SECP256K1_ENABLED",
#endif /* MBEDTLS_ECP_DP_SECP256K1_ENABLED */
#if defined(MBEDTLS_ECP_DP_BP256R1_ENABLED)
    "MBEDTLS_ECP_DP_BP256R1_ENABLED",
#endif /* MBEDTLS_ECP_DP_BP256R1_ENABLED */
#if defined(MBEDTLS_ECP_DP_BP384R1_ENABLED)
    "MBEDTLS_ECP_DP_BP384R1_ENABLED",
#endif /* MBEDTLS_ECP_DP_BP384R1_ENABLED */
#if defined(MBEDTLS_ECP_DP_BP512R1_ENABLED)
    "MBEDTLS_ECP_DP_BP512R1_ENABLED",
#endif /* MBEDTLS_ECP_DP_BP512R1_ENABLED */
#if defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED)
    "MBEDTLS_ECP_DP_CURVE25519_ENABLED",
#endif /* MBEDTLS_ECP_DP_CURVE25519_ENABLED */
#if defined(MBEDTLS_ECP_DP_CURVE448_ENABLED)
    "MBEDTLS_ECP_DP_CURVE448_ENABLED",
#endif /* MBEDTLS_ECP_DP_CURVE448_ENABLED */
#if defined(MBEDTLS_ECP_NIST_OPTIM)
    "MBEDTLS_ECP_NIST_OPTIM",
#endif /* MBEDTLS_ECP_NIST_OPTIM */
#if defined(MBEDTLS_ECP_RESTARTABLE)
    "MBEDTLS_ECP_RESTARTABLE",
#endif /* MBEDTLS_ECP_RESTARTABLE */
#if defined(MBEDTLS_ECDSA_DETERMINISTIC)
    "MBEDTLS_ECDSA_DETERMINISTIC",
#endif /* MBEDTLS_ECDSA_DETERMINISTIC */
#if defined(MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
    "MBEDTLS_KEY_EXCHANGE_PSK_ENABLED",
#endif /* MBEDTLS_KEY_EXCHANGE_PSK_ENABLED */
#if defined(MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED)
    "MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED",
#endif /* MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED */
#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED)
    "MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED",
#endif /* MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED */
#if defined(MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED)
    "MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED",
#endif /* MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED */
#if defined(MBEDTLS_KEY_EXCHANGE_RSA_ENABLED)
    "MBEDTLS_KEY_EXCHANGE_RSA_ENABLED",
#endif /* MBEDTLS_KEY_EXCHANGE_RSA_ENABLED */
#if defined(MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED)
    "MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED",
#endif /* MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED */
#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED)
    "MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED",
#endif /* MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED */
#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
    "MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED",
#endif /* MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED */
#if defined(MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED)
    "MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED",
#endif /* MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED */
#if defined(MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED)
    "MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED",
#endif /* MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED */
#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
    "MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED",
#endif /* MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED */
#if defined(MBEDTLS_PK_PARSE_EC_EXTENDED)
    "MBEDTLS_PK_PARSE_EC_EXTENDED",
#endif /* MBEDTLS_PK_PARSE_EC_EXTENDED */
#if defined(MBEDTLS_ERROR_STRERROR_DUMMY)
    "MBEDTLS_ERROR_STRERROR_DUMMY",
#endif /* MBEDTLS_ERROR_STRERROR_DUMMY */
#if defined(MBEDTLS_GENPRIME)
    "MBEDTLS_GENPRIME",
#endif /* MBEDTLS_GENPRIME */
#if defined(MBEDTLS_FS_IO)
    "MBEDTLS_FS_IO",
#endif /* MBEDTLS_FS_IO */
#if defined(MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES)
    "MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES",
#endif /* MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES */
#if defined(MBEDTLS_NO_PLATFORM_ENTROPY)
    "MBEDTLS_NO_PLATFORM_ENTROPY",
#endif /* MBEDTLS_NO_PLATFORM_ENTROPY */
#if defined(MBEDTLS_ENTROPY_FORCE_SHA256)
    "MBEDTLS_ENTROPY_FORCE_SHA256",
#endif /* MBEDTLS_ENTROPY_FORCE_SHA256 */
#if defined(MBEDTLS_ENTROPY_NV_SEED)
    "MBEDTLS_ENTROPY_NV_SEED",
#endif /* MBEDTLS_ENTROPY_NV_SEED */
#if defined(MBEDTLS_MEMORY_DEBUG)
    "MBEDTLS_MEMORY_DEBUG",
#endif /* MBEDTLS_MEMORY_DEBUG */
#if defined(MBEDTLS_MEMORY_BACKTRACE)
    "MBEDTLS_MEMORY_BACKTRACE",
#endif /* MBEDTLS_MEMORY_BACKTRACE */
#if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
    "MBEDTLS_PK_RSA_ALT_SUPPORT",
#endif /* MBEDTLS_PK_RSA_ALT_SUPPORT */
#if defined(MBEDTLS_PKCS1_V15)
    "MBEDTLS_PKCS1_V15",
#endif /* MBEDTLS_PKCS1_V15 */
#if defined(MBEDTLS_PKCS1_V21)
    "MBEDTLS_PKCS1_V21",
#endif /* MBEDTLS_PKCS1_V21 */
#if defined(MBEDTLS_RSA_NO_CRT)
    "MBEDTLS_RSA_NO_CRT",
#endif /* MBEDTLS_RSA_NO_CRT */
#if defined(MBEDTLS_SELF_TEST)
    "MBEDTLS_SELF_TEST",
#endif /* MBEDTLS_SELF_TEST */
#if defined(MBEDTLS_SHA256_SMALLER)
    "MBEDTLS_SHA256_SMALLER",
#endif /* MBEDTLS_SHA256_SMALLER */
#if defined(MBEDTLS_SHA256_NO_SHA224)
    "MBEDTLS_SHA256_NO_SHA224",
#endif /* MBEDTLS_SHA256_NO_SHA224 */
#if defined(MBEDTLS_SSL_ALL_ALERT_MESSAGES)
    "MBEDTLS_SSL_ALL_ALERT_MESSAGES",
#endif /* MBEDTLS_SSL_ALL_ALERT_MESSAGES */
#if defined(MBEDTLS_SSL_RECORD_CHECKING)
    "MBEDTLS_SSL_RECORD_CHECKING",
#endif /* MBEDTLS_SSL_RECORD_CHECKING */
#if defined(MBEDTLS_SSL_DTLS_CONNECTION_ID)
    "MBEDTLS_SSL_DTLS_CONNECTION_ID",
#endif /* MBEDTLS_SSL_DTLS_CONNECTION_ID */
#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
    "MBEDTLS_SSL_ASYNC_PRIVATE",
#endif /* MBEDTLS_SSL_ASYNC_PRIVATE */
#if defined(MBEDTLS_SSL_CONTEXT_SERIALIZATION)
    "MBEDTLS_SSL_CONTEXT_SERIALIZATION",
#endif /* MBEDTLS_SSL_CONTEXT_SERIALIZATION */
#if defined(MBEDTLS_SSL_DEBUG_ALL)
    "MBEDTLS_SSL_DEBUG_ALL",
#endif /* MBEDTLS_SSL_DEBUG_ALL */
#if defined(MBEDTLS_SSL_ENCRYPT_THEN_MAC)
    "MBEDTLS_SSL_ENCRYPT_THEN_MAC",
#endif /* MBEDTLS_SSL_ENCRYPT_THEN_MAC */
#if defined(MBEDTLS_SSL_EXTENDED_MASTER_SECRET)
    "MBEDTLS_SSL_EXTENDED_MASTER_SECRET",
#endif /* MBEDTLS_SSL_EXTENDED_MASTER_SECRET */
#if defined(MBEDTLS_SSL_FALLBACK_SCSV)
    "MBEDTLS_SSL_FALLBACK_SCSV",
#endif /* MBEDTLS_SSL_FALLBACK_SCSV */
#if defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
    "MBEDTLS_SSL_KEEP_PEER_CERTIFICATE",
#endif /* MBEDTLS_SSL_KEEP_PEER_CERTIFICATE */
#if defined(MBEDTLS_SSL_HW_RECORD_ACCEL)
    "MBEDTLS_SSL_HW_RECORD_ACCEL",
#endif /* MBEDTLS_SSL_HW_RECORD_ACCEL */
#if defined(MBEDTLS_SSL_CBC_RECORD_SPLITTING)
    "MBEDTLS_SSL_CBC_RECORD_SPLITTING",
#endif /* MBEDTLS_SSL_CBC_RECORD_SPLITTING */
#if defined(MBEDTLS_SSL_RENEGOTIATION)
    "MBEDTLS_SSL_RENEGOTIATION",
#endif /* MBEDTLS_SSL_RENEGOTIATION */
#if defined(MBEDTLS_SSL_SRV_SUPPORT_SSLV2_CLIENT_HELLO)
    "MBEDTLS_SSL_SRV_SUPPORT_SSLV2_CLIENT_HELLO",
#endif /* MBEDTLS_SSL_SRV_SUPPORT_SSLV2_CLIENT_HELLO */
#if defined(MBEDTLS_SSL_SRV_RESPECT_CLIENT_PREFERENCE)
    "MBEDTLS_SSL_SRV_RESPECT_CLIENT_PREFERENCE",
#endif /* MBEDTLS_SSL_SRV_RESPECT_CLIENT_PREFERENCE */
#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    "MBEDTLS_SSL_MAX_FRAGMENT_LENGTH",
#endif /* MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */
#if defined(MBEDTLS_SSL_PROTO_SSL3)
    "MBEDTLS_SSL_PROTO_SSL3",
#endif /* MBEDTLS_SSL_PROTO_SSL3 */
#if defined(MBEDTLS_SSL_PROTO_TLS1)
    "MBEDTLS_SSL_PROTO_TLS1",
#endif /* MBEDTLS_SSL_PROTO_TLS1 */
#if defined(MBEDTLS_SSL_PROTO_TLS1_1)
    "MBEDTLS_SSL_PROTO_TLS1_1",
#endif /* MBEDTLS_SSL_PROTO_TLS1_1 */
#if defined(MBEDTLS_SSL_PROTO_TLS1_2)
    "MBEDTLS_SSL_PROTO_TLS1_2",
#endif /* MBEDTLS_SSL_PROTO_TLS1_2 */
#if defined(MBEDTLS_SSL_PROTO_DTLS)
    "MBEDTLS_SSL_PROTO_DTLS",
#endif /* MBEDTLS_SSL_PROTO_DTLS */
#if defined(MBEDTLS_SSL_PROTO_NO_TLS)
    "MBEDTLS_SSL_PROTO_NO_TLS",
#endif /* MBEDTLS_SSL_PROTO_NO_TLS */
#if defined(MBEDTLS_SSL_ALPN)
    "MBEDTLS_SSL_ALPN",
#endif /* MBEDTLS_SSL_ALPN */
#if defined(MBEDTLS_SSL_DTLS_ANTI_REPLAY)
    "MBEDTLS_SSL_DTLS_ANTI_REPLAY",
#endif /* MBEDTLS_SSL_DTLS_ANTI_REPLAY */
#if defined(MBEDTLS_SSL_DTLS_HELLO_VERIFY)
    "MBEDTLS_SSL_DTLS_HELLO_VERIFY",
#endif /* MBEDTLS_SSL_DTLS_HELLO_VERIFY */
#if defined(MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE)
    "MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE",
#endif /* MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE */
#if defined(MBEDTLS_SSL_DTLS_BADMAC_LIMIT)
    "MBEDTLS_SSL_DTLS_BADMAC_LIMIT",
#endif /* MBEDTLS_SSL_DTLS_BADMAC_LIMIT */
#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    "MBEDTLS_SSL_SESSION_TICKETS",
#endif /* MBEDTLS_SSL_SESSION_TICKETS */
#if defined(MBEDTLS_SSL_NO_SESSION_CACHE)
    "MBEDTLS_SSL_NO_SESSION_CACHE",
#endif /* MBEDTLS_SSL_NO_SESSION_CACHE */
#if defined(MBEDTLS_SSL_NO_SESSION_RESUMPTION)
    "MBEDTLS_SSL_NO_SESSION_RESUMPTION",
#endif /* MBEDTLS_SSL_NO_SESSION_RESUMPTION */
#if defined(MBEDTLS_SSL_EXPORT_KEYS)
    "MBEDTLS_SSL_EXPORT_KEYS",
#endif /* MBEDTLS_SSL_EXPORT_KEYS */
#if defined(MBEDTLS_SSL_SERVER_NAME_INDICATION)
    "MBEDTLS_SSL_SERVER_NAME_INDICATION",
#endif /* MBEDTLS_SSL_SERVER_NAME_INDICATION */
#if defined(MBEDTLS_SSL_TRUNCATED_HMAC)
    "MBEDTLS_SSL_TRUNCATED_HMAC",
#endif /* MBEDTLS_SSL_TRUNCATED_HMAC */
#if defined(MBEDTLS_SSL_TRUNCATED_HMAC_COMPAT)
    "MBEDTLS_SSL_TRUNCATED_HMAC_COMPAT",
#endif /* MBEDTLS_SSL_TRUNCATED_HMAC_COMPAT */
#if defined(MBEDTLS_THREADING_ALT)
    "MBEDTLS_THREADING_ALT",
#endif /* MBEDTLS_THREADING_ALT */
#if defined(MBEDTLS_THREADING_PTHREAD)
    "MBEDTLS_THREADING_PTHREAD",
#endif /* MBEDTLS_THREADING_PTHREAD */
#if defined(MBEDTLS_VERSION_FEATURES)
    "MBEDTLS_VERSION_FEATURES",
#endif /* MBEDTLS_VERSION_FEATURES */
#if defined(MBEDTLS_X509_ON_DEMAND_PARSING)
    "MBEDTLS_X509_ON_DEMAND_PARSING",
#endif /* MBEDTLS_X509_ON_DEMAND_PARSING */
#if defined(MBEDTLS_X509_ALWAYS_FLUSH)
    "MBEDTLS_X509_ALWAYS_FLUSH",
#endif /* MBEDTLS_X509_ALWAYS_FLUSH */
#if defined(MBEDTLS_X509_ALLOW_EXTENSIONS_NON_V3)
    "MBEDTLS_X509_ALLOW_EXTENSIONS_NON_V3",
#endif /* MBEDTLS_X509_ALLOW_EXTENSIONS_NON_V3 */
#if defined(MBEDTLS_X509_ALLOW_UNSUPPORTED_CRITICAL_EXTENSION)
    "MBEDTLS_X509_ALLOW_UNSUPPORTED_CRITICAL_EXTENSION",
#endif /* MBEDTLS_X509_ALLOW_UNSUPPORTED_CRITICAL_EXTENSION */
#if defined(MBEDTLS_X509_CHECK_KEY_USAGE)
    "MBEDTLS_X509_CHECK_KEY_USAGE",
#endif /* MBEDTLS_X509_CHECK_KEY_USAGE */
#if defined(MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE)
    "MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE",
#endif /* MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE */
#if defined(MBEDTLS_X509_REMOVE_INFO)
    "MBEDTLS_X509_REMOVE_INFO",
#endif /* MBEDTLS_X509_REMOVE_INFO */
#if defined(MBEDTLS_X509_CRT_REMOVE_TIME)
    "MBEDTLS_X509_CRT_REMOVE_TIME",
#endif /* MBEDTLS_X509_CRT_REMOVE_TIME */
#if defined(MBEDTLS_X509_CRT_REMOVE_SUBJECT_ISSUER_ID)
    "MBEDTLS_X509_CRT_REMOVE_SUBJECT_ISSUER_ID",
#endif /* MBEDTLS_X509_CRT_REMOVE_SUBJECT_ISSUER_ID */
#if defined(MBEDTLS_X509_REMOVE_HOSTNAME_VERIFICATION)
    "MBEDTLS_X509_REMOVE_HOSTNAME_VERIFICATION",
#endif /* MBEDTLS_X509_REMOVE_HOSTNAME_VERIFICATION */
#if defined(MBEDTLS_X509_REMOVE_VERIFY_CALLBACK)
    "MBEDTLS_X509_REMOVE_VERIFY_CALLBACK",
#endif /* MBEDTLS_X509_REMOVE_VERIFY_CALLBACK */
#if defined(MBEDTLS_X509_RSASSA_PSS_SUPPORT)
    "MBEDTLS_X509_RSASSA_PSS_SUPPORT",
#endif /* MBEDTLS_X509_RSASSA_PSS_SUPPORT */
#if defined(MBEDTLS_ZLIB_SUPPORT)
    "MBEDTLS_ZLIB_SUPPORT",
#endif /* MBEDTLS_ZLIB_SUPPORT */
#if defined(MBEDTLS_AESNI_C)
    "MBEDTLS_AESNI_C",
#endif /* MBEDTLS_AESNI_C */
#if defined(MBEDTLS_AES_C)
    "MBEDTLS_AES_C",
#endif /* MBEDTLS_AES_C */
#if defined(MBEDTLS_ARC4_C)
    "MBEDTLS_ARC4_C",
#endif /* MBEDTLS_ARC4_C */
#if defined(MBEDTLS_ASN1_PARSE_C)
    "MBEDTLS_ASN1_PARSE_C",
#endif /* MBEDTLS_ASN1_PARSE_C */
#if defined(MBEDTLS_ASN1_WRITE_C)
    "MBEDTLS_ASN1_WRITE_C",
#endif /* MBEDTLS_ASN1_WRITE_C */
#if defined(MBEDTLS_BASE64_C)
    "MBEDTLS_BASE64_C",
#endif /* MBEDTLS_BASE64_C */
#if defined(MBEDTLS_BIGNUM_C)
    "MBEDTLS_BIGNUM_C",
#endif /* MBEDTLS_BIGNUM_C */
#if defined(MBEDTLS_BLOWFISH_C)
    "MBEDTLS_BLOWFISH_C",
#endif /* MBEDTLS_BLOWFISH_C */
#if defined(MBEDTLS_CAMELLIA_C)
    "MBEDTLS_CAMELLIA_C",
#endif /* MBEDTLS_CAMELLIA_C */
#if defined(MBEDTLS_ARIA_C)
    "MBEDTLS_ARIA_C",
#endif /* MBEDTLS_ARIA_C */
#if defined(MBEDTLS_CCM_C)
    "MBEDTLS_CCM_C",
#endif /* MBEDTLS_CCM_C */
#if defined(MBEDTLS_CERTS_C)
    "MBEDTLS_CERTS_C",
#endif /* MBEDTLS_CERTS_C */
#if defined(MBEDTLS_CHACHA20_C)
    "MBEDTLS_CHACHA20_C",
#endif /* MBEDTLS_CHACHA20_C */
#if defined(MBEDTLS_CHACHAPOLY_C)
    "MBEDTLS_CHACHAPOLY_C",
#endif /* MBEDTLS_CHACHAPOLY_C */
#if defined(MBEDTLS_CIPHER_C)
    "MBEDTLS_CIPHER_C",
#endif /* MBEDTLS_CIPHER_C */
#if defined(MBEDTLS_CMAC_C)
    "MBEDTLS_CMAC_C",
#endif /* MBEDTLS_CMAC_C */
#if defined(MBEDTLS_CTR_DRBG_C)
    "MBEDTLS_CTR_DRBG_C",
#endif /* MBEDTLS_CTR_DRBG_C */
#if defined(MBEDTLS_DEBUG_C)
    "MBEDTLS_DEBUG_C",
#endif /* MBEDTLS_DEBUG_C */
#if defined(MBEDTLS_DES_C)
    "MBEDTLS_DES_C",
#endif /* MBEDTLS_DES_C */
#if defined(MBEDTLS_DHM_C)
    "MBEDTLS_DHM_C",
#endif /* MBEDTLS_DHM_C */
#if defined(MBEDTLS_ECDH_C)
    "MBEDTLS_ECDH_C",
#endif /* MBEDTLS_ECDH_C */
#if defined(MBEDTLS_ECDSA_C)
    "MBEDTLS_ECDSA_C",
#endif /* MBEDTLS_ECDSA_C */
#if defined(MBEDTLS_ECJPAKE_C)
    "MBEDTLS_ECJPAKE_C",
#endif /* MBEDTLS_ECJPAKE_C */
#if defined(MBEDTLS_ECP_C)
    "MBEDTLS_ECP_C",
#endif /* MBEDTLS_ECP_C */
#if defined(MBEDTLS_ENABLE_TINYCRYPT)
    "MBEDTLS_ENABLE_TINYCRYPT",
#endif /* MBEDTLS_ENABLE_TINYCRYPT */
#if defined(MBEDTLS_USE_TINYCRYPT)
    "MBEDTLS_USE_TINYCRYPT",
#endif /* MBEDTLS_USE_TINYCRYPT */
#if defined(MBEDTLS_ENTROPY_C)
    "MBEDTLS_ENTROPY_C",
#endif /* MBEDTLS_ENTROPY_C */
#if defined(MBEDTLS_ERROR_C)
    "MBEDTLS_ERROR_C",
#endif /* MBEDTLS_ERROR_C */
#if defined(MBEDTLS_GCM_C)
    "MBEDTLS_GCM_C",
#endif /* MBEDTLS_GCM_C */
#if defined(MBEDTLS_HAVEGE_C)
    "MBEDTLS_HAVEGE_C",
#endif /* MBEDTLS_HAVEGE_C */
#if defined(MBEDTLS_HKDF_C)
    "MBEDTLS_HKDF_C",
#endif /* MBEDTLS_HKDF_C */
#if defined(MBEDTLS_HMAC_DRBG_C)
    "MBEDTLS_HMAC_DRBG_C",
#endif /* MBEDTLS_HMAC_DRBG_C */
#if defined(MBEDTLS_NIST_KW_C)
    "MBEDTLS_NIST_KW_C",
#endif /* MBEDTLS_NIST_KW_C */
#if defined(MBEDTLS_MD_C)
    "MBEDTLS_MD_C",
#endif /* MBEDTLS_MD_C */
#if defined(MBEDTLS_MD2_C)
    "MBEDTLS_MD2_C",
#endif /* MBEDTLS_MD2_C */
#if defined(MBEDTLS_MD4_C)
    "MBEDTLS_MD4_C",
#endif /* MBEDTLS_MD4_C */
#if defined(MBEDTLS_MD5_C)
    "MBEDTLS_MD5_C",
#endif /* MBEDTLS_MD5_C */
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    "MBEDTLS_MEMORY_BUFFER_ALLOC_C",
#endif /* MBEDTLS_MEMORY_BUFFER_ALLOC_C */
#if defined(MBEDTLS_NET_C)
    "MBEDTLS_NET_C",
#endif /* MBEDTLS_NET_C */
#if defined(MBEDTLS_OID_C)
    "MBEDTLS_OID_C",
#endif /* MBEDTLS_OID_C */
#if defined(MBEDTLS_PADLOCK_C)
    "MBEDTLS_PADLOCK_C",
#endif /* MBEDTLS_PADLOCK_C */
#if defined(MBEDTLS_PEM_PARSE_C)
    "MBEDTLS_PEM_PARSE_C",
#endif /* MBEDTLS_PEM_PARSE_C */
#if defined(MBEDTLS_PEM_WRITE_C)
    "MBEDTLS_PEM_WRITE_C",
#endif /* MBEDTLS_PEM_WRITE_C */
#if defined(MBEDTLS_PK_C)
    "MBEDTLS_PK_C",
#endif /* MBEDTLS_PK_C */
#if defined(MBEDTLS_PK_PARSE_C)
    "MBEDTLS_PK_PARSE_C",
#endif /* MBEDTLS_PK_PARSE_C */
#if defined(MBEDTLS_PK_WRITE_C)
    "MBEDTLS_PK_WRITE_C",
#endif /* MBEDTLS_PK_WRITE_C */
#if defined(MBEDTLS_PKCS5_C)
    "MBEDTLS_PKCS5_C",
#endif /* MBEDTLS_PKCS5_C */
#if defined(MBEDTLS_PKCS11_C)
    "MBEDTLS_PKCS11_C",
#endif /* MBEDTLS_PKCS11_C */
#if defined(MBEDTLS_PKCS12_C)
    "MBEDTLS_PKCS12_C",
#endif /* MBEDTLS_PKCS12_C */
#if defined(MBEDTLS_PLATFORM_C)
    "MBEDTLS_PLATFORM_C",
#endif /* MBEDTLS_PLATFORM_C */
#if defined(MBEDTLS_POLY1305_C)
    "MBEDTLS_POLY1305_C",
#endif /* MBEDTLS_POLY1305_C */
#if defined(MBEDTLS_RIPEMD160_C)
    "MBEDTLS_RIPEMD160_C",
#endif /* MBEDTLS_RIPEMD160_C */
#if defined(MBEDTLS_RSA_C)
    "MBEDTLS_RSA_C",
#endif /* MBEDTLS_RSA_C */
#if defined(MBEDTLS_SHA1_C)
    "MBEDTLS_SHA1_C",
#endif /* MBEDTLS_SHA1_C */
#if defined(MBEDTLS_SHA256_C)
    "MBEDTLS_SHA256_C",
#endif /* MBEDTLS_SHA256_C */
#if defined(MBEDTLS_SHA512_C)
    "MBEDTLS_SHA512_C",
#endif /* MBEDTLS_SHA512_C */
#if defined(MBEDTLS_SSL_CACHE_C)
    "MBEDTLS_SSL_CACHE_C",
#endif /* MBEDTLS_SSL_CACHE_C */
#if defined(MBEDTLS_SSL_COOKIE_C)
    "MBEDTLS_SSL_COOKIE_C",
#endif /* MBEDTLS_SSL_COOKIE_C */
#if defined(MBEDTLS_SSL_TICKET_C)
    "MBEDTLS_SSL_TICKET_C",
#endif /* MBEDTLS_SSL_TICKET_C */
#if defined(MBEDTLS_SSL_CLI_C)
    "MBEDTLS_SSL_CLI_C",
#endif /* MBEDTLS_SSL_CLI_C */
#if defined(MBEDTLS_SSL_SRV_C)
    "MBEDTLS_SSL_SRV_C",
#endif /* MBEDTLS_SSL_SRV_C */
#if defined(MBEDTLS_SSL_TLS_C)
    "MBEDTLS_SSL_TLS_C",
#endif /* MBEDTLS_SSL_TLS_C */
#if defined(MBEDTLS_THREADING_C)
    "MBEDTLS_THREADING_C",
#endif /* MBEDTLS_THREADING_C */
#if defined(MBEDTLS_TIMING_C)
    "MBEDTLS_TIMING_C",
#endif /* MBEDTLS_TIMING_C */
#if defined(MBEDTLS_VERSION_C)
    "MBEDTLS_VERSION_C",
#endif /* MBEDTLS_VERSION_C */
#if defined(MBEDTLS_X509_USE_C)
    "MBEDTLS_X509_USE_C",
#endif /* MBEDTLS_X509_USE_C */
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    "MBEDTLS_X509_CRT_PARSE_C",
#endif /* MBEDTLS_X509_CRT_PARSE_C */
#if defined(MBEDTLS_X509_CRL_PARSE_C)
    "MBEDTLS_X509_CRL_PARSE_C",
#endif /* MBEDTLS_X509_CRL_PARSE_C */
#if defined(MBEDTLS_X509_CSR_PARSE_C)
    "MBEDTLS_X509_CSR_PARSE_C",
#endif /* MBEDTLS_X509_CSR_PARSE_C */
#if defined(MBEDTLS_X509_CREATE_C)
    "MBEDTLS_X509_CREATE_C",
#endif /* MBEDTLS_X509_CREATE_C */
#if defined(MBEDTLS_X509_CRT_WRITE_C)
    "MBEDTLS_X509_CRT_WRITE_C",
#endif /* MBEDTLS_X509_CRT_WRITE_C */
#if defined(MBEDTLS_X509_CSR_WRITE_C)
    "MBEDTLS_X509_CSR_WRITE_C",
#endif /* MBEDTLS_X509_CSR_WRITE_C */
#if defined(MBEDTLS_XTEA_C)
    "MBEDTLS_XTEA_C",
#endif /* MBEDTLS_XTEA_C */
#endif /* MBEDTLS_VERSION_FEATURES */
    NULL
};

int mbedtls_version_check_feature( const char *feature )
{
    const char **idx = features;

    if( *idx == NULL )
        return( -2 );

    if( feature == NULL )
        return( -1 );

    while( *idx != NULL )
    {
        if( !strcmp( *idx, feature ) )
            return( 0 );
        idx++;
    }
    return( -1 );
}

#endif /* MBEDTLS_VERSION_C */
