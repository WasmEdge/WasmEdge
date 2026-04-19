// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wat/wat_util.h"

#include <cfenv>
#include <clocale>
#include <cstdlib>

#if defined(WASMEDGE_WAT_HAVE_STRTOD_L) ||                                     \
    defined(WASMEDGE_WAT_HAVE_WIN_STRTOD_L)
#include <locale.h>
#include <stdlib.h>
#endif
#if defined(WASMEDGE_WAT_HAVE_XLOCALE_H)
#include <xlocale.h>
#endif

namespace WasmEdge {
namespace WAT {

// CMake finds the locale-aware functions of the C library and defines one of
// the WASMEDGE_WAT_HAVE_* macros. It also defines the feature macro that the C
// library needs to declare the functions, such as _GNU_SOURCE.
#if defined(WASMEDGE_WAT_HAVE_WIN_STRTOD_L)
namespace {
_locale_t cLocale() {
  static const _locale_t Locale = _create_locale(LC_ALL, "C");
  return Locale;
}

float strtofLocale(const char *Ptr, char **End) {
  return _strtof_l(Ptr, End, cLocale());
}
double strtodLocale(const char *Ptr, char **End) {
  return _strtod_l(Ptr, End, cLocale());
}
} // namespace
#elif defined(WASMEDGE_WAT_HAVE_STRTOD_L)
namespace {
locale_t cLocale() {
  static const locale_t Locale =
      newlocale(LC_ALL_MASK, "C", static_cast<locale_t>(0));
  return Locale;
}

float strtofLocale(const char *Ptr, char **End) {
  return strtof_l(Ptr, End, cLocale());
}
double strtodLocale(const char *Ptr, char **End) {
  return strtod_l(Ptr, End, cLocale());
}
} // namespace
#else
// The C library has no locale-aware variant. Some C libraries, such as musl,
// always keep the "C" numeric locale in strtof and strtod.
namespace {
float strtofLocale(const char *Ptr, char **End) {
  return std::strtof(Ptr, End);
}
double strtodLocale(const char *Ptr, char **End) {
  return std::strtod(Ptr, End);
}
} // namespace
#endif

namespace {
// Run Parse with the rounding mode round-to-nearest, ties-to-even, which the
// text format needs. The caller can use another rounding mode or enable
// floating-point traps. This function saves the floating-point environment of
// the caller, and restores it after the parse. The exception flags that the
// parse raises are then discarded.
template <typename T>
T parseWithNearestRounding(T (*Parse)(const char *, char **), const char *Ptr,
                           char **End) {
  std::fenv_t Env;
  if (std::feholdexcept(&Env) != 0) {
    // The environment was not saved, so it cannot be restored.
    return Parse(Ptr, End);
  }
  std::fesetround(FE_TONEAREST);
  const T Val = Parse(Ptr, End);
  std::fesetenv(&Env);
  return Val;
}
} // namespace

float strtofC(const char *Ptr, char **End) {
  return parseWithNearestRounding(&strtofLocale, Ptr, End);
}
double strtodC(const char *Ptr, char **End) {
  return parseWithNearestRounding(&strtodLocale, Ptr, End);
}

} // namespace WAT
} // namespace WasmEdge
