/**
 * \file ssl_helpers.h
 *
 * \brief   Helper functions for tests that set up a TLS connection.
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

#ifndef SSL_HELPERS_H
#define SSL_HELPERS_H

#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/timing.h>
#include <mbedtls/debug.h>
#include <ssl_tls13_keys.h>
#include <ssl_tls13_invasive.h>
#include "test/certs.h"
#include "test/helpers.h"

#if defined(MBEDTLS_SSL_CACHE_C)
#include "mbedtls/ssl_cache.h"
#endif

#include <psa/crypto.h>


#include <test/constant_flow.h>

enum
{
#define MBEDTLS_SSL_TLS1_3_LABEL( name, string )   \
     tls13_label_ ## name,
MBEDTLS_SSL_TLS1_3_LABEL_LIST
#undef MBEDTLS_SSL_TLS1_3_LABEL
};

typedef struct mbedtls_test_ssl_log_pattern
{
    const char *pattern;
    size_t counter;
} mbedtls_test_ssl_log_pattern;

typedef struct mbedtls_test_handshake_test_options
{
    const char *cipher;
    mbedtls_ssl_protocol_version client_min_version;
    mbedtls_ssl_protocol_version client_max_version;
    mbedtls_ssl_protocol_version server_min_version;
    mbedtls_ssl_protocol_version server_max_version;
    mbedtls_ssl_protocol_version expected_negotiated_version;
    int expected_handshake_result;
    int expected_ciphersuite;
    int pk_alg;
    int opaque_alg;
    int opaque_alg2;
    int opaque_usage;
    data_t *psk_str;
    int dtls;
    int srv_auth_mode;
    int serialize;
    int mfl;
    int cli_msg_len;
    int srv_msg_len;
    int expected_cli_fragments;
    int expected_srv_fragments;
    int renegotiate;
    int legacy_renegotiation;
    void *srv_log_obj;
    void *cli_log_obj;
    void (*srv_log_fun)(void *, int, const char *, int, const char *);
    void (*cli_log_fun)(void *, int, const char *, int, const char *);
    int resize_buffers;
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_context *cache;
#endif
} mbedtls_test_handshake_test_options;

/*
 * Buffer structure for custom I/O callbacks.
 */

typedef struct mbedtls_test_ssl_buffer
{
    size_t start;
    size_t content_length;
    size_t capacity;
    unsigned char *buffer;
} mbedtls_test_ssl_buffer;

/*
 * Errors used in the message transport mock tests
 */
 #define MBEDTLS_TEST_ERROR_ARG_NULL -11
 #define MBEDTLS_TEST_ERROR_MESSAGE_TRUNCATED -44

/*
 * Context for a message metadata queue (fifo) that is on top of the ring buffer.
 */
typedef struct mbedtls_test_ssl_message_queue
{
    size_t *messages;
    int pos;
    int num;
    int capacity;
} mbedtls_test_ssl_message_queue;

/*
 * Context for the I/O callbacks simulating network connection.
 */

#define MBEDTLS_MOCK_SOCKET_CONNECTED 1

typedef struct mbedtls_test_mock_socket
{
    int status;
    mbedtls_test_ssl_buffer *input;
    mbedtls_test_ssl_buffer *output;
    struct mbedtls_test_mock_socket *peer;
} mbedtls_test_mock_socket;

/* Errors used in the message socket mocks */

#define MBEDTLS_TEST_ERROR_CONTEXT_ERROR -55
#define MBEDTLS_TEST_ERROR_SEND_FAILED -66
#define MBEDTLS_TEST_ERROR_RECV_FAILED -77

/*
 * Structure used as an addon, or a wrapper, around the mocked sockets.
 * Contains an input queue, to which the other socket pushes metadata,
 * and an output queue, to which this one pushes metadata. This context is
 * considered as an owner of the input queue only, which is initialized and
 * freed in the respective setup and free calls.
 */
typedef struct mbedtls_test_message_socket_context
{
    mbedtls_test_ssl_message_queue* queue_input;
    mbedtls_test_ssl_message_queue* queue_output;
    mbedtls_test_mock_socket* socket;
} mbedtls_test_message_socket_context;

#if defined(MBEDTLS_X509_CRT_PARSE_C) && \
    defined(MBEDTLS_ENTROPY_C) && \
    defined(MBEDTLS_CTR_DRBG_C)

/*
 * Structure with endpoint's certificates for SSL communication tests.
 */
typedef struct mbedtls_test_ssl_endpoint_certificate
{
    mbedtls_x509_crt ca_cert;
    mbedtls_x509_crt cert;
    mbedtls_pk_context pkey;
} mbedtls_test_ssl_endpoint_certificate;

/*
 * Endpoint structure for SSL communication tests.
 */
typedef struct mbedtls_test_ssl_endpoint
{
    const char *name;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    mbedtls_test_mock_socket socket;
    mbedtls_test_ssl_endpoint_certificate cert;
} mbedtls_test_ssl_endpoint;

#endif /* MBEDTLS_X509_CRT_PARSE_C && MBEDTLS_ENTROPY_C && MBEDTLS_CTR_DRBG_C */

/*
 * Helper function setting up inverse record transformations
 * using given cipher, hash, EtM mode, authentication tag length,
 * and version.
 */

#define CHK( x )                                \
    do                                          \
    {                                           \
        if( !( x ) )                            \
        {                                       \
            ret = -1;                           \
            goto cleanup;                       \
        }                                       \
    } while( 0 )

#if MBEDTLS_SSL_CID_OUT_LEN_MAX > MBEDTLS_SSL_CID_IN_LEN_MAX
#define SSL_CID_LEN_MIN MBEDTLS_SSL_CID_IN_LEN_MAX
#else
#define SSL_CID_LEN_MIN MBEDTLS_SSL_CID_OUT_LEN_MAX
#endif

#endif /* SSL_HELPERS_H */