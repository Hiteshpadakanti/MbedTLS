/*
 *  Edwards-curve Digital Signature Algorithm
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

/*
 * References:
 *
 * SEC1 http://www.secg.org/index.php?action=secg,docs_secg
 */

#include "common.h"

#if defined(MBEDTLS_EDDSA_C)

#include "mbedtls/eddsa.h"
#include "mbedtls/asn1write.h"

#include <string.h>

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#define mbedtls_calloc    calloc
#define mbedtls_free       free
#endif

#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"
#include "mbedtls/sha512.h"
#include "mbedtls/shake256.h"
   
int mbedtls_eddsa_can_do( mbedtls_ecp_group_id gid )
{
    switch( gid )
    {
#ifdef MBEDTLS_ECP_DP_ED25519_ENABLED
        case MBEDTLS_ECP_DP_ED25519: return 1;
#endif
#ifdef MBEDTLS_ECP_DP_ED448_ENABLED
        case MBEDTLS_ECP_DP_ED448: return 1;
#endif
    default: return 0;
    }
}
    
static int mbedtls_eddsa_put_dom2_ctx( int flag, const unsigned char *ctx, 
                size_t ctx_len, mbedtls_sha512_context *sha_ctx )
{
    unsigned char ct_init_string[] = "SigEd25519 no Ed25519 collisions";
    unsigned char ct_flag = flag;
    unsigned char ct_ctx_len = ctx_len & 0xff;
    
    mbedtls_sha512_update( sha_ctx, ct_init_string, 32 );
    mbedtls_sha512_update( sha_ctx, &ct_flag, 1 );
    mbedtls_sha512_update( sha_ctx, &ct_ctx_len, 1 );
    
    if( ctx && ctx_len > 0)
        mbedtls_sha512_update( sha_ctx, ctx, ctx_len );
    
    return( 0 );
}

static int mbedtls_eddsa_put_dom4_ctx( int flag, const unsigned char *ctx, 
                size_t ctx_len, mbedtls_shake256_context *sha_ctx )
{
    unsigned char ct_init_string[] = "SigEd448";
    unsigned char ct_flag = flag;
    unsigned char ct_ctx_len = ctx_len & 0xff;
    
    mbedtls_shake256_update( sha_ctx, ct_init_string, 8 );
    mbedtls_shake256_update( sha_ctx, &ct_flag, 1 );
    mbedtls_shake256_update( sha_ctx, &ct_ctx_len, 1 );
    
    if( ctx && ctx_len > 0)
        mbedtls_shake256_update( sha_ctx, ctx, ctx_len );
    
    return( 0 );
}

/*
 * Compute EdDSA signature of a message.
 * For PREHASH operation, the message is already previously hashed.
 * Obviously, for PREHASH, we skip hash message step.
 */
int mbedtls_eddsa_sign( mbedtls_ecp_group *grp,
                mbedtls_mpi *r, mbedtls_mpi *s,
                const mbedtls_mpi *d, const unsigned char *buf, size_t blen,
                int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
                mbedtls_eddsa_id eddsa_id,
                const unsigned char *ed_ctx, size_t ed_ctx_len )
{
    int ret;
    mbedtls_ecp_point Q, R;
    mbedtls_mpi q, prefix, rq, h;

    /* EdDSA only should be used with Ed25519 and Ed448 curves  */
    if( ! mbedtls_eddsa_can_do( grp->id ) || grp->N.p == NULL )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );
       
#ifdef MBEDTLS_ECP_DP_ED25519_ENABLED
    if( grp->id == MBEDTLS_ECP_DP_ED25519 && eddsa_id != MBEDTLS_EDDSA_PURE && eddsa_id != MBEDTLS_EDDSA_CTX && eddsa_id != MBEDTLS_EDDSA_PREHASH )
        return( MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE );
#endif
#ifdef MBEDTLS_ECP_DP_ED448_ENABLED
    if( grp->id == MBEDTLS_ECP_DP_ED448 && eddsa_id != MBEDTLS_EDDSA_PURE && eddsa_id != MBEDTLS_EDDSA_PREHASH )
        return( MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE );
#endif

    if( eddsa_id == MBEDTLS_EDDSA_PREHASH && blen != 64)
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );
           
    mbedtls_ecp_point_init( &Q ); mbedtls_ecp_point_init( &R );
        
    mbedtls_mpi_init( &q ); mbedtls_mpi_init( &prefix ); mbedtls_mpi_init( &rq ); mbedtls_mpi_init( &h);
    
    MBEDTLS_MPI_CHK( mbedtls_ecp_expand_edwards( grp, d, &q, &prefix ) );

    MBEDTLS_MPI_CHK( mbedtls_ecp_mul( grp, &Q, &q, &grp->G, f_rng, p_rng ) );
    
#ifdef MBEDTLS_ECP_DP_ED25519_ENABLED
    if( grp->id == MBEDTLS_ECP_DP_ED25519 )
    {
        mbedtls_sha512_context sha_ctx;
        unsigned char sha_buf[64], tmp_buf[32];
        size_t olen = 0;
        
        /* r computation */
        mbedtls_sha512_init( &sha_ctx );    
        mbedtls_sha512_starts( &sha_ctx , 0 );
        
        if( eddsa_id == MBEDTLS_EDDSA_CTX )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom2_ctx( 0, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }
        else if( eddsa_id == MBEDTLS_EDDSA_PREHASH )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom2_ctx( 1, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }
        
        /* Update SHA with prefix */
        MBEDTLS_MPI_CHK( mbedtls_mpi_write_binary_le( &prefix, tmp_buf, sizeof(tmp_buf) ) );
        
        mbedtls_sha512_update( &sha_ctx, tmp_buf, sizeof( tmp_buf ) );
        
        /* In EDDSA_PREHASH, buf should contain the SHA512 hash. It contains the whole message otherwise */
        mbedtls_sha512_update( &sha_ctx, buf, blen );
        
        mbedtls_sha512_finish( &sha_ctx, sha_buf );
        mbedtls_sha512_free( &sha_ctx );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary_le( &rq, sha_buf, sizeof( sha_buf ) ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &rq, &rq, &grp->N ) );
        
        MBEDTLS_MPI_CHK( mbedtls_ecp_mul( grp, &R, &rq, &grp->G, f_rng, p_rng ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_copy( r, &R.Y ) );
        if( mbedtls_mpi_get_bit( &R.X, 0 ) )
        {
            MBEDTLS_MPI_CHK( mbedtls_mpi_set_bit( r, 255, 1 ) );
        }
        
        /* s computation */
        mbedtls_sha512_init( &sha_ctx );    
        mbedtls_sha512_starts( &sha_ctx , 0 );

        if( eddsa_id == MBEDTLS_EDDSA_CTX )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom2_ctx( 0, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }
        else if( eddsa_id == MBEDTLS_EDDSA_PREHASH )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom2_ctx( 1, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }

        MBEDTLS_MPI_CHK( mbedtls_ecp_point_write_binary( grp, &R, MBEDTLS_ECP_PF_COMPRESSED, &olen, tmp_buf, sizeof( tmp_buf ) ) );
        mbedtls_sha512_update( &sha_ctx, tmp_buf, sizeof( tmp_buf ) );
        
        MBEDTLS_MPI_CHK( mbedtls_ecp_point_write_binary( grp, &Q, MBEDTLS_ECP_PF_COMPRESSED, &olen, tmp_buf, sizeof( tmp_buf ) ) );
        mbedtls_sha512_update( &sha_ctx, tmp_buf, sizeof( tmp_buf ) );
        
        /* In EDDSA_PREHASH, buf should contain the SHA512 hash. It contains the whole message otherwise */
        mbedtls_sha512_update( &sha_ctx, buf, blen );
        
        mbedtls_sha512_finish( &sha_ctx, sha_buf );
        mbedtls_sha512_free( &sha_ctx );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary_le( &h, sha_buf, sizeof( sha_buf ) ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &h, &h, &grp->N ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &h, &h, &q ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_add_mpi( s, &h, &rq ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( s, s, &grp->N ) );
    }
#endif
#ifdef MBEDTLS_ECP_DP_ED448_ENABLED
    if( grp->id == MBEDTLS_ECP_DP_ED448 )
    {
        mbedtls_shake256_context sha_ctx;
        unsigned char sha_buf[114], tmp_buf[57];
        size_t olen = 0;
        
        /* r computation */
        mbedtls_shake256_init( &sha_ctx );    
        mbedtls_shake256_starts( &sha_ctx );
        
        if( eddsa_id == MBEDTLS_EDDSA_PREHASH )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom4_ctx( 1, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }
        else if( eddsa_id == MBEDTLS_EDDSA_PURE )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom4_ctx( 0, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }
        
        /* Update SHAKE with prefix */
        MBEDTLS_MPI_CHK( mbedtls_mpi_write_binary_le( &prefix, tmp_buf, sizeof(tmp_buf) ) );
        
        mbedtls_shake256_update( &sha_ctx, tmp_buf, sizeof( tmp_buf ) );
        
        /* In EDDSA_PREHASH, buf should contain the SHAKE256 hash. It contains the whole message otherwise */
        mbedtls_shake256_update( &sha_ctx, buf, blen );
        
        mbedtls_shake256_finish( &sha_ctx, sha_buf, 114 );
        mbedtls_shake256_free( &sha_ctx );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary_le( &rq, sha_buf, sizeof( sha_buf ) ) );
               
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &rq, &rq, &grp->N ) );
        
        MBEDTLS_MPI_CHK( mbedtls_ecp_mul( grp, &R, &rq, &grp->G, f_rng, p_rng ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_copy( r, &R.Y ) );
        
        if( mbedtls_mpi_get_bit( &R.X, 0 ) )
        {
            MBEDTLS_MPI_CHK( mbedtls_mpi_set_bit( r, 455, 1 ) );
        }
        
        /* s computation */
        mbedtls_shake256_init( &sha_ctx );    
        mbedtls_shake256_starts( &sha_ctx );

        if( eddsa_id == MBEDTLS_EDDSA_PREHASH )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom4_ctx( 1, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }
        else if( eddsa_id == MBEDTLS_EDDSA_PURE )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom4_ctx( 0, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }        

        MBEDTLS_MPI_CHK( mbedtls_ecp_point_write_binary( grp, &R, MBEDTLS_ECP_PF_COMPRESSED, &olen, tmp_buf, sizeof( tmp_buf ) ) );
        mbedtls_shake256_update( &sha_ctx, tmp_buf, sizeof( tmp_buf ) );
        
        MBEDTLS_MPI_CHK( mbedtls_ecp_point_write_binary( grp, &Q, MBEDTLS_ECP_PF_COMPRESSED, &olen, tmp_buf, sizeof( tmp_buf ) ) );
        mbedtls_shake256_update( &sha_ctx, tmp_buf, sizeof( tmp_buf ) );
        
        /* In EDDSA_PREHASH, buf should contain the SHA512 hash. It contains the whole message otherwise */
        mbedtls_shake256_update( &sha_ctx, buf, blen );
        
        mbedtls_shake256_finish( &sha_ctx, sha_buf, 114 );
        mbedtls_shake256_free( &sha_ctx );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary_le( &h, sha_buf, sizeof( sha_buf ) ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &h, &h, &grp->N ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &h, &h, &q ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_add_mpi( s, &h, &rq ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( s, s, &grp->N ) );
    }
#endif
    
cleanup:
    mbedtls_mpi_free( &q );
    mbedtls_mpi_free( &prefix );
    mbedtls_mpi_free( &rq );
    mbedtls_mpi_free( &h );
    mbedtls_ecp_point_free( &Q );
    mbedtls_ecp_point_free( &R );

    return( ret );
    
}

int mbedtls_eddsa_verify( mbedtls_ecp_group *grp,
                          const unsigned char *buf, size_t blen,
                          const mbedtls_ecp_point *Q, const mbedtls_mpi *r,
                          const mbedtls_mpi *s, 
                          int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
                          mbedtls_eddsa_id eddsa_id,
                          const unsigned char *ed_ctx, size_t ed_ctx_len)
{
    int ret = 0;
    mbedtls_mpi h;
    mbedtls_ecp_point sB, hA, R;
    
    mbedtls_mpi_init( &h );
    mbedtls_ecp_point_init( &sB );
    mbedtls_ecp_point_init( &hA );
    mbedtls_ecp_point_init( &R );
    
    if( mbedtls_mpi_cmp_mpi( s, &grp->N ) >= 0 || mbedtls_mpi_cmp_int( s, 0 ) < 0 )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );
    
#ifdef MBEDTLS_ECP_DP_ED25519_ENABLED
    if( grp->id == MBEDTLS_ECP_DP_ED25519 )
    {
        mbedtls_sha512_context sha_ctx;
        unsigned char sha_buf[64], tmp_buf[32];
        size_t olen = 0;
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_write_binary_le( r, tmp_buf, sizeof(tmp_buf) ) );
        MBEDTLS_MPI_CHK( mbedtls_ecp_point_read_binary( grp, &R, tmp_buf, sizeof(tmp_buf) ) );
        
        mbedtls_sha512_init( &sha_ctx );    
        mbedtls_sha512_starts( &sha_ctx , 0 );
        
        if( eddsa_id == MBEDTLS_EDDSA_CTX )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom2_ctx( 0, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }
        else if( eddsa_id == MBEDTLS_EDDSA_PREHASH )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom2_ctx( 1, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }
        
        /* tmp_buf contains the R point */
        mbedtls_sha512_update( &sha_ctx, tmp_buf, sizeof( tmp_buf ) );
        
        MBEDTLS_MPI_CHK( mbedtls_ecp_point_write_binary( grp, Q, MBEDTLS_ECP_PF_COMPRESSED, &olen, tmp_buf, sizeof( tmp_buf ) ) );
        mbedtls_sha512_update( &sha_ctx, tmp_buf, sizeof( tmp_buf ) );
        
        /* In EDDSA_PREHASH, buf should contain the SHA512 hash. It contains the whole message otherwise */
        mbedtls_sha512_update( &sha_ctx, buf, blen );
        
        mbedtls_sha512_finish( &sha_ctx, sha_buf );
        mbedtls_sha512_free( &sha_ctx );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary_le( &h, sha_buf, sizeof( sha_buf ) ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &h, &h, &grp->N ) );   
        
        MBEDTLS_MPI_CHK( mbedtls_ecp_mul( grp, &sB, s, &grp->G, f_rng, p_rng ) );
        MBEDTLS_MPI_CHK( mbedtls_ecp_mul( grp, &hA, &h, Q, f_rng, p_rng ) );
        MBEDTLS_MPI_CHK( mbedtls_ecp_add( grp, &R, &R, &hA ) );
        
        MBEDTLS_MPI_CHK( mbedtls_ecp_point_write_binary( grp, &hA, MBEDTLS_ECP_PF_COMPRESSED, &olen, tmp_buf, sizeof( tmp_buf ) ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_sub_mpi( &sB.X, &sB.X, &R.X ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &sB.X, &sB.X, &grp->P ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_sub_mpi( &sB.Y, &sB.Y, &R.Y ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &sB.Y, &sB.Y, &grp->P ) );
        
        if( mbedtls_mpi_cmp_int( &sB.X, 0) != 0 || mbedtls_mpi_cmp_int( &sB.Y, 0 ) != 0 )
        {
            ret = MBEDTLS_ERR_ECP_VERIFY_FAILED;
            goto cleanup;
        }
    }
#endif
#ifdef MBEDTLS_ECP_DP_ED448_ENABLED
    if( grp->id == MBEDTLS_ECP_DP_ED448 )
    {
        mbedtls_shake256_context sha_ctx;
        unsigned char sha_buf[114], tmp_buf[57];
        size_t olen = 0;
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_write_binary_le( r, tmp_buf, sizeof(tmp_buf) ) );
        MBEDTLS_MPI_CHK( mbedtls_ecp_point_read_binary( grp, &R, tmp_buf, sizeof(tmp_buf) ) );
        
        mbedtls_shake256_init( &sha_ctx );    
        mbedtls_shake256_starts( &sha_ctx  );
        
        if( eddsa_id == MBEDTLS_EDDSA_PREHASH )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom4_ctx( 1, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }
        else if( eddsa_id == MBEDTLS_EDDSA_PURE )
        {
            MBEDTLS_MPI_CHK( mbedtls_eddsa_put_dom4_ctx( 0, ed_ctx, ed_ctx_len, &sha_ctx ) );
        }
        
        /* tmp_buf contains the R point */
        mbedtls_shake256_update( &sha_ctx, tmp_buf, sizeof( tmp_buf ) );
        
        MBEDTLS_MPI_CHK( mbedtls_ecp_point_write_binary( grp, Q, MBEDTLS_ECP_PF_COMPRESSED, &olen, tmp_buf, sizeof( tmp_buf ) ) );
        mbedtls_shake256_update( &sha_ctx, tmp_buf, sizeof( tmp_buf ) );
        
        /* In EDDSA_PREHASH, buf should contain the SHAKE256 hash. It contains the whole message otherwise */
        mbedtls_shake256_update( &sha_ctx, buf, blen );
        
        mbedtls_shake256_finish( &sha_ctx, sha_buf, 114 );
        mbedtls_shake256_free( &sha_ctx );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary_le( &h, sha_buf, sizeof( sha_buf ) ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &h, &h, &grp->N ) );   
        
        MBEDTLS_MPI_CHK( mbedtls_ecp_mul( grp, &sB, s, &grp->G, f_rng, p_rng ) );
        MBEDTLS_MPI_CHK( mbedtls_ecp_mul( grp, &hA, &h, Q, f_rng, p_rng ) );
        MBEDTLS_MPI_CHK( mbedtls_ecp_add( grp, &R, &R, &hA ) );
        
        MBEDTLS_MPI_CHK( mbedtls_ecp_point_write_binary( grp, &hA, MBEDTLS_ECP_PF_COMPRESSED, &olen, tmp_buf, sizeof( tmp_buf ) ) );
        
        MBEDTLS_MPI_CHK( mbedtls_mpi_sub_mpi( &sB.X, &sB.X, &R.X ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &sB.X, &sB.X, &grp->P ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_sub_mpi( &sB.Y, &sB.Y, &R.Y ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &sB.Y, &sB.Y, &grp->P ) );
        
        if( mbedtls_mpi_cmp_int( &sB.X, 0) != 0 || mbedtls_mpi_cmp_int( &sB.Y, 0 ) != 0 )
        {
            ret = MBEDTLS_ERR_ECP_VERIFY_FAILED;
            goto cleanup;
        }
    }
#endif
cleanup:
    mbedtls_mpi_free( &h );
    mbedtls_ecp_point_free( &sB );
    mbedtls_ecp_point_free( &hA );
    mbedtls_ecp_point_free( &R );
    return( ret );
}

/*
 * Convert a signature (given by context) to ASN.1
 */
static int eddsa_signature_to_asn1( const mbedtls_mpi *r, const mbedtls_mpi *s,
                                    unsigned char *sig, size_t sig_size,
                                    size_t *slen )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char buf[MBEDTLS_EDDSA_MAX_LEN] = {0};
    unsigned char *p = buf + sizeof( buf );
    size_t len = 0;

    MBEDTLS_ASN1_CHK_ADD( len, mbedtls_asn1_write_mpi( &p, buf, s ) );
    MBEDTLS_ASN1_CHK_ADD( len, mbedtls_asn1_write_mpi( &p, buf, r ) );

    MBEDTLS_ASN1_CHK_ADD( len, mbedtls_asn1_write_len( &p, buf, len ) );
    MBEDTLS_ASN1_CHK_ADD( len, mbedtls_asn1_write_tag( &p, buf,
                                       MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) );

    if( len > sig_size )
        return( MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL );

    memcpy( sig, p, len );
    *slen = len;

    return( 0 );
}

/*
 * Compute and write signature
 */
int mbedtls_eddsa_write_signature( mbedtls_ecp_keypair *ctx,
                           const unsigned char *hash, size_t hlen,
                           unsigned char *sig, size_t sig_size, size_t *slen,
                           int (*f_rng)(void *, unsigned char *, size_t),
                           void *p_rng, mbedtls_eddsa_id eddsa_id,
                           const unsigned char *ed_ctx, size_t ed_ctx_len )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi r, s;
    
    if( ctx == NULL || hash == NULL || sig == NULL || slen == NULL || f_rng == NULL )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );

    mbedtls_mpi_init( &r );
    mbedtls_mpi_init( &s );

    MBEDTLS_MPI_CHK( mbedtls_eddsa_sign( &ctx->grp, &r, &s, &ctx->d,
                                                 hash, hlen, f_rng,
                                                 p_rng, eddsa_id, ed_ctx,
                                                 ed_ctx_len ) );
                                                 
    MBEDTLS_MPI_CHK( eddsa_signature_to_asn1( &r, &s, sig, sig_size, slen ) );

cleanup:
    mbedtls_mpi_free( &r );
    mbedtls_mpi_free( &s );

    return( ret );
}

/*
 * Restartable read and check signature
 */
int mbedtls_eddsa_read_signature( mbedtls_ecp_keypair *ctx,
                          const unsigned char *hash, size_t hlen,
                          const unsigned char *sig, size_t slen,
                          int (*f_rng)(void *, unsigned char *, size_t),
                          void *p_rng, mbedtls_eddsa_id eddsa_id,
                          const unsigned char *ed_ctx, size_t ed_ctx_len )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *p = (unsigned char *) sig;
    const unsigned char *end = sig + slen;
    size_t len;
    mbedtls_mpi r, s;
    
    if( ctx == NULL || hash == NULL || sig == NULL )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );

    mbedtls_mpi_init( &r );
    mbedtls_mpi_init( &s );

    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret += MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

    if( p + len != end )
    {
        ret = MBEDTLS_ERROR_ADD( MBEDTLS_ERR_ECP_BAD_INPUT_DATA,
              MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );
        goto cleanup;
    }

    if( ( ret = mbedtls_asn1_get_mpi( &p, end, &r ) ) != 0 ||
        ( ret = mbedtls_asn1_get_mpi( &p, end, &s ) ) != 0 )
    {
        ret += MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

    if( ( ret = mbedtls_eddsa_verify( &ctx->grp, hash, hlen,
                                      &ctx->Q, &r, &s, f_rng, p_rng, 
                                      eddsa_id, ed_ctx, ed_ctx_len ) ) != 0 )
        goto cleanup;

    /* At this point we know that the buffer starts with a valid signature.
     * Return 0 if the buffer just contains the signature, and a specific
     * error code if the valid signature is followed by more data. */
    if( p != end )
        ret = MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH;

cleanup:
    mbedtls_mpi_free( &r );
    mbedtls_mpi_free( &s );

    return( ret );
}

/*
 * Generate key pair
 */
int mbedtls_eddsa_genkey( mbedtls_ecp_keypair *ctx, mbedtls_ecp_group_id gid,
                  int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int ret = 0;
    
    if( ctx == NULL || f_rng == NULL )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );

    ret = mbedtls_ecp_group_load( &ctx->grp, gid );
    if( ret != 0 )
        return( ret );

   return( mbedtls_ecp_gen_keypair( &ctx->grp, &ctx->d,
                                    &ctx->Q, f_rng, p_rng ) );
}

#endif /* MBEDTLS_EDDSA_C */
