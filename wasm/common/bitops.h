/**
 * NeoBoy - Bit Operation Utilities
 * 
 * Purpose: Advanced bit manipulation helpers for CPU emulation
 * 
 * Provides utilities for:
 * - Bit field extraction and insertion
 * - Rotation operations
 * - Sign extension
 * - Byte swapping
 */

#ifndef NEOBOY_BITOPS_H
#define NEOBOY_BITOPS_H

#include "common.h"

/* Extract bit field from value */
static inline u32 extract_bits(u32 value, u32 start, u32 length) {
    u32 mask = (1U << length) - 1;
    return (value >> start) & mask;
}

/* Insert bit field into value */
static inline u32 insert_bits(u32 value, u32 field, u32 start, u32 length) {
    u32 mask = ((1U << length) - 1) << start;
    return (value & ~mask) | ((field << start) & mask);
}

/* Rotate left */
static inline u8 rotate_left_8(u8 value, u8 shift) {
    shift &= 7;
    return (value << shift) | (value >> (8 - shift));
}

static inline u16 rotate_left_16(u16 value, u8 shift) {
    shift &= 15;
    return (value << shift) | (value >> (16 - shift));
}

static inline u32 rotate_left_32(u32 value, u8 shift) {
    shift &= 31;
    return (value << shift) | (value >> (32 - shift));
}

/* Rotate right */
static inline u8 rotate_right_8(u8 value, u8 shift) {
    shift &= 7;
    return (value >> shift) | (value << (8 - shift));
}

static inline u16 rotate_right_16(u16 value, u8 shift) {
    shift &= 15;
    return (value >> shift) | (value << (16 - shift));
}

static inline u32 rotate_right_32(u32 value, u8 shift) {
    shift &= 31;
    return (value >> shift) | (value << (32 - shift));
}

/* Sign extension */
static inline s16 sign_extend_8(u8 value) {
    return (s16)((s8)value);
}

static inline s32 sign_extend_16(u16 value) {
    return (s32)((s16)value);
}

/* Byte swapping (endianness conversion) */
static inline u16 swap_bytes_16(u16 value) {
    return (value >> 8) | (value << 8);
}

static inline u32 swap_bytes_32(u32 value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

#endif /* NEOBOY_BITOPS_H */
