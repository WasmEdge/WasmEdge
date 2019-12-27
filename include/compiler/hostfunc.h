// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/compiler/hostfunc.h - host function interface ----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the interface of host function class.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace SSVM {
namespace Compiler {

class Library;

class HostFunction {
public:
  HostFunction(Library &Lib) : Lib(Lib) {}

  virtual ~HostFunction() = default;

  virtual void *getFunction() = 0;

protected:
  template <typename T> struct Helper;
  template <typename R, typename C, class... A> struct Helper<R (C::*)(A...)> {
    static R proxy(HostFunction *This, A... Args) {
      return static_cast<C *>(This)->run(Args...);
    }
  };

  template <typename T> void *proxy() {
    return reinterpret_cast<void *>(&Helper<decltype(&T::run)>::proxy);
  }

  Library &Lib;
};

} // namespace Compiler
} // namespace SSVM
#include "library.h"
