// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/signature/sig_algorithm.h - Sig_algorithm class definition
//-------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the sig_algorithm class, which
/// contains methods for signature algorithms.
///
//===----------------------------------------------------------------------===//
#pragma once
#include "common/types.h"
#include "ed25519.h"
#include "loader/filemgr.h"

namespace WasmEdge {
namespace Signature {
class SigAlgorithm {
private:
public:
  SigAlgorithm() = default;
  ~SigAlgorithm() = default;
  std::vector<Byte> keygen(Span<const Byte>);
  int verify();
};

} // namespace Signature
} // namespace WasmEdge
