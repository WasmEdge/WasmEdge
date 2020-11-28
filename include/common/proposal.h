// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/proposal.h - proposal consiguration class -------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the configuration class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <bitset>
#include <cstdint>

namespace SSVM {

/// Wasm Proposal enum class.
enum class Proposal : uint8_t {
  Annotations = 0,
  BulkMemoryOperations,
  ExceptionHandling,
  FunctionReferences,
  Memory64,
  ReferenceTypes,
  SIMD,
  TailCall,
  Threads,
  Max,
};

class ProposalConfigure {
public:
  template <typename... ArgsT> ProposalConfigure(ArgsT... Args) noexcept {
    (addProposal(Args), ...);
  }

  void addProposal(const Proposal Type) noexcept {
    Proposals.set(static_cast<uint8_t>(Type));
  }

  void removeProposal(const Proposal Type) noexcept {
    Proposals.reset(static_cast<uint8_t>(Type));
  }

  bool hasProposal(const Proposal Type) const noexcept {
    return Proposals.test(static_cast<uint8_t>(Type));
  }

private:
  std::bitset<static_cast<uint8_t>(Proposal::Max)> Proposals;
};

} // namespace SSVM
