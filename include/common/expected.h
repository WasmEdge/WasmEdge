// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "experimental/expected.hpp"

namespace WasmEdge {

/// Type aliasing of expected class.
template <typename T, typename E> using Expected = cxx20::expected<T, E>;

/// Type aliasing of unexpected class.
template <typename E> using Unexpected = cxx20::unexpected<E>;

} // namespace WasmEdge

#define EXPECTED_OVERLOAD_GLUE(X, Y) X Y
#define EXPECTED_RETURN_ARG_COUNT(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_,      \
                                  Count, ...)                                  \
  Count
#define EXPECTED_EXPAND_ARGS(Args) EXPECTED_RETURN_ARG_COUNT Args
#define EXPECTED_COUNT_ARGS_MAX8(...)                                          \
  EXPECTED_EXPAND_ARGS((__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define EXPECTED_OVERLOAD_MACRO2(Name, Count) Name##Count
#define EXPECTED_OVERLOAD_MACRO1(Name, Count)                                  \
  EXPECTED_OVERLOAD_MACRO2(Name, Count)
#define EXPECTED_OVERLOAD_MACRO(Name, Count)                                   \
  EXPECTED_OVERLOAD_MACRO1(Name, Count)
#define EXPECTED_CALL_OVERLOAD(Name, ...)                                      \
  EXPECTED_OVERLOAD_GLUE(                                                      \
      EXPECTED_OVERLOAD_MACRO(Name, EXPECTED_COUNT_ARGS_MAX8(__VA_ARGS__)),    \
      (__VA_ARGS__))

#define _EXPECTED_OVERLOAD_GLUE(X, Y) X Y
#define _EXPECTED_RETURN_ARG_COUNT(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_,     \
                                   Count, ...)                                 \
  Count
#define _EXPECTED_EXPAND_ARGS(Args) _EXPECTED_RETURN_ARG_COUNT Args
#define _EXPECTED_COUNT_ARGS_MAX8(...)                                         \
  _EXPECTED_EXPAND_ARGS((__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define _EXPECTED_OVERLOAD_MACRO2(Name, Count) Name##Count
#define _EXPECTED_OVERLOAD_MACRO1(Name, Count)                                 \
  _EXPECTED_OVERLOAD_MACRO2(Name, Count)
#define _EXPECTED_OVERLOAD_MACRO(Name, Count)                                  \
  _EXPECTED_OVERLOAD_MACRO1(Name, Count)
#define _EXPECTED_CALL_OVERLOAD(Name, ...)                                     \
  _EXPECTED_OVERLOAD_GLUE(                                                     \
      _EXPECTED_OVERLOAD_MACRO(Name, _EXPECTED_COUNT_ARGS_MAX8(__VA_ARGS__)),  \
      (__VA_ARGS__))

#define EXPECTED_UNIQUE_NAME                                                   \
  EXPECTED_OVERLOAD_MACRO1(ExpectedUniqueNameTemporary, __COUNTER__)

#define EXPECTED_UNIQUE_STORAGE_AUTO(...) auto
#define EXPECTED_UNIQUE_STORAGE_UNPACK(...) __VA_ARGS__
#define EXPECTED_UNIQUE_STORAGE_DEDUCE3(Unique, ...)                           \
  EXPECTED_UNIQUE_STORAGE_AUTO(__VA_ARGS__) Unique = (__VA_ARGS__)
#define EXPECTED_UNIQUE_STORAGE_DEDUCE2(X) X
#define EXPECTED_UNIQUE_STORAGE_DEDUCE(Unique, X, ...)                         \
  EXPECTED_UNIQUE_STORAGE_DEDUCE2(                                             \
      EXPECTED_UNIQUE_STORAGE_DEDUCE3(Unique, __VA_ARGS__))
#define EXPECTED_UNIQUE_STORAGE_SPECIFIED3(Unique, X, Y, ...)                  \
  X Unique = (__VA_ARGS__)
#define EXPECTED_UNIQUE_STORAGE_SPECIFIED2(X) X
#define EXPECTED_UNIQUE_STORAGE_SPECIFIED(Unique, ...)                         \
  EXPECTED_UNIQUE_STORAGE_SPECIFIED2(                                          \
      EXPECTED_UNIQUE_STORAGE_SPECIFIED3(Unique, __VA_ARGS__))
#define EXPECTED_UNIQUE_STORAGE1(...) EXPECTED_UNIQUE_STORAGE_DEDUCE
#define EXPECTED_UNIQUE_STORAGE2(...) EXPECTED_UNIQUE_STORAGE_SPECIFIED
#define EXPECTED_UNIQUE_STORAGE(Unique, Spec, ...)                             \
  _EXPECTED_CALL_OVERLOAD(EXPECTED_UNIQUE_STORAGE,                             \
                          EXPECTED_UNIQUE_STORAGE_UNPACK Spec)                 \
  (Unique, EXPECTED_UNIQUE_STORAGE_UNPACK Spec, __VA_ARGS__)

#define EXPECTED_LIKELY_IF(Unique) if (likely(!!(Unique)))
#define EXPECTED_LIKELY_IF_SCOPED(Unique, Spec, ...)                           \
  if (EXPECTED_UNIQUE_STORAGE(Unique, Spec, __VA_ARGS__); likely(!!(Unique)))

#define EXPECTED_TRY2_VAR_SECOND2(X, Var) Var
#define EXPECTED_TRY2_VAR_SECOND3(X, Y, ...) X, Y
#define EXPECTED_TRY2_VAR(Spec)                                                \
  _EXPECTED_CALL_OVERLOAD(EXPECTED_TRY2_VAR_SECOND,                            \
                          EXPECTED_UNIQUE_STORAGE_UNPACK Spec, Spec)

#define EXPECTED_TRYV_SUCCESS_LIKELY(Unique, RetStmt, Spec, ...)               \
  EXPECTED_UNIQUE_STORAGE(Unique, Spec, __VA_ARGS__);                          \
  EXPECTED_LIKELY_IF(Unique);                                                  \
  else RetStmt ::cxx20::unexpected((Unique).error())
#define EXPECTED_TRYV_SUCCESS_LIKELY_SCOPED(Unique, RetStmt, Spec, ...)        \
  EXPECTED_LIKELY_IF_SCOPED(Unique, Spec, __VA_ARGS__);                        \
  else RetStmt ::cxx20::unexpected((Unique).error())
#define EXPECTED_TRY2_SUCCESS_LIKELY(Unique, RetStmt, Var, ...)                \
  EXPECTED_TRYV_SUCCESS_LIKELY(Unique, RetStmt, Var, __VA_ARGS__);             \
  EXPECTED_TRY2_VAR(Var) = std::move((Unique).value())

#define EXPECTED_TRYV(...)                                                     \
  EXPECTED_TRYV_SUCCESS_LIKELY_SCOPED(EXPECTED_UNIQUE_NAME, return, Deduce,    \
                                      __VA_ARGS__)
#define EXPECTED_TRYA(Var, ...)                                                \
  EXPECTED_TRY2_SUCCESS_LIKELY(EXPECTED_UNIQUE_NAME, return, Var, __VA_ARGS__)

#define EXPECTED_INVOKE_TRY8(A, B, C, D, E, F, G, H)                           \
  EXPECTED_TRYA(A, B, C, D, E, F, G, H)
#define EXPECTED_INVOKE_TRY7(A, B, C, D, E, F, G)                              \
  EXPECTED_TRYA(A, B, C, D, E, F, G)
#define EXPECTED_INVOKE_TRY6(A, B, C, D, E, F) EXPECTED_TRYA(A, B, C, D, E, F)
#define EXPECTED_INVOKE_TRY5(A, B, C, D, E) EXPECTED_TRYA(A, B, C, D, E)
#define EXPECTED_INVOKE_TRY4(A, B, C, D) EXPECTED_TRYA(A, B, C, D)
#define EXPECTED_INVOKE_TRY3(A, B, C) EXPECTED_TRYA(A, B, C)
#define EXPECTED_INVOKE_TRY2(A, B) EXPECTED_TRYA(A, B)
#define EXPECTED_INVOKE_TRY1(A) EXPECTED_TRYV(A)

#define EXPECTED_TRY(...)                                                      \
  EXPECTED_CALL_OVERLOAD(EXPECTED_INVOKE_TRY, __VA_ARGS__)
