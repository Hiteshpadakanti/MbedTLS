/*
 *  Public Key abstraction layer
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#include "common.h"

#if defined(MBEDTLS_PK_C)
#include "mbedtls/pk.h"
#include "pk_wrap.h"
#include "pkwrite.h"
#include "pk_internal.h"

#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"

#if defined(MBEDTLS_RSA_C)
#include "mbedtls/rsa.h"
#if defined(MBEDTLS_PKCS1_V21) && !defined(MBEDTLS_USE_PSA_CRYPTO)
#include "rsa_internal.h"
#endif
#endif
#if defined(MBEDTLS_PK_HAVE_ECC_KEYS)
#include "mbedtls/ecp.h"
#endif
#if defined(MBEDTLS_ECDSA_C)
#include "mbedtls/ecdsa.h"
#endif

#if defined(MBEDTLS_USE_PSA_CRYPTO)
#include "psa_util_internal.h"
#include "md_psa.h"
#endif

#include <limits.h>
#include <stdint.h>

/*
 * Initialise a mbedtls_pk_context
 */
void mbedtls_pk_init(mbedtls_pk_context *ctx)
{
    ctx->pk_info = NULL;
    ctx->pk_ctx = NULL;
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    ctx->priv_id = MBEDTLS_SVC_KEY_ID_INIT;
#endif /* MBEDTLS_USE_PSA_CRYPTO */
#if defined(MBEDTLS_PK_USE_PSA_EC_DATA)
    memset(ctx->pub_raw, 0, sizeof(ctx->pub_raw));
    ctx->pub_raw_len = 0;
    ctx->ec_family = 0;
    ctx->ec_bits = 0;
#endif /* MBEDTLS_PK_USE_PSA_EC_DATA */
}

/*
 * Free (the components of) a mbedtls_pk_context
 */
void mbedtls_pk_free(mbedtls_pk_context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if ((ctx->pk_info != NULL) && (ctx->pk_info->ctx_free_func != NULL)) {
        ctx->pk_info->ctx_free_func(ctx->pk_ctx);
    }

#if defined(MBEDTLS_PK_USE_PSA_EC_DATA)
    /* The ownership of the priv_id key for opaque keys is external of the PK
     * module. It's the user responsibility to clear it after use. */
    if ((ctx->pk_info != NULL) && (ctx->pk_info->type != MBEDTLS_PK_OPAQUE)) {
        psa_destroy_key(ctx->priv_id);
    }
#endif /* MBEDTLS_PK_USE_PSA_EC_DATA */

    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_pk_context));
}

#if defined(MBEDTLS_ECDSA_C) && defined(MBEDTLS_ECP_RESTARTABLE)
/*
 * Initialize a restart context
 */
void mbedtls_pk_restart_init(mbedtls_pk_restart_ctx *ctx)
{
    ctx->pk_info = NULL;
    ctx->rs_ctx = NULL;
}

/*
 * Free the components of a restart context
 */
void mbedtls_pk_restart_free(mbedtls_pk_restart_ctx *ctx)
{
    if (ctx == NULL || ctx->pk_info == NULL ||
        ctx->pk_info->rs_free_func == NULL) {
        return;
    }

    ctx->pk_info->rs_free_func(ctx->rs_ctx);

    ctx->pk_info = NULL;
    ctx->rs_ctx = NULL;
}
#endif /* MBEDTLS_ECDSA_C && MBEDTLS_ECP_RESTARTABLE */

/*
 * Get pk_info structure from type
 */
const mbedtls_pk_info_t *mbedtls_pk_info_from_type(mbedtls_pk_type_t pk_type)
{
    switch (pk_type) {
#if defined(MBEDTLS_RSA_C)
        case MBEDTLS_PK_RSA:
            return &mbedtls_rsa_info;
#endif /* MBEDTLS_RSA_C */
#if defined(MBEDTLS_PK_HAVE_ECC_KEYS)
        case MBEDTLS_PK_ECKEY:
            return &mbedtls_eckey_info;
        case MBEDTLS_PK_ECKEY_DH:
            return &mbedtls_eckeydh_info;
#endif /* MBEDTLS_PK_HAVE_ECC_KEYS */
#if defined(MBEDTLS_PK_CAN_ECDSA_SOME)
        case MBEDTLS_PK_ECDSA:
            return &mbedtls_ecdsa_info;
#endif /* MBEDTLS_PK_CAN_ECDSA_SOME */
        /* MBEDTLS_PK_RSA_ALT omitted on purpose */
        default:
            return NULL;
    }
}

/*
 * Initialise context
 */
int mbedtls_pk_setup(mbedtls_pk_context *ctx, const mbedtls_pk_info_t *info)
{
    if (info == NULL || ctx->pk_info != NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if ((info->ctx_alloc_func != NULL) &&
        ((ctx->pk_ctx = info->ctx_alloc_func()) == NULL)) {
        return MBEDTLS_ERR_PK_ALLOC_FAILED;
    }

    ctx->pk_info = info;

    return 0;
}

#if defined(MBEDTLS_USE_PSA_CRYPTO)
/*
 * Initialise a PSA-wrapping context
 */
int mbedtls_pk_setup_opaque(mbedtls_pk_context *ctx,
                            const mbedtls_svc_key_id_t key)
{
    const mbedtls_pk_info_t *info = NULL;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_type_t type;

    if (ctx == NULL || ctx->pk_info != NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (PSA_SUCCESS != psa_get_key_attributes(key, &attributes)) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }
    type = psa_get_key_type(&attributes);
    psa_reset_key_attributes(&attributes);

#if defined(MBEDTLS_PK_HAVE_ECC_KEYS)
    if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(type)) {
        info = &mbedtls_ecdsa_opaque_info;
    } else
#endif /* MBEDTLS_PK_HAVE_ECC_KEYS */
    if (type == PSA_KEY_TYPE_RSA_KEY_PAIR) {
        info = &mbedtls_rsa_opaque_info;
    } else {
        return MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE;
    }

    ctx->pk_info = info;
    ctx->priv_id = key;

    return 0;
}
#endif /* MBEDTLS_USE_PSA_CRYPTO */

#if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
/*
 * Initialize an RSA-alt context
 */
int mbedtls_pk_setup_rsa_alt(mbedtls_pk_context *ctx, void *key,
                             mbedtls_pk_rsa_alt_decrypt_func decrypt_func,
                             mbedtls_pk_rsa_alt_sign_func sign_func,
                             mbedtls_pk_rsa_alt_key_len_func key_len_func)
{
    mbedtls_rsa_alt_context *rsa_alt;
    const mbedtls_pk_info_t *info = &mbedtls_rsa_alt_info;

    if (ctx->pk_info != NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if ((ctx->pk_ctx = info->ctx_alloc_func()) == NULL) {
        return MBEDTLS_ERR_PK_ALLOC_FAILED;
    }

    ctx->pk_info = info;

    rsa_alt = (mbedtls_rsa_alt_context *) ctx->pk_ctx;

    rsa_alt->key = key;
    rsa_alt->decrypt_func = decrypt_func;
    rsa_alt->sign_func = sign_func;
    rsa_alt->key_len_func = key_len_func;

    return 0;
}
#endif /* MBEDTLS_PK_RSA_ALT_SUPPORT */

/*
 * Tell if a PK can do the operations of the given type
 */
int mbedtls_pk_can_do(const mbedtls_pk_context *ctx, mbedtls_pk_type_t type)
{
    /* A context with null pk_info is not set up yet and can't do anything.
     * For backward compatibility, also accept NULL instead of a context
     * pointer. */
    if (ctx == NULL || ctx->pk_info == NULL) {
        return 0;
    }

    return ctx->pk_info->can_do(type);
}

#if defined(MBEDTLS_USE_PSA_CRYPTO)
/*
 * Tell if a PK can do the operations of the given PSA algorithm
 */
int mbedtls_pk_can_do_ext(const mbedtls_pk_context *ctx, psa_algorithm_t alg,
                          psa_key_usage_t usage)
{
    psa_key_usage_t key_usage;

    /* A context with null pk_info is not set up yet and can't do anything.
     * For backward compatibility, also accept NULL instead of a context
     * pointer. */
    if (ctx == NULL || ctx->pk_info == NULL) {
        return 0;
    }

    /* Filter out non allowed algorithms */
    if (PSA_ALG_IS_ECDSA(alg) == 0 &&
        PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) == 0 &&
        PSA_ALG_IS_RSA_PSS(alg) == 0 &&
        alg != PSA_ALG_RSA_PKCS1V15_CRYPT &&
        PSA_ALG_IS_ECDH(alg) == 0) {
        return 0;
    }

    /* Filter out non allowed usage flags */
    if (usage == 0 ||
        (usage & ~(PSA_KEY_USAGE_SIGN_HASH |
                   PSA_KEY_USAGE_DECRYPT |
                   PSA_KEY_USAGE_DERIVE)) != 0) {
        return 0;
    }

    /* Wildcard hash is not allowed */
    if (PSA_ALG_IS_SIGN_HASH(alg) &&
        PSA_ALG_SIGN_GET_HASH(alg) == PSA_ALG_ANY_HASH) {
        return 0;
    }

    if (mbedtls_pk_get_type(ctx) != MBEDTLS_PK_OPAQUE) {
        mbedtls_pk_type_t type;

        if (PSA_ALG_IS_ECDSA(alg) || PSA_ALG_IS_ECDH(alg)) {
            type = MBEDTLS_PK_ECKEY;
        } else if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) ||
                   alg == PSA_ALG_RSA_PKCS1V15_CRYPT) {
            type = MBEDTLS_PK_RSA;
        } else if (PSA_ALG_IS_RSA_PSS(alg)) {
            type = MBEDTLS_PK_RSASSA_PSS;
        } else {
            return 0;
        }

        if (ctx->pk_info->can_do(type) == 0) {
            return 0;
        }

        switch (type) {
            case MBEDTLS_PK_ECKEY:
                key_usage = PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_DERIVE;
                break;
            case MBEDTLS_PK_RSA:
            case MBEDTLS_PK_RSASSA_PSS:
                key_usage = PSA_KEY_USAGE_SIGN_HASH |
                            PSA_KEY_USAGE_SIGN_MESSAGE |
                            PSA_KEY_USAGE_DECRYPT;
                break;
            default:
                /* Should never happen */
                return 0;
        }

        return (key_usage & usage) == usage;
    }

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t status;

    status = psa_get_key_attributes(ctx->priv_id, &attributes);
    if (status != PSA_SUCCESS) {
        return 0;
    }

    psa_algorithm_t key_alg = psa_get_key_algorithm(&attributes);
    /* Key's enrollment is available only when MBEDTLS_PSA_CRYPTO_CLIENT is
     * defined, i.e. when the Mbed TLS implementation of PSA Crypto is being used.
     * Even though we don't officially support using other implementations of PSA
     * Crypto with TLS and X.509 (yet), we're still trying to simplify the life of
     * people who would like to try it before it's officially supported. */
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT)
    psa_algorithm_t key_alg2 = psa_get_key_enrollment_algorithm(&attributes);
#endif /* MBEDTLS_PSA_CRYPTO_CLIENT */
    key_usage = psa_get_key_usage_flags(&attributes);
    psa_reset_key_attributes(&attributes);

    if ((key_usage & usage) != usage) {
        return 0;
    }

    /*
     * Common case: the key alg [or alg2] only allows alg.
     * This will match PSA_ALG_RSA_PKCS1V15_CRYPT & PSA_ALG_IS_ECDH
     * directly.
     * This would also match ECDSA/RSA_PKCS1V15_SIGN/RSA_PSS with
     * a fixed hash on key_alg [or key_alg2].
     */
    if (alg == key_alg) {
        return 1;
    }
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT)
    if (alg == key_alg2) {
        return 1;
    }
#endif /* MBEDTLS_PSA_CRYPTO_CLIENT */

    /*
     * If key_alg [or key_alg2] is a hash-and-sign with a wildcard for the hash,
     * and alg is the same hash-and-sign family with any hash,
     * then alg is compliant with this key alg
     */
    if (PSA_ALG_IS_SIGN_HASH(alg)) {

        if (PSA_ALG_IS_SIGN_HASH(key_alg) &&
            PSA_ALG_SIGN_GET_HASH(key_alg) == PSA_ALG_ANY_HASH &&
            (alg & ~PSA_ALG_HASH_MASK) == (key_alg & ~PSA_ALG_HASH_MASK)) {
            return 1;
        }
#if defined(MBEDTLS_PSA_CRYPTO_CLIENT)
        if (PSA_ALG_IS_SIGN_HASH(key_alg2) &&
            PSA_ALG_SIGN_GET_HASH(key_alg2) == PSA_ALG_ANY_HASH &&
            (alg & ~PSA_ALG_HASH_MASK) == (key_alg2 & ~PSA_ALG_HASH_MASK)) {
            return 1;
        }
#endif /* MBEDTLS_PSA_CRYPTO_CLIENT */
    }

    return 0;
}
#endif /* MBEDTLS_USE_PSA_CRYPTO */

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT)
int mbedtls_pk_guess_psa_attributes(const mbedtls_pk_context *pk,
                                    psa_key_attributes_t *attributes)
{
    int has_private = 0;
    psa_reset_key_attributes(attributes);

    switch (mbedtls_pk_get_type(pk)) {
#if defined(MBEDTLS_RSA_C)
        case MBEDTLS_PK_RSA:
        {
            mbedtls_rsa_context *rsa = mbedtls_pk_rsa(*pk);
            /* Detect the presence of a private key in a way that works both
             * in CRT and non-CRT configurations. */
            if (mbedtls_rsa_check_privkey(rsa) == 0) {
                has_private = 1;
            }
            psa_set_key_type(attributes,
                             has_private ?
                             PSA_KEY_TYPE_RSA_KEY_PAIR :
                             PSA_KEY_TYPE_RSA_PUBLIC_KEY);
            psa_set_key_bits(attributes, mbedtls_mpi_bitlen(&rsa->N));
            /* Assume a signature algorithm. Allow any hash. */
            psa_set_key_algorithm(attributes,
                                  rsa->padding == MBEDTLS_RSA_PKCS_V21 ?
                                  PSA_ALG_RSA_PSS_ANY_SALT(PSA_ALG_ANY_HASH) :
                                  PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_ANY_HASH));
            break;
        }
#endif /* MBEDTLS_RSA_C */

#if defined(MBEDTLS_ECP_C)
        case MBEDTLS_PK_ECKEY:
        case MBEDTLS_PK_ECKEY_DH:
        case MBEDTLS_PK_ECDSA:
        {
            mbedtls_ecp_keypair *ec = mbedtls_pk_ec(*pk);
            size_t bits = 0;
            psa_ecc_family_t family =
                mbedtls_ecc_group_to_psa(ec->grp.id, &bits);
            has_private = (ec->d.n != 0);
            psa_set_key_type(attributes,
                             has_private ?
                             PSA_KEY_TYPE_ECC_KEY_PAIR(family) :
                             PSA_KEY_TYPE_ECC_PUBLIC_KEY(family));
            psa_set_key_bits(attributes, bits);
            if (mbedtls_pk_get_type(pk) == MBEDTLS_PK_ECKEY_DH ||
                (mbedtls_pk_get_type(pk) == MBEDTLS_PK_ECKEY &&
                 family == PSA_ECC_FAMILY_MONTGOMERY)) {
                psa_set_key_algorithm(attributes, PSA_ALG_ECDH);
            } else {
#if defined(MBEDTLS_ECDSA_DETERMINISTIC)
                psa_set_key_algorithm(attributes,
                                      PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_ANY_HASH));
#else
                psa_set_key_algorithm(attributes, PSA_ALG_ECDSA_ANY);
#endif
            }
            break;
        }

#endif /* MBEDTLS_ECP_C */

#if defined(MBEDTLS_USE_PSA_CRYPTO)
        case MBEDTLS_PK_OPAQUE:
        {
            psa_status_t status = psa_get_key_attributes(pk->priv_id,
                                                         attributes);
            if (status != PSA_SUCCESS) {
                return PSA_PK_TO_MBEDTLS_ERR(status);
            }
            psa_set_key_lifetime(attributes, PSA_KEY_LIFETIME_VOLATILE);
            psa_set_key_id(attributes, MBEDTLS_SVC_KEY_ID_INIT);
            return 0;
        }
#endif /* MBEDTLS_USE_PSA_CRYPTO */

        default:
            return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    psa_key_usage_t usage = PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_COPY;
    if (psa_get_key_algorithm(attributes) == PSA_ALG_ECDH) {
        if (has_private) {
            usage |= PSA_KEY_USAGE_DERIVE;
        }
    } else {
        usage |= PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH;
        if (has_private) {
            usage |= PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_SIGN_HASH;
        }
    }
    psa_set_key_usage_flags(attributes, usage);

    return 0;
}

int mbedtls_pk_import_into_psa(const mbedtls_pk_context *pk,
                               const psa_key_attributes_t *attributes,
                               mbedtls_svc_key_id_t *key_id)
{
#if PSA_EXPORT_KEY_PAIR_MAX_SIZE > PSA_EXPORT_PUBLIC_KEY_MAX_SIZE
    unsigned char buf[PSA_EXPORT_KEY_PAIR_MAX_SIZE];
#else
    unsigned char buf[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
#endif
    unsigned char *p = NULL;
    size_t len = 0;
    psa_key_type_t type = psa_get_key_type(attributes);
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

#if defined(MBEDTLS_USE_PSA_CRYPTO)
    if (mbedtls_pk_get_type(pk) == MBEDTLS_PK_OPAQUE) {
        psa_status_t status = psa_copy_key(pk->priv_id, attributes, key_id);
        return PSA_PK_TO_MBEDTLS_ERR(status);
    }
#endif /* MBEDTLS_USE_PSA_CRYPTO */

#if defined(MBEDTLS_RSA_C)
    if (type == PSA_KEY_TYPE_RSA_KEY_PAIR) {
        if (mbedtls_pk_get_type(pk) != MBEDTLS_PK_RSA) {
            return MBEDTLS_ERR_PK_TYPE_MISMATCH;
        }
        ret = mbedtls_pk_write_key_der(pk, buf, sizeof(buf));
        if (ret < 0) {
            return ret;
        }
        len = ret;
        p = buf + sizeof(buf) - len;
    } else if (type == PSA_KEY_TYPE_RSA_PUBLIC_KEY) {
        if (mbedtls_pk_get_type(pk) != MBEDTLS_PK_RSA) {
            return MBEDTLS_ERR_PK_TYPE_MISMATCH;
        }
        p = buf + sizeof(buf);
        ret = mbedtls_pk_write_pubkey(&p, buf, pk);
        if (ret < 0) {
            return ret;
        }
        len = buf + sizeof(buf) - p;
    } else
#endif /* MBEDTLS_RSA_C */

#if defined(MBEDTLS_ECP_C)
    if (PSA_KEY_TYPE_IS_ECC(type)) {
        if (mbedtls_pk_get_type(pk) != MBEDTLS_PK_ECKEY &&
            mbedtls_pk_get_type(pk) != MBEDTLS_PK_ECKEY_DH &&
            mbedtls_pk_get_type(pk) != MBEDTLS_PK_ECDSA) {
            return MBEDTLS_ERR_PK_TYPE_MISMATCH;
        }

#if defined(MBEDTLS_PK_USE_PSA_EC_DATA)
        if (PSA_KEY_TYPE_IS_KEY_PAIR(type)) {
            psa_status_t status = psa_export_key(pk->priv_id,
                                                 buf, sizeof(buf), &len);
            if (status != PSA_SUCCESS) {
                return PSA_PK_TO_MBEDTLS_ERR(status);
            }
            p = buf;
        } else {
            if (pk->pub_raw_len > sizeof(buf)) {
                return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
            }
            memcpy(buf, pk->pub_raw, pk->pub_raw_len);
            len = pk->pub_raw_len;
            p = buf;
        }

#else /* MBEDTLS_PK_USE_PSA_EC_DATA */
        mbedtls_ecp_keypair *ec = mbedtls_pk_ec(*pk);

        if (PSA_KEY_TYPE_IS_KEY_PAIR(type)) {
            len = PSA_BITS_TO_BYTES(ec->grp.nbits);
            if (len > sizeof(buf)) {
                return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
            }
            ret = mbedtls_ecp_write_key(ec, buf, len);
        } else {
            ret = mbedtls_ecp_point_write_binary(&ec->grp, &ec->Q,
                                                 MBEDTLS_ECP_PF_UNCOMPRESSED,
                                                 &len, buf, sizeof(buf));
        }
        if (ret != 0) {
            return ret;
        }
        p = buf;
#endif /* MBEDTLS_PK_USE_PSA_EC_DATA */
    } else
#endif /* MBEDTLS_ECP_C */

    {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    psa_status_t status = psa_import_key(attributes, p, len, key_id);
    mbedtls_platform_zeroize(p, len);
    return PSA_PK_TO_MBEDTLS_ERR(status);
}
#endif /* MBEDTLS_PSA_CRYPTO_CLIENT */

/*
 * Helper for mbedtls_pk_sign and mbedtls_pk_verify
 */
static inline int pk_hashlen_helper(mbedtls_md_type_t md_alg, size_t *hash_len)
{
    if (*hash_len != 0) {
        return 0;
    }

    *hash_len = mbedtls_md_get_size_from_type(md_alg);

    if (*hash_len == 0) {
        return -1;
    }

    return 0;
}

#if defined(MBEDTLS_ECDSA_C) && defined(MBEDTLS_ECP_RESTARTABLE)
/*
 * Helper to set up a restart context if needed
 */
static int pk_restart_setup(mbedtls_pk_restart_ctx *ctx,
                            const mbedtls_pk_info_t *info)
{
    /* Don't do anything if already set up or invalid */
    if (ctx == NULL || ctx->pk_info != NULL) {
        return 0;
    }

    /* Should never happen when we're called */
    if (info->rs_alloc_func == NULL || info->rs_free_func == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if ((ctx->rs_ctx = info->rs_alloc_func()) == NULL) {
        return MBEDTLS_ERR_PK_ALLOC_FAILED;
    }

    ctx->pk_info = info;

    return 0;
}
#endif /* MBEDTLS_ECDSA_C && MBEDTLS_ECP_RESTARTABLE */

/*
 * Verify a signature (restartable)
 */
int mbedtls_pk_verify_restartable(mbedtls_pk_context *ctx,
                                  mbedtls_md_type_t md_alg,
                                  const unsigned char *hash, size_t hash_len,
                                  const unsigned char *sig, size_t sig_len,
                                  mbedtls_pk_restart_ctx *rs_ctx)
{
    if ((md_alg != MBEDTLS_MD_NONE || hash_len != 0) && hash == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (ctx->pk_info == NULL ||
        pk_hashlen_helper(md_alg, &hash_len) != 0) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

#if defined(MBEDTLS_ECDSA_C) && defined(MBEDTLS_ECP_RESTARTABLE)
    /* optimization: use non-restartable version if restart disabled */
    if (rs_ctx != NULL &&
        mbedtls_ecp_restart_is_enabled() &&
        ctx->pk_info->verify_rs_func != NULL) {
        int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

        if ((ret = pk_restart_setup(rs_ctx, ctx->pk_info)) != 0) {
            return ret;
        }

        ret = ctx->pk_info->verify_rs_func(ctx,
                                           md_alg, hash, hash_len, sig, sig_len, rs_ctx->rs_ctx);

        if (ret != MBEDTLS_ERR_ECP_IN_PROGRESS) {
            mbedtls_pk_restart_free(rs_ctx);
        }

        return ret;
    }
#else /* MBEDTLS_ECDSA_C && MBEDTLS_ECP_RESTARTABLE */
    (void) rs_ctx;
#endif /* MBEDTLS_ECDSA_C && MBEDTLS_ECP_RESTARTABLE */

    if (ctx->pk_info->verify_func == NULL) {
        return MBEDTLS_ERR_PK_TYPE_MISMATCH;
    }

    return ctx->pk_info->verify_func(ctx, md_alg, hash, hash_len,
                                     sig, sig_len);
}

/*
 * Verify a signature
 */
int mbedtls_pk_verify(mbedtls_pk_context *ctx, mbedtls_md_type_t md_alg,
                      const unsigned char *hash, size_t hash_len,
                      const unsigned char *sig, size_t sig_len)
{
    return mbedtls_pk_verify_restartable(ctx, md_alg, hash, hash_len,
                                         sig, sig_len, NULL);
}

/*
 * Verify a signature with options
 */
int mbedtls_pk_verify_ext(mbedtls_pk_type_t type, const void *options,
                          mbedtls_pk_context *ctx, mbedtls_md_type_t md_alg,
                          const unsigned char *hash, size_t hash_len,
                          const unsigned char *sig, size_t sig_len)
{
    if ((md_alg != MBEDTLS_MD_NONE || hash_len != 0) && hash == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (ctx->pk_info == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (!mbedtls_pk_can_do(ctx, type)) {
        return MBEDTLS_ERR_PK_TYPE_MISMATCH;
    }

    if (type != MBEDTLS_PK_RSASSA_PSS) {
        /* General case: no options */
        if (options != NULL) {
            return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
        }

        return mbedtls_pk_verify(ctx, md_alg, hash, hash_len, sig, sig_len);
    }

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PKCS1_V21)
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const mbedtls_pk_rsassa_pss_options *pss_opts;

#if SIZE_MAX > UINT_MAX
    if (md_alg == MBEDTLS_MD_NONE && UINT_MAX < hash_len) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }
#endif

    if (options == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    pss_opts = (const mbedtls_pk_rsassa_pss_options *) options;

#if defined(MBEDTLS_USE_PSA_CRYPTO)
    if (pss_opts->mgf1_hash_id == md_alg) {
        unsigned char buf[MBEDTLS_PK_RSA_PUB_DER_MAX_BYTES];
        unsigned char *p;
        int key_len;
        size_t signature_length;
        psa_status_t status = PSA_ERROR_DATA_CORRUPT;
        psa_status_t destruction_status = PSA_ERROR_DATA_CORRUPT;

        psa_algorithm_t psa_md_alg = mbedtls_md_psa_alg_from_type(md_alg);
        mbedtls_svc_key_id_t key_id = MBEDTLS_SVC_KEY_ID_INIT;
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
        psa_algorithm_t psa_sig_alg = PSA_ALG_RSA_PSS_ANY_SALT(psa_md_alg);
        p = buf + sizeof(buf);
        key_len = mbedtls_pk_write_pubkey(&p, buf, ctx);

        if (key_len < 0) {
            return key_len;
        }

        psa_set_key_type(&attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY);
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);
        psa_set_key_algorithm(&attributes, psa_sig_alg);

        status = psa_import_key(&attributes,
                                buf + sizeof(buf) - key_len, key_len,
                                &key_id);
        if (status != PSA_SUCCESS) {
            psa_destroy_key(key_id);
            return PSA_PK_TO_MBEDTLS_ERR(status);
        }

        /* This function requires returning MBEDTLS_ERR_PK_SIG_LEN_MISMATCH
         * on a valid signature with trailing data in a buffer, but
         * mbedtls_psa_rsa_verify_hash requires the sig_len to be exact,
         * so for this reason the passed sig_len is overwritten. Smaller
         * signature lengths should not be accepted for verification. */
        signature_length = sig_len > mbedtls_pk_get_len(ctx) ?
                           mbedtls_pk_get_len(ctx) : sig_len;
        status = psa_verify_hash(key_id, psa_sig_alg, hash,
                                 hash_len, sig, signature_length);
        destruction_status = psa_destroy_key(key_id);

        if (status == PSA_SUCCESS && sig_len > mbedtls_pk_get_len(ctx)) {
            return MBEDTLS_ERR_PK_SIG_LEN_MISMATCH;
        }

        if (status == PSA_SUCCESS) {
            status = destruction_status;
        }

        return PSA_PK_RSA_TO_MBEDTLS_ERR(status);
    } else
#endif /* MBEDTLS_USE_PSA_CRYPTO */
    {
        if (sig_len < mbedtls_pk_get_len(ctx)) {
            return MBEDTLS_ERR_RSA_VERIFY_FAILED;
        }

        ret = mbedtls_rsa_rsassa_pss_verify_ext(mbedtls_pk_rsa(*ctx),
                                                md_alg, (unsigned int) hash_len, hash,
                                                pss_opts->mgf1_hash_id,
                                                pss_opts->expected_salt_len,
                                                sig);
        if (ret != 0) {
            return ret;
        }

        if (sig_len > mbedtls_pk_get_len(ctx)) {
            return MBEDTLS_ERR_PK_SIG_LEN_MISMATCH;
        }

        return 0;
    }
#else
    return MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE;
#endif /* MBEDTLS_RSA_C && MBEDTLS_PKCS1_V21 */
}

/*
 * Make a signature (restartable)
 */
int mbedtls_pk_sign_restartable(mbedtls_pk_context *ctx,
                                mbedtls_md_type_t md_alg,
                                const unsigned char *hash, size_t hash_len,
                                unsigned char *sig, size_t sig_size, size_t *sig_len,
                                int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
                                mbedtls_pk_restart_ctx *rs_ctx)
{
    if ((md_alg != MBEDTLS_MD_NONE || hash_len != 0) && hash == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (ctx->pk_info == NULL || pk_hashlen_helper(md_alg, &hash_len) != 0) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

#if defined(MBEDTLS_ECDSA_C) && defined(MBEDTLS_ECP_RESTARTABLE)
    /* optimization: use non-restartable version if restart disabled */
    if (rs_ctx != NULL &&
        mbedtls_ecp_restart_is_enabled() &&
        ctx->pk_info->sign_rs_func != NULL) {
        int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

        if ((ret = pk_restart_setup(rs_ctx, ctx->pk_info)) != 0) {
            return ret;
        }

        ret = ctx->pk_info->sign_rs_func(ctx, md_alg,
                                         hash, hash_len,
                                         sig, sig_size, sig_len,
                                         f_rng, p_rng, rs_ctx->rs_ctx);

        if (ret != MBEDTLS_ERR_ECP_IN_PROGRESS) {
            mbedtls_pk_restart_free(rs_ctx);
        }

        return ret;
    }
#else /* MBEDTLS_ECDSA_C && MBEDTLS_ECP_RESTARTABLE */
    (void) rs_ctx;
#endif /* MBEDTLS_ECDSA_C && MBEDTLS_ECP_RESTARTABLE */

    if (ctx->pk_info->sign_func == NULL) {
        return MBEDTLS_ERR_PK_TYPE_MISMATCH;
    }

    return ctx->pk_info->sign_func(ctx, md_alg,
                                   hash, hash_len,
                                   sig, sig_size, sig_len,
                                   f_rng, p_rng);
}

/*
 * Make a signature
 */
int mbedtls_pk_sign(mbedtls_pk_context *ctx, mbedtls_md_type_t md_alg,
                    const unsigned char *hash, size_t hash_len,
                    unsigned char *sig, size_t sig_size, size_t *sig_len,
                    int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
    return mbedtls_pk_sign_restartable(ctx, md_alg, hash, hash_len,
                                       sig, sig_size, sig_len,
                                       f_rng, p_rng, NULL);
}

/*
 * Make a signature given a signature type.
 */
int mbedtls_pk_sign_ext(mbedtls_pk_type_t pk_type,
                        mbedtls_pk_context *ctx,
                        mbedtls_md_type_t md_alg,
                        const unsigned char *hash, size_t hash_len,
                        unsigned char *sig, size_t sig_size, size_t *sig_len,
                        int (*f_rng)(void *, unsigned char *, size_t),
                        void *p_rng)
{
    if (ctx->pk_info == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (!mbedtls_pk_can_do(ctx, pk_type)) {
        return MBEDTLS_ERR_PK_TYPE_MISMATCH;
    }

    if (pk_type != MBEDTLS_PK_RSASSA_PSS) {
        return mbedtls_pk_sign(ctx, md_alg, hash, hash_len,
                               sig, sig_size, sig_len, f_rng, p_rng);
    }

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PKCS1_V21)

#if defined(MBEDTLS_USE_PSA_CRYPTO)
    const psa_algorithm_t psa_md_alg = mbedtls_md_psa_alg_from_type(md_alg);
    if (psa_md_alg == 0) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (mbedtls_pk_get_type(ctx) == MBEDTLS_PK_OPAQUE) {
        psa_status_t status;

        status = psa_sign_hash(ctx->priv_id, PSA_ALG_RSA_PSS(psa_md_alg),
                               hash, hash_len,
                               sig, sig_size, sig_len);
        return PSA_PK_RSA_TO_MBEDTLS_ERR(status);
    }

    return mbedtls_pk_psa_rsa_sign_ext(PSA_ALG_RSA_PSS(psa_md_alg),
                                       ctx->pk_ctx, hash, hash_len,
                                       sig, sig_size, sig_len);
#else /* MBEDTLS_USE_PSA_CRYPTO */

    if (sig_size < mbedtls_pk_get_len(ctx)) {
        return MBEDTLS_ERR_PK_BUFFER_TOO_SMALL;
    }

    if (pk_hashlen_helper(md_alg, &hash_len) != 0) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    mbedtls_rsa_context *const rsa_ctx = mbedtls_pk_rsa(*ctx);

    const int ret = mbedtls_rsa_rsassa_pss_sign_no_mode_check(rsa_ctx, f_rng, p_rng, md_alg,
                                                              (unsigned int) hash_len, hash, sig);
    if (ret == 0) {
        *sig_len = rsa_ctx->len;
    }
    return ret;

#endif /* MBEDTLS_USE_PSA_CRYPTO */

#else
    return MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE;
#endif /* MBEDTLS_RSA_C && MBEDTLS_PKCS1_V21 */
}

/*
 * Decrypt message
 */
int mbedtls_pk_decrypt(mbedtls_pk_context *ctx,
                       const unsigned char *input, size_t ilen,
                       unsigned char *output, size_t *olen, size_t osize,
                       int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
    if (ctx->pk_info == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (ctx->pk_info->decrypt_func == NULL) {
        return MBEDTLS_ERR_PK_TYPE_MISMATCH;
    }

    return ctx->pk_info->decrypt_func(ctx, input, ilen,
                                      output, olen, osize, f_rng, p_rng);
}

/*
 * Encrypt message
 */
int mbedtls_pk_encrypt(mbedtls_pk_context *ctx,
                       const unsigned char *input, size_t ilen,
                       unsigned char *output, size_t *olen, size_t osize,
                       int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
    if (ctx->pk_info == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (ctx->pk_info->encrypt_func == NULL) {
        return MBEDTLS_ERR_PK_TYPE_MISMATCH;
    }

    return ctx->pk_info->encrypt_func(ctx, input, ilen,
                                      output, olen, osize, f_rng, p_rng);
}

/*
 * Check public-private key pair
 */
int mbedtls_pk_check_pair(const mbedtls_pk_context *pub,
                          const mbedtls_pk_context *prv,
                          int (*f_rng)(void *, unsigned char *, size_t),
                          void *p_rng)
{
    if (pub->pk_info == NULL ||
        prv->pk_info == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (f_rng == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (prv->pk_info->check_pair_func == NULL) {
        return MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE;
    }

    if (prv->pk_info->type == MBEDTLS_PK_RSA_ALT) {
        if (pub->pk_info->type != MBEDTLS_PK_RSA) {
            return MBEDTLS_ERR_PK_TYPE_MISMATCH;
        }
    } else {
        if ((prv->pk_info->type != MBEDTLS_PK_OPAQUE) &&
            (pub->pk_info != prv->pk_info)) {
            return MBEDTLS_ERR_PK_TYPE_MISMATCH;
        }
    }

    return prv->pk_info->check_pair_func((mbedtls_pk_context *) pub,
                                         (mbedtls_pk_context *) prv,
                                         f_rng, p_rng);
}

/*
 * Get key size in bits
 */
size_t mbedtls_pk_get_bitlen(const mbedtls_pk_context *ctx)
{
    /* For backward compatibility, accept NULL or a context that
     * isn't set up yet, and return a fake value that should be safe. */
    if (ctx == NULL || ctx->pk_info == NULL) {
        return 0;
    }

    return ctx->pk_info->get_bitlen((mbedtls_pk_context *) ctx);
}

/*
 * Export debug information
 */
int mbedtls_pk_debug(const mbedtls_pk_context *ctx, mbedtls_pk_debug_item *items)
{
    if (ctx->pk_info == NULL) {
        return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
    }

    if (ctx->pk_info->debug_func == NULL) {
        return MBEDTLS_ERR_PK_TYPE_MISMATCH;
    }

    ctx->pk_info->debug_func((mbedtls_pk_context *) ctx, items);
    return 0;
}

/*
 * Access the PK type name
 */
const char *mbedtls_pk_get_name(const mbedtls_pk_context *ctx)
{
    if (ctx == NULL || ctx->pk_info == NULL) {
        return "invalid PK";
    }

    return ctx->pk_info->name;
}

/*
 * Access the PK type
 */
mbedtls_pk_type_t mbedtls_pk_get_type(const mbedtls_pk_context *ctx)
{
    if (ctx == NULL || ctx->pk_info == NULL) {
        return MBEDTLS_PK_NONE;
    }

    return ctx->pk_info->type;
}

#if defined(MBEDTLS_USE_PSA_CRYPTO)
/*
 * Load the key to a PSA key slot,
 * then turn the PK context into a wrapper for that key slot.
 *
 * Currently only works for EC & RSA private keys.
 */
int mbedtls_pk_wrap_as_opaque(mbedtls_pk_context *pk,
                              mbedtls_svc_key_id_t *key,
                              psa_algorithm_t alg,
                              psa_key_usage_t usage,
                              psa_algorithm_t alg2)
{
#if !defined(MBEDTLS_PK_HAVE_ECC_KEYS) && !defined(MBEDTLS_RSA_C)
    ((void) pk);
    ((void) key);
    ((void) alg);
    ((void) usage);
    ((void) alg2);
#else /* !MBEDTLS_PK_HAVE_ECC_KEYS && !MBEDTLS_RSA_C */
#if defined(MBEDTLS_PK_HAVE_ECC_KEYS)
    if (mbedtls_pk_get_type(pk) == MBEDTLS_PK_ECKEY) {
        size_t d_len;
        psa_ecc_family_t curve_id;
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
        psa_key_type_t key_type;
        size_t bits;
        psa_status_t status;

        /* export the private key material in the format PSA wants */
#if defined(MBEDTLS_PK_USE_PSA_EC_DATA)
        unsigned char d[MBEDTLS_PSA_MAX_EC_KEY_PAIR_LENGTH];
        status = psa_export_key(pk->priv_id, d, sizeof(d), &d_len);
        if (status != PSA_SUCCESS) {
            return psa_pk_status_to_mbedtls(status);
        }

        curve_id = pk->ec_family;
        bits = pk->ec_bits;
#else /* MBEDTLS_PK_USE_PSA_EC_DATA */
        unsigned char d[MBEDTLS_ECP_MAX_BYTES];
        mbedtls_ecp_keypair *ec = mbedtls_pk_ec_rw(*pk);
        int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

        d_len = PSA_BITS_TO_BYTES(ec->grp.nbits);
        if ((ret = mbedtls_ecp_write_key(ec, d, d_len)) != 0) {
            return ret;
        }

        curve_id = mbedtls_ecc_group_to_psa(ec->grp.id, &bits);
#endif /* MBEDTLS_PK_USE_PSA_EC_DATA */
        key_type = PSA_KEY_TYPE_ECC_KEY_PAIR(curve_id);

        /* prepare the key attributes */
        psa_set_key_type(&attributes, key_type);
        psa_set_key_bits(&attributes, bits);
        psa_set_key_usage_flags(&attributes, usage);
        psa_set_key_algorithm(&attributes, alg);
        if (alg2 != PSA_ALG_NONE) {
            psa_set_key_enrollment_algorithm(&attributes, alg2);
        }

        /* import private key into PSA */
        status = psa_import_key(&attributes, d, d_len, key);
        mbedtls_platform_zeroize(d, sizeof(d));
        if (status != PSA_SUCCESS) {
            return PSA_PK_TO_MBEDTLS_ERR(status);
        }

        /* make PK context wrap the key slot */
        mbedtls_pk_free(pk);
        mbedtls_pk_init(pk);

        return mbedtls_pk_setup_opaque(pk, *key);
    } else
#endif /* MBEDTLS_PK_HAVE_ECC_KEYS */
#if defined(MBEDTLS_RSA_C)
    if (mbedtls_pk_get_type(pk) == MBEDTLS_PK_RSA) {
        unsigned char buf[MBEDTLS_PK_RSA_PRV_DER_MAX_BYTES];
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
        int key_len;
        psa_status_t status;

        /* export the private key material in the format PSA wants */
        key_len = mbedtls_pk_write_key_der(pk, buf, sizeof(buf));
        if (key_len <= 0) {
            return MBEDTLS_ERR_PK_BAD_INPUT_DATA;
        }

        /* prepare the key attributes */
        psa_set_key_type(&attributes, PSA_KEY_TYPE_RSA_KEY_PAIR);
        psa_set_key_bits(&attributes, mbedtls_pk_get_bitlen(pk));
        psa_set_key_usage_flags(&attributes, usage);
        psa_set_key_algorithm(&attributes, alg);
        if (alg2 != PSA_ALG_NONE) {
            psa_set_key_enrollment_algorithm(&attributes, alg2);
        }

        /* import private key into PSA */
        status = psa_import_key(&attributes,
                                buf + sizeof(buf) - key_len,
                                key_len, key);

        mbedtls_platform_zeroize(buf, sizeof(buf));

        if (status != PSA_SUCCESS) {
            return PSA_PK_TO_MBEDTLS_ERR(status);
        }

        /* make PK context wrap the key slot */
        mbedtls_pk_free(pk);
        mbedtls_pk_init(pk);

        return mbedtls_pk_setup_opaque(pk, *key);
    } else
#endif /* MBEDTLS_RSA_C */
#endif /* !MBEDTLS_PK_HAVE_ECC_KEYS && !MBEDTLS_RSA_C */
    return MBEDTLS_ERR_PK_TYPE_MISMATCH;
}
#endif /* MBEDTLS_USE_PSA_CRYPTO */
#endif /* MBEDTLS_PK_C */
