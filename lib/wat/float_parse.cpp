// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wat/wat_util.h"

#include <cfenv>
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <string>

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

namespace {
// Run Parse on the literal at Ptr in the LC_NUMERIC locale of the process.
// Parse reads the decimal point of that locale, so write the decimal point in
// place of the '.' of the literal first. The text format has at most one '.'
// in a float literal. End then points into Ptr, at the same character as for a
// parse in the "C" locale.
template <typename T>
T parseInAnyLocale(T (*Parse)(const char *, char **), const char *Ptr,
                   char **End) {
  const char *Point = std::localeconv()->decimal_point;
  const char *Dot = std::strchr(Ptr, '.');
  if (Point == nullptr || std::strcmp(Point, ".") == 0 || Dot == nullptr) {
    return Parse(Ptr, End);
  }
  const auto DotPos = static_cast<size_t>(Dot - Ptr);
  const size_t PointLen = std::strlen(Point);
  std::string Text(Ptr);
  Text.replace(DotPos, 1, Point);
  char *TextEnd = nullptr;
  const T Val = Parse(Text.c_str(), &TextEnd);
  if (End != nullptr) {
    auto Used = static_cast<size_t>(TextEnd - Text.c_str());
    if (Used >= DotPos + PointLen) {
      Used = Used - PointLen + 1;
    } else if (Used > DotPos) {
      Used = DotPos;
    }
    *End = const_cast<char *>(Ptr) + Used;
  }
  return Val;
}
} // namespace

float strtofAnyLocale(const char *Ptr, char **End) {
  return parseInAnyLocale<float>(&std::strtof, Ptr, End);
}
double strtodAnyLocale(const char *Ptr, char **End) {
  return parseInAnyLocale<double>(&std::strtod, Ptr, End);
}

// CMake finds the locale-aware functions of the C library and defines one of
// the WASMEDGE_WAT_HAVE_* macros. It also defines the feature macro that the C
// library needs to declare the functions, such as _GNU_SOURCE.
#if defined(WASMEDGE_WAT_HAVE_WIN_STRTOD_L)
namespace {
// The handle is null if the C library cannot create the locale. The parse
// then uses the fallback.
_locale_t cLocale() {
  static const _locale_t Locale = _create_locale(LC_ALL, "C");
  return Locale;
}

float strtofLocale(const char *Ptr, char **End) {
  if (auto Locale = cLocale(); Locale != nullptr) {
    return _strtof_l(Ptr, End, Locale);
  }
  return strtofAnyLocale(Ptr, End);
}
double strtodLocale(const char *Ptr, char **End) {
  if (auto Locale = cLocale(); Locale != nullptr) {
    return _strtod_l(Ptr, End, Locale);
  }
  return strtodAnyLocale(Ptr, End);
}
} // namespace
#elif defined(WASMEDGE_WAT_HAVE_STRTOD_L)
namespace {
// The handle is null if the C library cannot create the locale. The parse
// then uses the fallback.
locale_t cLocale() {
  static const locale_t Locale =
      newlocale(LC_ALL_MASK, "C", static_cast<locale_t>(0));
  return Locale;
}

float strtofLocale(const char *Ptr, char **End) {
  if (auto Locale = cLocale(); Locale != static_cast<locale_t>(0)) {
    return strtof_l(Ptr, End, Locale);
  }
  return strtofAnyLocale(Ptr, End);
}
double strtodLocale(const char *Ptr, char **End) {
  if (auto Locale = cLocale(); Locale != static_cast<locale_t>(0)) {
    return strtod_l(Ptr, End, Locale);
  }
  return strtodAnyLocale(Ptr, End);
}
} // namespace
#else
// The C library has no locale-aware variant, so strtof and strtod read the
// LC_NUMERIC locale of the process. The fallback writes the decimal point of
// that locale into the literal, so the result does not depend on the locale.
namespace {
float strtofLocale(const char *Ptr, char **End) {
  return strtofAnyLocale(Ptr, End);
}
double strtodLocale(const char *Ptr, char **End) {
  return strtodAnyLocale(Ptr, End);
}
} // namespace
#endif

const FloatEnvOps &defaultFloatEnvOps() noexcept {
  static const FloatEnvOps Ops{
      [](std::fenv_t *Env) { return std::feholdexcept(Env); },
      [](int Mode) { return std::fesetround(Mode); },
      [](const std::fenv_t *Env) { return std::fesetenv(Env); },
  };
  return Ops;
}

namespace {
// Set the rounding mode round-to-nearest, ties-to-even, which the text format
// needs, for the life of this object. The caller can use another rounding mode
// or enable floating-point traps. The object saves the floating-point
// environment of the caller, and restores it at the end. The exception flags
// that the parse raises are then discarded. If the environment cannot be
// saved, the object is not ready, because the flags and the traps of the caller
// cannot be kept.
class NearestRounding {
public:
  explicit NearestRounding(const FloatEnvOps &O) noexcept : Ops(O) {
    if (Ops.HoldExcept(&Env) != 0) {
      return;
    }
    Saved = true;
    Ready = Ops.SetRound(FE_TONEAREST) == 0;
  }
  ~NearestRounding() noexcept {
    if (Saved) {
      Ops.SetEnv(&Env);
    }
  }
  NearestRounding(const NearestRounding &) = delete;
  NearestRounding &operator=(const NearestRounding &) = delete;

  bool ready() const noexcept { return Ready; }

private:
  const FloatEnvOps &Ops;
  std::fenv_t Env{};
  bool Saved = false;
  bool Ready = false;
};

// Run Parse with the rounding mode round-to-nearest, ties-to-even. If this
// mode cannot be set in a saved environment, parse nothing, so that the caller
// reports an error.
template <typename T>
T parseWithNearestRounding(T (*Parse)(const char *, char **), const char *Ptr,
                           char **End, const FloatEnvOps &Ops) {
  NearestRounding Rounding(Ops);
  if (!Rounding.ready()) {
    if (End != nullptr) {
      *End = const_cast<char *>(Ptr);
    }
    return T(0);
  }
  return Parse(Ptr, End);
}
} // namespace

float strtofC(const char *Ptr, char **End, const FloatEnvOps &Ops) {
  return parseWithNearestRounding(&strtofLocale, Ptr, End, Ops);
}
double strtodC(const char *Ptr, char **End, const FloatEnvOps &Ops) {
  return parseWithNearestRounding(&strtodLocale, Ptr, End, Ops);
}

} // namespace WAT
} // namespace WasmEdge
