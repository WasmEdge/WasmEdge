// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/dwarf_reader.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFCompileUnit.h>
#if LLVM_VERSION_MAJOR < 21
#include <llvm/DebugInfo/DWARF/DWARFExpression.h>
#else
#include <llvm/DebugInfo/DWARF/LowLevel/DWARFExpression.h>
#endif
#include <llvm/Support/DataExtractor.h>
#include <llvm/Support/Error.h>

#include <fmt/format.h>

#include <algorithm>
#include <unordered_map>

namespace WasmEdge::LLVM::DebugInfo {

namespace {

bool readULEB(Span<const Byte> &Data, uint64_t &Value) noexcept {
  Value = 0;
  for (uint32_t Shift = 0; Shift < 64; Shift += 7) {
    if (Data.empty()) {
      return false;
    }
    const auto B = static_cast<uint8_t>(Data[0]);
    Data = Data.subspan(1);
    Value |= static_cast<uint64_t>(B & 0x7FU) << Shift;
    if ((B & 0x80U) == 0) {
      return true;
    }
  }
  return false;
}

#if LLVM_VERSION_MAJOR < 10
bool readSLEB(Span<const Byte> &Data, int64_t &Value) noexcept {
  uint64_t Result = 0;
  for (uint32_t Shift = 0; Shift < 64; Shift += 7) {
    if (Data.empty()) {
      return false;
    }
    const auto B = static_cast<uint8_t>(Data[0]);
    Data = Data.subspan(1);
    Result |= static_cast<uint64_t>(B & 0x7FU) << Shift;
    if ((B & 0x80U) == 0) {
      if (Shift + 7 < 64 && (B & 0x40U)) {
        Result |= ~UINT64_C(0) << (Shift + 7);
      }
      Value = static_cast<int64_t>(Result);
      return true;
    }
  }
  return false;
}

bool readU32(Span<const Byte> &Data, uint64_t &Value) noexcept {
  if (Data.size() < 4) {
    return false;
  }
  Value = 0;
  for (uint32_t I = 0; I < 4; ++I) {
    Value |= static_cast<uint64_t>(Data[I]) << (I * 8);
  }
  Data = Data.subspan(4);
  return true;
}
#endif

} // namespace

#if LLVM_VERSION_MAJOR < 10
WasmLocation decodeLocation(llvm::ArrayRef<uint8_t> Bytes,
                            llvm::DWARFUnit *U) noexcept {
  namespace dwarf = llvm::dwarf;
  constexpr uint8_t WasmLocationOp = 0xED;
  Span<const Byte> Data(Bytes.data(), Bytes.size());
  WasmLocation L;
  while (!Data.empty()) {
    const auto Op = static_cast<uint8_t>(Data[0]);
    Data = Data.subspan(1);
    uint64_t Kind = 0;
    uint64_t Value = 0;
    int64_t Offset = 0;
    switch (Op) {
    case WasmLocationOp:
      if (L.K != WasmLocation::Kind::None || !readULEB(Data, Kind)) {
        return {};
      }
      if (Kind == 3 ? !readU32(Data, Value) : !readULEB(Data, Value)) {
        return {};
      }
      if (Kind == 0) {
        L = {WasmLocation::Kind::Local, Value, 0};
      } else if (Kind == 1 || Kind == 3) {
        L = {WasmLocation::Kind::Global, Value, 0};
      } else {
        return {};
      }
      break;
    case dwarf::DW_OP_fbreg:
      if (!readSLEB(Data, Offset)) {
        return {};
      }
      L = {WasmLocation::Kind::FrameBaseOffset, 0, Offset};
      break;
    case dwarf::DW_OP_addr:
      if (!readU32(Data, Value)) {
        return {};
      }
      L = {WasmLocation::Kind::Address, Value, 0};
      break;
    case dwarf::DW_OP_addrx:
      if (!U || !readULEB(Data, Value)) {
        return {};
      }
      if (auto A = U->getAddrOffsetSectionItem(static_cast<uint32_t>(Value))) {
        L = {WasmLocation::Kind::Address, A->Address, 0};
      } else {
        return {};
      }
      break;
    case dwarf::DW_OP_stack_value:
      if (L.K == WasmLocation::Kind::FrameBaseOffset ||
          L.K == WasmLocation::Kind::Address) {
        return {};
      }
      break;
    default:
      return {};
    }
  }
  return L;
}
#else
WasmLocation decodeLocation(llvm::ArrayRef<uint8_t> Bytes,
                            llvm::DWARFUnit *U) noexcept {
  namespace dwarf = llvm::dwarf;
#if LLVM_VERSION_MAJOR < 23
  llvm::DataExtractor Data(
      llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                      Bytes.size()),
      true, 4);
#else
  llvm::DataExtractor Data(Bytes, true);
#endif
#if LLVM_VERSION_MAJOR < 11
  llvm::DWARFExpression Expr(Data, 4, 4);
#else
  llvm::DWARFExpression Expr(Data, 4);
#endif
  WasmLocation L;
  for (auto &Op : Expr) {
    if (Op.isError()) {
      return {};
    }
    switch (Op.getCode()) {
    case dwarf::DW_OP_WASM_location:
      if (L.K != WasmLocation::Kind::None) {
        return {};
      }
      if (Op.getRawOperand(0) == 0) {
        L = {WasmLocation::Kind::Local, Op.getRawOperand(1), 0};
      } else if (Op.getRawOperand(0) == 1 || Op.getRawOperand(0) == 3) {
        L = {WasmLocation::Kind::Global, Op.getRawOperand(1), 0};
      } else {
        return {};
      }
      break;
    case dwarf::DW_OP_fbreg:
      L = {WasmLocation::Kind::FrameBaseOffset, 0,
           static_cast<int64_t>(Op.getRawOperand(0))};
      break;
    case dwarf::DW_OP_addr:
      L = {WasmLocation::Kind::Address, Op.getRawOperand(0), 0};
      break;
    case dwarf::DW_OP_addrx:
      if (!U) {
        return {};
      }
      if (auto A = U->getAddrOffsetSectionItem(
              static_cast<uint32_t>(Op.getRawOperand(0)))) {
        L = {WasmLocation::Kind::Address, A->Address, 0};
      } else {
        return {};
      }
      break;
    case dwarf::DW_OP_stack_value:
      if (L.K == WasmLocation::Kind::FrameBaseOffset ||
          L.K == WasmLocation::Kind::Address) {
        return {};
      }
      break;
    default:
      return {};
    }
  }
  return L;
}
#endif

std::unique_ptr<DwarfReader>
DwarfReader::create(const AST::Module &Mod) noexcept {
  std::unique_ptr<DwarfReader> R(new DwarfReader());
  for (const auto &Custom : Mod.getCustomSections()) {
    std::string_view Name = Custom.getName();
    if (Name.substr(0, 7) != ".debug_") {
      continue;
    }
    const auto Content = Custom.getContent();
    R->Sections[llvm::StringRef(Name.data() + 1, Name.size() - 1)] =
        llvm::MemoryBuffer::getMemBuffer(
            llvm::StringRef(reinterpret_cast<const char *>(Content.data()),
                            Content.size()),
            llvm::StringRef(Name.data(), Name.size()), false);
  }
  if (!R->Sections.count("debug_info") || !R->Sections.count("debug_line")) {
    return nullptr;
  }
  auto Errors = R->Errors;
  auto Handler = [Errors](llvm::Error E) {
    ++*Errors;
    llvm::consumeError(std::move(E));
  };
#if LLVM_VERSION_MAJOR < 11
  R->Ctx = llvm::DWARFContext::create(R->Sections, 4, true);
#else
  R->Ctx = llvm::DWARFContext::create(R->Sections, 4, true, Handler, Handler);
#endif
  if (!R->Ctx) {
    return nullptr;
  }
  uint64_t UnitsEnd = 0;
  for (const auto &Unit : R->Ctx->info_section_units()) {
    UnitsEnd = std::max<uint64_t>(UnitsEnd, Unit->getNextUnitOffset());
  }
  if (UnitsEnd != R->Sections["debug_info"]->getBufferSize()) {
    ++*Errors;
  }
  R->CodeOffset = Mod.getCodeSection().getContentOffset();
  R->indexSubprograms(Mod);
  R->parseNames(Mod);
  return R;
}

void DwarfReader::indexSubprograms(const AST::Module &Mod) noexcept {
  std::unordered_map<uint64_t, llvm::DWARFDie> ByLowPC;
  for (const auto &CU : Ctx->compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie Die(CU.get(), &Entry);
      if (Die.getTag() != llvm::dwarf::DW_TAG_subprogram) {
        continue;
      }
      if (auto LowPC =
              llvm::dwarf::toAddress(Die.find(llvm::dwarf::DW_AT_low_pc))) {
        if (*LowPC != 0 && *LowPC != UINT32_C(0xFFFFFFFF)) {
          ByLowPC.emplace(*LowPC, Die);
        }
      }
    }
  }
  const auto Segs = Mod.getCodeSection().getContent();
  Subprograms.resize(Segs.size());
  for (size_t I = 0; I < Segs.size(); ++I) {
    for (uint64_t Off : {Segs[I].getSegOffset(), Segs[I].getBodyOffset()}) {
      if (auto It = ByLowPC.find(toAddress(Off)); It != ByLowPC.end()) {
        Subprograms[I] = It->second;
        break;
      }
    }
  }
}

void DwarfReader::parseNames(const AST::Module &Mod) noexcept {
  for (const auto &Custom : Mod.getCustomSections()) {
    if (Custom.getName() != "name") {
      continue;
    }
    Span<const Byte> Data = Custom.getContent();
    while (!Data.empty()) {
      const auto Id = static_cast<uint8_t>(Data[0]);
      Data = Data.subspan(1);
      uint64_t Size = 0;
      if (!readULEB(Data, Size) || Size > Data.size()) {
        return;
      }
      auto Sub = Data.first(Size);
      Data = Data.subspan(Size);
      if (Id != 1) {
        continue;
      }
      uint64_t Count = 0;
      if (!readULEB(Sub, Count)) {
        return;
      }
      for (uint64_t I = 0; I < Count; ++I) {
        uint64_t Index = 0, Len = 0;
        if (!readULEB(Sub, Index) || !readULEB(Sub, Len) || Len > Sub.size()) {
          return;
        }
        Names.emplace(
            static_cast<uint32_t>(Index),
            std::string(reinterpret_cast<const char *>(Sub.data()), Len));
        Sub = Sub.subspan(Len);
      }
    }
  }
}

llvm::DWARFDie
DwarfReader::getSubprogram(uint32_t DefinedIndex) const noexcept {
  if (DefinedIndex >= Subprograms.size()) {
    return {};
  }
  return Subprograms[DefinedIndex];
}

std::string DwarfReader::getFunctionName(uint32_t FuncIndex) const {
  if (auto It = Names.find(FuncIndex); It != Names.end()) {
    return It->second;
  }
  return fmt::format("func[{}]", FuncIndex);
}

} // namespace WasmEdge::LLVM::DebugInfo
