#include "common/errinfo.h"

#include "support/hexstr.h"
#include "support/log.h"

#include "easyloggingpp/easylogging++.h"

INITIALIZE_EASYLOGGINGPP

namespace SSVM {
namespace Log {

void passEasyloggingppArgs(int Argc, char *Argv[]) {
  START_EASYLOGGINGPP(Argc, Argv);
}

void setErrorLoggingLevel() {
  el::Loggers::addFlag(el::LoggingFlag::HierarchicalLogging);
  el::Loggers::setLoggingLevel(el::Level::Error);
}

} // namespace Log

namespace ErrInfo {
std::ostream &operator<<(std::ostream &OS, const struct InfoFile &Rhs) {
  if (Rhs.FileName != "") {
    OS << "    File name: " << Rhs.FileName;
  }
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoLoading &Rhs) {
  OS << "    Bytecode offset: " << Support::convertUIntToHexStr(Rhs.Offset);
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoAST &Rhs) {
  OS << "    At AST node: " << ASTNodeAttrStr[Rhs.NodeAttr];
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
  OS << "    Mismatched " << InstCategoryStr[Rhs.Category] << ". ";
  switch (Rhs.Category) {
  case InstCategory::ExternalType:
    OS << "Expected: " << ExternalTypeStr[Rhs.ExpExtType]
       << " , Got: " << ExternalTypeStr[Rhs.GotExtType];
    break;
  case InstCategory::FunctionType:
    OS << "Expected: params{";
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
    OS << "} , Got: params{";
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
    OS << "}";
    break;
  case InstCategory::Table:
  case InstCategory::Memory:
    OS << "Expected: limit {" << Rhs.ExpLimMin;
    if (Rhs.ExpLimHasMax) {
      OS << " , " << Rhs.ExpLimMax;
    }
    OS << "} , Got: limit {" << Rhs.GotLimMin;
    if (Rhs.GotLimHasMax) {
      OS << " , " << Rhs.GotLimMax;
    }
    OS << "}";
    break;
  case InstCategory::Global:
    OS << "Expected: global type {" << ValMutStr[Rhs.ExpValMut] << " "
       << ValTypeStr[Rhs.ExpValType] << "} , Got: global type {"
       << ValMutStr[Rhs.GotValMut] << " " << ValTypeStr[Rhs.GotValType] << "}";
    break;
  case InstCategory::Version:
    OS << "Expected: " << Rhs.ExpVersion << " , Got: " << Rhs.GotVersion;
    break;
  default:
    break;
  }
  return OS;
}

std::ostream &operator<<(std::ostream &OS, const struct InfoInstruction &Rhs) {
  OS << "    In instruction: " << OpCodeStr[Rhs.Code] << " ("
     << Support::convertUIntToHexStr(uint32_t(Rhs.Code), 2)
     << ") , Bytecode offset: " << Support::convertUIntToHexStr(Rhs.Offset);
  if (Rhs.Args.size() > 0) {
    OS << " , Args: [";
    for (uint32_t I = 0; I < Rhs.Args.size(); ++I) {
      switch (Rhs.ArgsTypes[I]) {
      case ValType::I32:
        if (Rhs.IsSigned) {
          OS << retrieveValue<int32_t>(Rhs.Args[I]);
        } else {
          OS << retrieveValue<uint32_t>(Rhs.Args[I]);
        }
        break;
      case ValType::I64:
        if (Rhs.IsSigned) {
          OS << retrieveValue<int64_t>(Rhs.Args[I]);
        } else {
          OS << retrieveValue<uint64_t>(Rhs.Args[I]);
        }
        break;
      case ValType::F32:
        OS << retrieveValue<float>(Rhs.Args[I]);
        break;
      case ValType::F64:
        OS << retrieveValue<double>(Rhs.Args[I]);
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
  OS << "    Accessing offset from: "
     << Support::convertUIntToHexStr(Rhs.Offset) << " to: "
     << Support::convertUIntToHexStr(Rhs.Offset + Rhs.Size -
                                     (Rhs.Size > 0 ? 1 : 0))
     << " , Out of boundary: " << Support::convertUIntToHexStr(Rhs.Limit);
  return OS;
}

} // namespace ErrInfo
} // namespace SSVM