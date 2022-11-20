#pragma once

#include "ast/module.h"
#include "ast/type.h"

#include <vector>

namespace WasmEdge {
namespace Serialize {

class Serialize {
public:
  // Serialize Types
  std::vector<uint8_t> serializeFunctionType(AST::FunctionType &FuncType);
  std::vector<uint8_t> serializeTableType(AST::TableType &TabType);
  std::vector<uint8_t> serializeMemType(AST::MemoryType &MemType);
  std::vector<uint8_t> serializeGlobType(AST::GlobalType &GlobType);

  // Serialize Description
  std::vector<uint8_t> serializeImportDesc(AST::ImportDesc &ImpDesc);
  std::vector<uint8_t> serializeExportDesc(AST::ExportDesc &ExpDesc);

  // Serialize Sections
  std::vector<uint8_t> serializeCustomSection(AST::CustomSection &CustomSec);
  std::vector<uint8_t> serializeTypeSection(AST::TypeSection &TypeSec);
  std::vector<uint8_t> serializeImportSection(AST::ImportSection &ImportSec);
  std::vector<uint8_t>
  serializeFunctionSection(AST::FunctionSection &FunctionSec);
  std::vector<uint8_t> serializeTableSection(AST::TableSection &TabSec);
  std::vector<uint8_t> serializeMemorySection(AST::MemorySection &MemSec);
  std::vector<uint8_t> serializeExportSection(AST::ExportSection &ExportSec);
  std::vector<uint8_t> serializeStartSection(AST::StartSection &StartSec);
  std::vector<uint8_t>
  serializeDataCountSection(AST::DataCountSection &DataCountSec);
};
} // namespace Serialize
} // namespace WasmEdge