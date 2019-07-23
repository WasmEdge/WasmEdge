#include "loader/loadmgr.h"

namespace Loader {

/// Set path to LoadMgr class. See "include/loader/loadmgr.h".
ErrCode LoadMgr::setPath(const std::string &FilePath) {
  /// Check is the correct state.
  if (Stat != State::Inited)
    return ErrCode::WrongLoaderFlow;

  /// Set path to file manager and check error.
  ErrCode Result = FMgr.setPath(FilePath);
  if (Result == ErrCode::Success)
    Stat = State::PathSet;
  return Result;
}

/// Load and parse module. See "include/loader/loadmgr.h".
ErrCode LoadMgr::parseModule() {
  /// Check is the correct state.
  if (Stat != State::PathSet)
    return ErrCode::WrongLoaderFlow;

  /// Make module node and parse it.
  Mod = std::make_unique<AST::Module>();
  ErrCode Result = Mod->loadBinary(FMgr);
  if (Result == ErrCode::Success)
    Stat = State::Parsed;
  else
    Mod.reset(nullptr);
  return Result;
}

/// Do the validation checking. See "include/loader/loadmgr.h".
ErrCode LoadMgr::validateModule() {
  /// Check is the correct state.
  if (Stat != State::Parsed)
    return ErrCode::WrongLoaderFlow;

  /// Call Module mode's checkValidation() to do recursive checking. TODO
  ErrCode Result = ErrCode::Success; // Mod->checkValidation();
  if (Result == ErrCode::Success)
    Stat = State::Validated;
  return Result;
}

/// Get Module node. See "include/loader/loadmgr.h".
ErrCode LoadMgr::getModule(std::unique_ptr<AST::Module> &OutModule) {
  /// Check is the correct state.
  if (Stat != State::Validated)
    return ErrCode::WrongLoaderFlow;

  /// Get the module node class.
  OutModule = std::move(Mod);
  Stat = State::Finished;
  return ErrCode::Success;
}

}