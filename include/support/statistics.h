// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "support/statistics_result.h"

#include <iostream>
#include <string>
#include <vector>

namespace SSVM {
namespace Support {

class Statistics {
public:
  void appendResult(std::unique_ptr<Result> &&result);
  void show();
private:
  std::vector<std::unique_ptr<Result>> results;
};

extern Statistics statistics;

} // namespace Support
} // namespace SSVM
