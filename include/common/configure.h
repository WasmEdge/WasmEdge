// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/common/configure.h - Configuration class -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the configuration class of proposals, pre-registration
/// host functions, etc.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <bitset>
#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <unordered_map>

namespace WasmEdge {

/// Wasm Proposal enum class.
/// This enum is also duplicated to "include/api/wasmedge.h" and should be the
/// same.
enum class Proposal : uint8_t {
  BulkMemoryOperations = 0,
  ReferenceTypes,
  SIMD,
  TailCall,
  Annotations,
  Memory64,
  Threads,
  ExceptionHandling,
  FunctionReferences,
  Max
};

/// Host Module Registration enum class.
enum class HostRegistration : uint8_t { Wasi = 0, WasmEdge_Process, Max };

/// Proposal name enumeration string mapping.
extern const std::unordered_map<Proposal, std::string_view> ProposalStr;

class Configure {
public:
  Configure() noexcept {
    addProposal(Proposal::BulkMemoryOperations);
    addProposal(Proposal::ReferenceTypes);
  }
  template <typename... ArgsT> Configure(ArgsT... Args) noexcept : Configure() {
    (addSet(Args), ...);
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

  void addHostRegistration(const HostRegistration Host) noexcept {
    Hosts.set(static_cast<uint8_t>(Host));
  }

  void removeHostRegistration(const HostRegistration Host) noexcept {
    Hosts.reset(static_cast<uint8_t>(Host));
  }

  bool hasHostRegistration(const HostRegistration Host) const noexcept {
    return Hosts.test(static_cast<uint8_t>(Host));
  }

  void setMaxMemoryPage(const uint32_t Page) noexcept { MaxMemPage = Page; }

  uint32_t getMaxMemoryPage() const noexcept { return MaxMemPage; }

  void addDataCountSection() noexcept { HasDataCountSection = true; }

  bool hasDataCountSection() const noexcept { return HasDataCountSection; }

private:
  void addSet(const Proposal P) noexcept { addProposal(P); }
  void addSet(const HostRegistration H) noexcept { addHostRegistration(H); }
  std::bitset<static_cast<uint8_t>(Proposal::Max)> Proposals;
  std::bitset<static_cast<uint8_t>(HostRegistration::Max)> Hosts;
  uint32_t MaxMemPage = 65536;

  /// Used in AST::Module only.
  bool HasDataCountSection = false;
};

} // namespace WasmEdge
