// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadCanonical(AST::Component::Canon &C) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
  };
  // canon ::= 0x00 0x00 f:<core:funcidx> opts:<opts> ft:<typeidx>
  //           => (canon lift f opts type-index-space[ft])
  //         | 0x01 0x00 f:<funcidx> opts:<opts>
  //           => (canon lower f opts (core func))
  //         | 0x02 rt:<typeidx>    => (canon resource.new rt (core func))
  //         | 0x03 rt:<typeidx>    => (canon resource.drop rt (core func))
  //         | 0x07 rt:<typeidx>
  //           => (canon resource.drop rt async (core func)) 🔀
  //         | 0x04 rt:<typeidx>    => (canon resource.rep rt (core func))
  //         | 0x08                 => (canon backpressure.set (core func)) 🔀
  //         | 0x09 rs:<resultlist> opts:<opts>
  //           => (canon task.return rs opts (core func)) 🔀
  //         | 0x05                 => (canon task.cancel (core func)) 🔀
  //         | 0x0a 0x7f i:<u32>    => (canon context.get i32 i (core func)) 🔀
  //         | 0x0b 0x7f i:<u32>    => (canon context.set i32 i (core func)) 🔀
  //         | 0x0c async?:<async>? => (canon yield async? (core func)) 🔀
  //         | 0x06 async?:<async?>
  //           => (canon subtask.cancel async? (core func)) 🔀
  //         | 0x0d                 => (canon subtask.drop (core func)) 🔀
  //         | 0x0e t:<typeidx>     => (canon stream.new t (core func)) 🔀
  //         | 0x0f t:<typeidx> opts:<opts>
  //           => (canon stream.read t opts (core func)) 🔀
  //         | 0x10 t:<typeidx> opts:<opts>
  //           => (canon stream.write t opts (core func)) 🔀
  //         | 0x11 t:<typeidx> async?:<async?>
  //           => (canon stream.cancel-read async? (core func)) 🔀
  //         | 0x12 t:<typeidx> async?:<async?>
  //           => (canon stream.cancel-write async? (core func)) 🔀
  //         | 0x13 t:<typeidx>
  //           => (canon stream.close-readable t (core func)) 🔀
  //         | 0x14 t:<typeidx>
  //           => (canon stream.close-writable t (core func)) 🔀
  //         | 0x15 t:<typeidx> => (canon future.new t (core func)) 🔀
  //         | 0x16 t:<typeidx> opts:<opts>
  //           => (canon future.read t opts (core func)) 🔀
  //         | 0x17 t:<typeidx> opts:<opts>
  //           => (canon future.write t opts (core func)) 🔀
  //         | 0x18 t:<typeidx> async?:<async?>
  //           => (canon future.cancel-read async? (core func)) 🔀
  //         | 0x19 t:<typeidx> async?:<async?>
  //           => (canon future.cancel-write async? (core func)) 🔀
  //         | 0x1a t:<typeidx>
  //           => (canon future.close-readable t (core func)) 🔀
  //         | 0x1b t:<typeidx>
  //           => (canon future.close-writable t (core func)) 🔀
  //         | 0x1c opts:<opts> => (canon error-context.new opts (core func)) 📝
  //         | 0x1d opts:<opts>
  //           => (canon error-context.debug-message opts (core func)) 📝
  //         | 0x1e                => (canon error-context.drop (core func)) 📝
  //         | 0x1f                => (canon waitable-set.new (core func)) 🔀
  //         | 0x20 async?:<async>? m:<core:memidx>
  //           => (canon waitable-set.wait async? (memory m) (core func)) 🔀
  //         | 0x21 async?:<async>? m:<core:memidx>
  //           => (canon waitable-set.poll async? (memory m) (core func)) 🔀
  //         | 0x22                => (canon waitable-set.drop (core func)) 🔀
  //         | 0x23                => (canon waitable.join (core func)) 🔀
  //         | 0x40 ft:<typeidx>   => (canon thread.spawn_ref ft (core func)) 🧵
  //         | 0x41 ft:<typeidx> tbl:<core:tableidx>
  //           => (canon thread.spawn_indirect ft tbl (core func)) 🧵
  //         | 0x42 => (canon thread.available_parallelism (core func)) 🧵
  // async? ::= 0x00 => ϵ
  //          | 0x01 => async

  // TODO: COMPONENT - collect the canon loading here.

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
  }));
  switch (Flag) {
  case 0x00: {
    EXPECTED_TRY(auto B, FMgr.readByte().map_error(ReportError));
    if (unlikely(B != 0x00)) {
      return logLoadError(ErrCode::Value::MalformedCanonical,
                          FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
    }
    EXPECTED_TRY(loadCanonical(C.emplace<AST::Component::Lift>()));
    return {};
  }
  case 0x01: {
    EXPECTED_TRY(auto B, FMgr.readByte().map_error(ReportError));
    if (unlikely(B != 0x00)) {
      return logLoadError(ErrCode::Value::MalformedCanonical,
                          FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
    }
    EXPECTED_TRY(loadCanonical(C.emplace<AST::Component::Lower>()));
    return {};
  }
  case 0x02: {
    EXPECTED_TRY(C.emplace<AST::Component::ResourceNew>().getTypeIndex(),
                 FMgr.readU32().map_error(ReportError));
    return {};
  }
  case 0x03: {
    EXPECTED_TRY(C.emplace<AST::Component::ResourceDrop>().getTypeIndex(),
                 FMgr.readU32().map_error(ReportError));
    return {};
  }
  case 0x04: {
    EXPECTED_TRY(C.emplace<AST::Component::ResourceRep>().getTypeIndex(),
                 FMgr.readU32().map_error(ReportError));
    return {};
  }
  default:
    return logLoadError(ErrCode::Value::MalformedCanonical,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
  }
}

Expect<void> Loader::loadCanonical(AST::Component::Lift &C) {
  EXPECTED_TRY(C.getCoreFuncIndex(), FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
  }));
  EXPECTED_TRY(loadVec<AST::Component::Canon>(
      C.getOptions(), [this](AST::Component::CanonOpt &Opt) {
        return loadCanonicalOption(Opt);
      }));
  EXPECTED_TRY(C.getFuncTypeIndex(), FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
  }));
  return {};
}

Expect<void> Loader::loadCanonical(AST::Component::Lower &C) {
  EXPECTED_TRY(C.getFuncIndex(), FMgr.readU32().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
  }));
  return loadVec<AST::Component::Canon>(C.getOptions(),
                                        [this](AST::Component::CanonOpt &Opt) {
                                          return loadCanonicalOption(Opt);
                                        });
}

Expect<void> Loader::loadCanonicalOption(AST::Component::CanonOpt &C) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_CanonOpt);
  };
  // canonopt ::= 0x00                  => string-encoding=utf8
  //            | 0x01                  => string-encoding=utf16
  //            | 0x02                  => string-encoding=latin1+utf16
  //            | 0x03 m:<core:memidx>  => (memory m)
  //            | 0x04 f:<core:funcidx> => (realloc f)
  //            | 0x05 f:<core:funcidx> => (post-return f)
  //            | 0x06                  => async 🔀
  //            | 0x07 f:<core:funcidx> => (callback f) 🔀
  //            | 0x08                  => always-task-return 🔀

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  switch (Flag) {
  case 0x00:
  case 0x01:
  case 0x02: {

    C.emplace<AST::Component::StringEncoding>() =
        static_cast<AST::Component::StringEncoding>(Flag);
    return {};
  }
  case 0x03: {
    EXPECTED_TRY(C.emplace<AST::Component::Memory>().getMemIndex(),
                 FMgr.readU32().map_error(ReportError));
    return {};
  }
  case 0x04: {
    EXPECTED_TRY(C.emplace<AST::Component::Realloc>().getFuncIndex(),
                 FMgr.readU32().map_error(ReportError));
    return {};
  }
  case 0x05: {
    EXPECTED_TRY(C.emplace<AST::Component::PostReturn>().getFuncIndex(),
                 FMgr.readU32().map_error(ReportError));
    return {};
  }
  case 0x06:
  case 0x07:
  case 0x08:
    // TODO: COMPONENT - implement these cases.
    return logLoadError(ErrCode::Value::ComponentNotImplLoader,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_CanonOpt);
  default:
    return logLoadError(ErrCode::Value::UnknownCanonicalOption,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_CanonOpt);
  }
}

} // namespace Loader
} // namespace WasmEdge
