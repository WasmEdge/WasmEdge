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

#include "common/enum_configure.h"

#include <bitset>
#include <cstdint>
#include <initializer_list>

namespace WasmEdge {

class CompilerConfigure {
public:
  /// AOT compiler optimization level enum class.
  enum class OptimizationLevel : uint8_t {
    /// Disable as many optimizations as possible.
    O0,
    /// Optimize quickly without destroying debuggability.
    O1,
    /// Optimize for fast execution as much as possible without triggering
    /// significant incremental compile time or code size growth.
    O2,
    /// Optimize for fast execution as much as possible.
    O3,
    /// Optimize for small code size as much as possible without triggering
    /// significant incremental compile time or execution time slowdowns.
    Os,
    /// Optimize for small code size as much as possible.
    Oz
  };
  void setOptimizationLevel(OptimizationLevel Level) noexcept {
    OptLevel = Level;
  }

  OptimizationLevel getOptimizationLevel() const noexcept { return OptLevel; }

  void setDumpIR(bool IsDump) noexcept { DumpIR = IsDump; }

  bool isDumpIR() const noexcept { return DumpIR; }

  void setGenericBinary(bool IsGenericBinary) noexcept {
    GenericBinary = IsGenericBinary;
  }

  bool isGenericBinary() const noexcept { return GenericBinary; }

  void setInstructionCounting(bool IsCount) noexcept {
    InstrCounting = IsCount;
  }

  bool isInstructionCounting() const noexcept { return InstrCounting; }

  void setCostMeasuring(bool IsMeasure) noexcept { CostMeasuring = IsMeasure; }

  bool isCostMeasuring() const noexcept { return CostMeasuring; }

  /// AOT compiler output binary format.
  enum class OutputFormat : uint8_t {
    /// Native dynamic library format.
    Native,
    /// Webassembly format with custom section.
    Wasm,
  };
  void setOutputFormat(OutputFormat Format) noexcept { OFormat = Format; }
  OutputFormat getOutputFormat() const noexcept { return OFormat; }

private:
  OptimizationLevel OptLevel = OptimizationLevel::O3;
  OutputFormat OFormat = OutputFormat::Native;
  bool DumpIR = false;
  bool InstrCounting = false;
  bool CostMeasuring = false;
  bool GenericBinary = false;
};

class RuntimeConfigure {
public:
  void setMaxMemoryPage(const uint32_t Page) noexcept { MaxMemPage = Page; }

  uint32_t getMaxMemoryPage() const noexcept { return MaxMemPage; }

private:
  uint32_t MaxMemPage = 65536;
};

class Configure {
public:
  Configure() noexcept {
    addProposal(Proposal::ImportExportMutGlobals);
    addProposal(Proposal::NonTrapFloatToIntConversions);
    addProposal(Proposal::SignExtensionOperators);
    addProposal(Proposal::MultiValue);
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

  const CompilerConfigure &getCompilerConfigure() const noexcept {
    return CompilerConf;
  }
  CompilerConfigure &getCompilerConfigure() noexcept { return CompilerConf; }

  const RuntimeConfigure &getRuntimeConfigure() const noexcept {
    return RuntimeConf;
  }
  RuntimeConfigure &getRuntimeConfigure() noexcept { return RuntimeConf; }

private:
  void addSet(const Proposal P) noexcept { addProposal(P); }
  void addSet(const HostRegistration H) noexcept { addHostRegistration(H); }
  std::bitset<static_cast<uint8_t>(Proposal::Max)> Proposals;
  std::bitset<static_cast<uint8_t>(HostRegistration::Max)> Hosts;

  CompilerConfigure CompilerConf;
  RuntimeConfigure RuntimeConf;
};

} // namespace WasmEdge
