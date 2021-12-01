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
#include "common/errinfo.h"
#include "common/log.h"
#include "common/types.h"
#include "loader/loader.h"
#include "sig_algorithm.h"
#include <filesystem>
#include <system_error>

namespace WasmEdge {
namespace Signature {

const std::string_view DEFAULT_CUSOTM_SECTION_NAME = "signature_wasmedge";

class Signature {
public:
  Expect<void> signWasmFile(const std::filesystem::path &Path);
  Expect<bool> verifyWasmFile(const std::filesystem::path &,
                              const std::filesystem::path &);

private:
  Expect<void> sign(std::filesystem::path, const std::vector<uint8_t>);
  Expect<bool> verify(const Span<Byte> CustomSec, std::filesystem::path Path,
                      const std::filesystem::path &PubKeyPath);

  Expect<std::vector<Byte>> keygen(const Span<uint8_t>,
                                   const std::filesystem::path &);
  SigAlgorithm Alg;
};

} // namespace Signature
} // namespace WasmEdge