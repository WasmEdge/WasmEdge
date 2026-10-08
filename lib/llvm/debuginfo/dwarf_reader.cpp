// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/dwarf_reader.h"

#include "common/spdlog.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFCompileUnit.h>
#include <llvm/Support/Error.h>

#include <fmt/format.h>

#include <algorithm>
#include <unordered_map>

namespace WasmEdge::LLVM::DebugInfo {

using namespace std::literals;

namespace {

#if LLVM_VERSION_MAJOR < 10
using DataOffset = uint32_t;
#else
using DataOffset = uint64_t;
#endif

llvm::DataExtractor extractor(llvm::ArrayRef<uint8_t> Bytes) noexcept {
#if LLVM_VERSION_MAJOR < 23
  return llvm::DataExtractor(
      llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                      Bytes.size()),
      true, 4);
#else
  return llvm::DataExtractor(Bytes, true);
#endif
}

bool readULEB(const llvm::DataExtractor &Data, DataOffset &Offset,
              uint64_t &Value) noexcept {
  const auto Start = Offset;
  Value = Data.getULEB128(&Offset);
  return Offset != Start;
}

} // namespace

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
  if (*Errors > 0) {
    spdlog::warn("debug info: found {} errors in the DWARF and name "
                 "sections, some debug info can be missing"sv,
                 *Errors);
  }
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
        if (!isTombstoneAddress(*LowPC)) {
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
    const auto Content = Custom.getContent();
    const auto Data = extractor(llvm::ArrayRef<uint8_t>(
        reinterpret_cast<const uint8_t *>(Content.data()), Content.size()));
    DataOffset Cursor = 0;
    while (Data.isValidOffset(Cursor)) {
      const auto Id = Data.getU8(&Cursor);
      uint64_t Size = 0;
      if (!readULEB(Data, Cursor, Size) ||
          !Data.isValidOffsetForDataOfSize(Cursor, Size)) {
        ++*Errors;
        return;
      }
      const auto End = Cursor + static_cast<DataOffset>(Size);
      if (Id != 1) {
        Cursor = End;
        continue;
      }
      uint64_t Count = 0;
      if (!readULEB(Data, Cursor, Count)) {
        ++*Errors;
        return;
      }
      for (uint64_t I = 0; I < Count; ++I) {
        uint64_t Index = 0, Len = 0;
        if (!readULEB(Data, Cursor, Index) || !readULEB(Data, Cursor, Len) ||
            Cursor > End || Len > End - Cursor) {
          ++*Errors;
          return;
        }
        Names.emplace(static_cast<uint32_t>(Index),
                      Data.getData().substr(Cursor, Len).str());
        Cursor += static_cast<DataOffset>(Len);
      }
      Cursor = End;
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
