// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "runtime/importobj.h"

namespace SSVM {
namespace Host {

class ONNCModule : public Runtime::ImportObject {
public:
  ONNCModule();
};

} // namespace Host
} // namespace SSVM
