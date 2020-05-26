#include "support/log.h"
#include "easyloggingpp/easylogging++.h"

INITIALIZE_EASYLOGGINGPP

namespace SSVM {
namespace Log {

void passEasyloggingppArgs(int Argc, char *Argv[]) {
  START_EASYLOGGINGPP(Argc, Argv);
}

void loggingError(ErrCode Code) {
  LOG(ERROR) << WasmPhaseStr[getErrCodePhase(Code)]
             << " failed: " << ErrCodeStr[Code] << ", Code: 0x" << std::hex
             << static_cast<uint32_t>(Code) << std::dec;
}

void setErrorLoggingLevel() {
  el::Loggers::addFlag(el::LoggingFlag::HierarchicalLogging);
  el::Loggers::setLoggingLevel(el::Level::Error);
}

} // namespace Log
} // namespace SSVM