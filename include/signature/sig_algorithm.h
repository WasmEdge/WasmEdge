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
#include "common/errinfo.h"
#include "common/types.h"
#include "ed25519.h"
#include "loader/filemgr.h"
#include "spdlog/spdlog.h"
#include <fstream>

namespace WasmEdge {
namespace Signature {
class SigAlgorithm {
private:
public:
  SigAlgorithm() = default;
  ~SigAlgorithm() = default;
  Expect<const std::vector<Byte>> keygen(Span<const Byte>,
                                         const std::filesystem::path &);
  int verify(Span<const Byte> Code, Span<const Byte> Signature,
             Span<const Byte> PublicKey);
};

} // namespace Signature
} // namespace WasmEdge
