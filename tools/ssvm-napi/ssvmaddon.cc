#include "ssvmaddon.h"
#include <iostream>

Napi::FunctionReference SSVMAddon::Constructor;

Napi::Object SSVMAddon::Init(Napi::Env Env, Napi::Object Exports) {
  Napi::HandleScope Scope(Env);

  Napi::Function Func = DefineClass(
      Env, "VM", {InstanceMethod("RunInt", &SSVMAddon::RunInt),
                  InstanceMethod("RunString", &SSVMAddon::RunString),
                  InstanceMethod("RunUint8Array", &SSVMAddon::RunUint8Array)});

  Constructor = Napi::Persistent(Func);
  Constructor.SuppressDestruct();

  Exports.Set("VM", Func);
  return Exports;
}

SSVMAddon::SSVMAddon(const Napi::CallbackInfo &Info)
    : Napi::ObjectWrap<SSVMAddon>(Info), VM(this->Configure),
      ResultData(nullptr) {
  Napi::Env Env = Info.Env();
  Napi::HandleScope Scope(Env);

  if (Info.Length() <= 0 || !Info[0].IsString()) {
    Napi::Error::New(Env, "wasm file expected").ThrowAsJavaScriptException();
    return;
  }

  Napi::String Path = Info[0].As<Napi::String>();
  this->VM.setPath(Path.Utf8Value());
}

SSVMAddon::~SSVMAddon() {
  if (this->ResultData) {
    delete[] ResultData;
  }
}

void SSVMAddon::Prepare(const Napi::CallbackInfo &Info,
                        std::vector<uint32_t> &Arguments,
                        std::vector<uint8_t> &MemData,
                        unsigned int &DataPageSize) {
  for (std::size_t I = 1; I < Info.Length(); I++) {
    Napi::Value Argument = Info[I];
    if (Argument.IsNumber()) {
      uint32_t U32Value = Argument.As<Napi::Number>().Uint32Value();
      Arguments.push_back(U32Value);
      continue;
    }

    uint32_t MallocSize = 0;
    if (Argument.IsString()) {
      std::string StrArg = Argument.As<Napi::String>().Utf8Value();
      MallocSize = StrArg.length();
    } else if (Argument.IsTypedArray() &&
               Argument.As<Napi::TypedArray>().TypedArrayType() ==
                   napi_uint8_array) {
      Napi::ArrayBuffer DataBuffer =
          Argument.As<Napi::TypedArray>().ArrayBuffer();
      MallocSize = DataBuffer.ByteLength();
    } else {
      // TODO: support other types
      Napi::TypeError::New(Info.Env(), "unsupported argument type")
          .ThrowAsJavaScriptException();
      return;
    }

    // Initialize VM
    this->VM.initVMEnv();
    this->VM.loadWasm();
    this->VM.validate();
    this->VM.setEntryFuncName("__wbindgen_malloc");
    this->VM.appendArgument(MallocSize);
    this->VM.instantiate();

    // Restore memory
    if (DataPageSize > 0) {
      this->VM.setMemoryDataPageSize(0, DataPageSize);
    }
    if (MemData.size() > 0) {
      this->VM.setMemoryWithBytes(MemData, 0, 0, MemData.size());
    }

    this->VM.runWasm();

    // Get malloc return address
    std::vector<SSVM::Executor::Value> Rets;
    this->VM.getReturnValue(Rets);
    uint32_t MallocAddr = std::get<uint32_t>(Rets[0]);

    // Update memory data
    MemData.clear();
    this->VM.getMemoryToBytesAll(0, MemData, DataPageSize);

    // Prepare arguments and memory data
    Arguments.push_back(MallocAddr);
    Arguments.push_back(MallocSize);

    // Setup memory
    if (Argument.IsString()) {
      std::string StrArg = Argument.As<Napi::String>().Utf8Value();
      std::vector<uint8_t> StrArgVec(StrArg.begin(), StrArg.end());
      for (std::size_t J = 0; J < StrArgVec.size(); J++) {
        MemData[MallocAddr + J] = StrArgVec[J];
      }
    } else if (Argument.IsTypedArray() &&
               Argument.As<Napi::TypedArray>().TypedArrayType() ==
                   napi_uint8_array) {
      Napi::ArrayBuffer DataBuffer =
          Argument.As<Napi::TypedArray>().ArrayBuffer();
      uint8_t *Data = (uint8_t *)DataBuffer.Data();
      for (size_t J = 0; J < DataBuffer.ByteLength(); J++) {
        MemData[MallocAddr + J] = Data[J];
      }
    }

    this->VM.cleanup();
  }
}

SSVM::VM::ErrCode SSVMAddon::Run(const Napi::CallbackInfo &Info,
                                 std::vector<uint32_t> &Arguments,
                                 std::vector<uint8_t> &MemData,
                                 unsigned int &DataPageSize) {

  // Prepare start function
  std::string StartFunction = "";
  if (Info.Length() > 0) {
    StartFunction = Info[0].As<Napi::String>().Utf8Value();
  }

  // Initialize VM
  this->VM.initVMEnv();
  this->VM.loadWasm();
  this->VM.validate();
  this->VM.setEntryFuncName(StartFunction);
  this->VM.instantiate();

  // Restore memory
  if (DataPageSize > 0) {
    this->VM.setMemoryDataPageSize(0, DataPageSize);
  }
  if (MemData.size() > 0) {
    this->VM.setMemoryWithBytes(MemData, 0, 0, MemData.size());
  }

  // Prepare arguments
  for (auto Argument : Arguments) {
    this->VM.appendArgument(Argument);
  }

  SSVM::VM::ErrCode Err = this->VM.runWasm();
  MemData.clear();
  this->VM.getMemoryToBytesAll(0, MemData, DataPageSize);

  return Err;
}

Napi::Value SSVMAddon::RunInt(const Napi::CallbackInfo &Info) {
  std::vector<uint32_t> Arguments;
  std::vector<uint8_t> MemData;
  unsigned int DataPageSize = 0;
  this->Prepare(Info, Arguments, MemData, DataPageSize);
  SSVM::VM::ErrCode Err = this->Run(Info, Arguments, MemData, DataPageSize);
  if (Err != SSVM::VM::ErrCode::Success) {
    Napi::Error::New(Info.Env(), "SSVM execution failed")
        .ThrowAsJavaScriptException();
    return Napi::Value();
  }

  std::vector<SSVM::Executor::Value> Rets;
  this->VM.getReturnValue(Rets);
  uint32_t ReturnValue = std::get<uint32_t>(Rets[0]);
  this->VM.cleanup();

  return Napi::Number::New(Info.Env(), ReturnValue);
}

Napi::Value SSVMAddon::RunString(const Napi::CallbackInfo &Info) {
  std::vector<uint32_t> Arguments;
  std::vector<uint8_t> MemData;
  unsigned int DataPageSize = 0;
  this->Prepare(Info, Arguments, MemData, DataPageSize);

  uint32_t ResultMemAddr = 8;
  Arguments.insert(Arguments.begin(), ResultMemAddr);
  SSVM::VM::ErrCode Err = this->Run(Info, Arguments, MemData, DataPageSize);

  if (Err != SSVM::VM::ErrCode::Success) {
    Napi::Error::New(Info.Env(), "SSVM execution failed")
        .ThrowAsJavaScriptException();
    return Napi::Value();
  }

  std::vector<uint8_t> ResultMem;
  this->VM.getMemoryToBytes(0, ResultMemAddr, ResultMem, 8);

  uint32_t ResultDataAddr = ResultMem[0] | (ResultMem[1] << 8) |
                            (ResultMem[2] << 16) | (ResultMem[3] << 24);
  uint32_t ResultDataLen = ResultMem[4] | (ResultMem[5] << 8) |
                           (ResultMem[6] << 16) | (ResultMem[7] << 24);

  std::vector<uint8_t> ResultData;
  this->VM.getMemoryToBytes(0, ResultDataAddr, ResultData, ResultDataLen);
  this->VM.cleanup();

  std::string ResultString(ResultData.begin(), ResultData.end());
  return Napi::String::New(Info.Env(), ResultString);
}

Napi::Value SSVMAddon::RunUint8Array(const Napi::CallbackInfo &Info) {
  std::vector<uint32_t> Arguments;
  std::vector<uint8_t> MemData;
  unsigned int DataPageSize = 0;
  this->Prepare(Info, Arguments, MemData, DataPageSize);

  uint32_t ResultMemAddr = 8;
  Arguments.insert(Arguments.begin(), ResultMemAddr);
  SSVM::VM::ErrCode Err = this->Run(Info, Arguments, MemData, DataPageSize);
  if (Err != SSVM::VM::ErrCode::Success) {
    Napi::Error::New(Info.Env(), "SSVM execution failed")
        .ThrowAsJavaScriptException();
    return Napi::Value();
  }

  std::vector<uint8_t> ResultMem;
  this->VM.getMemoryToBytes(0, ResultMemAddr, ResultMem, 8);

  uint32_t ResultDataAddr = ResultMem[0] | (ResultMem[1] << 8) |
                            (ResultMem[2] << 16) | (ResultMem[3] << 24);
  uint32_t ResultDataLen = ResultMem[4] | (ResultMem[5] << 8) |
                           (ResultMem[6] << 16) | (ResultMem[7] << 24);

  std::vector<uint8_t> ResultData;
  this->VM.getMemoryToBytes(0, ResultDataAddr, ResultData, ResultDataLen);
  this->VM.cleanup();

  this->ResultData = new uint8_t[ResultDataLen];
  std::memcpy(this->ResultData, ResultData.data(), ResultDataLen);
  Napi::ArrayBuffer ResultArrayBuffer =
      Napi::ArrayBuffer::New(Info.Env(), this->ResultData, ResultDataLen);
  Napi::Uint8Array ResultTypedArray = Napi::Uint8Array::New(
      Info.Env(), ResultDataLen, ResultArrayBuffer, 0, napi_uint8_array);

  return ResultTypedArray;
}
