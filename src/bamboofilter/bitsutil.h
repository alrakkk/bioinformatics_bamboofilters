#pragma once
/* bitsutil.h  –  FER Bioinformatics 1 2024/25
 *
 *  All routines are **constexpr** / header-only so the compiler can fully inline and optimise them away.
 */

#include <cstdint>

/* ===================================================================== */
/*  4-bit SIMD-within-a-register tricks                                   */
/* ===================================================================== */
/*
 *  We pack four 4-bit “nibbles” into a 16-bit word:
 *
 *        0xF   0xE   0xD   0xC      (Most-significant nibble first)
 *      ┌────┬────┬────┬────┐
 *      │xxxx│xxxx│xxxx│xxxx│
 *      └────┴────┴────┴────┘
 *
 *  has_zero4(x)
 *  ------------
 *  Returns **true** if *any* of the four nibbles inside @p x is **zero**.
 *  Implemented with a classic binary-trick (subtract, AND, magic mask)
 *  that avoids branches and individual nibble extraction.
 *
 *  has_value4(x, n)
 *  ----------------
 *  Same idea, but reports if any nibble equals the 4-bit value @p n.
 *  We simply XOR every nibble with @p n;   x⊕n == 0  ↔  nibble == n,
 *  and then reuse has_zero4().
 */

/** True if *any* 4-bit nibble inside @p x equals **0**. */
inline constexpr bool has_zero4(uint16_t x) noexcept
{
    /*  x - 0x1111 subtracts 1 from each nibble (with borrow between nibbles
     *  suppressed by masking later).  (~x) marks original zeros with 1-bits.
     *  The final 0x8888 mask picks the sign-bits of every nibble.          */
    return (((x - 0x1111u) & ~x) & 0x8888u) != 0;
}

/** True if *any* nibble of @p x equals the 4-bit constant @p n. */
inline constexpr bool has_value4(uint16_t x, uint16_t n) noexcept
{
    /* XOR turns “nibble==n” into “nibble==0”, then use the zero test. */
    return has_zero4(static_cast<uint16_t>(x ^ (0x1111u * n)));
}

/* ===================================================================== */
/*  Small utility: round **up** to the next power-of-two                  */
/* ===================================================================== */
/*
 *  upper_power2(x)
 *  --------------
 *  Fast bit-twiddling routine that returns the *smallest* power-of-two
 *  that is **≥ x**.  (By definition  upper_power2(0) == 1.)
 *
 *  Works by first spreading the most significant 1-bit to all lower
 *  positions (binary “smear”), then adding one..
 */
inline uint64_t upper_power2(uint64_t x)
{
    if (!x) return 1;          // edge-case: next power-of-two after 0 is 1
    --x;                       // smear highest-set bit to the right
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return ++x;                // one past the smeared value → exact power-of-2
}
