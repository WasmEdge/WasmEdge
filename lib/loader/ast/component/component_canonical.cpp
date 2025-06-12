// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  auto Code = static_cast<AST::Component::Canonical::OpCode>(Flag);
  switch (Code) {
  case AST::Component::Canonical::OpCode::Lift: {
    EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportError));
    if (unlikely(B != 0x00)) {
      return ReportError(ErrCode::Value::MalformedCanonical);
    }
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    std::vector<AST::Component::CanonOpt> Opts;
    EXPECTED_TRY(loadVec<AST::Component::Canonical>(
        Opts, [this](AST::Component::CanonOpt &Opt) {
          return loadCanonicalOption(Opt);
        }));
    EXPECTED_TRY(uint32_t TypeIdx, FMgr.readU32().map_error(ReportError));
    C.setIndex(Idx);
    C.setOptions(std::move(Opts));
    C.setTargetIndex(TypeIdx);
    break;
  }
  case AST::Component::Canonical::OpCode::Lower: {
    EXPECTED_TRY(auto B, FMgr.readByte().map_error(ReportError));
    if (unlikely(B != 0x00)) {
      return ReportError(ErrCode::Value::MalformedCanonical);
    }
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    std::vector<AST::Component::CanonOpt> Opts;
    EXPECTED_TRY(loadVec<AST::Component::Canonical>(
        Opts, [this](AST::Component::CanonOpt &Opt) {
          return loadCanonicalOption(Opt);
        }));
    C.setIndex(Idx);
    C.setOptions(std::move(Opts));
    break;
  }
  case AST::Component::Canonical::OpCode::Resource__new:
  case AST::Component::Canonical::OpCode::Resource__drop:
  case AST::Component::Canonical::OpCode::Resource__drop_async:
  case AST::Component::Canonical::OpCode::Resource__rep:
  case AST::Component::Canonical::OpCode::Stream__new:
  case AST::Component::Canonical::OpCode::Stream__close_readable:
  case AST::Component::Canonical::OpCode::Stream__close_writable:
  case AST::Component::Canonical::OpCode::Future__new:
  case AST::Component::Canonical::OpCode::Future__close_readable:
  case AST::Component::Canonical::OpCode::Future__close_writable:
  case AST::Component::Canonical::OpCode::Thread__spawn_ref: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    C.setIndex(Idx);
    break;
  }
  case AST::Component::Canonical::OpCode::Backpressure__set:
  case AST::Component::Canonical::OpCode::Task__cancel:
  case AST::Component::Canonical::OpCode::Subtask__drop:
  case AST::Component::Canonical::OpCode::Error_context__drop:
  case AST::Component::Canonical::OpCode::Waitable_set__new:
  case AST::Component::Canonical::OpCode::Waitable_set__drop:
  case AST::Component::Canonical::OpCode::Waitable__join:
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
  //            | 0x08                  => always-task-return 🔀

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  switch (Flag) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x06:
  case 0x08:
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
  Opt.setCode(static_cast<AST::Component::CanonOpt::OptCode>(Flag));
  return {};
}

} // namespace Loader
} // namespace WasmEdge
