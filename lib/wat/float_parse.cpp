// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wat/wat_util.h"

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
} // namespace

float strtofC(const char *Ptr, char **End) {
  return _strtof_l(Ptr, End, cLocale());
}
double strtodC(const char *Ptr, char **End) {
  return _strtod_l(Ptr, End, cLocale());
}
#elif defined(WASMEDGE_WAT_HAVE_STRTOD_L)
namespace {
locale_t cLocale() {
  static const locale_t Locale =
      newlocale(LC_ALL_MASK, "C", static_cast<locale_t>(0));
  return Locale;
}
} // namespace

float strtofC(const char *Ptr, char **End) {
  return strtof_l(Ptr, End, cLocale());
}
double strtodC(const char *Ptr, char **End) {
  return strtod_l(Ptr, End, cLocale());
}
#else
// The C library has no locale-aware variant. Some C libraries, such as musl,
// always keep the "C" numeric locale in strtof and strtod.
float strtofC(const char *Ptr, char **End) { return std::strtof(Ptr, End); }
double strtodC(const char *Ptr, char **End) { return std::strtod(Ptr, End); }
#endif

} // namespace WAT
} // namespace WasmEdge
