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

#endif /* SSL_HELPERS_H */