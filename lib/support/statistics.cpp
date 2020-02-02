#include "support/statistics.h"
#include "easyloggingpp/easylogging++.h"

namespace SSVM {
namespace Support {
Statistics statistics = Statistics();

void Statistics::appendResult(std::unique_ptr<Result> &&result) {
  results.emplace_back(std::move(result));
}

void Statistics::show() {
  for(auto &result:results)
      result->show();
}

} // namespace Validator
} // namespace Support