// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "converter.h"
#include "common/errcode.h"

using namespace std::string_view_literals;

namespace WasmEdge::WAT {

void Converter::SymbolTable::clear() {
  Types.clear();
  Funcs.clear();
  Tables.clear();
  Memories.clear();
  Globals.clear();
  Tags.clear();
  Elems.clear();
  Datas.clear();
  Locals.clear();
  Labels.clear();
  NextType = NextFunc = NextTable = NextMemory = 0;
  NextGlobal = NextTag = NextElem = NextData = NextLocal = 0;
}

void Converter::SymbolTable::clearLocals() {
  Locals.clear();
  Labels.clear();
  NextLocal = 0;
}

Expect<uint32_t> Converter::SymbolTable::resolveIdx(
    const std::unordered_map<std::string, uint32_t, Hash::Hash> &Map,
    std::string_view Ref, uint32_t /*Max*/, ErrCode::Value Err) const {
  if (Ref.empty()) {
    return Unexpect(Err);
  }
  if (Ref[0] == '$') {
    auto It = Map.find(std::string(Ref));
    if (It == Map.end()) {
      return Unexpect(ErrCode::Value::WatUnknownId);
    }
    return It->second;
  }
  // Numeric index
  uint32_t Idx = 0;
  for (char C : Ref) {
    if (C == '_') {
      continue;
    }
    if (C < '0' || C > '9') {
      return Unexpect(Err);
    }
    Idx = Idx * 10 + (C - '0');
  }
  return Idx;
}

Expect<uint32_t>
Converter::SymbolTable::resolveType(std::string_view Ref) const {
  return resolveIdx(Types, Ref, NextType, ErrCode::Value::InvalidFuncTypeIdx);
}
Expect<uint32_t>
Converter::SymbolTable::resolveFunc(std::string_view Ref) const {
  return resolveIdx(Funcs, Ref, NextFunc, ErrCode::Value::InvalidFuncIdx);
}
Expect<uint32_t>
Converter::SymbolTable::resolveTable(std::string_view Ref) const {
  return resolveIdx(Tables, Ref, NextTable, ErrCode::Value::InvalidTableIdx);
}
Expect<uint32_t>
Converter::SymbolTable::resolveMemory(std::string_view Ref) const {
  return resolveIdx(Memories, Ref, NextMemory,
                    ErrCode::Value::InvalidMemoryIdx);
}
Expect<uint32_t>
Converter::SymbolTable::resolveGlobal(std::string_view Ref) const {
  return resolveIdx(Globals, Ref, NextGlobal, ErrCode::Value::InvalidGlobalIdx);
}
Expect<uint32_t>
Converter::SymbolTable::resolveTag(std::string_view Ref) const {
  return resolveIdx(Tags, Ref, NextTag, ErrCode::Value::InvalidTagIdx);
}
Expect<uint32_t>
Converter::SymbolTable::resolveElem(std::string_view Ref) const {
  return resolveIdx(Elems, Ref, NextElem, ErrCode::Value::InvalidElemIdx);
}
Expect<uint32_t>
Converter::SymbolTable::resolveData(std::string_view Ref) const {
  return resolveIdx(Datas, Ref, NextData, ErrCode::Value::InvalidDataIdx);
}
Expect<uint32_t>
Converter::SymbolTable::resolveLocal(std::string_view Ref) const {
  return resolveIdx(Locals, Ref, NextLocal, ErrCode::Value::InvalidLocalIdx);
}

Expect<uint32_t>
Converter::SymbolTable::resolveLabel(std::string_view Ref) const {
  if (Ref[0] == '$') {
    std::string Name(Ref);
    for (int I = static_cast<int>(Labels.size()) - 1; I >= 0; --I) {
      if (Labels[I] == Name) {
        return static_cast<uint32_t>(Labels.size() - 1 - I);
      }
    }
    return Unexpect(ErrCode::Value::InvalidLabelIdx);
  }
  // Numeric label
  uint32_t Idx = 0;
  for (char C : Ref) {
    if (C == '_') {
      continue;
    }
    Idx = Idx * 10 + (C - '0');
  }
  return Idx;
}

std::string_view Converter::nodeText(Node N) const { return N.text(Source); }

std::string_view Converter::nodeType(Node N) const { return N.type(); }

Expect<AST::Module> Converter::convert(const Tree &ParseTree,
                                       std::string_view Src) {
  Source = Src;
  Syms.clear();
  PendingExports.clear();

  Node Root = ParseTree.rootNode();
  if (Root.isNull() || Root.hasError()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  // Pass 1: collect all identifiers and assign indices
  EXPECTED_TRY(collectIndices(Root));

  // Pass 2: build AST::Module
  AST::Module Mod;
  EXPECTED_TRY(buildModule(Root, Mod));

  // Emit deferred inline exports
  for (const auto &Exp : PendingExports) {
    auto &Exports = Mod.getExportSection().getContent();
    Exports.emplace_back();
    Exports.back().setExternalName(Exp.Name);
    Exports.back().setExternalType(Exp.Type);
    Exports.back().setExternalIndex(Exp.Index);
  }

  // Set data count section if data segments exist
  auto &DataSec = Mod.getDataSection().getContent();
  if (!DataSec.empty()) {
    Mod.getDataCountSection().setContent(static_cast<uint32_t>(DataSec.size()));
  }

  return Mod;
}

Expect<void> Converter::collectIndices(Node Root) {
  // If root is a module node, iterate its children
  // If root is bare module fields, iterate them directly
  if (nodeType(Root) == "module"sv) {
    for (uint32_t I = 0; I < Root.namedChildCount(); ++I) {
      EXPECTED_TRY(collectModuleField(Root.namedChild(I)));
    }
  } else {
    // root node with bare module fields
    for (uint32_t I = 0; I < Root.namedChildCount(); ++I) {
      Node Child = Root.namedChild(I);
      if (nodeType(Child) == "module"sv) {
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          EXPECTED_TRY(collectModuleField(Child.namedChild(J)));
        }
      } else {
        EXPECTED_TRY(collectModuleField(Child));
      }
    }
  }
  return {};
}

Expect<void> Converter::collectModuleField(Node Field) {
  auto Type = nodeType(Field);

  // Extract identifier from first named child if it's an identifier
  auto GetId = [&](Node N) -> std::string {
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      if (nodeType(N.namedChild(I)) == "identifier"sv) {
        return std::string(nodeText(N.namedChild(I)));
      }
    }
    return {};
  };

  if (Type == "type_definition"sv) {
    std::string Id = GetId(Field);
    if (!Id.empty()) {
      Syms.Types[Id] = Syms.NextType;
    }
    Syms.NextType++;
  } else if (Type == "rec"sv) {
    // Recursive type: each child type_definition gets an index
    for (uint32_t I = 0; I < Field.namedChildCount(); ++I) {
      Node Child = Field.namedChild(I);
      if (nodeType(Child) == "type_definition"sv) {
        std::string Id = GetId(Child);
        if (!Id.empty()) {
          Syms.Types[Id] = Syms.NextType;
        }
        Syms.NextType++;
      }
    }
  } else if (Type == "import"sv) {
    // Imports consume indices in their respective spaces
    for (uint32_t I = 0; I < Field.namedChildCount(); ++I) {
      Node Child = Field.namedChild(I);
      auto ChildType = nodeType(Child);
      if (ChildType == "import_func"sv) {
        std::string Id = GetId(Child);
        if (!Id.empty()) {
          Syms.Funcs[Id] = Syms.NextFunc;
        }
        Syms.NextFunc++;
      } else if (ChildType == "import_table"sv) {
        std::string Id = GetId(Child);
        if (!Id.empty()) {
          Syms.Tables[Id] = Syms.NextTable;
        }
        Syms.NextTable++;
      } else if (ChildType == "import_memory"sv) {
        std::string Id = GetId(Child);
        if (!Id.empty()) {
          Syms.Memories[Id] = Syms.NextMemory;
        }
        Syms.NextMemory++;
      } else if (ChildType == "import_global"sv) {
        std::string Id = GetId(Child);
        if (!Id.empty()) {
          Syms.Globals[Id] = Syms.NextGlobal;
        }
        Syms.NextGlobal++;
      } else if (ChildType == "import_tag"sv) {
        std::string Id = GetId(Child);
        if (!Id.empty()) {
          Syms.Tags[Id] = Syms.NextTag;
        }
        Syms.NextTag++;
      }
    }
  } else if (Type == "func"sv) {
    std::string Id = GetId(Field);
    if (!Id.empty()) {
      Syms.Funcs[Id] = Syms.NextFunc;
    }
    Syms.NextFunc++;
  } else if (Type == "table"sv) {
    std::string Id = GetId(Field);
    if (!Id.empty()) {
      Syms.Tables[Id] = Syms.NextTable;
    }
    Syms.NextTable++;
  } else if (Type == "memory"sv) {
    std::string Id = GetId(Field);
    if (!Id.empty()) {
      Syms.Memories[Id] = Syms.NextMemory;
    }
    Syms.NextMemory++;
  } else if (Type == "global"sv) {
    std::string Id = GetId(Field);
    if (!Id.empty()) {
      Syms.Globals[Id] = Syms.NextGlobal;
    }
    Syms.NextGlobal++;
  } else if (Type == "tag"sv) {
    std::string Id = GetId(Field);
    if (!Id.empty()) {
      Syms.Tags[Id] = Syms.NextTag;
    }
    Syms.NextTag++;
  } else if (Type == "elem"sv) {
    std::string Id = GetId(Field);
    if (!Id.empty()) {
      Syms.Elems[Id] = Syms.NextElem;
    }
    Syms.NextElem++;
  } else if (Type == "data"sv) {
    std::string Id = GetId(Field);
    if (!Id.empty()) {
      Syms.Datas[Id] = Syms.NextData;
    }
    Syms.NextData++;
  }
  return {};
}

Expect<void> Converter::buildModule(Node Root, AST::Module &Mod) {
  auto Visit = [&](Node Field) -> Expect<void> {
    return buildModuleField(Field, Mod);
  };

  if (nodeType(Root) == "module"sv) {
    for (uint32_t I = 0; I < Root.namedChildCount(); ++I) {
      EXPECTED_TRY(Visit(Root.namedChild(I)));
    }
  } else {
    for (uint32_t I = 0; I < Root.namedChildCount(); ++I) {
      Node Child = Root.namedChild(I);
      if (nodeType(Child) == "module"sv) {
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          EXPECTED_TRY(Visit(Child.namedChild(J)));
        }
      } else {
        EXPECTED_TRY(Visit(Child));
      }
    }
  }
  return {};
}

Expect<void> Converter::buildModuleField(Node Field, AST::Module &Mod) {
  auto Type = nodeType(Field);
  if (Type == "type_definition"sv) {
    return convertTypeDefinition(Field, Mod);
  } else if (Type == "rec"sv) {
    return convertRecType(Field, Mod);
  } else if (Type == "import"sv) {
    return convertImport(Field, Mod);
  } else if (Type == "export"sv) {
    return convertExport(Field, Mod);
  } else if (Type == "func"sv) {
    return convertFunc(Field, Mod);
  } else if (Type == "table"sv) {
    return convertTable(Field, Mod);
  } else if (Type == "memory"sv) {
    return convertMemory(Field, Mod);
  } else if (Type == "global"sv) {
    return convertGlobal(Field, Mod);
  } else if (Type == "start"sv) {
    return convertStart(Field, Mod);
  } else if (Type == "elem"sv) {
    return convertElem(Field, Mod);
  } else if (Type == "data"sv) {
    return convertData(Field, Mod);
  } else if (Type == "tag"sv) {
    return convertTag(Field, Mod);
  }
  // Ignore unknown fields (annotations, comments)
  return {};
}

} // namespace WasmEdge::WAT
