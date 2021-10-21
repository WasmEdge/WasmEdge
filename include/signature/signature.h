// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/signature/signature.h - signature class definition -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the signature class, which controls
/// flow of WASM signature.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "ast/section.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "sig_algorithm.h"

namespace WasmEdge {
namespace Signature {

class Signature {
public:
  Signature(const Configure &Conf) noexcept : Conf(Conf) {}
  ~Signature() noexcept = default;

  Expect<void> sign(const AST::Module &Mod, Span<Byte>);
  Expect<void> verify(const AST::Module &Mod);
  // Expect<Span<Byte>> keygen(Span<const uint8_t>);
  int keygen(Span<const uint8_t>);

private:
  /// Proposal configure
  const Configure Conf;
  Sig_algorithm alg;
};

} // namespace Signature
} // namespace WasmEdge