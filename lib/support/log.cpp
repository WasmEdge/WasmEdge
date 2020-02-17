#include "support/log.h"
#include "easyloggingpp/easylogging++.h"

INITIALIZE_EASYLOGGINGPP

namespace SSVM {
namespace Log {

void passEasyloggingppArgs(int Argc, char *Argv[]) {
  START_EASYLOGGINGPP(Argc, Argv);
}

} // namespace Log
} // namespace SSVM