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

namespace fs = std::filesystem;
namespace WasmEdge {
namespace Signature {

const std::string_view DEFAULT_CUSOTM_SECTION_NAME = "signature_wasmedge";

class Signature {
public:
  Expect<void> signWasmFile(const fs::path &, const fs::path &,
                            const fs::path &, const fs::path &);
  Expect<bool> verifyWasmFile(const fs::path &, const fs::path &);

private:
  Expect<void> sign(fs::path, fs::path, const std::vector<uint8_t>);
  Expect<bool> verify(const Span<Byte>, const Span<Byte>,
                      const fs::path &PubKeyPath);
  Expect<Span<Byte>> readBytes(const fs::path &);

  Expect<std::vector<Byte>> keygen(const Span<uint8_t>, const fs::path &);
  SigAlgorithm Alg;
};

} // namespace Signature
} // namespace WasmEdge