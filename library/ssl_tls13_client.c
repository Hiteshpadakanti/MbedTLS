/*
 *  TLS 1.3 client-side functions
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
 *
 *  This file is part of mbed TLS ( https://tls.mbed.org )
 */

#include "common.h"

#if defined(MBEDTLS_SSL_PROTO_TLS1_3_EXPERIMENTAL)

#if defined(MBEDTLS_SSL_CLI_C)

#include <string.h>

#include "ssl_misc.h"
#include <mbedtls/debug.h>

#define CLIENT_HELLO_RAND_BYTES_LEN 32
#define CLIENT_HELLO_VERSION_LEN    2
/* Main entry point; orchestrates the other functions */
static int ssl_tls13_write_client_hello( mbedtls_ssl_context *ssl );

int mbedtls_ssl_tls13_handshake_client_step( mbedtls_ssl_context *ssl )
{
    int ret = 0;

    if( ssl->state == MBEDTLS_SSL_HANDSHAKE_OVER || ssl->handshake == NULL )
    {
        MBEDTLS_SSL_DEBUG_MSG( 2, ( "Handshake completed but ssl->handshake is NULL.\n" ) );
        return( MBEDTLS_ERR_SSL_BAD_INPUT_DATA );
    }

    MBEDTLS_SSL_DEBUG_MSG( 2, ( "client state: %d", ssl->state ) );

    switch( ssl->state )
    {
        /*
         * ssl->state is initialized as HELLO_REQUEST. It is same
         * with CLIENT_HELLO status
         */
        case MBEDTLS_SSL_HELLO_REQUEST:
        case MBEDTLS_SSL_CLIENT_HELLO:
            ret = ssl_tls13_write_client_hello( ssl );
            break;

        case MBEDTLS_SSL_SERVER_HELLO:
            // Stop here : we haven't finished whole flow
            ret = MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE;
            mbedtls_ssl_handshake_set_state( ssl, MBEDTLS_SSL_ENCRYPTED_EXTENSIONS );
            break;

        default:
            MBEDTLS_SSL_DEBUG_MSG( 1, ( "invalid state %d", ssl->state ) );
            return( MBEDTLS_ERR_SSL_BAD_INPUT_DATA );
    }

    return( ret );
}


static int ssl_tls13_prepare_client_hello( mbedtls_ssl_context *ssl );
static int ssl_tls13_write_exts_client_hello( mbedtls_ssl_context *ssl,
                                           unsigned char *buf, size_t buflen,
                                           size_t *len_with_binders );
static int ssl_tls13_finalize_client_hello( mbedtls_ssl_context *ssl );

static int ssl_tls13_write_client_hello( mbedtls_ssl_context *ssl )
{
    int ret = 0;
    unsigned char *buf;
    size_t buf_len, msg_len;

    MBEDTLS_SSL_DEBUG_MSG( 2, ( "=> write client hello" ) );

    MBEDTLS_SSL_PROC_CHK( ssl_tls13_prepare_client_hello, ( ssl ) );

    MBEDTLS_SSL_PROC_CHK( mbedtls_ssl_tls13_start_handshake_msg,
                          ( ssl, MBEDTLS_SSL_HS_CLIENT_HELLO,
                          &buf, &buf_len ) );

    MBEDTLS_SSL_PROC_CHK( ssl_tls13_write_exts_client_hello,
                          ( ssl, buf, buf_len, &msg_len ) );

    mbedtls_ssl_tls13_add_hs_hdr_to_checksum( ssl, MBEDTLS_SSL_HS_CLIENT_HELLO,
                                        msg_len );
    ssl->handshake->update_checksum( ssl, buf, 0 );

    MBEDTLS_SSL_PROC_CHK( ssl_tls13_finalize_client_hello, ( ssl ) );
    MBEDTLS_SSL_PROC_CHK( mbedtls_ssl_tls13_finish_handshake_msg,
                          ( ssl, buf_len, msg_len ) );

cleanup:

    MBEDTLS_SSL_DEBUG_MSG( 2, ( "<= write client hello" ) );
    /* client_hello_process haven't finished */
    ret = MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE;
    return ret;
}

static int ssl_tls13_prepare_client_hello( mbedtls_ssl_context *ssl )
{
    int ret;

    if( ssl->conf->f_rng == NULL )
    {
        MBEDTLS_SSL_DEBUG_MSG( 1, ( "no RNG provided" ) );
        return( MBEDTLS_ERR_SSL_NO_RNG );
    }

    if( ( ret = ssl->conf->f_rng( ssl->conf->p_rng,
                                  ssl->handshake->randbytes,
                                  CLIENT_HELLO_RAND_BYTES_LEN ) ) != 0 )
    {
        MBEDTLS_SSL_DEBUG_RET( 1, "ssl_generate_random", ret );
        return( ret );
    }

    return( 0 );
}

static int ssl_tls13_finalize_client_hello( mbedtls_ssl_context* ssl )
{
    mbedtls_ssl_handshake_set_state( ssl, MBEDTLS_SSL_SERVER_HELLO );

    return( 0 );
}

/* Write extensions */

static int ssl_tls13_write_supported_versions_ext( mbedtls_ssl_context *ssl,
                                              unsigned char *buf,
                                              unsigned char *end,
                                              size_t *olen );

#if defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)

static int ssl_tls13_write_supported_groups_ext( mbedtls_ssl_context *ssl,
                                          unsigned char *buf,
                                          unsigned char *end,
                                          size_t *olen );

static int ssl_tls13_write_key_shares_ext( mbedtls_ssl_context *ssl,
                                     unsigned char *buf,
                                     unsigned char *end,
                                     size_t *olen );

#endif /* MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED */

static int ssl_tls13_write_exts_client_hello( mbedtls_ssl_context *ssl,
                                           unsigned char *buf, size_t buflen,
                                           size_t *len_with_binders )
{
     /* Extensions */

    /* extension_start
     *    Used during extension writing where the
     *    buffer pointer to the beginning of the
     *    extension list must be kept to write
     *    the total extension list size in the end.
     */
#if defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)
    int ret;
#endif /* MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED */
    unsigned char* extension_start;
    size_t cur_ext_len;          /* Size of the current extension */
    size_t total_ext_len;        /* Size of list of extensions    */

    /* Buffer management */
    unsigned char* start = buf;
    unsigned char* end = buf + buflen;

    /* Ciphersuite-related variables */
    const int* ciphersuites;
    const mbedtls_ssl_ciphersuite_t* ciphersuite_info;
    /*  ciphersuite_start points to the start of
        the ciphersuite list, i.e. to the length field*/
    unsigned char* ciphersuite_start;
    size_t ciphersuite_count;

    /* Keeping track of the included extensions */
    ssl->handshake->extensions_present = MBEDTLS_SSL_EXT_NONE;

    /* NOTE:
     * Even for DTLS 1.3, we are writing a TLS handshake header here.
     * The actual DTLS 1.3 handshake header is inserted in
     * the record writing routine mbedtls_ssl_write_record().
     *
     * For cTLS the length, and the version field
     * are elided. The random bytes are shorter.
     */

    if( ssl->conf->max_major_ver == 0 )
    {
        MBEDTLS_SSL_DEBUG_MSG( 1, ( "configured max major version is invalid, "
                                    "consider using mbedtls_ssl_config_defaults()" ) );
        return( MBEDTLS_ERR_SSL_BAD_INPUT_DATA );
    }

    ssl->major_ver = ssl->conf->min_major_ver;
    ssl->minor_ver = ssl->conf->min_minor_ver;

    /* For TLS 1.3 we use the legacy version number {0x03, 0x03}
     *  instead of the true version number.
     *
     *  For DTLS 1.3 we use the legacy version number
     *  {254,253}.
     *
     *  In cTLS the version number is elided.
     */
    MBEDTLS_SSL_CHK_BUF_PTR( buf, end, CLIENT_HELLO_VERSION_LEN);
    MBEDTLS_PUT_UINT16_BE( 0x0303, buf, 0);
    buf += 2;
    buflen -= CLIENT_HELLO_VERSION_LEN;

    /* Write random bytes */
    MBEDTLS_SSL_CHK_BUF_PTR( buf, end, CLIENT_HELLO_RAND_BYTES_LEN);
    memcpy( buf, ssl->handshake->randbytes, CLIENT_HELLO_RAND_BYTES_LEN );
    MBEDTLS_SSL_DEBUG_BUF( 3, "client hello, random bytes",
                           buf, CLIENT_HELLO_RAND_BYTES_LEN );

    buf += CLIENT_HELLO_RAND_BYTES_LEN;
    buflen -= CLIENT_HELLO_RAND_BYTES_LEN;

    /* Versions of TLS before TLS 1.3 supported a
     * "session resumption" feature which has been merged with pre-shared
     * keys in this version. A client which has a
     * cached session ID set by a pre-TLS 1.3 server SHOULD set this
     * field to that value. In compatibility mode,
     * this field MUST be non-empty, so a client not offering a
     * pre-TLS 1.3 session MUST generate a new 32-byte value. This value
     * need not be random but SHOULD be unpredictable to avoid
     * implementations fixating on a specific value ( also known as
     * ossification ). Otherwise, it MUST be set as a zero-length vector
     * ( i.e., a zero-valued single byte length field ).
     */
    if( buflen < 1 )
    {
        MBEDTLS_SSL_DEBUG_MSG( 1, ( "buffer too small to hold ClientHello" ) );
        return( MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL );
    }

    *buf++ = 0; /* session id length set to zero */
    buflen -= 1;

    /*
     * Ciphersuite list
     *
     * This is a list of the symmetric cipher options supported by
     * the client, specifically the record protection algorithm
     * ( including secret key length ) and a hash to be used with
     * HKDF, in descending order of client preference.
     */
    ciphersuites = ssl->conf->ciphersuite_list;

    if( buflen < 2 /* for ciphersuite list length */ )
    {
        MBEDTLS_SSL_DEBUG_MSG( 1, ( "buffer too small to hold ClientHello" ) );
        return( MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL );
    }

    /* Skip writing ciphersuite length for now */
    ciphersuite_count = 0;
    ciphersuite_start = buf;
    buf += 2;
    buflen -= 2;

    for ( size_t i = 0; ciphersuites[i] != 0; i++ )
    {
        ciphersuite_info = mbedtls_ssl_ciphersuite_from_id( ciphersuites[i] );

        if( ciphersuite_info == NULL )
            continue;

        if( ciphersuite_info->min_minor_ver != MBEDTLS_SSL_MINOR_VERSION_4 ||
            ciphersuite_info->max_minor_ver != MBEDTLS_SSL_MINOR_VERSION_4 )
            continue;

        MBEDTLS_SSL_DEBUG_MSG( 3, ( "client hello, add ciphersuite: %04x, %s",
                                    (unsigned int) ciphersuites[i],
                                    ciphersuite_info->name ) );

        ciphersuite_count++;

        if( buflen < 2 /* for ciphersuite list length */ )
        {
            MBEDTLS_SSL_DEBUG_MSG( 1, ( "buffer too small to hold ClientHello" ) );
            return( MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL );
        }

        MBEDTLS_PUT_UINT16_BE( ciphersuites[i], buf, 0);

        buf += 2;
        buflen -= 2;

    }

    /* write ciphersuite length now */
    MBEDTLS_PUT_UINT16_BE( ciphersuite_count*2, ciphersuite_start, 0);
    ciphersuite_start += 2;

    MBEDTLS_SSL_DEBUG_MSG( 3,
                           ( "client hello, got %" MBEDTLS_PRINTF_SIZET " ciphersuites",
                             ciphersuite_count ) );

    /* For every TLS 1.3 ClientHello, this vector MUST contain exactly
     * one byte set to zero, which corresponds to the 'null' compression
     * method in prior versions of TLS.
     *
     * For cTLS this field is elided.
     */
    if( buflen < 2 /* for ciphersuite list length */ )
    {
        MBEDTLS_SSL_DEBUG_MSG( 1, ( "buffer too small to hold ClientHello" ) );
        return( MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL );
    }

    *buf++ = 1;
    *buf++ = MBEDTLS_SSL_COMPRESS_NULL;

    buflen -= 2;

    /* First write extensions, then the total length */
    extension_start = buf;
    total_ext_len = 0;
    buf += 2;

    /* Supported Versions Extension is mandatory with TLS 1.3.
     *
     * For cTLS we only need to provide it if there is more than one version
     * and currently there is only one.
     */
    ssl_tls13_write_supported_versions_ext( ssl, buf, end, &cur_ext_len );
    total_ext_len += cur_ext_len;
    buf += cur_ext_len;

#if defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)
    /* The supported_groups and the key_share extensions are
     * REQUIRED for ECDHE ciphersuites.
     */
    ret = ssl_tls13_write_supported_groups_ext( ssl, buf, end, &cur_ext_len );
    if( ret != 0 )
        return( ret );

    total_ext_len += cur_ext_len;
    buf += cur_ext_len;

    /* The supported_signature_algorithms extension is REQUIRED for
     * certificate authenticated ciphersuites. */
    ret = mbedtls_ssl_tls13_write_signature_algorithms_ext( ssl, buf,
                                                            end, &cur_ext_len );
    if( ret != 0 )
        return( ret );

    total_ext_len += cur_ext_len;
    buf += cur_ext_len;

    /* We need to send the key shares under three conditions:
     * 1 ) A certificate-based ciphersuite is being offered. In this case
     *    supported_groups and supported_signature extensions have been successfully added.
     * 2 ) A PSK-based ciphersuite with ECDHE is offered. In this case the
     *    psk_key_exchange_modes has been added as the last extension.
     * 3 ) Or, in case all ciphers are supported ( which includes #1 and #2 from above )
     */

    ret = ssl_tls13_write_key_shares_ext( ssl, buf, end, &cur_ext_len );
    if( ret != 0 )
        return( ret );

    total_ext_len += cur_ext_len;
    buf += cur_ext_len;
#endif /* MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED */

    /* Add more extensions here */

    MBEDTLS_SSL_DEBUG_MSG( 3, ( "client hello, total extension length: %" MBEDTLS_PRINTF_SIZET ,
                                total_ext_len ) );

    MBEDTLS_SSL_DEBUG_BUF( 3, "client hello extensions", extension_start, total_ext_len );

    /* Write extension length */
    MBEDTLS_PUT_UINT16_BE( total_ext_len, extension_start, 0);
    extension_start += 2;

    *len_with_binders = ( extension_start + total_ext_len ) - start;
    return( 0 );
}

/*
 * ssl_tls13_write_supported_versions_ext():
 *
 * struct {
 *      ProtocolVersion versions<2..254>;
 * } SupportedVersions;
 */
static int ssl_tls13_write_supported_versions_ext( mbedtls_ssl_context *ssl,
                                              unsigned char *buf,
                                              unsigned char *end,
                                              size_t *olen )
{
    unsigned char *p = buf;

    *olen = 0;

    MBEDTLS_SSL_DEBUG_MSG( 3, ( "client hello, adding supported version extension" ) );

    MBEDTLS_SSL_CHK_BUF_PTR( p, end, 7 );

    MBEDTLS_PUT_UINT16_BE( MBEDTLS_TLS_EXT_SUPPORTED_VERSIONS, p, 0);

    /* total length */
    MBEDTLS_PUT_UINT16_BE( 3, p, 2);

    p+=4;

    /* length of next field */
    *p++ = 0x2;

    /* This implementation only supports a single TLS version, and only
     * advertises a single value.
     */
    mbedtls_ssl_write_version( ssl->conf->max_major_ver, ssl->conf->max_minor_ver,
                              ssl->conf->transport, p );

    MBEDTLS_SSL_DEBUG_MSG( 3, ( "supported version: [%d:%d]",
                                ssl->conf->max_major_ver, ssl->conf->max_minor_ver ) );

    *olen = 7;

    return( 0 );
}

#if defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)

static int ssl_tls13_write_supported_groups_ext( mbedtls_ssl_context *ssl,
                                           unsigned char *buf,
                                           unsigned char *end,
                                           size_t *olen )
{
    ((void) ssl);
    ((void) buf);
    ((void) end);
    ((void) olen);
    return( MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE );
}

static int ssl_tls13_write_key_shares_ext( mbedtls_ssl_context *ssl,
                                     unsigned char *buf,
                                     unsigned char *end,
                                     size_t *olen )
{
    ((void) ssl);
    ((void) buf);
    ((void) end);
    ((void) olen);
    return( MBEDTLS_ERR_SSL_FEATURE_UNAVAILABLE );
}

#endif /* MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED */

#endif /* MBEDTLS_SSL_CLI_C */

#endif /* MBEDTLS_SSL_PROTO_TLS1_3_EXPERIMENTAL */
