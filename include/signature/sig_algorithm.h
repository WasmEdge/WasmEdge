// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/signature/sig_algorithm.h - Sig_algorithm class definition -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the sig_algorithm class, which contains 
/// methods for signature algorithms.
///
//===----------------------------------------------------------------------===//
#pragma once
#include "ed25519/src/ed25519.h"
#include "loader/filemgr.h"
#include "common/types.h"

namespace WasmEdge {
namespace Signature {
class Sig_algorithm
{
private: 
public:
  std::vector<Byte> sign(std::vector<Byte>);
  int verify();
};

  
} // namespace Signature 
} // namespace WasmEdge
