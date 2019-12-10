// SPDX-License-Identifier: Apache-2.0
#include "loader/loader.h"

namespace SSVM {
namespace Loader {

/// Set path to Loader class. See "include/loader/loader.h".
ErrCode Loader::setPath(const std::string &FilePath) {
  /// Check is the correct state.
  if (Stat != State::Inited)
    return ErrCode::WrongLoaderFlow;

  /// Set path to file manager and check error.
  ErrCode Result = FSMgr.setPath(FilePath);
  if (Result == ErrCode::Success)
    Stat = State::PathSet_FStream;
  return Result;
}

/// Set byte code to Loader class. See "include/loader/loader.h".
ErrCode Loader::setCode(const std::vector<uint8_t> &Code) {
  /// Check is the correct state.
  if (Stat != State::Inited)
    return ErrCode::WrongLoaderFlow;

  /// Set vector to file manager and check error.
  ErrCode Result = FVMgr.setCode(Code);
  if (Result == ErrCode::Success)
    Stat = State::PathSet_Vector;
  return Result;
}

/// Load and parse module. See "include/loader/loader.h".
ErrCode Loader::parseModule() {
  /// Check is the correct state.
  if (Stat != State::PathSet_FStream && Stat != State::PathSet_Vector)
    return ErrCode::WrongLoaderFlow;

  /// Make module node and parse it.
  Mod = std::make_unique<AST::Module>();
  ErrCode Result = ErrCode::ReadError;
  if (Stat == State::PathSet_FStream)
    Result = Mod->loadBinary(FSMgr);
  else if (Stat == State::PathSet_Vector)
    Result = Mod->loadBinary(FVMgr);

  /// Check load error code.
  if (Result == ErrCode::Success)
    Stat = State::Parsed;
  else
    Mod.reset(nullptr);
  return Result;
}

/// Do the validation checking. See "include/loader/loader.h".
ErrCode Loader::validateModule() {
  /// Check is the correct state.
  if (Stat != State::Parsed)
    return ErrCode::WrongLoaderFlow;

  /// Call Module mode's checkValidation() to do recursive checking. TODO
  ErrCode Result = ErrCode::Success; // Mod->checkValidation();
  if (Result == ErrCode::Success)
    Stat = State::Validated;
  return Result;
}

/// Get Module node. See "include/loader/loader.h".
ErrCode Loader::getModule(std::unique_ptr<AST::Module> &OutModule) {
  /// Check is the correct state.
  if (Stat != State::Validated)
    return ErrCode::WrongLoaderFlow;

  /// Get the module node class.
  OutModule = std::move(Mod);
  Stat = State::Finished;
  return ErrCode::Success;
}

/// Reset Loader. See "include/loader/loader.h".
ErrCode Loader::reset(bool Force) {
  if (!Force && Stat != State::Finished) {
    return ErrCode::WrongLoaderFlow;
  }
  Mod.reset();
  Stat = State::Inited;
  return ErrCode::Success;
}

} // namespace Loader
} // namespace SSVM
