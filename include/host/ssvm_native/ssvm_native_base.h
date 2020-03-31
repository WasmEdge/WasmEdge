// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "runtime/hostfunc.h"

namespace SSVM {
namespace Host {

template <typename T> class SSVMNative : public Runtime::HostFunction<T> {
public:
  SSVMNative() : Runtime::HostFunction<T>(0) {}

protected:
  /// TODO: Put storage here.
};

} // namespace Host
} // namespace SSVM