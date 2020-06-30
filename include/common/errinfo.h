// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/errinfo.h - Error information definition --------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the enumerations of SSVM error infomation.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast.h"
#include "types.h"
#include "value.h"

#include <iomanip>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace SSVM {
namespace ErrInfo {

/// Error info type enumeration class.
enum class InfoType : uint8_t {
  File,        /// Information about file name which loading from
  Loading,     /// Information about bytecode offset
  AST,         /// Information about tracing AST nodes
  Registering, /// Information about instantiating modules
  Linking,     /// Information about linking instances
  Executing,   /// Information about running functions
  Mismatch,    /// Information about comparison of instances when error
  Instruction, /// Information about aborted instructions and parameters
  Boundary     /// Information about forbidden offset accessing
};

/// Instance addressing type enumeration class.
enum class PtrType : uint8_t {
  Index,  /// Index of instances
  Address /// Absolute address
};

static inline std::unordered_map<PtrType, std::string> PtrTypeStr = {
    {PtrType::Index, "index"}, {PtrType::Address, "address"}};

/// Wasm instance category.
enum class InstCategory : uint8_t {
  ExternalType, /// External typing
  FunctionType, /// Function type
  Table,        /// Table instance
  Memory,       /// Memory instance
  Global,       /// Global instance
  Version       /// Versions
};

static inline std::unordered_map<InstCategory, std::string> InstCategoryStr = {
    {InstCategory::ExternalType, "external type"},
    {InstCategory::FunctionType, "function type"},
    {InstCategory::Table, "table"},
    {InstCategory::Memory, "memory"},
    {InstCategory::Global, "global"},
    {InstCategory::Version, "version"}};

/// Information structures.
struct InfoFile {
  InfoFile() = default;
  InfoFile(std::string_view FName) noexcept : FileName(FName) {}

  friend std::ostream &operator<<(std::ostream &OS, const struct InfoFile &Rhs);

  std::string FileName;
};

struct InfoLoading {
  InfoLoading() = default;
  InfoLoading(const uint32_t Off) noexcept : Offset(Off) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoLoading &Rhs);

  uint32_t Offset;
};

struct InfoAST {
  InfoAST() = default;
  InfoAST(const ASTNodeAttr Attr) noexcept : NodeAttr(Attr) {}

  friend std::ostream &operator<<(std::ostream &OS, const struct InfoAST &Rhs);

  ASTNodeAttr NodeAttr;
};

struct InfoRegistering {
  InfoRegistering() = default;
  InfoRegistering(std::string_view Mod) noexcept : ModName(Mod) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoRegistering &Rhs);

  std::string ModName;
};

struct InfoLinking {
  InfoLinking() = default;
  InfoLinking(std::string_view Mod, std::string_view Ext,
              const ExternalType ExtT = ExternalType::Function) noexcept
      : ModName(Mod), ExtName(Ext), ExtType(ExtT) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoLinking &Rhs);

  std::string ModName;
  std::string ExtName;
  ExternalType ExtType;
};

struct InfoExecuting {
  InfoExecuting() = default;
  InfoExecuting(std::string_view Mod, std::string_view Func) noexcept
      : ModName(Mod), FuncName(Func) {}
  InfoExecuting(std::string_view Func) noexcept : ModName(""), FuncName(Func) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoExecuting &Rhs);

  std::string ModName;
  std::string FuncName;
};

struct InfoMismatch {
  InfoMismatch() = default;
  /// Case 1: unexpected external types
  InfoMismatch(const ExternalType ExpExt, const ExternalType GotExt) noexcept
      : Category(InstCategory::ExternalType), ExpExtType(ExpExt),
        GotExtType(GotExt) {}

  /// Case 2: unexpected function types
  InfoMismatch(const std::vector<ValType> &ExpP,
               const std::vector<ValType> &ExpR,
               const std::vector<ValType> &GotP,
               const std::vector<ValType> &GotR) noexcept
      : Category(InstCategory::FunctionType), ExpParams(ExpP), ExpReturns(ExpR),
        GotParams(GotP), GotReturns(GotR) {}

  /// Case 3: unexpected table types
  InfoMismatch(const ElemType ExpEType, /// Element type
               const bool ExpHasMax, const uint32_t ExpMin,
               const uint32_t ExpMax,   /// Expect Limit
               const ElemType GotEType, /// Got element type
               const bool GotHasMax, const uint32_t GotMin,
               const uint32_t GotMax /// Got limit
               ) noexcept
      : Category(InstCategory::Table), ExpElemType(ExpEType),
        ExpLimHasMax(ExpHasMax), ExpLimMin(ExpMin), ExpLimMax(ExpMax),
        GotElemType(GotEType), GotLimHasMax(GotHasMax), GotLimMin(GotMin),
        GotLimMax(GotMax) {}

  /// Case 4: unexpected memory limits
  InfoMismatch(const bool ExpHasMax, const uint32_t ExpMin,
               const uint32_t ExpMax, /// Expect Limit
               const bool GotHasMax, const uint32_t GotMin,
               const uint32_t GotMax /// Got limit
               ) noexcept
      : Category(InstCategory::Memory), ExpLimHasMax(ExpHasMax),
        ExpLimMin(ExpMin), ExpLimMax(ExpMax), GotLimHasMax(GotHasMax),
        GotLimMin(GotMin), GotLimMax(GotMax) {}

  /// Case 5: unexpected global types
  InfoMismatch(const ValType ExpVType, const ValMut ExpVMut,
               const ValType GotVType, const ValMut GotVMut) noexcept
      : Category(InstCategory::Global), ExpValType(ExpVType),
        ExpValMut(ExpVMut), GotValType(GotVType), GotValMut(GotVMut) {}

  /// Case 6: unexpected version
  InfoMismatch(const uint32_t ExpV, const uint32_t GotV) noexcept
      : Category(InstCategory::Version), ExpVersion(ExpV), GotVersion(GotV) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoMismatch &Rhs);

  /// Mismatched category
  InstCategory Category;

  /// Case 1: unexpected external type
  ExternalType ExpExtType, GotExtType;

  /// Case 2: unexpected function type
  std::vector<ValType> ExpParams, GotParams;
  std::vector<ValType> ExpReturns, GotReturns;

  /// Case 3 & 4: unexpected table or memory limit
  ElemType ExpElemType, GotElemType;
  bool ExpLimHasMax, GotLimHasMax;
  uint32_t ExpLimMin, GotLimMin;
  uint32_t ExpLimMax, GotLimMax;

  /// Case 5: unexpected global type
  ValType ExpValType, GotValType;
  ValMut ExpValMut, GotValMut;

  /// Case 6: unexpected version
  uint32_t ExpVersion, GotVersion;
};

struct InfoInstruction {
  InfoInstruction() = default;
  InfoInstruction(const OpCode Op, const uint32_t Off, const uint8_t SOp,
                  const std::vector<ValVariant> &ArgsVec = {},
                  const std::vector<ValType> &ArgsTypesVec = {},
                  const bool Signed = false) noexcept
      : Code(Op), SubOp(SOp), Offset(Off), Args(ArgsVec),
        ArgsTypes(ArgsTypesVec), HasSubOp(true), IsSigned(Signed) {}
  InfoInstruction(const OpCode Op, const uint32_t Off,
                  const std::vector<ValVariant> &ArgsVec = {},
                  const std::vector<ValType> &ArgsTypesVec = {},
                  const bool Signed = false) noexcept
      : Code(Op), SubOp(0), Offset(Off), Args(ArgsVec), ArgsTypes(ArgsTypesVec),
        HasSubOp(false), IsSigned(Signed) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoInstruction &Rhs);

  OpCode Code;
  uint8_t SubOp;
  uint32_t Offset;
  std::vector<ValVariant> Args;
  std::vector<ValType> ArgsTypes;
  bool HasSubOp;
  bool IsSigned;
};

struct InfoBoundary {
  InfoBoundary() = default;
  InfoBoundary(
      const uint64_t Off, const uint32_t Len = 0,
      const uint32_t Lim = std::numeric_limits<uint32_t>::max()) noexcept
      : Offset(Off), Size(Len), Limit(Lim) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoBoundary &Rhs);

  uint64_t Offset;
  uint32_t Size;
  uint32_t Limit;
};

} // namespace ErrInfo
} // namespace SSVM