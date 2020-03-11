#include "ssvmaddon.h"
#include "vm/configure.h"
#include "vm/vm.h"

Napi::FunctionReference SSVMAddon::constructor;

Napi::Object SSVMAddon::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func =
      DefineClass(env, "VM", {InstanceMethod("run", &SSVMAddon::Run)});

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("VM", func);
  return exports;
}

SSVMAddon::SSVMAddon(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<SSVMAddon>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  int length = info.Length();

  if (length <= 0 || !info[0].IsString()) {
    Napi::TypeError::New(env, "wasm file expected")
        .ThrowAsJavaScriptException();
    return;
  }

  Napi::String value = info[0].As<Napi::String>();
  this->path = value.Utf8Value();
  this->snapshot.SetObject();
  this->snapshot.AddMember("latest", rapidjson::Value(rapidjson::kObjectType),
                           this->snapshot.GetAllocator());
}

Napi::Value SSVMAddon::Run(const Napi::CallbackInfo &info) {
  SSVM::VM::Configure Conf;
  SSVM::VM::VM VM(Conf);
  SSVM::VM::Result Result;
  std::vector<uint32_t> Arguments;
  std::vector<uint8_t> MemData;
  uint32_t ResultMemAddr;

  VM.setPath(this->path);

  // Prepare arguments
  int length = info.Length();
  for (int i = 1; i < length; i++) {
    if (info[i].IsString()) {
      std::string StrArg = info[i].As<Napi::String>().Utf8Value();
      std::vector<uint8_t> StrArgVec(StrArg.begin(), StrArg.end());
      std::vector<SSVM::Executor::Value> Rets;

      // Initialize VM
      VM.initVMEnv();
      VM.loadWasm();
      VM.validate();
      VM.setEntryFuncName("__wbindgen_malloc");
      VM.appendArgument(StrArg.length());
      VM.instantiate();

      // Restore memory
      if (MemData.size() > 0) {
        VM.setMemoryWithBytes(MemData, 0, 0, MemData.size());
      }

      VM.runWasm();

      // Get malloc return address
      VM.getReturnValue(Rets);
      uint32_t StrAddr = std::get<uint32_t>(Rets[0]);

      // Update memory data
      MemData.clear();
      VM.getMemoryToBytesAll(0, MemData);

      // get malloc address
      Arguments.push_back(StrAddr);
      Arguments.push_back(StrArg.length());
      for (std::size_t i = 0; i < StrArgVec.size(); i++) {
        MemData[StrAddr + i] = StrArgVec[i];
      }

      VM.cleanup();
    } else if (info[i].IsNumber()) {
      uint32_t Arg = info[i].As<Napi::Number>().Uint32Value();
      Arguments.push_back(Arg);
    } else {
      Napi::TypeError::New(info.Env(), "unsupported argument type")
          .ThrowAsJavaScriptException();
      return Napi::Value();
    }
  }

  // Prepare start function
  std::string StartFunction = "";
  if (length > 0) {
    StartFunction = info[0].As<Napi::String>().Utf8Value();
  }

  // Initialize vm
  VM.initVMEnv();
  VM.loadWasm();
  VM.validate();
  VM.setEntryFuncName(StartFunction);
  VM.instantiate();

  // Set memory
  VM.setMemoryWithBytes(MemData, 0, 0, MemData.size());

  // Prepare arguments
  VM.appendArgument(ResultMemAddr);
  for (auto a : Arguments) {
    VM.appendArgument(a);
  }

  VM.runWasm();

  std::vector<uint8_t> ResultMem;
  VM.getMemoryToBytes(0, ResultMemAddr, ResultMem, 8);
  uint32_t ResultStringAddr = ResultMem[0] | (ResultMem[1] << 8) |
                              (ResultMem[2] << 16) | (ResultMem[3] << 24);
  uint32_t ResultStringLen = ResultMem[4] | (ResultMem[5] << 8) |
                             (ResultMem[6] << 16) | (ResultMem[7] << 24);
  std::vector<uint8_t> ResultData;
  VM.getMemoryToBytes(0, ResultStringAddr, ResultData, ResultStringLen);
  VM.cleanup();

  std::string ResultString(ResultData.begin(), ResultData.end());
  return Napi::String::New(info.Env(), ResultString);
}
