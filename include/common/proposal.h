// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/proposal.h - proposal configuration class -------------===//
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
#include <string_view>
#include <unordered_map>

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

/// Proposal name enumeration string mapping.
extern const std::unordered_map<Proposal, std::string_view> ProposalStr;

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
