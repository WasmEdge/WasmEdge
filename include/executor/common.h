#pragma once

namespace Executor {

enum class ErrCode : unsigned int {
  Success = 0,
  UndefinedError,
  RuntimeDataMgrInitFailed,
  ExecutionEngineInitFailed,
  ExecutionFailed
};

class Exe;
}