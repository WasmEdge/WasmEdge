// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/module.h"
#include "common/configure.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "driver/tool.h"
#include "loader/loader.h"

#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <string_view>

using namespace std::literals;

namespace WasmEdge {
namespace Driver {

namespace {

struct NameSection {
  std::map<uint32_t, std::string> FuncNames;
  std::map<uint32_t, std::string> GlobalNames;
};

uint32_t decodeLEB128U32(Span<const Byte> Data, uint32_t &Offset) {
  uint32_t Result = 0;
  uint32_t Shift = 0;
  while (Offset < Data.size()) {
    uint8_t B = static_cast<uint8_t>(Data[Offset++]);
    Result |= static_cast<uint32_t>(B & 0x7F) << Shift;
    if ((B & 0x80) == 0)
      break;
    Shift += 7;
  }
  return Result;
}

std::string decodeName(Span<const Byte> Data, uint32_t &Offset) {
  uint32_t Len = decodeLEB128U32(Data, Offset);
  if (Offset + Len > Data.size())
    return {};
  std::string Name(reinterpret_cast<const char *>(&Data[Offset]), Len);
  Offset += Len;
  return Name;
}

std::map<uint32_t, std::string> decodeNameMap(Span<const Byte> Data,
                                              uint32_t &Offset) {
  std::map<uint32_t, std::string> Map;
  uint32_t Count = decodeLEB128U32(Data, Offset);
  for (uint32_t I = 0; I < Count && Offset < Data.size(); I++) {
    uint32_t Idx = decodeLEB128U32(Data, Offset);
    std::string Name = decodeName(Data, Offset);
    Map[Idx] = std::move(Name);
  }
  return Map;
}

NameSection parseNameSection(Span<const Byte> Data) {
  NameSection NS;
  uint32_t Offset = 0;
  while (Offset < Data.size()) {
    uint8_t SubId = static_cast<uint8_t>(Data[Offset++]);
    uint32_t SubSize = decodeLEB128U32(Data, Offset);
    uint32_t SubEnd = Offset + SubSize;
    if (SubEnd > Data.size())
      break;
    if (SubId == 1) {
      NS.FuncNames = decodeNameMap(Data, Offset);
    } else if (SubId == 7) {
      NS.GlobalNames = decodeNameMap(Data, Offset);
    }
    Offset = SubEnd;
  }
  return NS;
}

std::string getInitExprStr(const AST::Expression &Expr) {
  const auto Instrs = Expr.getInstrs();
  if (Instrs.empty())
    return {};
  const auto &Instr = Instrs[0];
  switch (Instr.getOpCode()) {
  case OpCode::I32__const:
    return fmt::format("i32={}", Instr.getNum().get<int32_t>());
  case OpCode::I64__const:
    return fmt::format("i64={}", Instr.getNum().get<int64_t>());
  case OpCode::F32__const:
    return fmt::format("f32={}", Instr.getNum().get<float>());
  case OpCode::F64__const:
    return fmt::format("f64={}", Instr.getNum().get<double>());
  case OpCode::Global__get:
    return fmt::format("global.get {}", Instr.getTargetIndex());
  case OpCode::Ref__func:
    return fmt::format("ref.func {}", Instr.getTargetIndex());
  case OpCode::Ref__null:
    return fmt::format("ref.null {}", Instr.getValType());
  default:
    return {};
  }
}

// Renders a single element segment entry, i.e. one of the init expressions
// in an element segment's `getInitExprs()` list.
std::string getElemEntryStr(const AST::Expression &Expr) {
  const auto Instrs = Expr.getInstrs();
  if (Instrs.empty())
    return "ref.null";
  const auto &Instr = Instrs[0];
  switch (Instr.getOpCode()) {
  case OpCode::Ref__func:
    return fmt::format("func[{}]", Instr.getTargetIndex());
  case OpCode::Ref__null:
    return "ref.null";
  default:
    return getInitExprStr(Expr);
  }
}

// Number of bytes that the unsigned LEB128 encoding of V occupies.
static uint32_t uleb128Width(uint64_t V) noexcept {
  if (V == 0)
    return 1;
  uint32_t W = 0;
  while (V > 0) {
    V >>= 7;
    ++W;
  }
  return W;
}

// Print one summary row. start/end are byte offsets into the file.
static void printSummaryRow(std::string_view Name, uint64_t Start, uint64_t End,
                            uint64_t Count, std::string_view Extra) noexcept {
  using namespace std::literals;
  if (Extra.empty()) {
    fmt::print("  {:>10}   start=0x{:08x}   end=0x{:08x}   "
               "(size=0x{:08x})   count={}\n"sv,
               Name, Start, End, End - Start, Count);
  } else {
    fmt::print("  {:>10}   start=0x{:08x}   end=0x{:08x}   "
               "(size=0x{:08x})   {}\n"sv,
               Name, Start, End, End - Start, Extra);
  }
}

static void PrintSummary(const AST::Module &Mod,
                         const std::filesystem::path &InputPath) noexcept {
  using namespace std::literals;

  // Header line.
  std::error_code EC;
  const auto FileSize = std::filesystem::file_size(InputPath, EC);
  if (!EC) {
    fmt::print("{}:  file format wasm 0x1  ({} bytes)\n\n"sv,
               InputPath.filename().string(), FileSize);
  } else {
    fmt::print("{}:  file format wasm 0x1\n\n"sv,
               InputPath.filename().string());
  }
  fmt::print("Section Summary:\n\n"sv);

  // Helper: given a Section base, compute content start and end offsets.
  // StartOffset is the position of the LEB128-encoded content-size field;
  // content begins immediately after that field.
  auto bounds = [](const AST::Section &Sec,
                   uint64_t &Start,
                   uint64_t &End) noexcept {
    const uint64_t SzFieldWidth = uleb128Width(Sec.getContentSize());
    Start = Sec.getStartOffset() + SzFieldWidth;
    End = Start + Sec.getContentSize();
  };

  uint64_t S = 0, E = 0;

  const auto &Types = Mod.getTypeSection().getContent();
  if (!Types.empty()) {
    bounds(Mod.getTypeSection(), S, E);
    printSummaryRow("Type"sv, S, E, Types.size(), ""sv);
  }

  const auto &Imports = Mod.getImportSection().getContent();
  if (!Imports.empty()) {
    bounds(Mod.getImportSection(), S, E);
    printSummaryRow("Import"sv, S, E, Imports.size(), ""sv);
  }

  const auto &Funcs = Mod.getFunctionSection().getContent();
  if (!Funcs.empty()) {
    bounds(Mod.getFunctionSection(), S, E);
    printSummaryRow("Function"sv, S, E, Funcs.size(), ""sv);
  }

  const auto &Tables = Mod.getTableSection().getContent();
  if (!Tables.empty()) {
    bounds(Mod.getTableSection(), S, E);
    printSummaryRow("Table"sv, S, E, Tables.size(), ""sv);
  }

  const auto &Mems = Mod.getMemorySection().getContent();
  if (!Mems.empty()) {
    bounds(Mod.getMemorySection(), S, E);
    printSummaryRow("Memory"sv, S, E, Mems.size(), ""sv);
  }

  const auto &Tags = Mod.getTagSection().getContent();
  if (!Tags.empty()) {
    bounds(Mod.getTagSection(), S, E);
    printSummaryRow("Tag"sv, S, E, Tags.size(), ""sv);
  }

  const auto &Globals = Mod.getGlobalSection().getContent();
  if (!Globals.empty()) {
    bounds(Mod.getGlobalSection(), S, E);
    printSummaryRow("Global"sv, S, E, Globals.size(), ""sv);
  }

  const auto &Exports = Mod.getExportSection().getContent();
  if (!Exports.empty()) {
    bounds(Mod.getExportSection(), S, E);
    printSummaryRow("Export"sv, S, E, Exports.size(), ""sv);
  }

  if (Mod.getStartSection().getContent().has_value()) {
    bounds(Mod.getStartSection(), S, E);
    printSummaryRow("Start"sv, S, E, 1, ""sv);
  }

  const auto &Elems = Mod.getElementSection().getContent();
  if (!Elems.empty()) {
    bounds(Mod.getElementSection(), S, E);
    printSummaryRow("Element"sv, S, E, Elems.size(), ""sv);
  }

  if (auto DC = Mod.getDataCountSection().getContent()) {
    bounds(Mod.getDataCountSection(), S, E);
    printSummaryRow("DataCount"sv, S, E, *DC, ""sv);
  }

  const auto &Codes = Mod.getCodeSection().getContent();
  if (!Codes.empty()) {
    bounds(Mod.getCodeSection(), S, E);
    printSummaryRow("Code"sv, S, E, Codes.size(), ""sv);
  }

  const auto &Datas = Mod.getDataSection().getContent();
  if (!Datas.empty()) {
    bounds(Mod.getDataSection(), S, E);
    printSummaryRow("Data"sv, S, E, Datas.size(), ""sv);
  }

  for (const auto &Custom : Mod.getCustomSections()) {
    bounds(Custom, S, E);
    printSummaryRow("Custom"sv, S, E, 0,
                    fmt::format("name=\"{}\""sv, Custom.getName()));
  }
}

} // namespace

int ParseTool(struct DriverToolOptions &Opt) noexcept {
  std::ios::sync_with_stdio(false);

  Configure Conf = createConfigure(Opt);
  const auto InputPath =
      std::filesystem::absolute(std::filesystem::u8path(Opt.SoName.value()));

  Loader::Loader Loader(Conf);
  auto Res = Loader.parseModule(InputPath);
  if (!Res) {
    spdlog::error("Failed to parse WASM module."sv);
    return EXIT_FAILURE;
  }
  const auto &Mod = **Res;

  if (Opt.ParseSummary.value()) {
    PrintSummary(Mod, InputPath);
    return EXIT_SUCCESS;
  }

  const auto &Customs = Mod.getCustomSections();
  NameSection NS;
  for (const auto &Custom : Customs) {
    if (Custom.getName() == "name") {
      NS = parseNameSection(Custom.getContent());
      break;
    }
  }

  const auto &Imports = Mod.getImportSection().getContent();
  uint32_t ImportedFuncCount = 0;
  uint32_t ImportedGlobalCount = 0;
  uint32_t ImportedTableCount = 0;
  uint32_t ImportedMemCount = 0;
  uint32_t ImportedTagCount = 0;
  for (const auto &Imp : Imports) {
    if (Imp.getExternalType() == ExternalType::Function)
      ImportedFuncCount++;
    else if (Imp.getExternalType() == ExternalType::Global)
      ImportedGlobalCount++;
    else if (Imp.getExternalType() == ExternalType::Table)
      ImportedTableCount++;
    else if (Imp.getExternalType() == ExternalType::Memory)
      ImportedMemCount++;
    else if (Imp.getExternalType() == ExternalType::Tag)
      ImportedTagCount++;
  }

  auto FuncName = [&](uint32_t Idx) -> std::string {
    auto It = NS.FuncNames.find(Idx);
    if (It != NS.FuncNames.end())
      return fmt::format(" <{}>", It->second);
    return {};
  };

  auto GlobalName = [&](uint32_t Idx) -> std::string {
    auto It = NS.GlobalNames.find(Idx);
    if (It != NS.GlobalNames.end())
      return fmt::format(" <{}>", It->second);
    return {};
  };

  fmt::print("{}:  file format wasm 0x1\n\n", InputPath.filename().string());
  fmt::print("Section Details:\n\n");

  // Type Section
  const auto &Types = Mod.getTypeSection().getContent();
  if (!Types.empty()) {
    fmt::print("Type[{}]:\n", Types.size());
    for (uint32_t I = 0; I < Types.size(); I++) {
      const auto &FuncType = Types[I].getCompositeType().getFuncType();
      fmt::print(" - type[{}] (", I);
      const auto &Params = FuncType.getParamTypes();
      for (uint32_t J = 0; J < Params.size(); J++) {
        if (J > 0)
          fmt::print(", ");
        fmt::print("{}", Params[J]);
      }
      fmt::print(") -> ");
      const auto &Returns = FuncType.getReturnTypes();
      if (Returns.empty()) {
        fmt::print("nil");
      } else {
        for (uint32_t J = 0; J < Returns.size(); J++) {
          if (J > 0)
            fmt::print(", ");
          fmt::print("{}", Returns[J]);
        }
      }
      fmt::print("\n");
    }
  }

  // Import Section
  if (!Imports.empty()) {
    fmt::print("Import[{}]:\n", Imports.size());
    uint32_t ImpFuncIdx = 0;
    uint32_t ImpGlobalIdx = 0;
    uint32_t ImpMemIdx = 0;
    uint32_t ImpTableIdx = 0;
    uint32_t ImpTagIdx = 0;
    for (const auto &Imp : Imports) {
      switch (Imp.getExternalType()) {
      case ExternalType::Function:
        fmt::print(" - func[{}] sig={}{} <- {}.{}\n", ImpFuncIdx,
                   Imp.getExternalFuncTypeIdx(), FuncName(ImpFuncIdx),
                   Imp.getModuleName(), Imp.getExternalName());
        ImpFuncIdx++;
        break;
      case ExternalType::Memory: {
        const auto &Limit = Imp.getExternalMemoryType().getLimit();
        fmt::print(" - memory[{}] pages: initial={}", ImpMemIdx,
                   Limit.getMin());
        if (Limit.hasMax())
          fmt::print(" max={}", Limit.getMax());
        fmt::print(" <- {}.{}\n", Imp.getModuleName(), Imp.getExternalName());
        ImpMemIdx++;
        break;
      }
      case ExternalType::Table:
        fmt::print(" - table[{}] <- {}.{}\n", ImpTableIdx, Imp.getModuleName(),
                   Imp.getExternalName());
        ImpTableIdx++;
        break;
      case ExternalType::Global: {
        const auto &GT = Imp.getExternalGlobalType();
        fmt::print(" - global[{}] {} mutable={}{} <- {}.{}\n", ImpGlobalIdx,
                   GT.getValType(), GT.getValMut() == ValMut::Var ? 1 : 0,
                   GlobalName(ImpGlobalIdx), Imp.getModuleName(),
                   Imp.getExternalName());
        ImpGlobalIdx++;
        break;
      }
      case ExternalType::Tag:
        fmt::print(" - tag[{}] sig={} <- {}.{}\n", ImpTagIdx,
                   Imp.getExternalTagType().getTypeIdx(), Imp.getModuleName(),
                   Imp.getExternalName());
        ImpTagIdx++;
        break;
      default:
        break;
      }
    }
  }

  // Function Section
  const auto &Funcs = Mod.getFunctionSection().getContent();
  if (!Funcs.empty()) {
    fmt::print("Function[{}]:\n", Funcs.size());
    for (uint32_t I = 0; I < Funcs.size(); I++) {
      uint32_t Idx = I + ImportedFuncCount;
      fmt::print(" - func[{}] sig={}{}\n", Idx, Funcs[I], FuncName(Idx));
    }
  }

  // Table Section
  const auto &Tables = Mod.getTableSection().getContent();
  if (!Tables.empty()) {
    fmt::print("Table[{}]:\n", Tables.size());
    for (uint32_t I = 0; I < Tables.size(); I++) {
      uint32_t Idx = I + ImportedTableCount;
      const auto &TT = Tables[I].getTableType();
      const auto &Lim = TT.getLimit();
      fmt::print(" - table[{}] type={} initial={}", Idx, TT.getRefType(),
                 Lim.getMin());
      if (Lim.hasMax())
        fmt::print(" max={}", Lim.getMax());
      fmt::print("\n");
    }
  }

  // Memory Section
  const auto &Mems = Mod.getMemorySection().getContent();
  if (!Mems.empty()) {
    fmt::print("Memory[{}]:\n", Mems.size());
    for (uint32_t I = 0; I < Mems.size(); I++) {
      uint32_t Idx = I + ImportedMemCount;
      const auto &Lim = Mems[I].getLimit();
      fmt::print(" - memory[{}] pages: initial={}", Idx, Lim.getMin());
      if (Lim.hasMax())
        fmt::print(" max={}", Lim.getMax());
      fmt::print("\n");
    }
  }

  // Tag Section
  const auto &Tags = Mod.getTagSection().getContent();
  if (!Tags.empty()) {
    fmt::print("Tag[{}]:\n", Tags.size());
    for (uint32_t I = 0; I < Tags.size(); I++) {
      uint32_t Idx = I + ImportedTagCount;
      fmt::print(" - tag[{}] sig={}\n", Idx, Tags[I].getTypeIdx());
    }
  }

  // Global Section
  const auto &Globals = Mod.getGlobalSection().getContent();
  if (!Globals.empty()) {
    fmt::print("Global[{}]:\n", Globals.size());
    for (uint32_t I = 0; I < Globals.size(); I++) {
      uint32_t Idx = I + ImportedGlobalCount;
      const auto &GT = Globals[I].getGlobalType();
      std::string InitStr = getInitExprStr(Globals[I].getExpr());
      fmt::print(" - global[{}] {} mutable={}{}", Idx, GT.getValType(),
                 GT.getValMut() == ValMut::Var ? 1 : 0, GlobalName(Idx));
      if (!InitStr.empty())
        fmt::print(" - init {}", InitStr);
      fmt::print("\n");
    }
  }

  // Export Section
  const auto &Exports = Mod.getExportSection().getContent();
  if (!Exports.empty()) {
    fmt::print("Export[{}]:\n", Exports.size());
    for (const auto &Exp : Exports) {
      uint32_t Idx = Exp.getExternalIndex();
      switch (Exp.getExternalType()) {
      case ExternalType::Function:
        fmt::print(" - func[{}]{} -> \"{}\"\n", Idx, FuncName(Idx),
                   Exp.getExternalName());
        break;
      case ExternalType::Global:
        fmt::print(" - global[{}] -> \"{}\"\n", Idx, Exp.getExternalName());
        break;
      case ExternalType::Memory:
        fmt::print(" - memory[{}] -> \"{}\"\n", Idx, Exp.getExternalName());
        break;
      case ExternalType::Table:
        fmt::print(" - table[{}] -> \"{}\"\n", Idx, Exp.getExternalName());
        break;
      default:
        break;
      }
    }
  }

  // Start Section
  if (auto StartIdx = Mod.getStartSection().getContent()) {
    fmt::print("Start:\n");
    fmt::print(" - func[{}]{}\n", *StartIdx, FuncName(*StartIdx));
  }

  // Element Section
  const auto &Elems = Mod.getElementSection().getContent();
  if (!Elems.empty()) {
    fmt::print("Element[{}]:\n", Elems.size());
    for (uint32_t I = 0; I < Elems.size(); I++) {
      const auto &Elem = Elems[I];
      const auto &Entries = Elem.getInitExprs();
      switch (Elem.getMode()) {
      case AST::ElementSegment::ElemMode::Active: {
        fmt::print(" - segment[{}] flags=0 table={} type={} count={}", I,
                   Elem.getIdx(), Elem.getRefType(), Entries.size());
        std::string OffsetStr = getInitExprStr(Elem.getExpr());
        if (!OffsetStr.empty())
          fmt::print(" - init {}", OffsetStr);
        fmt::print("\n");
        break;
      }
      case AST::ElementSegment::ElemMode::Passive:
        fmt::print(" - segment[{}] flags=1 passive type={} count={}\n", I,
                   Elem.getRefType(), Entries.size());
        break;
      case AST::ElementSegment::ElemMode::Declarative:
        fmt::print(" - segment[{}] flags=3 declarative type={} count={}\n", I,
                   Elem.getRefType(), Entries.size());
        break;
      }
      for (uint32_t J = 0; J < Entries.size(); J++) {
        fmt::print("  - elem[{}] = {}\n", J, getElemEntryStr(Entries[J]));
      }
    }
  }

  // DataCount Section
  if (auto Count = Mod.getDataCountSection().getContent()) {
    fmt::print("DataCount section: {}\n", *Count);
  }

  // Code Section
  const auto &Codes = Mod.getCodeSection().getContent();
  if (!Codes.empty()) {
    fmt::print("Code[{}]:\n", Codes.size());
    for (uint32_t I = 0; I < Codes.size(); I++) {
      uint32_t Idx = I + ImportedFuncCount;
      fmt::print(" - func[{}] size={}{}\n", Idx, Codes[I].getSegSize(),
                 FuncName(Idx));
    }
  }

  // Data Section
  const auto &Datas = Mod.getDataSection().getContent();
  if (!Datas.empty()) {
    fmt::print("Data[{}]:\n", Datas.size());
    for (uint32_t I = 0; I < Datas.size(); I++) {
      const auto &Data = Datas[I];
      switch (Data.getMode()) {
      case AST::DataSegment::DataMode::Active: {
        fmt::print(" - segment[{}] memory={} size={}", I, Data.getIdx(),
                   Data.getData().size());
        std::string OffsetStr = getInitExprStr(Data.getExpr());
        if (!OffsetStr.empty())
          fmt::print(" - init {}", OffsetStr);
        fmt::print("\n");
        break;
      }
      case AST::DataSegment::DataMode::Passive:
        fmt::print(" - segment[{}] passive size={}\n", I,
                   Data.getData().size());
        break;
      }
    }
  }

  // Custom Sections
  for (const auto &Custom : Customs) {
    fmt::print("Custom:\n");
    fmt::print(" - name: \"{}\"\n", Custom.getName());
    if (Custom.getName() == "name") {
      for (const auto &[Idx, Name] : NS.FuncNames) {
        fmt::print(" - func[{}] <{}>\n", Idx, Name);
      }
      for (const auto &[Idx, Name] : NS.GlobalNames) {
        fmt::print(" - global[{}] <{}>\n", Idx, Name);
      }
    }
  }

  return EXIT_SUCCESS;
}

} // namespace Driver
} // namespace WasmEdge
