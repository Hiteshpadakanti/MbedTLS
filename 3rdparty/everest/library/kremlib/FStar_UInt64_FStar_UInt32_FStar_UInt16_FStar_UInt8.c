/* Copyright (c) INRIA and Microsoft Corporation. All rights reserved.
   Licensed under the Apache 2.0 License. */

/* This file was generated by KreMLin <https://github.com/FStarLang/kremlin>
 * KreMLin invocation: ../krml -fc89 -fparentheses -fno-shadow -header /mnt/e/everest/verify/hdrB9w -minimal -fparentheses -fcurly-braces -fno-shadow -header copyright-header.txt -minimal -tmpdir dist/minimal -skip-compilation -extract-uints -add-include <inttypes.h> -add-include <stdbool.h> -add-include "kremlin/internal/compat.h" -add-include "kremlin/internal/types.h" -bundle FStar.UInt64+FStar.UInt32+FStar.UInt16+FStar.UInt8=* extracted/prims.krml extracted/FStar_Pervasives_Native.krml extracted/FStar_Pervasives.krml extracted/FStar_Mul.krml extracted/FStar_Squash.krml extracted/FStar_Classical.krml extracted/FStar_StrongExcludedMiddle.krml extracted/FStar_FunctionalExtensionality.krml extracted/FStar_List_Tot_Base.krml extracted/FStar_List_Tot_Properties.krml extracted/FStar_List_Tot.krml extracted/FStar_Seq_Base.krml extracted/FStar_Seq_Properties.krml extracted/FStar_Seq.krml extracted/FStar_Math_Lib.krml extracted/FStar_Math_Lemmas.krml extracted/FStar_BitVector.krml extracted/FStar_UInt.krml extracted/FStar_UInt32.krml extracted/FStar_Int.krml extracted/FStar_Int16.krml extracted/FStar_Preorder.krml extracted/FStar_Ghost.krml extracted/FStar_ErasedLogic.krml extracted/FStar_UInt64.krml extracted/FStar_Set.krml extracted/FStar_PropositionalExtensionality.krml extracted/FStar_PredicateExtensionality.krml extracted/FStar_TSet.krml extracted/FStar_Monotonic_Heap.krml extracted/FStar_Heap.krml extracted/FStar_Map.krml extracted/FStar_Monotonic_HyperHeap.krml extracted/FStar_Monotonic_HyperStack.krml extracted/FStar_HyperStack.krml extracted/FStar_Monotonic_Witnessed.krml extracted/FStar_HyperStack_ST.krml extracted/FStar_HyperStack_All.krml extracted/FStar_Date.krml extracted/FStar_Universe.krml extracted/FStar_GSet.krml extracted/FStar_ModifiesGen.krml extracted/LowStar_Monotonic_Buffer.krml extracted/LowStar_Buffer.krml extracted/Spec_Loops.krml extracted/LowStar_BufferOps.krml extracted/C_Loops.krml extracted/FStar_UInt8.krml extracted/FStar_Kremlin_Endianness.krml extracted/FStar_UInt63.krml extracted/FStar_Exn.krml extracted/FStar_ST.krml extracted/FStar_All.krml extracted/FStar_Dyn.krml extracted/FStar_Int63.krml extracted/FStar_Int64.krml extracted/FStar_Int32.krml extracted/FStar_Int8.krml extracted/FStar_UInt16.krml extracted/FStar_Int_Cast.krml extracted/FStar_UInt128.krml extracted/C_Endianness.krml extracted/FStar_List.krml extracted/FStar_Float.krml extracted/FStar_IO.krml extracted/C.krml extracted/FStar_Char.krml extracted/FStar_String.krml extracted/LowStar_Modifies.krml extracted/C_String.krml extracted/FStar_Bytes.krml extracted/FStar_HyperStack_IO.krml extracted/C_Failure.krml extracted/TestLib.krml extracted/FStar_Int_Cast_Full.krml
 * F* version: 059db0c8
 * KreMLin version: 916c37ac
 */


#include "FStar_UInt64_FStar_UInt32_FStar_UInt16_FStar_UInt8.h"

uint64_t FStar_UInt64_eq_mask(uint64_t a, uint64_t b)
{
    uint64_t x = a ^ b;
    uint64_t minus_x = ~x + (uint64_t)1U;
    uint64_t x_or_minus_x = x | minus_x;
    uint64_t xnx = x_or_minus_x >> (uint32_t)63U;
    return xnx - (uint64_t)1U;
}

uint64_t FStar_UInt64_gte_mask(uint64_t a, uint64_t b)
{
    uint64_t x = a;
    uint64_t y = b;
    uint64_t x_xor_y = x ^ y;
    uint64_t x_sub_y = x - y;
    uint64_t x_sub_y_xor_y = x_sub_y ^ y;
    uint64_t q = x_xor_y | x_sub_y_xor_y;
    uint64_t x_xor_q = x ^ q;
    uint64_t x_xor_q_ = x_xor_q >> (uint32_t)63U;
    return x_xor_q_ - (uint64_t)1U;
}

uint32_t FStar_UInt32_eq_mask(uint32_t a, uint32_t b)
{
    uint32_t x = a ^ b;
    uint32_t minus_x = ~x + (uint32_t)1U;
    uint32_t x_or_minus_x = x | minus_x;
    uint32_t xnx = x_or_minus_x >> (uint32_t)31U;
    return xnx - (uint32_t)1U;
}

uint32_t FStar_UInt32_gte_mask(uint32_t a, uint32_t b)
{
    uint32_t x = a;
    uint32_t y = b;
    uint32_t x_xor_y = x ^ y;
    uint32_t x_sub_y = x - y;
    uint32_t x_sub_y_xor_y = x_sub_y ^ y;
    uint32_t q = x_xor_y | x_sub_y_xor_y;
    uint32_t x_xor_q = x ^ q;
    uint32_t x_xor_q_ = x_xor_q >> (uint32_t)31U;
    return x_xor_q_ - (uint32_t)1U;
}

uint16_t FStar_UInt16_eq_mask(uint16_t a, uint16_t b)
{
    uint16_t x = a ^ b;
    uint16_t minus_x = ~x + (uint16_t)1U;
    uint16_t x_or_minus_x = x | minus_x;
    uint16_t xnx = x_or_minus_x >> (uint32_t)15U;
    return xnx - (uint16_t)1U;
}

uint16_t FStar_UInt16_gte_mask(uint16_t a, uint16_t b)
{
    uint16_t x = a;
    uint16_t y = b;
    uint16_t x_xor_y = x ^ y;
    uint16_t x_sub_y = x - y;
    uint16_t x_sub_y_xor_y = x_sub_y ^ y;
    uint16_t q = x_xor_y | x_sub_y_xor_y;
    uint16_t x_xor_q = x ^ q;
    uint16_t x_xor_q_ = x_xor_q >> (uint32_t)15U;
    return x_xor_q_ - (uint16_t)1U;
}

uint8_t FStar_UInt8_eq_mask(uint8_t a, uint8_t b)
{
    uint8_t x = a ^ b;
    uint8_t minus_x = ~x + (uint8_t)1U;
    uint8_t x_or_minus_x = x | minus_x;
    uint8_t xnx = x_or_minus_x >> (uint32_t)7U;
    return xnx - (uint8_t)1U;
}

uint8_t FStar_UInt8_gte_mask(uint8_t a, uint8_t b)
{
    uint8_t x = a;
    uint8_t y = b;
    uint8_t x_xor_y = x ^ y;
    uint8_t x_sub_y = x - y;
    uint8_t x_sub_y_xor_y = x_sub_y ^ y;
    uint8_t q = x_xor_y | x_sub_y_xor_y;
    uint8_t x_xor_q = x ^ q;
    uint8_t x_xor_q_ = x_xor_q >> (uint32_t)7U;
    return x_xor_q_ - (uint8_t)1U;
}
