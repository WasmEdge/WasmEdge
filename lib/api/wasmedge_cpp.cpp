#include "wasmedge/wasmedge.hh"

#include "driver/compiler.h"
#include "driver/tool.h"
#include "host/wasi/wasimodule.h"
#include "plugin/plugin.h"
#include "vm/vm.h"

namespace {

// Helper function for returning a struct uint128_t / int128_t
// from class WasmEdge::uint128_t / WasmEdge::int128_t.
template <typename C>
inline constexpr ::uint128_t to_uint128_t(C Val) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
  return Val;
#else
  return {.Low = Val.low(), .High = static_cast<uint64_t>(Val.high())};
#endif
}
template <typename C> inline constexpr ::int128_t to_int128_t(C Val) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
  return Val;
#else
  return {.Low = Val.low(), .High = Val.high()};
#endif
}

// Helper function for returning a class WasmEdge::uint128_t /
// WasmEdge::int128_t from struct uint128_t / int128_t.
template <typename C, typename T>
inline constexpr C to_WasmEdge_128_t(T Val) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
  return Val;
#else
  return C(Val.High, Val.Low);
#endif
}

// Helper template to run and return result
auto EmptyThen = [](auto &&) noexcept {};
template <typename T, typename U, typename... CxtT>
inline constexpr WasmEdge::SDK::Result wrap(T &&Proc, U &&Then,
                                            CxtT &... Cxts) noexcept {
  auto Res = Proc();
  if (Res) {
    Then(Res);
    return WasmEdge::SDK::Result::ResultFactory::GenResult(
        WasmEdge::ErrCode::Value::Success);
  }
  return WasmEdge::SDK::Result::ResultFactory::GenResult(Res.error());
}

} // namespace

namespace WasmEdge {
namespace SDK {

class Value::ValueUtils {
  ValueUtils() = default;
  ~ValueUtils() = default;

public:
  // Helper function for converting a WasmEdge_Value array to a ValVariant
  // vector.
  static std::pair<std::vector<WasmEdge::ValVariant>,
                   std::vector<WasmEdge::ValType>>
  GenParamPair(const std::vector<WasmEdge::SDK::Value> &Val) noexcept {

    std::vector<WasmEdge::ValVariant> VVec;
    std::vector<WasmEdge::ValType> TVec;

    auto Len = Val.size();
    VVec.resize(Len);
    TVec.resize(Len);
    for (uint32_t I = 0; I < Len; I++) {
      TVec[I] = static_cast<WasmEdge::ValType>(Val[I].Type);
      switch (TVec[I]) {
      case WasmEdge::ValType::I32:
        VVec[I] = ValVariant::wrap<uint32_t>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::I64:
        VVec[I] = ValVariant::wrap<uint64_t>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::F32:
        VVec[I] = ValVariant::wrap<float>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::F64:
        VVec[I] = ValVariant::wrap<double>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::V128:
        VVec[I] = ValVariant::wrap<WasmEdge::uint128_t>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::FuncRef:
        VVec[I] = ValVariant::wrap<WasmEdge::FuncRef>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::ExternRef:
        VVec[I] = ValVariant::wrap<WasmEdge::ExternRef>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      default:
        // TODO: Return error
        assumingUnreachable();
      }
    }
    return {VVec, TVec};
  }

  static inline void
  FillValueArr(Span<const std::pair<ValVariant, WasmEdge::ValType>> Vec,
               std::vector<Value> &Returns) {
    Returns.resize(Vec.size());
    for (uint32_t I = 0; I < Vec.size(); I++) {
      Returns[I] = std::move(Value(to_uint128_t(Vec[I].first.unwrap()),
                                   static_cast<ValType>(Vec[I].second)));
    }
  }

  static inline void FillValue(Value &Data, WasmEdge::ValVariant Val,
                               ValType T) {
    Data.Val = to_uint128_t(Val.unwrap());
    Data.Type = T;
  }

  static inline uint128_t GetValue(const Value &Data) { return Data.Val; }

  static inline ValType GetValueType(const Value &Data) { return Data.Type; }

  static inline Value GenValueFromValVariant(WasmEdge::ValVariant &Val,
                                             ValType T) {
    return Value(to_uint128_t(Val.unwrap()), T);
  }
};

// >>>>>>>> WasmEdge Context members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class ASTModuleContext : public ASTModule {
public:
  ASTModuleContext() = default;
  ASTModuleContext(const ASTModuleContext &Module) : Content(Module.Content) {}
  ASTModuleContext(ASTModuleContext &&Module)
      : Content(Module.Content), IsOwn(Module.IsOwn) {
    Module.IsOwn = false;
  }
  ~ASTModuleContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  void OwnASTModulePointer(WasmEdge::AST::Module *Module) {
    if (IsOwn) {
      delete Content;
    }
    Content = Module;
    IsOwn = true;
  }

  const WasmEdge::AST::Module *GetConstPointer() const { return Content; }

  std::vector<ImportType> ListImports() {
    std::vector<ImportType> ImpList;
    if (Content) {
      const auto &ImpSec = Content->getImportSection().getContent();
      for (uint32_t I = 0; I < ImpSec.size(); I++) {
        ImpList.emplace_back(ImpSec[I]);
      }
    }
    return ImpList;
  }

  std::vector<ExportType> ListExports() {
    std::vector<ExportType> ExpList;
    if (Content) {
      const auto &ExpSec = Content->getExportSection().getContent();
      for (uint32_t I = 0; I < ExpSec.size(); I++) {
        ExpList.emplace_back(ExpSec[I]);
      }
    }
    return ExpList;
  }

private:
  WasmEdge::AST::Module *Content = nullptr;
  bool IsOwn = false;
};

class FunctionTypeContext : public FunctionType {
public:
  FunctionTypeContext(const AST::FunctionType *Func)
      : Content(const_cast<AST::FunctionType *>(Func)) {}

  FunctionTypeContext(const std::vector<ValType> &ParamList,
                      const std::vector<ValType> &ReturnList) {
    Content = new WasmEdge::AST::FunctionType;
    if (ParamList.size() > 0) {
      Content->getParamTypes().resize(ParamList.size());
    }
    for (uint32_t I = 0; I < ParamList.size(); I++) {
      Content->getParamTypes()[I] =
          static_cast<WasmEdge::ValType>(ParamList[I]);
    }
    if (ReturnList.size() > 0) {
      Content->getReturnTypes().resize(ReturnList.size());
    }
    for (uint32_t I = 0; I < ReturnList.size(); I++) {
      Content->getReturnTypes()[I] =
          static_cast<WasmEdge::ValType>(ReturnList[I]);
    }
    IsOwn = true;
  }

  ~FunctionTypeContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  const std::vector<ValType> GetParameters() {
    std::vector<ValType> Params;
    if (Content) {
      auto FuncParams = Content->getParamTypes();
      std::transform(FuncParams.begin(), FuncParams.end(), Params.begin(),
                     [](auto ValT) { return static_cast<ValType>(ValT) });
    }
    return Params;
  }

  const std::vector<ValType> GetReturns() {
    std::vector<ValType> Returns;
    if (Content) {
      auto FuncReturns = Content->getReturnTypes();
      std::transform(FuncReturns.begin(), FuncReturns.end(), Returns.begin(),
                     [](auto ValT) { return static_cast<ValType>(ValT) });
    }
    return Returns;
  }

private:
  WasmEdge::AST::FunctionType *Content = nullptr;
  bool IsOwn = false;

  friend class FunctionInstanceContext;
};

class TableTypeContext : public TableType {
public:
  TableTypeContext(const AST::TableType *TabType)
      : Content(const_cast<AST::TableType *>(TabType)) {}

  TableTypeContext(const RefType RefT, const Limit &Lim) {
    WasmEdge::RefType Type = static_cast<WasmEdge::RefType>(RefT);
    if (Lim.HasMax) {
      Content = new WasmEdge::AST::TableType(Type, Lim.Min, Lim.Max);
    } else {
      Content = new WasmEdge::AST::TableType(Type, Lim.Min);
    }
    IsOwn = true;
  }

  ~TableTypeContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  RefType GetRefType() {
    if (Content) {
      return static_cast<RefType>(Content->getRefType());
    }
    return RefType::FuncRef;
  }

  const Limit GetLimit() {
    Limit OutputLim;
    if (Content) {
      const auto &Lim = Content->getLimit();
      OutputLim.HasMax = Lim.hasMax();
      OutputLim.Shared = Lim.isShared();
      OutputLim.Min = Lim.getMin();
      OutputLim.Max = Lim.getMax();
    }
    return OutputLim;
  }

private:
  WasmEdge::AST::TableType *Content = nullptr;
  bool IsOwn = false;

  friend class TableInstanceContext;
};

class MemoryTypeContext : public MemoryType {
public:
  MemoryTypeContext(const AST::MemoryType *MemType)
      : Content(const_cast<AST::MemoryType *>(MemType)) {}

  MemoryTypeContext(const Limit &Lim) {
    if (Lim.Shared) {
      Content = new WasmEdge::AST::MemoryType(Lim.Min, Lim.Max, true);
    } else if (Lim.HasMax) {
      Content = new WasmEdge::AST::MemoryType(Lim.Min, Lim.Max);
    } else {
      Content = new WasmEdge::AST::MemoryType(Lim.Min);
    }
    IsOwn = true;
  }

  ~MemoryTypeContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  const Limit GetLimit() {
    Limit OutputLim;
    if (Content) {
      const auto &Lim = Content->getLimit();
      OutputLim.HasMax = Lim.hasMax();
      OutputLim.Shared = Lim.isShared();
      OutputLim.Min = Lim.getMin();
      OutputLim.Max = Lim.getMax();
    }
    return OutputLim;
  }

private:
  WasmEdge::AST::MemoryType *Content = nullptr;
  bool IsOwn = false;

  friend class MemoryInstanceContext;
};

class GlobalTypeContext : public GlobalType {
public:
  GlobalTypeContext(const AST::GlobalType *GlobType)
      : Content(const_cast<AST::GlobalType *>(GlobType)) {}

  GlobalTypeContext(const ValType ValT, const Mutability Mut) {
    Content =
        new WasmEdge::AST::GlobalType(static_cast<WasmEdge::ValType>(ValT),
                                      static_cast<WasmEdge::ValMut>(Mut));
    IsOwn = true;
  }

  ~GlobalTypeContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  ValType GetValType() {
    if (Content) {
      return static_cast<ValType>(Content->getValType());
    }
    return ValType::I32;
  }

  Mutability GetMutability() {
    if (Content) {
      return static_cast<Mutability>(Content->getValMut());
    }
    return Mutability::Const;
  }

private:
  WasmEdge::AST::GlobalType *Content = nullptr;
  bool IsOwn = false;

  friend class GlobalInstanceContext;
};

class ImportType::ImportTypeContext : public WasmEdge::AST::ImportDesc {};
class ExportType::ExportTypeContext : public WasmEdge::AST::ExportDesc {};

class Async::AsyncContext
    : public WasmEdge::VM::Async<WasmEdge::Expect<
          std::vector<std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>>> {
  template <typename... Args>
  AsyncContext(Args &&... Vals)
      : WasmEdge::VM::Async(std::forward<Args>(Vals)...) {}
};

class Configuration::ConfigureContext : public WasmEdge::Configure {};
class StatisticsContext : public Statistics {
public:
  StatisticsContext()
      : Content(new WasmEdge::Statistics::Statistics), IsOwn(true) {}
  StatisticsContext(WasmEdge::Statistics::Statistics *Content)
      : Content(Content) {}
  StatisticsContext(const StatisticsContext &StatCxt)
      : Content(StatCxt.Content) {}
  StatisticsContext(StatisticsContext &&StatCxt)
      : Content(StatCxt.Content), IsOwn(StatCxt.IsOwn) {
    StatCxt.IsOwn = false;
  }

  ~StatisticsContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  uint64_t GetInstrCount() {
    if (Content) {
      return Content->getInstrCount();
    }
    return 0;
  }

  double GetInstrPerSecond() {
    if (Content) {
      return Content->getInstrPerSecond();
    }
    return 0.0;
  }

  uint64_t GetTotalCost() {
    if (Content) {
      return Content->getTotalCost();
    }
    return 0;
  }

  void SetCostTable(std::vector<uint64_t> &CostArr) {
    if (Content) {
      Content->setCostTable(CostArr);
    }
  }

  void SetCostLimit(const uint64_t Limit) {
    if (Content) {
      Content->setCostLimit(Limit);
    }
  }

  void Clear() {
    if (Content) {
      Content->clear();
    }
  }

  WasmEdge::Statistics::Statistics *GetPointer() { return Content; }

private:
  WasmEdge::Statistics::Statistics *Content = nullptr;
  bool IsOwn = false;
};

class CallingFrameContext : public CallingFrame {
public:
  CallingFrameContext(const WasmEdge::Runtime::CallingFrame *Cxt)
      : Content(const_cast<WasmEdge::Runtime::CallingFrame *>(Cxt)) {}
  ~CallingFrameContext() = default;

private:
  WasmEdge::Runtime::CallingFrame *Content = nullptr;
};

class CPPAPIHostFunc : public WasmEdge::Runtime::HostFunctionBase {
public:
  CPPAPIHostFunc(const AST::FunctionType *Type,
                 FunctionInstance::HostFunc_t FuncPtr, void *ExtData,
                 const uint64_t FuncCost = 0) noexcept
      : Runtime::HostFunctionBase(FuncCost), Func(FuncPtr), Wrap(nullptr),
        Binding(nullptr), Data(ExtData) {
    FuncType = *Type;
  }
  CPPAPIHostFunc(const AST::FunctionType *Type,
                 FunctionInstance::WrapFunc_t WrapPtr, void *BindingPtr,
                 void *ExtData, const uint64_t FuncCost = 0) noexcept
      : Runtime::HostFunctionBase(FuncCost), Func(nullptr), Wrap(WrapPtr),
        Binding(BindingPtr), Data(ExtData) {
    FuncType = *Type;
  }
  ~CPPAPIHostFunc() noexcept override = default;

  Expect<void> run(const Runtime::CallingFrame &CallFrame,
                   Span<const ValVariant> Args,
                   Span<ValVariant> Rets) override {
    std::vector<Value> Params(FuncType.getParamTypes().size()),
        Returns(FuncType.getReturnTypes().size());
    for (uint32_t I = 0; I < Args.size(); I++) {
      Params.emplace_back(Args[I].get<WasmEdge::uint128_t>(),
                          static_cast<ValType>(FuncType.getParamTypes()[I]));
    }
    CallingFrameContext CallFrameCxt(&CallFrame);
    Result *Stat;
    if (Func) {
      *Stat = Func(Data, CallFrameCxt, Params, Returns);
    } else {
      *Stat = Wrap(Binding, Data, CallFrameCxt, Params, Returns);
    }
    for (uint32_t I = 0; I < Rets.size(); I++) {
      Rets[I] = to_WasmEdge_128_t<WasmEdge::uint128_t>(
          Value::ValueUtils::GetValue(Returns[I]));
    }
    if (Stat->IsOk()) {
      if (Stat->GetCode() == 0x01U) {
        return Unexpect(ErrCode::Value::Terminated);
      }
    } else {
      return Unexpect(static_cast<ErrCategory>(Stat->GetCategory()),
                      Stat->GetCode());
    }
    return {};
  }

private:
  FunctionInstance::HostFunc_t Func;
  FunctionInstance::WrapFunc_t Wrap;
  void *Binding;
  void *Data;
};

class FunctionInstanceContext : public FunctionInstance {
public:
  FunctionInstanceContext(const FunctionTypeContext &Type, HostFunc_t HostFunc,
                          void *Data, const uint64_t Cost) {
    if (HostFunc) {
      Content = new WasmEdge::Runtime::Instance::FunctionInstance(
          nullptr,
          std::make_unique<CPPAPIHostFunc>(Type.Content, HostFunc, Data, Cost));
      IsOwn = true;
    }
  }

  FunctionInstanceContext(const FunctionTypeContext &Type, WrapFunc_t WrapFunc,
                          void *Binding, void *Data, const uint64_t Cost) {
    if (WrapFunc) {
      Content = new WasmEdge::Runtime::Instance::FunctionInstance(
          nullptr, std::make_unique<CPPAPIHostFunc>(Type.Content, WrapFunc,
                                                    Binding, Data, Cost));
      IsOwn = true;
    }
  }

  FunctionInstanceContext(const Runtime::Instance::FunctionInstance *Inst)
      : Content(const_cast<Runtime::Instance::FunctionInstance *>(Inst)) {}

  ~FunctionInstanceContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  const FunctionTypeContext GetFunctionTypeContext() {
    if (Content) {
      return FunctionTypeContext(
          const_cast<AST::FunctionType *>(&Content->getFuncType()));
    }
    return FunctionTypeContext(nullptr);
  }

  const Runtime::Instance::FunctionInstance *GetConstContent() const {
    return Content;
  }

private:
  WasmEdge::Runtime::Instance::FunctionInstance *Content = nullptr;
  bool IsOwn = false;

  friend class ModuleInstanceContext;
};

class TableInstanceContext : public TableInstance {
public:
  TableInstanceContext(Runtime::Instance::TableInstance *Inst)
      : Content(Inst) {}

  TableInstanceContext(const TableTypeContext &TabType) {
    Content = new WasmEdge::Runtime::Instance::TableInstance(*TabType.Content);
    IsOwn = true;
  }

  ~TableInstanceContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  const TableTypeContext GetTableTypeContext() {
    if (Content) {
      TableTypeContext(const_cast<AST::TableType *>(&Content->getTableType()));
    }
    return TableTypeContext(nullptr);
  }

  Result GetData(Value &Data, const uint32_t Offset) {
    return wrap(
        [&]() { return Content->getRefAddr(Offset); },
        [&](auto &&Res) {
          Value::ValueUtils::FillValue(
              Data, Res->template get<WasmEdge::UnknownRef>(),
              static_cast<ValType>(Content->getTableType().getRefType()));
        },
        Content);
  }

  Result SetData(Value &Data, const uint32_t Offset) {
    return wrap(
        [&]() -> WasmEdge::Expect<void> {
          WasmEdge::RefType expType = Content->getTableType().getRefType();
          if (expType != static_cast<WasmEdge::RefType>(
                             Value::ValueUtils::GetValue(Data))) {
            spdlog::error(WasmEdge::ErrCode::Value::RefTypeMismatch);
            spdlog::error(WasmEdge::ErrInfo::InfoMismatch(
                static_cast<WasmEdge::ValType>(expType),
                static_cast<WasmEdge::ValType>(
                    Value::ValueUtils::GetValueType(Data))));
            return Unexpect(WasmEdge::ErrCode::Value::RefTypeMismatch);
          }
          return Content->setRefAddr(
              Offset,
              WasmEdge::ValVariant(to_WasmEdge_128_t<WasmEdge::uint128_t>(
                                       Value::ValueUtils::GetValue(Data)))
                  .get<UnknownRef>());
        },
        EmptyThen, Content);
  }

  uint32_t GetSize() {
    if (Content) {
      return Content->getSize();
    }
    return 0;
  }

  Result Grow(const uint32_t Size) {
    return wrap(
        [&]() -> WasmEdge::Expect<void> {
          if (Content->growTable(Size)) {
            return {};
          } else {
            spdlog::error(WasmEdge::ErrCode::Value::TableOutOfBounds);
            return WasmEdge::Unexpect(
                WasmEdge::ErrCode::Value::TableOutOfBounds);
          }
        },
        EmptyThen, Content);
  }

private:
  WasmEdge::Runtime::Instance::TableInstance *Content = nullptr;
  bool IsOwn = false;

  friend class ModuleInstanceContext;
};

class MemoryInstanceContext : public MemoryInstance {
public:
  MemoryInstanceContext(Runtime::Instance::MemoryInstance *Inst)
      : Content(Inst) {}

  MemoryInstanceContext(const MemoryTypeContext &MemType) {
    Content = new WasmEdge::Runtime::Instance::MemoryInstance(*MemType.Content);
    IsOwn = true;
  }

  ~MemoryInstanceContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  const MemoryTypeContext GetMemoryTypeContext() {
    if (Content) {
      return MemoryTypeContext(
          const_cast<AST::MemoryType *>(&Content->getMemoryType()));
    }
    return MemoryTypeContext(nullptr);
  }

  Result GetData(std::vector<uint8_t> &Data, const uint32_t Offset,
                 const uint32_t Length) {
    return wrap(
        [&]() { return Content->getBytes(Offset, Length); },
        [&](auto &&Res) { std::copy_n((*Res).begin(), Length, Data.begin()); },
        Content);
  }

  Result SetData(const std::vector<uint8_t> &Data, const uint32_t Offset) {
    return wrap(
        [&]() { return Content->setBytes(Data, Offset, 0, Data.size()); },
        EmptyThen, Content);
  }

  // TODO: Is returning only `uint8_t *` simpler?
  std::vector<uint8_t> GetReference(const uint32_t Offset,
                                    const uint32_t Length) {
    std::vector<uint8_t> ref;
    if (Content) {
      auto *ptr = Content->getPointer<uint8_t *>(Offset, Length);
      ref.insert(ref.end(), ptr, ptr + Length);
    }
    return ref;
  }

  const std::vector<uint8_t> GetReferenceConst(const uint32_t Offset,
                                               const uint32_t Length) {
    std::vector<uint8_t> ref;
    if (Content) {
      auto *ptr = Content->getPointer<const uint8_t *>(Offset, Length);
      ref.insert(ref.end(), ptr, ptr + Length);
    }
    return ref;
  }

  uint32_t GetPageSize() {
    if (Content) {
      return Content->getPageSize();
    }
    return 0;
  }

  Result GrowPage(const uint32_t Page) {
    return wrap(
        [&]() -> WasmEdge::Expect<void> {
          if (Content->growPage(Page)) {
            return {};
          } else {
            spdlog::error(WasmEdge::ErrCode::Value::MemoryOutOfBounds);
            return WasmEdge::Unexpect(
                WasmEdge::ErrCode::Value::MemoryOutOfBounds);
          }
        },
        EmptyThen, Content);
  }

private:
  WasmEdge::Runtime::Instance::MemoryInstance *Content = nullptr;
  bool IsOwn = false;

  friend class ModuleInstanceContext;
};

class GlobalInstanceContext : public GlobalInstance {
public:
  GlobalInstanceContext(Runtime::Instance::GlobalInstance *Inst)
      : Content(Inst) {}

  GlobalInstanceContext(const GlobalTypeContext &GlobType, const Value &Val) {
    Content = new WasmEdge::Runtime::Instance::GlobalInstance(
        *GlobType.Content, to_WasmEdge_128_t<WasmEdge::uint128_t>(
                               Value::ValueUtils::GetValue(Val)));
    IsOwn = true;
  }

  ~GlobalInstanceContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  const GlobalTypeContext GetGlobalTypeContext() {
    if (Content) {
      return GlobalTypeContext(
          const_cast<AST::GlobalType *>(&Content->getGlobalType()));
    }
    return GlobalTypeContext(nullptr);
  }

  Value GetValue() {
    if (Content) {
      return Value::ValueUtils::GenValueFromValVariant(
          Content->getValue(),
          static_cast<ValType>(Content->getGlobalType().getValType()));
    }
    auto Val = WasmEdge::ValVariant(static_cast<WasmEdge::uint128_t>(0));
    return Value::ValueUtils::GenValueFromValVariant(Val, ValType::I32);
  }

  void SetValue(const Value &Val) {
    if (Content &&
        Content->getGlobalType().getValMut() == WasmEdge::ValMut::Var &&
        static_cast<WasmEdge::ValType>(Value::ValueUtils::GetValueType(Val)) ==
            Content->getGlobalType().getValType()) {
      Content->getValue() = to_WasmEdge_128_t<WasmEdge::uint128_t>(
          Value::ValueUtils::GetValue(Val));
    }
  }

private:
  WasmEdge::Runtime::Instance::GlobalInstance *Content = nullptr;
  bool IsOwn = false;

  friend class ModuleInstanceContext;
};

class ModuleInstanceContext : public ModuleInstance {
public:
  ModuleInstanceContext(const Runtime::Instance::ModuleInstance *Inst)
      : Content(const_cast<Runtime::Instance::ModuleInstance *>(Inst)) {}
  ModuleInstanceContext(const std::string &ModuleName)
      : Content(new WasmEdge::Runtime::Instance::ModuleInstance(ModuleName)),
        IsOwn(true) {}
  ModuleInstanceContext(const std::vector<const std::string> &Args,
                        const std::vector<const std::string> &Envs,
                        const std::vector<const std::string> &Preopens) {
    this->Content = new WasmEdge::Host::WasiModule();
    this->IsOwn = true;
    this->InitWASI(Args, Envs, Preopens);
  }

  // Copy Constructor
  ModuleInstanceContext(const ModuleInstanceContext &ModInst)
      : Content(ModInst.Content), IsOwn(false) {}

  // Move Constructor
  ModuleInstanceContext(ModuleInstanceContext &&ModInst)
      : Content(ModInst.Content), IsOwn(ModInst.IsOwn) {
    ModInst.IsOwn = false;
  }

  ~ModuleInstanceContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  // Internal use
  void OwnModuleInstPointer(Runtime::Instance::ModuleInstance *Inst) {
    if (IsOwn) {
      delete Content;
    }
    Content = Inst;
    IsOwn = true;
  }

  const Runtime::Instance::ModuleInstance *GetConstContent() const {
    return Content;
  }

  void InitWASI(const std::vector<const std::string> &Args,
                const std::vector<const std::string> &Envs,
                const std::vector<const std::string> &Preopens) {
    auto *WasiMod = dynamic_cast<WasmEdge::Host::WasiModule *>(Content);
    if (!WasiMod) {
      return;
    }
    std::string ProgName;
    if (Args.size() > 0) {
      ProgName = Args[0];
    }
    auto &WasiEnv = WasiMod->getEnv();
    WasiEnv.init(Preopens, ProgName, Args, Envs);
  }

  uint32_t WASIGetExitCode() {
    auto *WasiMod = dynamic_cast<const WasmEdge::Host::WasiModule *>(Content);
    if (!WasiMod) {
      return EXIT_FAILURE;
    }
    return WasiMod->getEnv().getExitCode();
  }

  uint32_t WASIGetNativeHandler(int32_t Fd, uint64_t &NativeHandler) {
    auto *WasiMod = dynamic_cast<const WasmEdge::Host::WasiModule *>(Content);
    if (!WasiMod) {
      return 2;
    }
    auto Handler = WasiMod->getEnv().getNativeHandler(Fd);
    if (!Handler) {
      return 2;
    }
    NativeHandler = *Handler;
    return 0;
  }

  void InitWasmEdgeProcess(const std::vector<const std::string> &AllowedCmds,
                           const bool AllowAll) {
    using namespace std::literals::string_view_literals;
    if (const auto *Plugin =
            WasmEdge::Plugin::Plugin::find("wasmedge_process"sv)) {
      WasmEdge::PO::ArgumentParser Parser;
      Plugin->registerOptions(Parser);
      Parser.set_raw_value<std::vector<std::string>>(
          "allow-command"sv,
          std::vector<std::string>(AllowedCmds.begin(), AllowedCmds.end()));
      Parser.set_raw_value<bool>("allow-command-all"sv, AllowAll);
    }
  }

  std::string GetModuleName() {
    if (Content) {
      std::string str(Content->getModuleName());
      return str;
    }
    return "";
  }

  FunctionInstanceContext FindFunctionInstance(const std::string &Name) {
    if (Content) {
      return FunctionInstanceContext(Content->findFuncExports(Name));
    }
    return FunctionInstanceContext(nullptr);
  }

  TableInstanceContext FindTableInstance(const std::string &Name) {
    if (Content) {
      return TableInstanceContext(Content->findTableExports(Name));
    }
    return TableInstanceContext(nullptr);
  }

  MemoryInstanceContext FindMemoryInstance(const std::string &Name) {
    if (Content) {
      return MemoryInstanceContext(Content->findMemoryExports(Name));
    }
    return MemoryInstanceContext(nullptr);
  }

  GlobalInstanceContext FindGlobalInstance(const std::string &Name) {
    if (Content) {
      return GlobalInstanceContext(Content->findGlobalExports(Name));
    }
    return GlobalInstanceContext(nullptr);
  }

  std::vector<std::string> ListFunction() {
    std::vector<std::string> str;
    if (Content) {
      Content->getFuncExports([&](auto &Map) {
        for (auto &&Pair : Map) {
          str.emplace_back(Pair.first);
        }
      });
    }
    return str;
  }

  std::vector<std::string> ListTable() {
    std::vector<std::string> str;
    if (Content) {
      Content->getTableExports([&](auto &Map) {
        for (auto &&Pair : Map) {
          str.emplace_back(Pair.first);
        }
      });
    }
    return str;
  }

  std::vector<std::string> ListMemory() {
    std::vector<std::string> str;
    if (Content) {
      Content->getMemoryExports([&](auto &Map) {
        for (auto &&Pair : Map) {
          str.emplace_back(Pair.first);
        }
      });
    }
    return str;
  }

  std::vector<std::string> ListGlobal() {
    std::vector<std::string> str;
    if (Content) {
      Content->getGlobalExports([&](auto &Map) {
        for (auto &&Pair : Map) {
          str.emplace_back(Pair.first);
        }
      });
    }
    return str;
  }

  void AddFunction(const std::string &Name, FunctionInstanceContext &&FuncCxt) {
    if (Content) {
      Content->addHostFunc(
          Name, std::unique_ptr<WasmEdge::Runtime::Instance::FunctionInstance>(
                    FuncCxt.Content));
    }
  }

  void AddTable(const std::string &Name, TableInstanceContext &&TableCxt) {
    if (Content) {
      Content->addHostTable(
          Name, std::unique_ptr<WasmEdge::Runtime::Instance::TableInstance>(
                    TableCxt.Content));
    }
  }

  void AddMemory(const std::string &Name, MemoryInstanceContext &&MemoryCxt) {
    if (Content) {
      Content->addHostMemory(
          Name, std::unique_ptr<WasmEdge::Runtime::Instance::MemoryInstance>(
                    MemoryCxt.Content));
    }
  }

  void AddGlobal(const std::string &Name, GlobalInstanceContext &&GlobalCxt) {
    if (Content) {
      Content->addHostGlobal(
          Name, std::unique_ptr<WasmEdge::Runtime::Instance::GlobalInstance>(
                    GlobalCxt.Content));
    }
  }

private:
  WasmEdge::Runtime::Instance::ModuleInstance *Content = nullptr;
  bool IsOwn = false;
};

class LoaderContext : public Loader {
public:
  LoaderContext(const WasmEdge::Configure &ConfCxt)
      : Content(new WasmEdge::Loader::Loader(
            ConfCxt, &WasmEdge::Executor::Executor::Intrinsics)),
        IsOwn(true) {}
  LoaderContext(const LoaderContext &LoadCxt) : Content(LoadCxt.Content) {}
  LoaderContext(LoaderContext &&LoadCxt)
      : Content(LoadCxt.Content), IsOwn(LoadCxt.IsOwn) {
    LoadCxt.IsOwn = false;
  }

  Result Parse(ASTModuleContext &Module, const std::string &Path) {
    return wrap(
        [&]() { return Content->parseModule(std::filesystem::absolute(Path)); },
        [&](auto &&Res) { Module.OwnASTModulePointer((*Res).release()); },
        Content);
  }

  Result Parse(ASTModuleContext &Module, const std::vector<uint8_t> &Buf) {
    return wrap(
        [&]() { return Content->parseModule(Buf); },
        [&](auto &&Res) { Module.OwnASTModulePointer((*Res).release()); },
        Content);
  }

private:
  WasmEdge::Loader::Loader *Content = nullptr;
  bool IsOwn = false;
};

class ValidatorContext : public Validator {
public:
  ValidatorContext(WasmEdge::Configure &Conf)
      : Content(new WasmEdge::Validator::Validator(Conf)), IsOwn(true) {}
  ValidatorContext(WasmEdge::Validator::Validator *Content)
      : Content(Content) {}
  ValidatorContext(const ValidatorContext &ValidCxt)
      : Content(ValidCxt.Content) {}
  ValidatorContext(ValidatorContext &&ValidCxt)
      : Content(ValidCxt.Content), IsOwn(ValidCxt.IsOwn) {
    ValidCxt.IsOwn = false;
  }

  Result Validate(const ASTModuleContext &ASTCxt) {
    return wrap([&]() { return Content->validate(*ASTCxt.GetConstPointer()); },
                EmptyThen, Content);
  }

private:
  WasmEdge::Validator::Validator *Content = nullptr;
  bool IsOwn = false;
};

class ExecutorContext : public Executor {
public:
  ExecutorContext(WasmEdge::Configure &Conf,
                  WasmEdge::Statistics::Statistics *Stat)
      : Content(new WasmEdge::Executor::Executor(Conf, Stat)), IsOwn(true) {}
  ExecutorContext(WasmEdge::Executor::Executor *Content) : Content(Content) {}
  ExecutorContext(const ExecutorContext &ExecCxt) : Content(ExecCxt.Content) {}
  ExecutorContext(ExecutorContext &&ExecCxt)
      : Content(ExecCxt.Content), IsOwn(ExecCxt.IsOwn) {
    ExecCxt.IsOwn = false;
  }

  ~ExecutorContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  Result Instantiate(ModuleInstance &ModuleCxt, Store &StoreCxt,
                     const ASTModuleContext &ASTCxt) {
    return wrap(
        [&]() {
          return Content->instantiateModule(
              *static_cast<StoreContext *>(&StoreCxt)->GetContentPtr(),
              *ASTCxt.GetConstPointer());
        },
        [&](auto &&Res) {
          static_cast<ModuleInstanceContext *>(&ModuleCxt)
              ->OwnModuleInstPointer((*Res).release());
        },
        Content);
  }

  Result Register(ModuleInstance &ModuleCxt, Store &StoreCxt,
                  const ASTModuleContext &ASTCxt,
                  const std::string &ModuleName) {
    return wrap(
        [&]() {
          return Content->registerModule(
              *static_cast<StoreContext *>(&StoreCxt)->GetContentPtr(),
              *ASTCxt.GetConstPointer(), std::string_view(ModuleName));
        },
        [&](auto &&Res) {
          static_cast<ModuleInstanceContext *>(&ModuleCxt)
              ->OwnModuleInstPointer((*Res).release());
        },
        Content);
  }

  Result Register(Store &StoreCxt, const ModuleInstance &ImportCxt) {
    return wrap(
        [&]() {
          return Content->registerModule(
              *static_cast<StoreContext *>(&StoreCxt)->GetContentPtr(),
              *static_cast<const ModuleInstanceContext *>(&ImportCxt)
                   ->GetConstContent());
        },
        EmptyThen, Content);
  }

  Result Invoke(const FunctionInstance &FuncCxt,
                const std::vector<Value> &Params, std::vector<Value> &Returns) {
    auto ParamPair = Value::ValueUtils::GenParamPair(Params);
    return wrap(
        [&]() -> WasmEdge::Expect<std::vector<
                  std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>> {
          return Content->invoke(
              *static_cast<const FunctionInstanceContext *>(&FuncCxt)
                   ->GetConstContent(),
              ParamPair.first, ParamPair.second);
        },
        [&](auto &&Res) { Value::ValueUtils::FillValueArr(*Res, Returns); },
        Content);
  }

private:
  WasmEdge::Executor::Executor *Content = nullptr;
  bool IsOwn = false;
};

class StoreContext : public Store {
public:
  StoreContext() : Content(new WasmEdge::Runtime::StoreManager), IsOwn(true) {}
  StoreContext(WasmEdge::Runtime::StoreManager *Cxt) : Content(Cxt) {}
  StoreContext(StoreContext &&Cxt) : Content(Cxt.Content), IsOwn(Cxt.IsOwn) {
    Cxt.IsOwn = false;
  }
  StoreContext(const StoreContext &Cxt) : Content(Cxt.Content) {}

  ~StoreContext() {
    if (IsOwn) {
      delete Content;
    }
  }

  const ModuleInstanceContext FindModuleContext(const std::string &Name) {
    if (Content) {
      return ModuleInstanceContext(Content->findModule(Name));
    }
    return ModuleInstanceContext(nullptr);
  }

  WasmEdge::Runtime::StoreManager *GetContentPtr() { return Content; }

  std::vector<std::string> ListModule() {
    std::vector<std::string> ModuleList;
    if (Content) {
      Content->getModuleList([&](auto &Map) {
        for (auto &&Pair : Map) {
          ModuleList.emplace_back(Pair.first);
        }
      });
    }
    return ModuleList;
  }

private:
  WasmEdge::Runtime::StoreManager *Content = nullptr;
  bool IsOwn = false;
};

class VM::VMContext : public WasmEdge::VM::VM {};

// <<<<<<<< WasmEdge Context members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Version members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

std::string Version::Get() { return WASMEDGE_VERSION; }

uint32_t Version::GetMajor() { return WASMEDGE_VERSION_MAJOR; }

uint32_t Version::GetMinor() { return WASMEDGE_VERSION_MINOR; }

uint32_t Version::GetPatch() { return WASMEDGE_VERSION_PATCH; }

// <<<<<<<< WasmEdge Version members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Log members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void Log::SetErrorLevel() { WasmEdge::Log::setErrorLoggingLevel(); }

void Log::SetDebugLevel() { WasmEdge::Log::setDebugLoggingLevel(); }

void Log::Off() { WasmEdge::Log::setLogOff(); }

// <<<<<<<< WasmEdge Log members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Value members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Value::Value(const int32_t Val) : Val(to_uint128_t(Val)), Type(ValType::I32) {}

Value::Value(const int64_t Val) : Val(to_uint128_t(Val)), Type(ValType::I64) {}

Value::Value(const float Val) : Val(to_uint128_t(Val)), Type(ValType::F32) {}

Value::Value(const double Val) : Val(to_uint128_t(Val)), Type(ValType::F64) {}

Value::Value(const ::int128_t Val)
    : Val(to_WasmEdge_128_t<WasmEdge::int128_t>(Val)),
      Type(static_cast<ValType>(WasmEdge::ValTypeFromType<::int128_t>())) {}

Value::Value(const RefType ValT)
    : Val(to_uint128_t(WasmEdge::UnknownRef())),
      Type(static_cast<ValType>(ValT)) {}

Value::Value(const FunctionInstance &Cxt) {
  auto ValV = WasmEdge::FuncRef(
      static_cast<const FunctionInstanceContext *>(&Cxt)->GetConstContent());
  this->Val = to_uint128_t(WasmEdge::ValVariant(ValV).unwrap());
  this->Type = ValType::FuncRef;
}

Value::Value(std::shared_ptr<void> ExtRef)
    : Val(to_uint128_t(WasmEdge::ExternRef(ExtRef.get()))),
      Type(ValType::ExternRef) {}

int32_t Value::GetI32() {
  return WasmEdge::ValVariant::wrap<int32_t>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val))
      .get<int32_t>();
}

int64_t Value::GetI64() {
  return WasmEdge::ValVariant::wrap<int64_t>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val))
      .get<int64_t>();
}

float Value::GetF32() {
  return WasmEdge::ValVariant::wrap<float>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val))
      .get<float>();
}

double Value::GetF64() {
  return WasmEdge::ValVariant::wrap<double>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val))
      .get<double>();
}

int128_t Value::GetV128() {
  return to_int128_t(WasmEdge::ValVariant::wrap<WasmEdge::int128_t>(
                         to_WasmEdge_128_t<WasmEdge::uint128_t>(Val))
                         .get<WasmEdge::int128_t>());
}

const FunctionInstance Value::GetFuncRef() {
  return FunctionInstanceContext(
      WasmEdge::retrieveFuncRef(WasmEdge::ValVariant::wrap<WasmEdge::FuncRef>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val))));
}

std::shared_ptr<void> Value::GetExternRef() {
  auto ExternRef = WasmEdge::retrieveExternRef<uint32_t>(
      WasmEdge::ValVariant::wrap<WasmEdge::ExternRef>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val)));
  return std::make_shared<void>(ExternRef);
}

bool Value::IsNullRef() {
  return WasmEdge::isNullRef(WasmEdge::ValVariant::wrap<WasmEdge::UnknownRef>(
      to_WasmEdge_128_t<WasmEdge::uint128_t>(Val)));
}

// <<<<<<<< WasmEdge Value members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Result members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class Result::ResultFactory {
  ResultFactory() = default;
  ~ResultFactory() = default;

public:
  static Result GenResult(const WasmEdge::ErrCode::Value &Code) {
    return Result{static_cast<uint32_t>(Code) & 0x00FFFFFFU};
  }

  static Result GenResult(const WasmEdge::ErrCode &Code) {
    return Result{Code.operator uint32_t()};
  }
};

Result::Result(const ErrCategory Category, const uint32_t Code) {
  this->Code = (static_cast<uint32_t>(Category) << 24) + (Code & 0x00FFFFFFU);
}

bool Result::IsOk() {
  if (GetCategory() == ErrCategory::WASM &&
      (static_cast<WasmEdge::ErrCode::Value>(GetCode()) ==
           WasmEdge::ErrCode::Value::Success ||
       static_cast<WasmEdge::ErrCode::Value>(GetCode()) ==
           WasmEdge::ErrCode::Value::Terminated)) {
    return true;
  } else {
    return false;
  }
}

uint32_t Result::GetCode() { return Code & 0x00FFFFFFU; }

ErrCategory Result::GetCategory() {
  return static_cast<ErrCategory>(Code >> 24);
}

const std::string Result::GetMessage() {
  if (GetCategory() != ErrCategory::WASM) {
    auto str = WasmEdge::ErrCodeStr[WasmEdge::ErrCode::Value::UserDefError];
    return std::string{str};
  }
  auto str =
      WasmEdge::ErrCodeStr[static_cast<WasmEdge::ErrCode::Value>(GetCode())];
  return std::string{str};
}

// <<<<<<<< WasmEdge Result members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Data Structures >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

bool Limit::operator==(const Limit &Lim) {
  return this->HasMax == Lim.HasMax && this->Shared == Lim.Shared &&
         this->Min == Lim.Min && this->Max == Lim.Max;
}

// >>>>>>>> WasmEdge FunctionType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

FunctionType FunctionType::New(const std::vector<ValType> &ParamList,
                               const std::vector<ValType> &ReturnList) {
  return FunctionTypeContext(ParamList, ReturnList);
}

const std::vector<ValType> FunctionType::GetParameters() {
  return static_cast<FunctionTypeContext *>(this)->GetParameters();
}

const std::vector<ValType> FunctionType::GetReturns() {
  return static_cast<FunctionTypeContext *>(this)->GetReturns();
}

// <<<<<<<< WasmEdge FunctionType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge TableType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

TableType TableType::New(const RefType RefT, const Limit &Lim) {
  return TableTypeContext(RefT, Lim);
}

RefType TableType::GetRefType() {
  return static_cast<TableTypeContext *>(this)->GetRefType();
}

const Limit TableType::GetLimit() {
  return static_cast<TableTypeContext *>(this)->GetLimit();
}

// <<<<<<<< WasmEdge TableType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge MemoryType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

MemoryType MemoryType::New(const Limit &Lim) { return MemoryTypeContext(Lim); }

const Limit MemoryType::GetLimit() {
  return static_cast<MemoryTypeContext *>(this)->GetLimit();
}

// <<<<<<<< WasmEdge MemoryType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge GlobalType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

GlobalType GlobalType::New(const ValType ValT, const Mutability Mut) {
  return GlobalTypeContext(ValT, Mut);
}

ValType GlobalType::GetValType() {
  return static_cast<GlobalTypeContext *>(this)->GetValType();
}

Mutability GlobalType::GetMutability() {
  return static_cast<GlobalTypeContext *>(this)->GetMutability();
}

// <<<<<<<< WasmEdge GlobalType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge ImportType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

ImportType::ImportType(ImportType::ImportTypeContext &Cxt) : Cxt(Cxt) {}

ExternalType ImportType::GetExternalType() {
  return static_cast<ExternalType>(Cxt.getExternalType());
}

std::string ImportType::GetModuleName() {
  std::string str{Cxt.getModuleName()};
  return str;
}

std::string ImportType::GetExternalName() {
  std::string str{Cxt.getExternalName()};
  return str;
}

const FunctionType ImportType::GetFunctionType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Function) {
    uint32_t Idx = Cxt.getExternalFuncTypeIdx();
    const auto &FuncTypes = static_cast<const ASTModuleContext *>(&ASTCxt)
                                ->GetConstPointer()
                                ->getTypeSection()
                                .getContent();
    if (Idx >= FuncTypes.size())
      return FunctionTypeContext(nullptr);

    return FunctionTypeContext(&FuncTypes[Idx]);
  }
  return FunctionTypeContext(nullptr);
}

const TableType ImportType::GetTableType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Table) {
    return TableTypeContext(&Cxt.getExternalTableType());
  }
  return TableTypeContext(nullptr);
}

const MemoryType ImportType::GetMemoryType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Memory) {
    return MemoryTypeContext(&Cxt.getExternalMemoryType());
  }
  return MemoryTypeContext(nullptr);
}

const GlobalType ImportType::GetGlobalType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Global) {
    return GlobalTypeContext(&Cxt.getExternalGlobalType());
  }
  return GlobalTypeContext(nullptr);
}

// <<<<<<<< WasmEdge ImportType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge ExportType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

ExportType::ExportType(ExportType::ExportTypeContext &Cxt) : Cxt(Cxt) {}

ExternalType ExportType::GetExternalType() {
  return static_cast<ExternalType>(Cxt.getExternalType());
}

std::string ExportType::GetExternalName() {
  std::string str{Cxt.getExternalName()};
  return str;
}

const FunctionType ExportType::GetFunctionType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Function) {
    // `external_index` = `func_index` + `import_func_nums`
    uint32_t ExtIdx = Cxt.getExternalIndex();
    const auto &ImpDescs = static_cast<const ASTModuleContext *>(&ASTCxt)
                               ->GetConstPointer()
                               ->getImportSection()
                               .getContent();
    for (auto &&ImpDesc : ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Function) {
        ExtIdx--;
      }
    }
    // Get the function type index by the function index
    const auto &FuncIdxs = static_cast<const ASTModuleContext *>(&ASTCxt)
                               ->GetConstPointer()
                               ->getFunctionSection()
                               .getContent();
    if (ExtIdx >= FuncIdxs.size()) {
      return FunctionTypeContext(nullptr);
    }
    uint32_t TypeIdx = FuncIdxs[ExtIdx];
    // Get the function type
    const auto &FuncTypes = static_cast<const ASTModuleContext *>(&ASTCxt)
                                ->GetConstPointer()
                                ->getTypeSection()
                                .getContent();
    if (TypeIdx >= FuncTypes.size()) {
      return FunctionTypeContext(nullptr);
    }

    return FunctionTypeContext(&FuncTypes[TypeIdx]);
  }
  return FunctionTypeContext(nullptr);
}

const TableType ExportType::GetTableType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Table) {
    // `external_index` = `table_type_index` + `import_table_nums`
    uint32_t ExtIdx = Cxt.getExternalIndex();
    const auto &ImpDescs = static_cast<const ASTModuleContext *>(&ASTCxt)
                               ->GetConstPointer()
                               ->getImportSection()
                               .getContent();
    for (auto &&ImpDesc : ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Table) {
        ExtIdx--;
      }
    }
    // Get the table type
    const auto &TabTypes = static_cast<const ASTModuleContext *>(&ASTCxt)
                               ->GetConstPointer()
                               ->getTableSection()
                               .getContent();
    if (ExtIdx >= TabTypes.size()) {
      return TableTypeContext(nullptr);
    }
    return TableTypeContext(&TabTypes[ExtIdx]);
  }
  return TableTypeContext(nullptr);
}

const MemoryType ExportType::GetMemoryType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Memory) {
    // `external_index` = `memory_type_index` + `import_memory_nums`
    uint32_t ExtIdx = Cxt.getExternalIndex();
    const auto &ImpDescs = static_cast<const ASTModuleContext *>(&ASTCxt)
                               ->GetConstPointer()
                               ->getImportSection()
                               .getContent();
    for (auto &&ImpDesc : ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Memory) {
        ExtIdx--;
      }
    }
    // Get the memory type
    const auto &MemTypes = static_cast<const ASTModuleContext *>(&ASTCxt)
                               ->GetConstPointer()
                               ->getMemorySection()
                               .getContent();
    if (ExtIdx >= MemTypes.size()) {
      return MemoryTypeContext(nullptr);
    }
    return MemoryTypeContext(&MemTypes[ExtIdx]);
  }
  return MemoryTypeContext(nullptr);
}

const GlobalType ExportType::GetGlobalType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Global) {
    // `external_index` = `global_type_index` + `import_global_nums`
    uint32_t ExtIdx = Cxt.getExternalIndex();
    const auto &ImpDescs = static_cast<const ASTModuleContext *>(&ASTCxt)
                               ->GetConstPointer()
                               ->getImportSection()
                               .getContent();
    for (auto &&ImpDesc : ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Global) {
        ExtIdx--;
      }
    }
    // Get the global type
    const auto &GlobDescs = static_cast<const ASTModuleContext *>(&ASTCxt)
                                ->GetConstPointer()
                                ->getGlobalSection()
                                .getContent();
    if (ExtIdx >= GlobDescs.size()) {
      return GlobalTypeContext(nullptr);
    }
    return GlobalTypeContext(&GlobDescs[ExtIdx].getGlobalType());
  }
  return GlobalTypeContext(nullptr);
}

// <<<<<<<< WasmEdge ExportType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// <<<<<<<< WasmEdge Data Structures <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Async >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

template <typename... Args> Async::Async(Args &&... Vals) {
  this->Cxt = std::make_unique<AsyncContext>();
}

void Async::Wait() { return Cxt->wait(); }

bool Async::WaitFor(uint64_t Milliseconds) {
  return Cxt->waitFor(std::chrono::milliseconds(Milliseconds));
}

void Async::Cancel() { Cxt->cancel(); }

Result Async::Get(std::vector<Value> &Returns) {
  auto Res = Cxt->get();
  if (Res) {
    for (uint32_t I = 0; I < Res->size(); I++) {
      auto Val = to_uint128_t(Res->at(I).first.unwrap());
      Returns[I] = Value(Val, static_cast<ValType>(Res->at(I).second));
    }
    return Result{static_cast<uint32_t>(ErrCode::Value::Success) & 0x00FFFFFFU};
  }
  return Result{static_cast<uint32_t>(Res.error()) & 0x00FFFFFFU};
}

// <<<<<<<< WasmEdge Async <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Configuration >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Configuration::Configuration() {
  this->Cxt = std::make_unique<Configuration::ConfigureContext>();
}

void Configuration::AddProposal(const Proposal Prop) {
  Cxt->addProposal(static_cast<WasmEdge::Proposal>(Prop));
}

void Configuration::RemoveProposal(const Proposal Prop) {
  Cxt->removeProposal(static_cast<WasmEdge::Proposal>(Prop));
}

bool Configuration::HasProposal(const Proposal Prop) {
  return Cxt->hasProposal(static_cast<WasmEdge::Proposal>(Prop));
}

void Configuration::AddHostRegistration(const HostRegistration Host) {
  Cxt->addHostRegistration(static_cast<WasmEdge::HostRegistration>(Host));
}

void Configuration::RemoveHostRegistration(const HostRegistration Host) {
  Cxt->removeHostRegistration(static_cast<WasmEdge::HostRegistration>(Host));
}

bool Configuration::HasHostRegistration(const HostRegistration Host) {
  return Cxt->hasHostRegistration(
      static_cast<WasmEdge::HostRegistration>(Host));
}

void Configuration::SetMaxMemoryPage(const uint32_t Page) {
  Cxt->getRuntimeConfigure().setMaxMemoryPage(Page);
}

uint32_t Configuration::GetMaxMemoryPage() {
  return Cxt->getRuntimeConfigure().getMaxMemoryPage();
}

void Configuration::SetForceInterpreter(const bool IsForceInterpreter) {
  Cxt->getRuntimeConfigure().setForceInterpreter(IsForceInterpreter);
}

bool Configuration::IsForceInterpreter() {
  return Cxt->getRuntimeConfigure().isForceInterpreter();
}

void Configuration::CompilerSetOptimizationLevel(
    const CompilerOptimizationLevel Level) {
  Cxt->getCompilerConfigure().setOptimizationLevel(
      static_cast<WasmEdge::CompilerConfigure::OptimizationLevel>(Level));
}

CompilerOptimizationLevel Configuration::CompilerGetOptimizationLevel() {
  return static_cast<CompilerOptimizationLevel>(
      Cxt->getCompilerConfigure().getOptimizationLevel());
}

void Configuration::CompilerSetOutputFormat(const CompilerOutputFormat Format) {
  Cxt->getCompilerConfigure().setOutputFormat(
      static_cast<WasmEdge::CompilerConfigure::OutputFormat>(Format));
}

CompilerOutputFormat Configuration::CompilerGetOutputFormat() {
  return static_cast<CompilerOutputFormat>(
      Cxt->getCompilerConfigure().getOutputFormat());
}

void Configuration::CompilerSetDumpIR(const bool IsDump) {
  Cxt->getCompilerConfigure().setDumpIR(IsDump);
}

bool Configuration::CompilerIsDumpIR() {
  return Cxt->getCompilerConfigure().isDumpIR();
}

void Configuration::CompilerSetGenericBinary(const bool IsGeneric) {
  Cxt->getCompilerConfigure().setGenericBinary(IsGeneric);
}

bool Configuration::CompilerIsGenericBinary() {
  return Cxt->getCompilerConfigure().isGenericBinary();
}

void Configuration::CompilerSetInterruptible(const bool IsInterruptible) {
  Cxt->getCompilerConfigure().setInterruptible(IsInterruptible);
}

bool Configuration::CompilerIsInterruptible() {
  return Cxt->getCompilerConfigure().isInterruptible();
}

void Configuration::StatisticsSetInstructionCounting(const bool IsCount) {
  Cxt->getStatisticsConfigure().setInstructionCounting(IsCount);
}

bool Configuration::StatisticsIsInstructionCounting() {
  return Cxt->getStatisticsConfigure().isInstructionCounting();
}

void Configuration::StatisticsSetCostMeasuring(const bool IsMeasure) {
  Cxt->getStatisticsConfigure().setCostMeasuring(IsMeasure);
}

bool Configuration::StatisticsIsCostMeasuring() {
  return Cxt->getStatisticsConfigure().isCostMeasuring();
}

void Configuration::StatisticsSetTimeMeasuring(const bool IsMeasure) {
  Cxt->getStatisticsConfigure().setTimeMeasuring(IsMeasure);
}

bool Configuration::StatisticsIsTimeMeasuring() {
  return Cxt->getStatisticsConfigure().isTimeMeasuring();
}

// <<<<<<<< WasmEdge Configuration <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Statistics >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Statistics Statistics::New() { return StatisticsContext(); }

Statistics Statistics::Move(Statistics &&StatCxt) {
  return StatisticsContext(static_cast<StatisticsContext &&>(StatCxt));
}

uint64_t Statistics::GetInstrCount() {
  return static_cast<StatisticsContext *>(this)->GetInstrCount();
}

double Statistics::GetInstrPerSecond() {
  return static_cast<StatisticsContext *>(this)->GetInstrPerSecond();
}

uint64_t Statistics::GetTotalCost() {
  return static_cast<StatisticsContext *>(this)->GetTotalCost();
}

void Statistics::SetCostTable(std::vector<uint64_t> &CostArr) {
  static_cast<StatisticsContext *>(this)->SetCostTable(CostArr);
}

void Statistics::SetCostLimit(const uint64_t Limit) {
  static_cast<StatisticsContext *>(this)->SetCostLimit(Limit);
}

void Statistics::Clear() { static_cast<StatisticsContext *>(this)->Clear(); }

// <<<<<<<< WasmEdge Statistics <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Runtime >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// >>>>>>>> WasmEdge Loader >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Loader Loader::New(const Configuration &ConfCxt) {
  return LoaderContext(*ConfCxt.Cxt.get());
}

Loader Loader::Move(Loader &&LoadCxt) {
  return LoaderContext(static_cast<LoaderContext &&>(LoadCxt));
}

Result Loader::Parse(ASTModule &Module, const std::string &Path) {
  return static_cast<LoaderContext *>(this)->Parse(
      *static_cast<ASTModuleContext *>(&Module), Path);
}

Result Loader::Parse(ASTModule &Module, const std::vector<uint8_t> &Buf) {
  return static_cast<LoaderContext *>(this)->Parse(
      *static_cast<ASTModuleContext *>(&Module), Buf);
}

// <<<<<<<< WasmEdge Loader <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Validator >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Validator Validator::New(Configuration &ConfCxt) {
  return ValidatorContext(*ConfCxt.Cxt.get());
}

Validator Validator::Move(Validator &&ValidCxt) {
  return ValidatorContext(static_cast<ValidatorContext &&>(ValidCxt));
}

Result Validator::Validate(const ASTModule &ASTCxt) {
  return static_cast<ValidatorContext *>(this)->Validate(
      *static_cast<const ASTModuleContext *>(&ASTCxt));
}

// <<<<<<<< WasmEdge Validator <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Executor >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Executor Executor::New(const Configuration &ConfCxt, Statistics &StatCxt) {
  return ExecutorContext(
      *ConfCxt.Cxt, static_cast<StatisticsContext &>(StatCxt).GetPointer());
}

Executor Executor::Move(Executor &&ExecCxt) {
  return ExecutorContext(static_cast<ExecutorContext &&>(ExecCxt));
}

Result Executor::Instantiate(ModuleInstance &ModuleCxt, Store &StoreCxt,
                             const ASTModule &ASTCxt) {
  return static_cast<ExecutorContext *>(this)->Instantiate(
      ModuleCxt, StoreCxt, *static_cast<const ASTModuleContext *>(&ASTCxt));
}

Result Executor::Register(ModuleInstance &ModuleCxt, Store &StoreCxt,
                          const ASTModule &ASTCxt,
                          const std::string &ModuleName) {
  return static_cast<ExecutorContext *>(this)->Register(
      ModuleCxt, StoreCxt, *static_cast<const ASTModuleContext *>(&ASTCxt),
      ModuleName);
}

Result Executor::Register(Store &StoreCxt, const ModuleInstance &ImportCxt) {
  return static_cast<ExecutorContext *>(this)->Register(StoreCxt, ImportCxt);
}

Result Executor::Invoke(const FunctionInstance &FuncCxt,
                        const std::vector<Value> &Params,
                        std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return static_cast<ExecutorContext *>(this)->Invoke(FuncCxt, Params, Returns);
}

// <<<<<<<< WasmEdge Executor <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge ASTModule >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

ASTModule ASTModule::New() { return ASTModuleContext(); }

ASTModule ASTModule::Move(ASTModule &&Module) {
  return ASTModuleContext(static_cast<ASTModuleContext &&>(Module));
}

std::vector<ImportType> ASTModule::ListImports() {
  return static_cast<ASTModuleContext *>(this)->ListImports();
}

std::vector<ExportType> ASTModule::ListExports() {
  return static_cast<ASTModuleContext *>(this)->ListExports();
}

// <<<<<<<< WasmEdge ASTModule <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Store >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Store Store::New() { return StoreContext(); }

Store Store::Move(Store &&StoreCxt) {
  return StoreContext(static_cast<StoreContext &&>(StoreCxt));
}

const ModuleInstance Store::FindModule(const std::string &Name) {
  return static_cast<StoreContext *>(this)->FindModuleContext(Name);
}

std::vector<std::string> Store::ListModule() {
  return static_cast<StoreContext *>(this)->ListModule();
}

// <<<<<<<< WasmEdge Store <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge FunctionInstance >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

FunctionInstance FunctionInstance::New(const FunctionType &Type,
                                       HostFunc_t HostFunc, void *Data,
                                       const uint64_t Cost) {
  return FunctionInstanceContext(static_cast<const FunctionTypeContext &>(Type),
                                 HostFunc, Data, Cost);
}

FunctionInstance FunctionInstance::New(const FunctionType &Type,
                                       WrapFunc_t WrapFunc, void *Binding,
                                       void *Data, const uint64_t Cost) {
  return FunctionInstanceContext(static_cast<const FunctionTypeContext &>(Type),
                                 WrapFunc, Binding, Data, Cost);
}

const FunctionType FunctionInstance::GetFunctionType() {
  return static_cast<FunctionInstanceContext *>(this)->GetFunctionTypeContext();
}

// <<<<<<<< WasmEdge FunctionInstance <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge TableInstance >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

TableInstance TableInstance::New(const TableType &TabType) {
  return TableInstanceContext(static_cast<const TableTypeContext &>(TabType));
}

const TableType TableInstance::GetTableType() {
  return static_cast<TableInstanceContext *>(this)->GetTableTypeContext();
}

Result TableInstance::GetData(Value &Data, const uint32_t Offset) {
  return static_cast<TableInstanceContext *>(this)->GetData(Data, Offset);
}

Result TableInstance::SetData(Value &Data, const uint32_t Offset) {
  return static_cast<TableInstanceContext *>(this)->SetData(Data, Offset);
}

uint32_t TableInstance::GetSize() {
  return static_cast<TableInstanceContext *>(this)->GetSize();
}

Result TableInstance::Grow(const uint32_t Size) {
  return static_cast<TableInstanceContext *>(this)->Grow(Size);
}

// <<<<<<<< WasmEdge TableInstance <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge MemoryInstance >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

MemoryInstance MemoryInstance::New(const MemoryType &MemType) {
  return MemoryInstanceContext(static_cast<const MemoryTypeContext &>(MemType));
}

const MemoryType MemoryInstance::GetMemoryType() {
  return static_cast<MemoryInstanceContext *>(this)->GetMemoryTypeContext();
}

Result MemoryInstance::GetData(std::vector<uint8_t> &Data,
                               const uint32_t Offset, const uint32_t Length) {
  return static_cast<MemoryInstanceContext *>(this)->GetData(Data, Offset,
                                                             Length);
}

Result MemoryInstance::SetData(const std::vector<uint8_t> &Data,
                               const uint32_t Offset) {
  return static_cast<MemoryInstanceContext *>(this)->SetData(Data, Offset);
}

std::vector<uint8_t> MemoryInstance::GetReference(const uint32_t Offset,
                                                  const uint32_t Length) {
  return static_cast<MemoryInstanceContext *>(this)->GetReference(Offset,
                                                                  Length);
}

const std::vector<uint8_t>
MemoryInstance::GetReferenceConst(const uint32_t Offset,
                                  const uint32_t Length) {
  return static_cast<MemoryInstanceContext *>(this)->GetReferenceConst(Offset,
                                                                       Length);
}

uint32_t MemoryInstance::GetPageSize() {
  return static_cast<MemoryInstanceContext *>(this)->GetPageSize();
}

Result MemoryInstance::GrowPage(const uint32_t Page) {
  return static_cast<MemoryInstanceContext *>(this)->GrowPage(Page);
}

// <<<<<<<< WasmEdge MemoryInstance <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge GlobalInstance >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

GlobalInstance GlobalInstance::New(const GlobalType &GlobType,
                                   const Value &Val) {
  return GlobalInstanceContext(static_cast<const GlobalTypeContext &>(GlobType),
                               Val);
}

const GlobalType GlobalInstance::GetGlobalType() {
  return static_cast<GlobalInstanceContext *>(this)->GetGlobalTypeContext();
}

Value GlobalInstance::GetValue() {
  return static_cast<GlobalInstanceContext *>(this)->GetValue();
}

void GlobalInstance::SetValue(const Value &Val) {
  return static_cast<GlobalInstanceContext *>(this)->SetValue(Val);
}

// <<<<<<<< WasmEdge GlobalInstance <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge ModuleInstance >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

ModuleInstance ModuleInstance::New() { return ModuleInstanceContext(nullptr); }

ModuleInstance ModuleInstance::New(const std::string &ModuleName) {
  return ModuleInstanceContext(ModuleName);
}

ModuleInstance
ModuleInstance::New(const std::vector<const std::string> &Args,
                    const std::vector<const std::string> &Envs,
                    const std::vector<const std::string> &Preopens) {
  return ModuleInstanceContext(Args, Envs, Preopens);
}

ModuleInstance ModuleInstance::Move(ModuleInstance &ModInst) {
  return ModuleInstanceContext(
      std::move(*static_cast<ModuleInstanceContext *>(&ModInst)));
}

void ModuleInstance::InitWASI(const std::vector<const std::string> &Args,
                              const std::vector<const std::string> &Envs,
                              const std::vector<const std::string> &Preopens) {
  static_cast<ModuleInstanceContext *>(this)->InitWASI(Args, Envs, Preopens);
}

uint32_t ModuleInstance::WASIGetExitCode() {
  return static_cast<ModuleInstanceContext *>(this)->WASIGetExitCode();
}

uint32_t ModuleInstance::WASIGetNativeHandler(int32_t Fd,
                                              uint64_t &NativeHandler) {
  return static_cast<ModuleInstanceContext *>(this)->WASIGetNativeHandler(
      Fd, NativeHandler);
}

void ModuleInstance::InitWasmEdgeProcess(
    const std::vector<const std::string> &AllowedCmds, const bool AllowAll) {
  static_cast<ModuleInstanceContext *>(this)->InitWasmEdgeProcess(AllowedCmds,
                                                                  AllowAll);
}

std::string ModuleInstance::GetModuleName() {
  return static_cast<ModuleInstanceContext *>(this)->GetModuleName();
}

FunctionInstance ModuleInstance::FindFunction(const std::string &Name) {
  return static_cast<ModuleInstanceContext *>(this)->FindFunction(Name);
}

TableInstance ModuleInstance::FindTable(const std::string &Name) {
  return static_cast<ModuleInstanceContext *>(this)->FindTable(Name);
}

MemoryInstance ModuleInstance::FindMemory(const std::string &Name) {
  return static_cast<ModuleInstanceContext *>(this)->FindMemory(Name);
}

GlobalInstance ModuleInstance::FindGlobal(const std::string &Name) {
  return static_cast<ModuleInstanceContext *>(this)->FindGlobal(Name);
}

std::vector<std::string> ModuleInstance::ListFunction() {
  return static_cast<ModuleInstanceContext *>(this)->ListFunction();
}

std::vector<std::string> ModuleInstance::ListTable() {
  return static_cast<ModuleInstanceContext *>(this)->ListTable();
}

std::vector<std::string> ModuleInstance::ListMemory() {
  return static_cast<ModuleInstanceContext *>(this)->ListMemory();
}

std::vector<std::string> ModuleInstance::ListGlobal() {
  return static_cast<ModuleInstanceContext *>(this)->ListGlobal();
}

void ModuleInstance::AddFunction(const std::string &Name,
                                 FunctionInstance &&FuncCxt) {
  static_cast<ModuleInstanceContext *>(this)->AddFunction(
      Name, static_cast<FunctionInstanceContext &&>(FuncCxt));
}

void ModuleInstance::AddTable(const std::string &Name,
                              TableInstance &&TableCxt) {
  static_cast<ModuleInstanceContext *>(this)->AddTable(
      Name, static_cast<TableInstanceContext &&>(TableCxt));
}

void ModuleInstance::AddMemory(const std::string &Name,
                               MemoryInstance &&MemoryCxt) {
  static_cast<ModuleInstanceContext *>(this)->AddMemory(
      Name, static_cast<MemoryInstanceContext &&>(MemoryCxt));
}

void ModuleInstance::AddGlobal(const std::string &Name,
                               GlobalInstance &&GlobalCxt) {
  static_cast<ModuleInstanceContext *>(this)->AddGlobal(
      Name, static_cast<GlobalInstanceContext &&>(GlobalCxt));
}

// <<<<<<<< WasmEdge ModuleInstance <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// <<<<<<<< WasmEdge Runtime <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge VM >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

VM::VM(const Configuration &ConfCxt, Store &StoreCxt) {
  this->Cxt = std::make_unique<VM::VMContext>(
      *ConfCxt.Cxt, *static_cast<StoreContext *>(&StoreCxt)->GetContentPtr());
}

VM::VM(const Configuration &ConfCxt) {
  this->Cxt = std::make_unique<VM::VMContext>(*ConfCxt.Cxt);
}

VM::VM(Store &StoreCxt) {
  this->Cxt = std::make_unique<VM::VMContext>(
      WasmEdge::Configure(),
      *static_cast<StoreContext *>(&StoreCxt)->GetContentPtr());
}

VM::VM() { this->Cxt = std::make_unique<VM::VMContext>(WasmEdge::Configure()); }

Result VM::RegisterModule(const std::string &ModuleName,
                          const std::string &Path) {
  return wrap(
      [&]() {
        return Cxt->registerModule(std::string_view(ModuleName),
                                   std::filesystem::absolute(Path));
      },
      EmptyThen);
}

Result VM::RegisterModule(const std::string &ModuleName,
                          const std::vector<uint8_t> &Buf) {
  return wrap([&]() { return Cxt->registerModule(ModuleName, Buf); },
              EmptyThen);
}

Result VM::RegisterModule(const std::string &ModuleName,
                          const ASTModule &ASTCxt) {
  return wrap(
      [&]() {
        return Cxt->registerModule(
            ModuleName,
            *static_cast<const ASTModuleContext *>(&ASTCxt)->GetConstPointer());
      },
      EmptyThen);
}

Result VM::RegisterModule(const ModuleInstance &ImportCxt) {
  return wrap(
      [&]() {
        return Cxt->registerModule(
            *static_cast<const ModuleInstanceContext *>(&ImportCxt)
                 ->GetConstContent());
      },
      EmptyThen);
}

Result VM::RunWasm(const std::string &Path, const std::string &FuncName,
                   const std::vector<Value> &Params,
                   std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return wrap(
      [&]() {
        return Cxt->runWasmFile(std::filesystem::absolute(Path),
                                std::string_view(FuncName), ParamPair.first,
                                ParamPair.second);
      },
      [&](auto Res) { Value::ValueUtils::FillValueArr(*Res, Returns); });
}

Result VM::RunWasm(const std::vector<uint8_t> &Buf, const std::string &FuncName,
                   const std::vector<Value> &Params,
                   std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return wrap(
      [&]() {
        return Cxt->runWasmFile(Buf, std::string_view(FuncName),
                                ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { Value::ValueUtils::FillValueArr(*Res, Returns); });
}

Result VM::RunWasm(const ASTModule &ASTCxt, const std::string &FuncName,
                   const std::vector<Value> &Params,
                   std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return wrap(
      [&]() {
        return Cxt->runWasmFile(
            *static_cast<const ASTModuleContext *>(&ASTCxt)->GetConstPointer(),
            std::string_view(FuncName), ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) {
        Value::ValueUtils::FillValueArr(*Res, Returns);
      }); // TODO
}

std::unique_ptr<Async> VM::AsyncRunWasm(const std::string &Path,
                                        const std::string &FuncName,
                                        const std::vector<Value> &Params) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return std::make_unique<Async>(Cxt->asyncRunWasmFile(
      std::filesystem::absolute(Path), std::string_view(FuncName),
      ParamPair.first, ParamPair.second));
}

std::unique_ptr<Async> VM::AsyncRunWasm(const std::vector<uint8_t> &Buf,
                                        const std::string &FuncName,
                                        const std::vector<Value> &Params) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return std::make_unique<Async>(Cxt->asyncRunWasmFile(
      Buf, std::string_view(FuncName), ParamPair.first, ParamPair.second));
}

std::unique_ptr<Async> VM::AsyncRunWasm(const ASTModule &ASTCxt,
                                        const std::string &FuncName,
                                        const std::vector<Value> &Params) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return std::make_unique<Async>(Cxt->asyncRunWasmFile(
      *static_cast<const ASTModuleContext *>(&ASTCxt)->GetConstPointer(),
      std::string_view(FuncName), ParamPair.first, ParamPair.second));
}

Result VM::LoadWasm(const std::string &Path) {
  return wrap([&]() { return Cxt->loadWasm(std::filesystem::absolute(Path)); },
              EmptyThen);
}

Result VM::LoadWasm(const std::vector<uint8_t> &Buf) {
  return wrap([&]() { return Cxt->loadWasm(Buf); }, EmptyThen);
}

Result VM::LoadWasm(const ASTModule &ASTCxt) {
  return wrap(
      [&]() {
        return Cxt->loadWasm(
            *static_cast<const ASTModuleContext *>(&ASTCxt)->GetConstPointer());
      },
      EmptyThen);
}

Result VM::Validate() {
  return wrap([&]() { return Cxt->validate(); }, EmptyThen);
}

Result VM::Instantiate() {
  return wrap([&]() { return Cxt->instantiate(); }, EmptyThen);
}

Result VM::Execute(const std::string &FuncName,
                   const std::vector<Value> &Params,
                   std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return wrap(
      [&]() {
        return Cxt->execute(std::string_view(FuncName), ParamPair.first,
                            ParamPair.second);
      },
      [&](auto &&Res) {
        Value::ValueUtils::FillValueArr(*Res, Returns);
      }); // TODO
}

Result VM::Execute(const std::string &ModuleName, const std::string &FuncName,
                   const std::vector<Value> &Params,
                   std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return wrap(
      [&]() {
        return Cxt->execute(std::string_view(ModuleName),
                            std::string_view(FuncName), ParamPair.first,
                            ParamPair.second);
      },
      [&](auto &&Res) {
        Value::ValueUtils::FillValueArr(*Res, Returns);
      }); // TODO
}

std::unique_ptr<Async> VM::AsyncExecute(const std::string &FuncName,
                                        const std::vector<Value> &Params) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return std::make_unique<Async>(Cxt->asyncExecute(
      std::string_view(FuncName), ParamPair.first, ParamPair.second));
}

std::unique_ptr<Async> VM::AsyncExecute(const std::string &ModuleName,
                                        const std::string &FuncName,
                                        const std::vector<Value> &Params) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return std::make_unique<Async>(Cxt->asyncExecute(
      std::string_view(ModuleName), std::string_view(FuncName), ParamPair.first,
      ParamPair.second));
}

const FunctionType VM::GetFunctionType(const std::string &FuncName) {
  const auto FuncList = Cxt->getFunctionList();
  for (const auto &It : FuncList) {
    if (It.first == std::string_view(FuncName)) {
      return FunctionTypeContext(&It.second);
    }
  }
  return FunctionTypeContext(nullptr);
}

const FunctionType VM::GetFunctionType(const std::string &ModuleName,
                                       const std::string &FuncName) {
  const auto *ModInst =
      Cxt->getStoreManager().findModule(std::string_view(ModuleName));
  if (ModInst != nullptr) {
    const auto *FuncInst = ModInst->findFuncExports(std::string_view(FuncName));
    if (FuncInst != nullptr) {
      return FunctionTypeContext(&FuncInst->getFuncType());
    }
  }
  return FunctionTypeContext(nullptr);
}

void VM::Cleanup() { Cxt->cleanup(); }

uint32_t VM::GetFunctionList(std::vector<std::string> &Names,
                             std::vector<FunctionType> &FuncTypes) {
  // Not to use VM::getFunctionList() here because not to allocate the
  // returned function name strings.
  const auto *ModInst = Cxt->getActiveModule();
  if (ModInst != nullptr) {
    return ModInst->getFuncExports([&](const auto &FuncExp) {
      uint32_t I = 0;
      for (auto It = FuncExp.cbegin(); It != FuncExp.cend(); It++, I++) {
        const auto *FuncInst = It->second;
        const auto &FuncType = FuncInst->getFuncType();

        Names[I] = It->first;
        FuncTypes[I] = FunctionTypeContext(&FuncType);
      }
      return static_cast<uint32_t>(FuncExp.size());
    });
  }
  return 0;
}

ModuleInstance VM::GetImportModuleContext(const HostRegistration Reg) {
  return ModuleInstanceContext(
      Cxt->getImportModule(static_cast<WasmEdge::HostRegistration>(Reg)));
}

const ModuleInstance VM::GetActiveModule() {
  return ModuleInstanceContext(Cxt->getActiveModule());
}

const ModuleInstance VM::GetRegisteredModule(const std::string &ModuleName) {
  return ModuleInstanceContext(Cxt->getStoreManager().findModule(ModuleName));
}

std::vector<std::string> VM::ListRegisteredModule() {
  std::vector<std::string> str;
  Cxt->getStoreManager().getModuleList([&](auto &Map) {
    for (auto &&Pair : Map) {
      str.emplace_back(Pair.first);
    }
  });

  return str;
}

Store VM::GetStoreContext() { return StoreContext(&Cxt->getStoreManager()); }

Loader VM::GetLoaderContext() { return LoaderContext(&Cxt->getLoader()); }

Validator VM::GetValidatorContext() {
  return ValidatorContext(&Cxt->getValidator());
}

Executor VM::GetExecutorContext() {
  return ExecutorContext(&Cxt->getExecutor());
}

Statistics VM::GetStatisticsContext() {
  return StatisticsContext(&Cxt->getStatistics());
}

// <<<<<<<< WasmEdge VM <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

} // namespace SDK
} // namespace WasmEdge
