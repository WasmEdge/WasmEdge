// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/errinfo.h"

#include "common/errcode.h"
#include "common/hexstr.h"

namespace WasmEdge {
namespace ErrInfo {
std::ostream &operator<<(std::ostream &OS, const struct InfoFile &Rhs) {
  OS << "    File name: " << Rhs.FileName;
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoLoading &Rhs) {
  OS << "    Bytecode offset: " << convertUIntToHexStr(Rhs.Offset);
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoAST &Rhs) {
  OS << "    At AST node: " << ASTNodeAttrStr[Rhs.NodeAttr];
  return OS;
}

std::ostream &operator<<(std::ostream &OS,
                         const struct InfoInstanceBound &Rhs) {
  OS << "    Instance " << ExternalTypeStr[Rhs.Instance]
     << " has limited number " << Rhs.Limited << " , Got: " << Rhs.Number;
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoForbidIndex &Rhs) {
  OS << "    When checking " << IndexCategoryStr[Rhs.Category]
     << " index: " << Rhs.Index << " , Out of boundary: ";
  if (Rhs.Boundary > 0) {
    OS << (Rhs.Boundary - 1);
  } else {
    OS << "empty";
  }
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoExporting &Rhs) {
  OS << "    Duplicated exporting name: \"" << Rhs.ExtName << "\"";
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoLimit &Rhs) {
  OS << "    In Limit type: { min: " << Rhs.LimMin;
  if (Rhs.LimHasMax) {
    OS << " , max: " << Rhs.LimMax;
  }
  OS << " }";
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoRegistering &Rhs) {
  OS << "    Module name: \"" << Rhs.ModName << "\"";
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoLinking &Rhs) {
  OS << "    When linking module: \"" << Rhs.ModName << "\" , "
     << ExternalTypeStr[Rhs.ExtType] << " name: \"" << Rhs.ExtName << "\"";
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoExecuting &Rhs) {
  OS << "    When executing ";
  if (Rhs.ModName != "") {
    OS << "module name: \"" << Rhs.ModName << "\" , ";
  }
  OS << "function name: \"" << Rhs.FuncName << "\"";
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoMismatch &Rhs) {
  OS << "    Mismatched " << MismatchCategoryStr[Rhs.Category] << ". ";
  switch (Rhs.Category) {
  case MismatchCategory::Alignment:
    OS << "Expected: need to <= " << static_cast<uint32_t>(Rhs.ExpAlignment)
       << " , Got: " << (1UL << Rhs.GotAlignment);
    break;
  case MismatchCategory::ValueType:
    OS << "Expected: " << ValTypeStr[Rhs.ExpValType]
       << " , Got: " << ValTypeStr[Rhs.GotValType];
    break;
  case MismatchCategory::ValueTypes:
    OS << "Expected: types{";
    for (uint32_t I = 0; I < Rhs.ExpParams.size(); ++I) {
      OS << ValTypeStr[Rhs.ExpParams[I]];
      if (I < Rhs.ExpParams.size() - 1) {
        OS << " , ";
      }
    }
    OS << "} , Got: types{";
    for (uint32_t I = 0; I < Rhs.GotParams.size(); ++I) {
      OS << ValTypeStr[Rhs.GotParams[I]];
      if (I < Rhs.GotParams.size() - 1) {
        OS << " , ";
      }
    }
    OS << "}";
    break;
  case MismatchCategory::Mutation:
    OS << "Expected: " << ValMutStr[Rhs.ExpValMut]
       << " , Got: " << ValMutStr[Rhs.GotValMut];
    break;
  case MismatchCategory::ExternalType:
    OS << "Expected: " << ExternalTypeStr[Rhs.ExpExtType]
       << " , Got: " << ExternalTypeStr[Rhs.GotExtType];
    break;
  case MismatchCategory::FunctionType:
    OS << "Expected: FuncType {params{";
    for (uint32_t I = 0; I < Rhs.ExpParams.size(); ++I) {
      OS << ValTypeStr[Rhs.ExpParams[I]];
      if (I < Rhs.ExpParams.size() - 1) {
        OS << " , ";
      }
    }
    OS << "} returns{";
    for (uint32_t I = 0; I < Rhs.ExpReturns.size(); ++I) {
      OS << ValTypeStr[Rhs.ExpReturns[I]];
      if (I < Rhs.ExpReturns.size() - 1) {
        OS << " , ";
      }
    }
    OS << "}} , Got: FuncType {params{";
    for (uint32_t I = 0; I < Rhs.GotParams.size(); ++I) {
      OS << ValTypeStr[Rhs.GotParams[I]];
      if (I < Rhs.GotParams.size() - 1) {
        OS << " , ";
      }
    }
    OS << "} returns{";
    for (uint32_t I = 0; I < Rhs.GotReturns.size(); ++I) {
      OS << ValTypeStr[Rhs.GotReturns[I]];
      if (I < Rhs.GotReturns.size() - 1) {
        OS << " , ";
      }
    }
    OS << "}}";
    break;
  case MismatchCategory::Table:
    OS << "Expected: TableType {RefType{"
       << ValTypeStr[static_cast<ValType>(Rhs.ExpRefType)] << "} Limit{"
       << Rhs.ExpLimMin;
    if (Rhs.ExpLimHasMax) {
      OS << " , " << Rhs.ExpLimMax;
    }
    OS << "}} , Got: TableType {RefType{"
       << ValTypeStr[static_cast<ValType>(Rhs.GotRefType)] << "} Limit{"
       << Rhs.GotLimMin;
    if (Rhs.GotLimHasMax) {
      OS << " , " << Rhs.GotLimMax;
    }
    OS << "}}";
    break;
  case MismatchCategory::Memory:
    OS << "Expected: MemoryType {Limit{" << Rhs.ExpLimMin;
    if (Rhs.ExpLimHasMax) {
      OS << " , " << Rhs.ExpLimMax;
    }
    OS << "}} , Got: MemoryType {Limit{" << Rhs.GotLimMin;
    if (Rhs.GotLimHasMax) {
      OS << " , " << Rhs.GotLimMax;
    }
    OS << "}}";
    break;
  case MismatchCategory::Global:
    OS << "Expected: GlobalType {Mutation{" << ValMutStr[Rhs.ExpValMut]
       << "} ValType{" << ValTypeStr[Rhs.ExpValType]
       << "}} , Got: GlobalType {Mutation{" << ValMutStr[Rhs.GotValMut]
       << "} ValType{" << ValTypeStr[Rhs.GotValType] << "}}";
    break;
  case MismatchCategory::Version:
    OS << "Expected: " << Rhs.ExpVersion << " , Got: " << Rhs.GotVersion;
    break;
  default:
    break;
  }
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoInstruction &Rhs) {
  uint16_t Payload = static_cast<uint16_t>(Rhs.Code);
  OS << "    In instruction: " << OpCodeStr[Rhs.Code] << " (";
  if ((Payload >> 8) >= static_cast<uint16_t>(0xFCU)) {
    OS << convertUIntToHexStr(Payload >> 8, 2) << " ";
  }
  OS << convertUIntToHexStr(Payload & 0xFFU, 2)
     << ") , Bytecode offset: " << convertUIntToHexStr(Rhs.Offset);
  if (Rhs.Args.size() > 0) {
    OS << " , Args: [";
    for (uint32_t I = 0; I < Rhs.Args.size(); ++I) {
      switch (Rhs.ArgsTypes[I]) {
      case ValType::I32:
        if (Rhs.IsSigned) {
          OS << Rhs.Args[I].get<int32_t>();
        } else {
          OS << Rhs.Args[I].get<uint32_t>();
        }
        break;
      case ValType::I64:
        if (Rhs.IsSigned) {
          OS << Rhs.Args[I].get<int64_t>();
        } else {
          OS << Rhs.Args[I].get<uint64_t>();
        }
        break;
      case ValType::F32:
        OS << Rhs.Args[I].get<float>();
        break;
      case ValType::F64:
        OS << Rhs.Args[I].get<double>();
        break;
      case ValType::V128: {
        const auto Value = Rhs.Args[I].get<uint64x2_t>();
        OS << std::hex << Value[0] << Value[1];
        break;
      }
      case ValType::FuncRef:
        OS << ValTypeStr[Rhs.ArgsTypes[I]];
        if (isNullRef(Rhs.Args[I])) {
          OS << ":null";
        } else {
          OS << ":" << &Rhs.Args[I].get<uint64_t>();
        }
        break;
      case ValType::ExternRef:
        OS << ValTypeStr[Rhs.ArgsTypes[I]];
        if (isNullRef(Rhs.Args[I])) {
          OS << ":null";
        } else {
          OS << ":" << &Rhs.Args[I].get<uint64_t>();
        }
        break;
      default:
        break;
      }
      if (I < Rhs.Args.size() - 1) {
        OS << " , ";
      }
    }
    OS << "]";
  }
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoBoundary &Rhs) {
  OS << "    Accessing offset from: " << convertUIntToHexStr(Rhs.Offset)
     << " to: "
     << convertUIntToHexStr(Rhs.Offset + Rhs.Size - (Rhs.Size > 0 ? 1 : 0))
     << " , Out of boundary: " << convertUIntToHexStr(Rhs.Limit);
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoProposal &Rhs) {
  if (auto Iter = ProposalStr.find(Rhs.P); Iter != ProposalStr.end()) {
    OS << "    This instruction or syntax requires enabling proposal "
       << Iter->second;
  } else {
    OS << "    Unknown proposal, Code "
       << convertUIntToHexStr(static_cast<uint32_t>(Rhs.P));
  }
  return OS;
}

} // namespace ErrInfo
} // namespace WasmEdge
