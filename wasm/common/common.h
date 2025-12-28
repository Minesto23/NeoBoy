/**
 * NeoBoy - Common Definitions and Utilities
 * 
 * Purpose: Shared types, macros, and utilities used across all WASM cores
 * 
 * This file provides common definitions for:
 * - Standard integer types
 * - Boolean type
 * - Common macros for bit manipulation and array sizing
 * - Memory alignment helpers
 */

#ifndef NEOBOY_COMMON_H
#define NEOBOY_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Standard type aliases for clarity */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

/* Common macros */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) (MIN(MAX(x, min), max))

/* Bit manipulation helpers */
#define BIT(n) (1U << (n))
#define SET_BIT(val, n) ((val) |= BIT(n))
#define CLEAR_BIT(val, n) ((val) &= ~BIT(n))
#define TOGGLE_BIT(val, n) ((val) ^= BIT(n))
#define CHECK_BIT(val, n) (((val) >> (n)) & 1)

/* Memory alignment */
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define ALIGN_UP(addr, align) ALIGN_DOWN((addr) + (align) - 1, align)

/* Color conversion utilities */
#define RGB15_TO_RGB32(rgb15) \
    (0xFF000000 | \
     (((rgb15) & 0x001F) << 19) | \
     (((rgb15) & 0x03E0) << 6) | \
     (((rgb15) & 0x7C00) >> 7))

#endif /* NEOBOY_COMMON_H */
