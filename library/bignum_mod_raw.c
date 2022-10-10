/*
 *  Low-level modular bignum functions
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

#include "common.h"

#if defined(MBEDTLS_BIGNUM_C)

#include <string.h>

#include "mbedtls/error.h"
#include "mbedtls/platform_util.h"

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_printf      printf
#define mbedtls_calloc      calloc
#define mbedtls_free        free
#endif

#include "bignum_core.h"
#include "bignum_mod_raw.h"
#include "bignum_mod.h"
#include "constant_time_internal.h"

int mbedtls_mpi_mod_raw_read( mbedtls_mpi_uint *X,
                              const mbedtls_mpi_mod_modulus *m,
                              const unsigned char *input,
                              size_t input_length )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

    switch( m->ext_rep )
    {
        case MBEDTLS_MPI_MOD_EXT_REP_LE:
            ret = mbedtls_mpi_core_read_le( X, m->limbs,
                                            input, input_length );
            break;
        case MBEDTLS_MPI_MOD_EXT_REP_BE:
            ret = mbedtls_mpi_core_read_be( X, m->limbs,
                                            input, input_length );
            break;
        default:
            return( MBEDTLS_ERR_MPI_BAD_INPUT_DATA );
    }

    if( ret != 0 )
        goto cleanup;

    if( !mbedtls_mpi_core_lt_ct( X, m->p, m->limbs ) )
    {
        ret = MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
        goto cleanup;
    }

cleanup:

    return( ret );
}

int mbedtls_mpi_mod_raw_write( const mbedtls_mpi_uint *A,
                               const mbedtls_mpi_mod_modulus *m,
                               unsigned char *output,
                               size_t output_length )
{
    switch( m->ext_rep )
    {
        case MBEDTLS_MPI_MOD_EXT_REP_LE:
            return( mbedtls_mpi_core_write_le( A, m->limbs,
                                               output, output_length ) );
        case MBEDTLS_MPI_MOD_EXT_REP_BE:
            return( mbedtls_mpi_core_write_be( A, m->limbs,
                                               output, output_length ) );
        default:
            return( MBEDTLS_ERR_MPI_BAD_INPUT_DATA );
    }
}

int mbedtls_mpi_set_montgomery_constant_unsafe( const mbedtls_mpi_uint **X,
                                                const mbedtls_mpi_uint *A,
                                                size_t limbs )
                                                
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi N;
    mbedtls_mpi RR;

    mbedtls_mpi_init( &N ); 
    mbedtls_mpi_init( &RR );

    if ( A == NULL || limbs == 0 || limbs >= ( MBEDTLS_MPI_MAX_LIMBS / 2 ) - 2 )
        goto cleanup;

    if ( !mbedtls_mpi_grow( &N,  limbs ))
        memcpy( N.p, A, sizeof(mbedtls_mpi_uint) *  limbs );
    else
        goto cleanup;

    MBEDTLS_MPI_CHK( mbedtls_mpi_lset( &RR, 1 ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_shift_l( &RR, N.n * 2 * biL ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &RR, &RR, &N ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_shrink( &RR, N.n ) );

    *X = RR.p;
    RR.p = NULL;
    ret = 0;

cleanup:
    mbedtls_mpi_free(&N);
    mbedtls_mpi_free(&RR);
    ret = ( ret != 0 ) ? MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED : 0;
    return( ret );
}

int mbedtls_mpi_mod_raw_from_mont_rep( mbedtls_mpi_uint *X,
                                       const mbedtls_mpi_mod_modulus *m )
{
    const mbedtls_mpi_uint one = 1;
    const size_t t_sz = ( m->limbs * 2 ) + 1;
    mbedtls_mpi_uint *t;

    if( ( t = (mbedtls_mpi_uint*)mbedtls_calloc( t_sz, ciL ) ) == NULL )
        return( MBEDTLS_ERR_MPI_ALLOC_FAILED );
    mbedtls_mpi_core_montmul( X, X, &one, 1, m->p, m->limbs,
                              m->rep.mont.mm, t );
    mbedtls_platform_zeroize( t, t_sz * ciL );
    mbedtls_free( t );
    return( 0 );
}

int mbedtls_mpi_mod_raw_to_mont_rep( mbedtls_mpi_uint *X,
                                     const mbedtls_mpi_mod_modulus *m )
{
    mbedtls_mpi_uint *t;
    const size_t t_sz = ( m->limbs * 2 ) + 1;

    if( ( t = (mbedtls_mpi_uint*)mbedtls_calloc( t_sz, ciL ) ) == NULL )
        return( MBEDTLS_ERR_MPI_ALLOC_FAILED );
    mbedtls_mpi_core_montmul( X, X, m->rep.mont.rr, m->limbs, m->p, m->limbs,
                              m->rep.mont.mm, t );
    mbedtls_platform_zeroize( t, t_sz * ciL );
    mbedtls_free( t );
    return( 0 );
}

#endif /* MBEDTLS_BIGNUM_C */
