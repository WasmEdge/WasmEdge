/**
 * THIS FILE IS AUTO-GENERATED from the following files:
 *   wasi_ephemeral_nn.witx
 *
 * @file
 * This file describes the [WASI] interface, consisting of functions, types,
 * and defined values (macros).
 *
 * The interface described here is greatly inspired by [CloudABI]'s clean,
 * thoughtfully-designed, capability-oriented, POSIX-style API.
 *
 * [CloudABI]: https://github.com/NuxiNL/cloudlibc
 * [WASI]: https://github.com/WebAssembly/WASI/
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

using const_uint8_t_ptr = uint32_t;
using uint8_t_ptr = uint32_t;

#define DEFINE_ENUM_OPERATORS(type)                                            \
  inline constexpr type operator~(type a) noexcept {                           \
    return static_cast<type>(~static_cast<std::underlying_type_t<type>>(a));   \
  }                                                                            \
  inline constexpr type operator|(type a, type b) noexcept {                   \
    return static_cast<type>(static_cast<std::underlying_type_t<type>>(a) |    \
                             static_cast<std::underlying_type_t<type>>(b));    \
  }                                                                            \
  inline constexpr type &operator|=(type &a, type b) noexcept {                \
    a = a | b;                                                                 \
    return a;                                                                  \
  }                                                                            \
  inline constexpr type operator&(type a, type b) noexcept {                   \
    return static_cast<type>(static_cast<std::underlying_type_t<type>>(a) &    \
                             static_cast<std::underlying_type_t<type>>(b));    \
  }                                                                            \
  inline constexpr type &operator&=(type &a, type b) noexcept {                \
    a = a & b;                                                                 \
    return a;                                                                  \
  }

static_assert(alignof(int8_t) == 1, "non-wasi data layout");
static_assert(alignof(uint8_t) == 1, "non-wasi data layout");
static_assert(alignof(int16_t) == 2, "non-wasi data layout");
static_assert(alignof(uint16_t) == 2, "non-wasi data layout");
static_assert(alignof(int32_t) == 4, "non-wasi data layout");
static_assert(alignof(uint32_t) == 4, "non-wasi data layout");
static_assert(alignof(int64_t) == 8, "non-wasi data layout");
static_assert(alignof(uint64_t) == 8, "non-wasi data layout");
static_assert(alignof(const_uint8_t_ptr) == 4, "non-wasi data layout");
static_assert(alignof(uint8_t_ptr) == 4, "non-wasi data layout");

/**
 * The size of a graph buffer. This is equivalent to `$size` in `typenames.witx`
 * but renamed since `typenames.witx` is not included here but is included in
 * the overall ephemeral phase.
 */
using __wasi_buffer_size_t = uint32_t;

static_assert(sizeof(__wasi_buffer_size_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_buffer_size_t) == 4, "witx calculated align");

/**
 * Error codes returned by functions in this API. This is prefixed to avoid
 * conflicts with the `$errno` in `typenames.witx`.
 */
enum __wasi_nn_errno_t : uint16_t {
  /**
   * No error occurred.
   */
  __WASI_NN_ERRNO_SUCCESS = 0,

  /**
   * Caller module passed an invalid argument.
   */
  __WASI_NN_ERRNO_INVALID_ARGUMENT = 1,

  /**
   * Caller module is missing a memory export.
   */
  __WASI_NN_ERRNO_MISSING_MEMORY = 2,

  /**
   * Device or resource busy.
   */
  __WASI_NN_ERRNO_BUSY = 3,

};
static_assert(sizeof(__wasi_nn_errno_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_nn_errno_t) == 2, "witx calculated align");

/**
 * The dimensions of a tensor.
 *
 * The array length matches the tensor rank and each element in the array
 * describes the size of each dimension.
 */

/**
 * The type of the elements in a tensor.
 */
enum __wasi_tensor_type_t : uint8_t {
  __WASI_TENSOR_TYPE_F16 = 0,

  __WASI_TENSOR_TYPE_F32 = 1,

  __WASI_TENSOR_TYPE_U8 = 2,

  __WASI_TENSOR_TYPE_I32 = 3,

};
static_assert(sizeof(__wasi_tensor_type_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_tensor_type_t) == 1, "witx calculated align");

/**
 * The tensor data
 *
 * Initially conceived as a sparse representation, each empty cell would be
 * filled with zeroes and the array length must match the product of all of the
 * dimensions and the number of bytes in the type (e.g. a 2x2 tensor with 4-byte
 * f32 elements would have a data array of length 16). Naturally, this
 * representation requires some knowledge of how to lay out data in memory--e.g.
 * using row-major ordering--and could perhaps be improved by future witx
 * features (TODO).
 */
