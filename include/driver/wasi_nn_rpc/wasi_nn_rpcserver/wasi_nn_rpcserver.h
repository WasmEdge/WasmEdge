#pragma once

#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "wasi_ephemeral_nn.grpc.pb.h"

#include <grpc/grpc.h>
#include <string_view>

using namespace std::literals;
using namespace WasmEdge;

namespace WasmEdge {
namespace WasiNNRPC {
namespace Server {

void writeUInt32WithBuilderPtr(
    WasmEdge::Runtime::Instance::MemoryInstance &MemInst, uint32_t Value,
    uint32_t &BuilderPtr) {
  uint32_t *BufPtr = MemInst.getPointer<uint32_t *>(BuilderPtr);
  *BufPtr = Value;
  BuilderPtr += 4;
}

void writeFatPointerWithBuilderPtr(
    WasmEdge::Runtime::Instance::MemoryInstance &MemInst, uint32_t PtrVal,
    uint32_t PtrSize, uint32_t &BuilderPtr) {
  writeUInt32WithBuilderPtr(MemInst, PtrVal, BuilderPtr);
  writeUInt32WithBuilderPtr(MemInst, PtrSize, BuilderPtr);
}

template <typename T>
void writeBinaries(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                   std::vector<T> Binaries, uint32_t Ptr) noexcept {
  std::copy(Binaries.begin(), Binaries.end(), MemInst.getPointer<T *>(Ptr));
}

template <typename T>
void writeBinaries(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                   google::protobuf::RepeatedField<T> Binaries,
                   uint32_t Ptr) noexcept {
  std::copy(Binaries.begin(), Binaries.end(), MemInst.getPointer<T *>(Ptr));
}

void writeBinaries(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                   const std::string &Binaries, uint32_t Ptr) noexcept {
  std::copy(Binaries.begin(), Binaries.end(),
            MemInst.getPointer<uint8_t *>(Ptr));
}

class HostFuncCaller {
public:
  HostFuncCaller(const Runtime::Instance::ModuleInstance &NNM,
                 std::string_view FuncName, uint32_t MemorySize) noexcept
      : FuncInst(NNM.findFuncExports(FuncName)), FuncName(FuncName), Mod(""),
        Frame(nullptr, &Mod) {
    Mod.addHostMemory(
        "memory"sv,
        std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
            WasmEdge::AST::MemoryType(MemorySize)));
  }

  Runtime::Instance::MemoryInstance &getMemInst(void) {
    return *Frame.getMemoryByIndex(0);
  }

  uint32_t call(std::initializer_list<WasmEdge::ValVariant> Args) {
    if (FuncInst == nullptr) {
      spdlog::error(
          "[WASI-NN-RPCSERVER] HostFuncCaller: FuncInst not found for {}"sv,
          FuncName);
      return 0xFFFFFFFF;
    }
    std::array<ValVariant, 1> Rets = {UINT32_C(0)};
    if (!FuncInst->getHostFunc().run(Frame, Args, Rets)) {
      spdlog::error(
          "[WASI-NN-RPCSERVER] HostFuncCaller: failed to run HostFunc for {}"sv,
          FuncName);
      return 0xFFFFFFFF;
    }
    uint32_t Errno = Rets[0].get<uint32_t>();
    return Errno;
  }

private:
  Runtime::Instance::FunctionInstance *FuncInst;
  std::string_view FuncName;
  Runtime::Instance::ModuleInstance Mod;
  Runtime::CallingFrame Frame;
};

grpc::Status createRPCStatusFromErrno(grpc::ServerContext *RPCContext,
                                      std::string_view FuncName,
                                      uint32_t Errno) {
  RPCContext->AddTrailingMetadata("errno", std::to_string(Errno));
  return grpc::Status(
      grpc::StatusCode::UNKNOWN,
      fmt::format("failed to call host function \"{}\", errno={}"sv, FuncName,
                  Errno));
}

class GraphService final : public wasi_ephemeral_nn::Graph::Service {
public:
  GraphService(const Runtime::Instance::ModuleInstance &NNM) noexcept
      : NNMod(NNM) {}
  /*
    Expect<WASINN::ErrNo>
    WasiNNLoadByName::bodyImpl(const Runtime::CallingFrame &Frame, uint32_t
    NamePtr, uint32_t NameLen, uint32_t GraphIdPtr)
  */
  virtual grpc::Status
  LoadByName(grpc::ServerContext *RPCContext,
             const wasi_ephemeral_nn::LoadByNameRequest *RPCRequest,
             wasi_ephemeral_nn::LoadByNameResult *RPCResult) {
    std::string_view FuncName = "load_by_name"sv;
    auto Name = RPCRequest->name();
    uint32_t NamePtr = UINT32_C(0);
    uint32_t NameLen = static_cast<uint32_t>(
        Name.size()); // does not include the '\0' terminator
    uint32_t OutPtr = NamePtr + NameLen + 1; // 1 is for the '\0' terminator
    uint32_t MemorySize = OutPtr + 4;        // 4 is for sizeof(OutPtr)

    HostFuncCaller HostFuncCaller(NNMod, FuncName, MemorySize);
    auto &MemInst = HostFuncCaller.getMemInst();
    std::vector<char> NameVec(Name.begin(), Name.end());
    writeBinaries(MemInst, NameVec, NamePtr);
    uint32_t Errno = HostFuncCaller.call({NamePtr, NameLen, OutPtr});
    if (Errno != 0) {
      return createRPCStatusFromErrno(RPCContext, FuncName, Errno);
    }
    uint32_t GraphHandle = *MemInst.getPointer<uint32_t *>(OutPtr);
    RPCResult->set_graph_handle(GraphHandle);
    return grpc::Status::OK;
  }

  /*
    Expect<WASINN::ErrNo>
    WasiNNLoadByNameWithConfig::bodyImpl(
      const Runtime::CallingFrame &Frame,
      uint32_t NamePtr, uint32_t NameLen,
      uint32_t ConfigPtr, uint32_t ConfigLen,
      uint32_t GraphIdPtr
    )
  */
  virtual grpc::Status LoadByNameWithConfig(
      grpc::ServerContext *RPCContext,
      const wasi_ephemeral_nn::LoadByNameWithConfigRequest *RPCRequest,
      wasi_ephemeral_nn::LoadByNameWithConfigResult *RPCResult) {
    std::string_view FuncName = "load_by_name_with_config"sv;
    auto Name = RPCRequest->name();
    auto Config = RPCRequest->config();
    uint32_t NamePtr = UINT32_C(0);
    uint32_t NameLen = static_cast<uint32_t>(
        Name.size()); // does not include the '\0' terminator
    uint32_t ConfigPtr = static_cast<uint32_t>(
        Name.size() + 1);               // 1 is for the '\0' terminator of Name
    uint32_t ConfigLen = Config.size(); // does not include the '\0' terminator
    uint32_t OutPtr =
        NamePtr + NameLen + 1 + ConfigLen + 1; // 1 is for the '\0' terminator
    uint32_t MemorySize = OutPtr + 4;          // 4 is for sizeof(OutPtr)

    HostFuncCaller HostFuncCaller(NNMod, FuncName, MemorySize);
    auto &MemInst = HostFuncCaller.getMemInst();
    std::vector<char> NameVec(Name.begin(), Name.end());
    std::vector<char> ConfigVec(Config.begin(), Config.end());
    writeBinaries(MemInst, NameVec, NamePtr);
    writeBinaries(MemInst, ConfigVec, ConfigPtr);
    uint32_t Errno =
        HostFuncCaller.call({NamePtr, NameLen, ConfigPtr, ConfigLen, OutPtr});
    if (Errno != 0) {
      return createRPCStatusFromErrno(RPCContext, FuncName, Errno);
    }
    uint32_t GraphHandle = *MemInst.getPointer<uint32_t *>(OutPtr);
    RPCResult->set_graph_handle(GraphHandle);
    return grpc::Status::OK;
  }

private:
  const Runtime::Instance::ModuleInstance &NNMod;
};

class GraphResourceService final
    : public wasi_ephemeral_nn::GraphResource::Service {
public:
  GraphResourceService(const Runtime::Instance::ModuleInstance &NNM) noexcept
      : NNMod(NNM) {}
  /*
    Expect<ErrNo> initExecCtx(WasiNNEnvironment &Env, uint32_t GraphId,
                              uint32_t &ContextId) noexcept
  */
  virtual grpc::Status InitExecutionContext(
      grpc::ServerContext *RPCContext,
      const wasi_ephemeral_nn::InitExecutionContextRequest *RPCRequest,
      wasi_ephemeral_nn::InitExecutionContextResult *RPCResult) {
    std::string_view FuncName = "init_execution_context"sv;
    uint32_t ResourceHandle = RPCRequest->resource_handle();
    uint32_t OutPtr = UINT32_C(0);
    uint32_t MemorySize = OutPtr + 4; // 4 is for sizeof(OutPtr)

    HostFuncCaller HostFuncCaller(NNMod, FuncName, MemorySize);
    auto &MemInst = HostFuncCaller.getMemInst();
    uint32_t Errno = HostFuncCaller.call({ResourceHandle, OutPtr});
    if (Errno != 0) {
      return createRPCStatusFromErrno(RPCContext, FuncName, Errno);
    }
    uint32_t CtxHandle = *MemInst.getPointer<uint32_t *>(OutPtr);
    RPCResult->set_ctx_handle(CtxHandle);
    return grpc::Status::OK;
  }

private:
  const Runtime::Instance::ModuleInstance &NNMod;
};

class GraphExecutionContextResourceService final
    : public wasi_ephemeral_nn::GraphExecutionContextResource::Service {
public:
  GraphExecutionContextResourceService(
      const Runtime::Instance::ModuleInstance &NNM) noexcept
      : NNMod(NNM) {}

  /*
    Expect<ErrNo> setInput(WasiNNEnvironment &Env, uint32_t ContextId,
                           uint32_t Index, const TensorData &Tensor) noexcept
  */
  virtual grpc::Status
  SetInput(grpc::ServerContext *RPCContext,
           const wasi_ephemeral_nn::SetInputRequest *RPCRequest,
           google::protobuf::Empty * /*RPCResult*/) {
    std::string_view FuncName = "set_input"sv;
    uint32_t ResourceHandle = RPCRequest->resource_handle();
    uint32_t Index = RPCRequest->index();
    auto Tensor = RPCRequest->tensor();
    auto TensorDim = Tensor.dimensions();
    uint32_t TensorDimSize = TensorDim.size();
    uint32_t TensorTy = Tensor.ty();
    auto TensorData = Tensor.data();
    uint32_t TensorDataSize = static_cast<uint32_t>(TensorData.size());

    /* clang-format off */
    /**
       0                    : FatPointer (20, TensorDimSize)
       8                    : TensorTy
      12                    : FatPointer (20 + TensorDimSize * 4, TensorDataSize)
      20                    : TensorDim
      20 + TensorDimSize * 4: TensorData
    */
    /* clang-format on */
    uint32_t BuilderPtr = UINT32_C(0);
    uint32_t SetInputEntryPtr = BuilderPtr;
    uint32_t TensorDimPtr = UINT32_C(20);
    uint32_t TensorDataPtr = TensorDimPtr + TensorDimSize * 4;
    uint32_t MemorySize = TensorDataPtr + TensorDataSize;

    HostFuncCaller HostFuncCaller(NNMod, FuncName, MemorySize);
    auto &MemInst = HostFuncCaller.getMemInst();
    writeFatPointerWithBuilderPtr(MemInst, TensorDimPtr, TensorDimSize,
                                  BuilderPtr);
    writeUInt32WithBuilderPtr(MemInst, TensorTy, BuilderPtr);
    writeFatPointerWithBuilderPtr(MemInst, TensorDataPtr, TensorDataSize,
                                  BuilderPtr);
    writeBinaries<uint32_t>(MemInst, TensorDim, TensorDimPtr);
    writeBinaries(MemInst, TensorData, TensorDataPtr);

    uint32_t Errno =
        HostFuncCaller.call({ResourceHandle, Index, SetInputEntryPtr});
    if (Errno != 0) {
      return createRPCStatusFromErrno(RPCContext, FuncName, Errno);
    }
    return grpc::Status::OK;
  }

  /*
    Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept
  */
  virtual grpc::Status
  Compute(grpc::ServerContext *RPCContext,
          const wasi_ephemeral_nn::ComputeRequest *RPCRequest,
          google::protobuf::Empty * /*RPCResult*/) {
    std::string_view FuncName = "compute"sv;
    uint32_t ResourceHandle = RPCRequest->resource_handle();
    uint32_t MemorySize = UINT32_C(0);
    HostFuncCaller HostFuncCaller(NNMod, FuncName, MemorySize);
    uint32_t Errno = HostFuncCaller.call({ResourceHandle});
    if (Errno != 0) {
      return createRPCStatusFromErrno(RPCContext, FuncName, Errno);
    }
    return grpc::Status::OK;
  }

  /*
    Expect<ErrNo> computeSingle(WasiNNEnvironment &Env, uint32_t ContextId)
    noexcept
  */
  virtual grpc::Status
  ComputeSingle(grpc::ServerContext *RPCContext,
                const wasi_ephemeral_nn::ComputeRequest *RPCRequest,
                google::protobuf::Empty * /*RPCResult*/) {
    std::string_view FuncName = "compute_single"sv;
    uint32_t ResourceHandle = RPCRequest->resource_handle();
    uint32_t MemorySize = UINT32_C(0);
    HostFuncCaller HostFuncCaller(NNMod, FuncName, MemorySize);
    uint32_t Errno = HostFuncCaller.call({ResourceHandle});
    if (Errno != 0) {
      return createRPCStatusFromErrno(RPCContext, FuncName, Errno);
    }
    return grpc::Status::OK;
  }

  /*
    Expect<ErrNo> getOutput(WasiNNEnvironment &Env, uint32_t ContextId,
                            uint32_t Index, Span<uint8_t> OutBuffer,
                            uint32_t &BytesWritten) noexcept
  */
  virtual grpc::Status
  GetOutput(grpc::ServerContext *RPCContext,
            const wasi_ephemeral_nn::GetOutputRequest *RPCRequest,
            wasi_ephemeral_nn::GetOutputResult *RPCResult) {
    std::string_view FuncName = "get_output"sv;
    uint32_t ResourceHandle = RPCRequest->resource_handle();
    uint32_t Index = RPCRequest->index();
    uint32_t MemorySize = UINT32_C(65536); // FIXME
    uint32_t BytesWrittenPtr = UINT32_C(0);
    uint32_t BufPtr = BytesWrittenPtr + UINT32_C(4);
    uint32_t BufMaxSize = MemorySize - BufPtr;
    HostFuncCaller HostFuncCaller(NNMod, FuncName, MemorySize);
    auto &MemInst = HostFuncCaller.getMemInst();
    uint32_t Errno = HostFuncCaller.call(
        {ResourceHandle, Index, BufPtr, BufMaxSize, BytesWrittenPtr});
    if (Errno != 0) {
      return createRPCStatusFromErrno(RPCContext, FuncName, Errno);
    }
    /* clang-format off */
    /**
       0                    : BytesWritten
       4                    : Buf
    */
    /* clang-format on */
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BytesWrittenPtr);
    auto *Buf = MemInst.getPointer<char *>(BufPtr);
    RPCResult->set_data(Buf, BytesWritten);
    return grpc::Status::OK;
  }

  /*
    Expect<ErrNo> getOutputSingle(WasiNNEnvironment &Env, uint32_t ContextId,
                                  uint32_t Index, Span<uint8_t> OutBuffer,
                                  uint32_t &BytesWritten) noexcept
  */
  virtual grpc::Status
  GetOutputSingle(grpc::ServerContext *RPCContext,
                  const wasi_ephemeral_nn::GetOutputRequest *RPCRequest,
                  wasi_ephemeral_nn::GetOutputResult *RPCResult) {
    std::string_view FuncName = "get_output_single"sv;
    uint32_t ResourceHandle = RPCRequest->resource_handle();
    uint32_t Index = RPCRequest->index();
    uint32_t MemorySize = UINT32_C(65536); // FIXME
    uint32_t BytesWrittenPtr = UINT32_C(0);
    uint32_t BufPtr = BytesWrittenPtr + UINT32_C(4);
    uint32_t BufMaxSize = MemorySize - BufPtr;
    HostFuncCaller HostFuncCaller(NNMod, FuncName, MemorySize);
    auto &MemInst = HostFuncCaller.getMemInst();
    uint32_t Errno = HostFuncCaller.call(
        {ResourceHandle, Index, BufPtr, BufMaxSize, BytesWrittenPtr});
    if (Errno != 0) {
      return createRPCStatusFromErrno(RPCContext, FuncName, Errno);
    }
    /* clang-format off */
    /**
       0                    : BytesWritten
       4                    : Buf
    */
    /* clang-format on */
    auto BytesWritten = *MemInst.getPointer<uint32_t *>(BytesWrittenPtr);
    auto *Buf = MemInst.getPointer<char *>(BufPtr);
    RPCResult->set_data(Buf, BytesWritten);
    return grpc::Status::OK;
  }

  /*
    Expect<ErrNo> finiSingle(WasiNNEnvironment &Env, uint32_t ContextId)
    noexcept
  */
  virtual grpc::Status
  FiniSingle(grpc::ServerContext *RPCContext,
             const wasi_ephemeral_nn::FiniSingleRequest *RPCRequest,
             google::protobuf::Empty * /*RPCResult*/) {
    std::string_view FuncName = "fini_single"sv;
    uint32_t ResourceHandle = RPCRequest->resource_handle();
    uint32_t MemorySize = UINT32_C(0);
    HostFuncCaller HostFuncCaller(NNMod, FuncName, MemorySize);
    uint32_t Errno = HostFuncCaller.call({ResourceHandle});
    if (Errno != 0) {
      return createRPCStatusFromErrno(RPCContext, FuncName, Errno);
    }
    return grpc::Status::OK;
  }

private:
  const Runtime::Instance::ModuleInstance &NNMod;
};

class ServiceSet {
public:
  ServiceSet(const Runtime::Instance::ModuleInstance &NNM) noexcept
      : Graph(GraphService(NNM)), GraphResource(GraphResourceService(NNM)),
        GraphExecutionContextResource(
            GraphExecutionContextResourceService(NNM)) {}

  std::vector<grpc::Service *> services(void) {
    return std::vector<grpc::Service *>{
        &Graph,
        &GraphResource,
        &GraphExecutionContextResource,
    };
  }

private:
  GraphService Graph;
  GraphResourceService GraphResource;
  GraphExecutionContextResourceService GraphExecutionContextResource;
};
} // namespace Server
} // namespace WasiNNRPC
} // namespace WasmEdge
