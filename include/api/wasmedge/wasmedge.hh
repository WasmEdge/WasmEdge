// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/wasmedge.hh - WasmEdge C++ API ------------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the class declarations of WasmEdge C++ API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_CPP_API_HH
#define WASMEDGE_CPP_API_HH

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
#ifdef WASMEDGE_COMPILE_LIBRARY
#define WASMEDGE_CPP_API_EXPORT __declspec(dllexport)
#else
#define WASMEDGE_CPP_API_EXPORT __declspec(dllimport)
#endif // WASMEDGE_COMPILE_LIBRARY
#ifdef WASMEDGE_PLUGIN
#define WASMEDGE_CPP_API_PLUGIN_EXPORT __declspec(dllexport)
#else
#define WASMEDGE_CPP_API_PLUGIN_EXPORT __declspec(dllimport)
#endif // WASMEDGE_PLUGIN
#else
#define WASMEDGE_CPP_API_EXPORT __attribute__((visibility("default")))
#define WASMEDGE_CPP_API_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif // _WIN32

#include "string"

namespace WasmEdge {
  enum ErrCategory {
    WASM = 0x00,
    UserLevelError = 0x01
  };

  class WASMEDGE_CPP_API_EXPORT String {
  public:
    String(const std::string Str);
    String(const std::string Buf, const uint32_t Len);
    ~String();
    static String Wrap(const std::string Buf, const uint32_t Len);

    bool IsEqual(const String Str);
    uint32_t Copy(std::string Buf, const uint32_t Len);

  private:
    uint32_t Len;
    const std::string Buf;
  };

  class WASMEDGE_CPP_API_EXPORT Result {
  public:
    Result(const ErrCategory Category, const uint32_t Code);
    ~Result();

    // static methods
    static Result Success() { return Result{ 0x00 }; }
    static Result Terminate() { return Result{ 0x01 }; }
    static Result Fail() { return Result{ 0x02 }; }

    bool IsOk();
    uint32_t GetCode();
    ErrCategory GetCategory();
    const char * GetMessage();

  private:
    uint32_t Code;
    Result(const uint32_t Code);
  };

  class WASMEDGE_CPP_API_EXPORT Limit {
  public:
    bool HasMax;
    bool Shared;
    uint32_t Min;
    uint32_t Max;

    bool IsEqual(const Limit Lim);
  };
}

#endif // WASMEDGE_CPP_API_HH