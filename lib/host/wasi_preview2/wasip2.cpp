// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasip2.cpp - WASI Preview 2 host component instances --------------===//
//
// Each wasi 0.2 interface becomes a host ComponentInstance named after the
// interface (e.g. "wasi:clocks/monotonic-clock@0.2.0") exporting host
// component functions and host-destructed resources. Streams carry the file
// descriptor as the resource representation.
//
//===----------------------------------------------------------------------===//

#include "host/wasi_preview2/wasip2.h"

#include "ast/component/type.h"
#include "common/errcode.h"
#include "common/types.h"
#include "runtime/instance/component/function.h"

#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASIPreview2 {

using namespace AST::Component;
using Runtime::Instance::ComponentInstance;
using CompFuncInst = Runtime::Instance::Component::FunctionInstance;
using HostRets = std::vector<std::pair<ComponentValVariant, ComponentValType>>;

namespace {

// Small builder over a host interface instance: owned types, host functions.
struct Iface {
  std::unique_ptr<ComponentInstance> Inst;

  Iface(std::string_view Name)
      : Inst(std::make_unique<ComponentInstance>(std::string(Name))) {}

  uint32_t addType(DefValType &&DVT) {
    DefType DT;
    DT.setDefValType(std::move(DVT));
    return Inst->addOwnedType(std::move(DT));
  }

  uint32_t addResource(std::string_view ExportName,
                       std::function<void(uint32_t)> Dtor = nullptr) {
    const uint32_t Idx = Inst->addHostResourceType(std::move(Dtor));
    Inst->exportType(ExportName, Idx);
    return Idx;
  }

  // Share a resource type defined by another interface (WIT `use`).
  uint32_t useResourceIdx(const ComponentInstance &From,
                          std::string_view ExportName) {
    Inst->addTypeWithResource(nullptr, From.findTypeResource(ExportName));
    return Inst->getTypeCount() - 1;
  }

  void addFunc(std::string_view Name, std::vector<LabelValType> Params,
               std::vector<LabelValType> Results,
               CompFuncInst::HostFuncCallback &&Cb) {
    auto FT = std::make_unique<FuncType>(std::move(Params), std::move(Results));
    Inst->addHostFunc(Name, std::make_unique<CompFuncInst>(
                                std::move(FT), std::move(Cb), Inst.get()));
  }
};

ComponentValType prim(ComponentTypeCode C) { return ComponentValType(C); }
ComponentValType tyIdx(uint32_t I) { return ComponentValType(I); }

// result<_, stream-error> ok value.
ComponentValVariant okResult() {
  return makeComponentVal(ResultVal{true, std::nullopt});
}

} // namespace

std::vector<std::unique_ptr<ComponentInstance>> createInterfaces() {
  std::vector<std::unique_ptr<ComponentInstance>> Out;

  // ==== wasi:clocks/monotonic-clock@0.2.0 ====
  {
    Iface I("wasi:clocks/monotonic-clock@0.2.0");
    I.addFunc("now", {}, {LabelValType(prim(ComponentTypeCode::U64))},
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                const auto Now =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
                HostRets R;
                R.emplace_back(ComponentValVariant{uint64_t(Now)},
                               prim(ComponentTypeCode::U64));
                return R;
              });
    I.addFunc("resolution", {}, {LabelValType(prim(ComponentTypeCode::U64))},
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                HostRets R;
                R.emplace_back(ComponentValVariant{uint64_t(1)},
                               prim(ComponentTypeCode::U64));
                return R;
              });
    Out.push_back(std::move(I.Inst));
  }

  // ==== wasi:clocks/wall-clock@0.2.0 ====
  {
    Iface I("wasi:clocks/wall-clock@0.2.0");
    // record datetime { seconds: u64, nanoseconds: u32 }
    RecordTy DateTime;
    DateTime.LabelTypes.emplace_back("seconds", prim(ComponentTypeCode::U64));
    DateTime.LabelTypes.emplace_back("nanoseconds",
                                     prim(ComponentTypeCode::U32));
    DefValType DVT;
    DVT.setRecord(std::move(DateTime));
    const uint32_t DtIdx = I.addType(std::move(DVT));
    I.Inst->exportType("datetime", DtIdx);

    auto WallNow = [](Span<const ComponentValVariant>) -> Expect<HostRets> {
      const auto Now = std::chrono::system_clock::now().time_since_epoch();
      const auto Sec = std::chrono::duration_cast<std::chrono::seconds>(Now);
      const auto Nsec =
          std::chrono::duration_cast<std::chrono::nanoseconds>(Now - Sec);
      RecordVal DT;
      DT.Fields.emplace_back("seconds",
                             ComponentValVariant{uint64_t(Sec.count())});
      DT.Fields.emplace_back("nanoseconds",
                             ComponentValVariant{uint32_t(Nsec.count())});
      HostRets R;
      R.emplace_back(makeComponentVal(std::move(DT)), ComponentValType(0));
      return R;
    };
    I.addFunc("now", {}, {LabelValType(tyIdx(DtIdx))}, WallNow);
    I.addFunc(
        "resolution", {}, {LabelValType(tyIdx(DtIdx))},
        [](Span<const ComponentValVariant>) -> Expect<HostRets> {
          RecordVal DT;
          DT.Fields.emplace_back("seconds", ComponentValVariant{uint64_t(0)});
          DT.Fields.emplace_back("nanoseconds",
                                 ComponentValVariant{uint32_t(1)});
          HostRets R;
          R.emplace_back(makeComponentVal(std::move(DT)), ComponentValType(0));
          return R;
        });
    Out.push_back(std::move(I.Inst));
  }

  // ==== wasi:random/random@0.2.0 ====
  {
    Iface I("wasi:random/random@0.2.0");
    // list<u8>
    DefValType ListU8;
    ListU8.setList(ListTy{prim(ComponentTypeCode::U8), std::nullopt});
    const uint32_t ListIdx = I.addType(std::move(ListU8));

    I.addFunc("get-random-u64", {},
              {LabelValType(prim(ComponentTypeCode::U64))},
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                static std::mt19937_64 Gen{std::random_device{}()};
                HostRets R;
                R.emplace_back(ComponentValVariant{uint64_t(Gen())},
                               prim(ComponentTypeCode::U64));
                return R;
              });
    I.addFunc(
        "get-random-bytes", {LabelValType("len", prim(ComponentTypeCode::U64))},
        {LabelValType(tyIdx(ListIdx))},
        [](Span<const ComponentValVariant> Args) -> Expect<HostRets> {
          static std::mt19937_64 Gen{std::random_device{}()};
          const uint64_t Len = std::get<uint64_t>(Args[0]);
          ListVal L;
          L.Elements.reserve(Len);
          for (uint64_t K = 0; K < Len; ++K) {
            L.Elements.emplace_back(uint8_t(Gen() & 0xFF));
          }
          HostRets R;
          R.emplace_back(makeComponentVal(std::move(L)), ComponentValType(0));
          return R;
        });
    Out.push_back(std::move(I.Inst));
  }

  // ==== wasi:random/insecure-seed@0.2.0 ====
  {
    Iface I("wasi:random/insecure-seed@0.2.0");
    DefValType Tup;
    Tup.setTuple(
        TupleTy{{prim(ComponentTypeCode::U64), prim(ComponentTypeCode::U64)}});
    const uint32_t TupIdx = I.addType(std::move(Tup));
    I.addFunc("insecure-seed", {}, {LabelValType(tyIdx(TupIdx))},
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                TupleVal T;
                T.Values.emplace_back(uint64_t(0x9E3779B97F4A7C15ULL));
                T.Values.emplace_back(uint64_t(0xBF58476D1CE4E5B9ULL));
                HostRets R;
                R.emplace_back(makeComponentVal(std::move(T)),
                               ComponentValType(0));
                return R;
              });
    Out.push_back(std::move(I.Inst));
  }

  // ==== wasi:cli/environment@0.2.0 ====
  {
    Iface I("wasi:cli/environment@0.2.0");
    // tuple<string, string>, list<tuple<string,string>>, list<string>,
    // option<string>
    DefValType TupSS;
    TupSS.setTuple(TupleTy{
        {prim(ComponentTypeCode::String), prim(ComponentTypeCode::String)}});
    const uint32_t TupIdx = I.addType(std::move(TupSS));
    DefValType ListTup;
    ListTup.setList(ListTy{tyIdx(TupIdx), std::nullopt});
    const uint32_t ListTupIdx = I.addType(std::move(ListTup));
    DefValType ListStr;
    ListStr.setList(ListTy{prim(ComponentTypeCode::String), std::nullopt});
    const uint32_t ListStrIdx = I.addType(std::move(ListStr));
    DefValType OptStr;
    OptStr.setOption(OptionTy{prim(ComponentTypeCode::String)});
    const uint32_t OptStrIdx = I.addType(std::move(OptStr));

    I.addFunc("get-environment", {}, {LabelValType(tyIdx(ListTupIdx))},
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                HostRets R;
                R.emplace_back(makeComponentVal(ListVal{}),
                               ComponentValType(0));
                return R;
              });
    I.addFunc("get-arguments", {}, {LabelValType(tyIdx(ListStrIdx))},
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                HostRets R;
                R.emplace_back(makeComponentVal(ListVal{}),
                               ComponentValType(0));
                return R;
              });
    I.addFunc("initial-cwd", {}, {LabelValType(tyIdx(OptStrIdx))},
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                HostRets R;
                R.emplace_back(makeComponentVal(OptionVal{std::nullopt}),
                               ComponentValType(0));
                return R;
              });
    Out.push_back(std::move(I.Inst));
  }

  // ==== wasi:cli/exit@0.2.0 ====
  {
    Iface I("wasi:cli/exit@0.2.0");
    DefValType Res;
    Res.setResult(ResultTy{std::nullopt, std::nullopt});
    const uint32_t ResIdx = I.addType(std::move(Res));
    I.addFunc("exit", {LabelValType("status", tyIdx(ResIdx))}, {},
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                // WIT: exit the current instance. Mapped to termination,
                // like preview 1 proc_exit.
                return Unexpect(ErrCode::Value::Terminated);
              });
    Out.push_back(std::move(I.Inst));
  }

  // ==== wasi:io/error@0.2.0 ====
  Iface IoError("wasi:io/error@0.2.0");
  const ComponentInstance *IoErrorInst = IoError.Inst.get();
  {
    const uint32_t ErrIdx = IoError.addResource("error");
    // [method]error.to-debug-string: func(self: borrow<error>) -> string
    DefValType BorrowErr;
    BorrowErr.setBorrow(BorrowTy{ErrIdx});
    const uint32_t BorrowIdx = IoError.addType(std::move(BorrowErr));
    IoError.addFunc("[method]error.to-debug-string",
                    {LabelValType("self", tyIdx(BorrowIdx))},
                    {LabelValType(prim(ComponentTypeCode::String))},
                    [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                      HostRets R;
                      R.emplace_back(
                          ComponentValVariant{std::string("io error")},
                          prim(ComponentTypeCode::String));
                      return R;
                    });
    Out.push_back(std::move(IoError.Inst));
  }

  // ==== wasi:io/streams@0.2.0 ====
  Iface Streams("wasi:io/streams@0.2.0");
  const ComponentInstance *StreamsInst = Streams.Inst.get();
  {
    // use wasi:io/error.{error}; streams carry the fd as representation.
    const uint32_t ErrIdx = Streams.useResourceIdx(*IoErrorInst, "error");
    const uint32_t InIdx = Streams.addResource("input-stream");
    const uint32_t OutIdx = Streams.addResource("output-stream");

    // variant stream-error { last-operation-failed(own<error>), closed }
    DefValType OwnErr;
    OwnErr.setOwn(OwnTy{ErrIdx});
    const uint32_t OwnErrIdx = Streams.addType(std::move(OwnErr));
    VariantTy StreamErr;
    StreamErr.Cases.emplace_back("last-operation-failed", tyIdx(OwnErrIdx));
    StreamErr.Cases.emplace_back("closed", std::nullopt);
    DefValType StreamErrDVT;
    StreamErrDVT.setVariant(std::move(StreamErr));
    const uint32_t StreamErrIdx = Streams.addType(std::move(StreamErrDVT));
    Streams.Inst->exportType("stream-error", StreamErrIdx);

    // list<u8>
    DefValType ListU8;
    ListU8.setList(ListTy{prim(ComponentTypeCode::U8), std::nullopt});
    const uint32_t ListIdx = Streams.addType(std::move(ListU8));

    // result<_, stream-error>, result<u64, stream-error>,
    // result<list<u8>, stream-error>
    DefValType ResUnit;
    ResUnit.setResult(ResultTy{std::nullopt, tyIdx(StreamErrIdx)});
    const uint32_t ResUnitIdx = Streams.addType(std::move(ResUnit));
    DefValType ResU64;
    ResU64.setResult(
        ResultTy{prim(ComponentTypeCode::U64), tyIdx(StreamErrIdx)});
    const uint32_t ResU64Idx = Streams.addType(std::move(ResU64));
    DefValType ResList;
    ResList.setResult(ResultTy{tyIdx(ListIdx), tyIdx(StreamErrIdx)});
    const uint32_t ResListIdx = Streams.addType(std::move(ResList));

    DefValType BorrowIn;
    BorrowIn.setBorrow(BorrowTy{InIdx});
    const uint32_t BorrowInIdx = Streams.addType(std::move(BorrowIn));
    DefValType BorrowOut;
    BorrowOut.setBorrow(BorrowTy{OutIdx});
    const uint32_t BorrowOutIdx = Streams.addType(std::move(BorrowOut));

    // Write the list<u8> contents to the stream's fd (rep 1/2).
    auto WriteImpl =
        [](Span<const ComponentValVariant> Args) -> Expect<HostRets> {
      const uint32_t Fd =
          std::get<BorrowVal>(std::get<std::shared_ptr<ValComp>>(Args[0])->V)
              .Handle;
      const auto &L =
          std::get<ListVal>(std::get<std::shared_ptr<ValComp>>(Args[1])->V);
      std::string Buf;
      Buf.reserve(L.Elements.size());
      for (const auto &E : L.Elements) {
        Buf.push_back(static_cast<char>(std::get<uint8_t>(E)));
      }
      std::FILE *F = Fd == 2 ? stderr : stdout;
      std::fwrite(Buf.data(), 1, Buf.size(), F);
      std::fflush(F);
      HostRets R;
      R.emplace_back(okResult(), ComponentValType(0));
      return R;
    };
    Streams.addFunc("[method]output-stream.write",
                    {LabelValType("self", tyIdx(BorrowOutIdx)),
                     LabelValType("contents", tyIdx(ListIdx))},
                    {LabelValType(tyIdx(ResUnitIdx))}, WriteImpl);
    Streams.addFunc("[method]output-stream.blocking-write-and-flush",
                    {LabelValType("self", tyIdx(BorrowOutIdx)),
                     LabelValType("contents", tyIdx(ListIdx))},
                    {LabelValType(tyIdx(ResUnitIdx))}, WriteImpl);

    auto FlushImpl = [](Span<const ComponentValVariant>) -> Expect<HostRets> {
      HostRets R;
      R.emplace_back(okResult(), ComponentValType(0));
      return R;
    };
    Streams.addFunc("[method]output-stream.flush",
                    {LabelValType("self", tyIdx(BorrowOutIdx))},
                    {LabelValType(tyIdx(ResUnitIdx))}, FlushImpl);
    Streams.addFunc("[method]output-stream.blocking-flush",
                    {LabelValType("self", tyIdx(BorrowOutIdx))},
                    {LabelValType(tyIdx(ResUnitIdx))}, FlushImpl);
    Streams.addFunc(
        "[method]output-stream.check-write",
        {LabelValType("self", tyIdx(BorrowOutIdx))},
        {LabelValType(tyIdx(ResU64Idx))},
        [](Span<const ComponentValVariant>) -> Expect<HostRets> {
          HostRets R;
          R.emplace_back(makeComponentVal(ResultVal{
                             true, ComponentValVariant{uint64_t(1024 * 1024)}}),
                         ComponentValType(0));
          return R;
        });

    auto ReadImpl =
        [](Span<const ComponentValVariant> Args) -> Expect<HostRets> {
      const uint64_t Len = std::get<uint64_t>(Args[1]);
      std::string Buf(Len, '\0');
      const size_t Got = std::fread(Buf.data(), 1, Len, stdin);
      ListVal L;
      L.Elements.reserve(Got);
      for (size_t K = 0; K < Got; ++K) {
        L.Elements.emplace_back(uint8_t(Buf[K]));
      }
      HostRets R;
      if (Got == 0) {
        // EOF reports closed.
        R.emplace_back(
            makeComponentVal(ResultVal{
                false, makeComponentVal(VariantVal{1, std::nullopt, {}})}),
            ComponentValType(0));
      } else {
        R.emplace_back(
            makeComponentVal(ResultVal{true, makeComponentVal(std::move(L))}),
            ComponentValType(0));
      }
      return R;
    };
    Streams.addFunc("[method]input-stream.read",
                    {LabelValType("self", tyIdx(BorrowInIdx)),
                     LabelValType("len", prim(ComponentTypeCode::U64))},
                    {LabelValType(tyIdx(ResListIdx))}, ReadImpl);
    Streams.addFunc("[method]input-stream.blocking-read",
                    {LabelValType("self", tyIdx(BorrowInIdx)),
                     LabelValType("len", prim(ComponentTypeCode::U64))},
                    {LabelValType(tyIdx(ResListIdx))}, ReadImpl);
    Out.push_back(std::move(Streams.Inst));
  }

  // ==== wasi:cli/std{in,out,err}@0.2.0 ====
  {
    Iface Stdin("wasi:cli/stdin@0.2.0");
    const uint32_t InIdx = Stdin.useResourceIdx(*StreamsInst, "input-stream");
    DefValType OwnIn;
    OwnIn.setOwn(OwnTy{InIdx});
    const uint32_t OwnInIdx = Stdin.addType(std::move(OwnIn));
    Stdin.addFunc("get-stdin", {}, {LabelValType(tyIdx(OwnInIdx))},
                  [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                    HostRets R;
                    R.emplace_back(makeComponentVal(OwnVal{0}),
                                   ComponentValType(0));
                    return R;
                  });
    Out.push_back(std::move(Stdin.Inst));

    auto MakeOutIface = [&](std::string_view Name, std::string_view Fn,
                            uint32_t Fd) {
      Iface I(Name);
      const uint32_t OIdx = I.useResourceIdx(*StreamsInst, "output-stream");
      DefValType OwnOut;
      OwnOut.setOwn(OwnTy{OIdx});
      const uint32_t OwnOutIdx = I.addType(std::move(OwnOut));
      I.addFunc(Fn, {}, {LabelValType(tyIdx(OwnOutIdx))},
                [Fd](Span<const ComponentValVariant>) -> Expect<HostRets> {
                  HostRets R;
                  R.emplace_back(makeComponentVal(OwnVal{Fd}),
                                 ComponentValType(0));
                  return R;
                });
      Out.push_back(std::move(I.Inst));
    };
    MakeOutIface("wasi:cli/stdout@0.2.0", "get-stdout", 1);
    MakeOutIface("wasi:cli/stderr@0.2.0", "get-stderr", 2);
  }

  return Out;
}

} // namespace WASIPreview2
} // namespace Host
} // namespace WasmEdge
