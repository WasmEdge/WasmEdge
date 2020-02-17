#include "support/statistics.h"
#include "easyloggingpp/easylogging++.h"

INITIALIZE_EASYLOGGINGPP

namespace SSVM {
namespace Support {

void passEasyloggingppArgs(int Argc, char *Argv[]) {
  START_EASYLOGGINGPP(Argc, Argv);
}

} // namespace Support
} // namespace SSVM