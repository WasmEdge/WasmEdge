#include "support/statistics.h"
#include "easyloggingpp/easylogging++.h"

INITIALIZE_EASYLOGGINGPP

namespace SSVM {
namespace Support {
Statistics statistics = Statistics();

void Statistics::appendResult(std::unique_ptr<Result> &&Res) {
  Results.emplace_back(std::move(Res));
}

void Statistics::show() {
  for (auto &result : Results)
    result->show();
}

void Statistics::reset() {
  Results.clear();
}

void passEasyloggingppArgs(int Argc, char *Argv[])
{
  START_EASYLOGGINGPP(Argc, Argv);
}

} // namespace Support
} // namespace SSVM