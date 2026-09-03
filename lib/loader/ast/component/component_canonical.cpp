// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadCanonical(AST::Component::Canonical &C) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Canonical);
  };
  // canon ::= 0x00 0x00 f:<core:funcidx> opts:<opts> ft:<typeidx>
  //           => (canon lift f opts type-index-space[ft])
  //         | 0x01 0x00 f:<funcidx> opts:<opts>
  //           => (canon lower f opts (core func))
  //         | 0x02 rt:<typeidx>    => (canon resource.new rt (core func))
  //         | 0x03 rt:<typeidx>    => (canon resource.drop rt (core func))
  //         | 0x04 rt:<typeidx>    => (canon resource.rep rt (core func))
  //         | 0x09 rs:<resultlist> opts:<opts>
  //           => (canon task.return rs opts (core func)) 🔀
  //         | 0x05                 => (canon task.cancel (core func)) 🔀
  //         | 0x0a t:<core:valtype> i:<u32>
  //           => (canon context.get t i (core func)) 🔀
  //         | 0x0b t:<core:valtype> i:<u32>
  //           => (canon context.set t i (core func)) 🔀
  //         | 0x0c cancel?:<cancel?>
  //           => (canon thread.yield cancel? (core func)) 🔀
  //         | 0x06 async?:<async?>
  //           => (canon subtask.cancel async? (core func)) 🔀
  //         | 0x0d                 => (canon subtask.drop (core func)) 🔀
  //         | 0x0e t:<typeidx>     => (canon stream.new t (core func)) 🔀
  //         | 0x0f t:<typeidx> opts:<opts>
  //           => (canon stream.read t opts (core func)) 🔀
  //         | 0x10 t:<typeidx> opts:<opts>
  //           => (canon stream.write t opts (core func)) 🔀
  //         | 0x11 t:<typeidx> async?:<async?>
  //           => (canon stream.cancel-read t async? (core func)) 🔀
  //         | 0x12 t:<typeidx> async?:<async?>
  //           => (canon stream.cancel-write t async? (core func)) 🔀
  //         | 0x13 t:<typeidx>
  //           => (canon stream.drop-readable t (core func)) 🔀
  //         | 0x14 t:<typeidx>
  //           => (canon stream.drop-writable t (core func)) 🔀
  //         | 0x15 t:<typeidx> => (canon future.new t (core func)) 🔀
  //         | 0x16 t:<typeidx> opts:<opts>
  //           => (canon future.read t opts (core func)) 🔀
  //         | 0x17 t:<typeidx> opts:<opts>
  //           => (canon future.write t opts (core func)) 🔀
  //         | 0x18 t:<typeidx> async?:<async?>
  //           => (canon future.cancel-read t async? (core func)) 🔀
  //         | 0x19 t:<typeidx> async?:<async?>
  //           => (canon future.cancel-write t async? (core func)) 🔀
  //         | 0x1a t:<typeidx>
  //           => (canon future.drop-readable t (core func)) 🔀
  //         | 0x1b t:<typeidx>
  //           => (canon future.drop-writable t (core func)) 🔀
  //         | 0x1c opts:<opts> => (canon error-context.new opts (core func)) 📝
  //         | 0x1d opts:<opts>
  //           => (canon error-context.debug-message opts (core func)) 📝
  //         | 0x1e                => (canon error-context.drop (core func)) 📝
  //         | 0x1f                => (canon waitable-set.new (core func)) 🔀
  //         | 0x20 cancel?:<cancel?> m:<core:memoryidx>
  //           => (canon waitable-set.wait cancel? (memory m) (core func)) 🔀
  //         | 0x21 cancel?:<cancel?> m:<core:memoryidx>
  //           => (canon waitable-set.poll cancel? (memory m) (core func)) 🔀
  //         | 0x22                => (canon waitable-set.drop (core func)) 🔀
  //         | 0x23                => (canon waitable.join (core func)) 🔀
  //         | 0x24                => (canon backpressure.inc (core func)) 🔀
  //         | 0x25                => (canon backpressure.dec (core func)) 🔀
  //         | 0x26                => (canon thread.index (core func)) 🧵
  //         | 0x27 ft:<core:typeidx> tbl:<core:tableidx>
  //           => (canon thread.new-indirect ft tbl (core func)) 🧵
  //         | 0x28                => (canon thread.resume-later (core func)) 🧵
  //         | 0x29 cancel?:<cancel?>
  //           => (canon thread.suspend cancel? (core func)) 🧵
  //         | 0x2a cancel?:<cancel?>
  //           => (canon thread.suspend-then-resume cancel? (core func)) 🧵
  //         | 0x2b cancel?:<cancel?>
  //           => (canon thread.yield-then-resume cancel? (core func)) 🧵
  //         | 0x2c cancel?:<cancel?>
  //           => (canon thread.suspend-then-promote cancel? (core func)) 🧵
  //         | 0x2d cancel?:<cancel?>
  //           => (canon thread.yield-then-promote cancel? (core func)) 🧵
  //         | 0x40 shared?:<sh?> ft:<core:typeidx>
  //           => (canon thread.spawn-ref shared? ft (core func)) 🧵②
  //         | 0x41 shared?:<sh?> ft:<core:typeidx> tbl:<core:tableidx>
  //           => (canon thread.spawn-indirect shared? ft tbl (core func)) 🧵②
  //         | 0x42 shared?:<sh?>
  //           => (canon thread.available-parallelism shared? (core func)) 🧵②
  // async?  ::= 0x00 => ϵ
  //           | 0x01 => async
  // cancel? ::= 0x00 => ϵ
  //           | 0x01 => cancellable
  // sh?     ::= 0x00 => ϵ
  //           | 0x01 => shared 🧵②

  // Helper: load the async? / cancel? immediate. They share one encoding.
  auto LoadFlagImm = [this, &ReportError, &C]() -> Expect<void> {
    EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportError));
    if (B == 0x00) {
      C.setFlagImmediate(false);
    } else if (B == 0x01) {
      C.setFlagImmediate(true);
    } else {
      return ReportError(ErrCode::Value::MalformedCanonical);
    }
    return {};
  };

  // Helper: load the 🧵② shared? flag.
  auto LoadShared = [this, &ReportError]() -> Expect<void> {
    EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportError));
    if (B > 0x01) {
      return ReportError(ErrCode::Value::MalformedCanonical);
    }
    return {};
  };

  // Helper: load opts (vec of canonopt).
  auto LoadOpts = [this, &C]() -> Expect<void> {
    std::vector<AST::Component::CanonOpt> Opts;
    EXPECTED_TRY(loadVec<AST::Component::Canonical>(
        Opts, [this](AST::Component::CanonOpt &Opt) {
          return loadCanonicalOption(Opt);
        }));
    C.setOptions(std::move(Opts));
    return {};
  };

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  auto Code = static_cast<ComponentCanonOpCode>(Flag);
  switch (Code) {

  // 0x00 0x00 f:<core:funcidx> opts:<opts> ft:<typeidx>
  case ComponentCanonOpCode::Lift: {
    EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportError));
    if (unlikely(B != 0x00)) {
      return ReportError(ErrCode::Value::MalformedCanonical);
    }
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    C.setIndex(Idx);
    EXPECTED_TRY(LoadOpts());
    EXPECTED_TRY(uint32_t TypeIdx, FMgr.readU32().map_error(ReportError));
    C.setTargetIndex(TypeIdx);
    break;
  }

  // 0x01 0x00 f:<funcidx> opts:<opts>
  case ComponentCanonOpCode::Lower: {
    EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportError));
    if (unlikely(B != 0x00)) {
      return ReportError(ErrCode::Value::MalformedCanonical);
    }
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    C.setIndex(Idx);
    EXPECTED_TRY(LoadOpts());
    break;
  }

  // typeidx-only opcodes
  case ComponentCanonOpCode::Resource__new:
  case ComponentCanonOpCode::Resource__drop:
  case ComponentCanonOpCode::Resource__rep:
  case ComponentCanonOpCode::Stream__new:
  case ComponentCanonOpCode::Stream__drop_readable:
  case ComponentCanonOpCode::Stream__drop_writable:
  case ComponentCanonOpCode::Future__new:
  case ComponentCanonOpCode::Future__drop_readable:
  case ComponentCanonOpCode::Future__drop_writable: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    C.setIndex(Idx);
    break;
  }

  // no-arg opcodes
  case ComponentCanonOpCode::Backpressure__inc:
  case ComponentCanonOpCode::Backpressure__dec:
  case ComponentCanonOpCode::Thread__index:
  case ComponentCanonOpCode::Thread__resume_later:
  case ComponentCanonOpCode::Task__cancel:
  case ComponentCanonOpCode::Subtask__drop:
  case ComponentCanonOpCode::Error_context__drop:
  case ComponentCanonOpCode::Waitable_set__new:
  case ComponentCanonOpCode::Waitable_set__drop:
  case ComponentCanonOpCode::Waitable__join:
    break;

  // 0x09 rs:<resultlist> opts:<opts>
  case ComponentCanonOpCode::Task__return: {
    // Load resultlist (same encoding as functype resultlist).
    EXPECTED_TRY(uint8_t RFlag, FMgr.readByte().map_error(ReportError));
    switch (RFlag) {
    case 0x00: {
      ComponentValType VT;
      EXPECTED_TRY(loadType(VT).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
        return E;
      }));
      C.setResultList(VT);
      break;
    }
    case 0x01: {
      std::vector<AST::Component::LabelValType> ResultList;
      EXPECTED_TRY(loadVec<AST::Component::Canonical>(
          ResultList,
          [this](AST::Component::LabelValType &LV) { return loadType(LV); }));
      C.setResultList(std::move(ResultList));
      break;
    }
    default:
      return ReportError(ErrCode::Value::MalformedCanonical);
    }
    EXPECTED_TRY(LoadOpts());
    break;
  }

  // 0x0a t:<core:valtype> i:<u32> and 0x0b t:<core:valtype> i:<u32>
  case ComponentCanonOpCode::Context__get:
  case ComponentCanonOpCode::Context__set: {
    EXPECTED_TRY(ValType T, loadValType(ASTNodeAttr::Comp_Canonical));
    C.setContextType(T);
    EXPECTED_TRY(uint32_t Val, FMgr.readU32().map_error(ReportError));
    C.setConstVal(Val);
    break;
  }

  // async?/cancel?-only opcodes
  case ComponentCanonOpCode::Yield:
  case ComponentCanonOpCode::Subtask__cancel:
  case ComponentCanonOpCode::Thread__suspend:
  case ComponentCanonOpCode::Thread__suspend_then_resume:
  case ComponentCanonOpCode::Thread__yield_then_resume:
  case ComponentCanonOpCode::Thread__suspend_then_promote:
  case ComponentCanonOpCode::Thread__yield_then_promote: {
    EXPECTED_TRY(LoadFlagImm());
    break;
  }

  // typeidx + opts opcodes
  case ComponentCanonOpCode::Stream__read:
  case ComponentCanonOpCode::Stream__write:
  case ComponentCanonOpCode::Future__read:
  case ComponentCanonOpCode::Future__write: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    C.setIndex(Idx);
    EXPECTED_TRY(LoadOpts());
    break;
  }

  // typeidx + async? opcodes
  case ComponentCanonOpCode::Stream__cancel_read:
  case ComponentCanonOpCode::Stream__cancel_write:
  case ComponentCanonOpCode::Future__cancel_read:
  case ComponentCanonOpCode::Future__cancel_write: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    C.setIndex(Idx);
    EXPECTED_TRY(LoadFlagImm());
    break;
  }

  // opts-only opcodes
  case ComponentCanonOpCode::Error_context__new:
  case ComponentCanonOpCode::Error_context__debug_message: {
    EXPECTED_TRY(LoadOpts());
    break;
  }

  // async? + memidx opcodes
  case ComponentCanonOpCode::Waitable_set__wait:
  case ComponentCanonOpCode::Waitable_set__poll: {
    EXPECTED_TRY(LoadFlagImm());
    EXPECTED_TRY(uint32_t MemIdx, FMgr.readU32().map_error(ReportError));
    C.setIndex(MemIdx);
    break;
  }

  // 0x27 ft:<core:typeidx> tbl:<core:tableidx>
  case ComponentCanonOpCode::Thread__new_indirect: {
    EXPECTED_TRY(uint32_t TypeIdx, FMgr.readU32().map_error(ReportError));
    C.setIndex(TypeIdx);
    EXPECTED_TRY(uint32_t TblIdx, FMgr.readU32().map_error(ReportError));
    C.setTargetIndex(TblIdx);
    break;
  }

  // 🧵② shared?-prefixed opcodes. The flag is parsed but not kept, because
  // these built-ins belong to shared-everything-threads and are not
  // instantiated.
  case ComponentCanonOpCode::Thread__spawn_ref: {
    EXPECTED_TRY(LoadShared());
    EXPECTED_TRY(uint32_t TypeIdx, FMgr.readU32().map_error(ReportError));
    C.setIndex(TypeIdx);
    break;
  }
  case ComponentCanonOpCode::Thread__spawn_indirect: {
    EXPECTED_TRY(LoadShared());
    EXPECTED_TRY(uint32_t TypeIdx, FMgr.readU32().map_error(ReportError));
    C.setIndex(TypeIdx);
    EXPECTED_TRY(uint32_t TblIdx, FMgr.readU32().map_error(ReportError));
    C.setTargetIndex(TblIdx);
    break;
  }
  case ComponentCanonOpCode::Thread__available_parallelism:
    EXPECTED_TRY(LoadShared());
    break;

  default:
    return ReportError(ErrCode::Value::MalformedCanonical);
  }
  C.setOpCode(Code);
  return {};
}

Expect<void> Loader::loadCanonicalOption(AST::Component::CanonOpt &Opt) {
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

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  switch (Flag) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x06:
    break;
  case 0x03:
  case 0x04:
  case 0x05:
  case 0x07: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    Opt.setIndex(Idx);
    break;
  }
  default:
    return ReportError(ErrCode::Value::UnknownCanonicalOption);
  }
  Opt.setCode(static_cast<ComponentCanonOptCode>(Flag));
  return {};
}

} // namespace Loader
} // namespace WasmEdge
