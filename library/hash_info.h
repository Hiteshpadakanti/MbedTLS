/**
 * Hash information that's independent from the crypto implementation.
 *
 *  This can be used by:
 *  - code based on PSA
 *  - code based on the legacy API
 *  - code based on either of them depending on MBEDTLS_USE_PSA_CRYPTO
 *  - code based on either of them depending on what's available
 *
 *  Note: this internal module will go away when everything becomes based on
 *  PSA Crypto; it is a helper for the transition while hash algorithms are
 *  still represented using mbedtls_md_type_t in most places even when PSA is
 *  used for the actual crypto computations.
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
#ifndef MBEDTLS_HASH_INFO_H
#define MBEDTLS_HASH_INFO_H

#include "md_psa.h"

#endif /* MBEDTLS_HASH_INFO_H */
