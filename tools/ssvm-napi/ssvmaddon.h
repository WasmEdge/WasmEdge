#ifndef SSVMADDON_H
#define SSVMADDON_H

#include "vm/configure.h"
#include "vm/vm.h"
#include <napi.h>

class SSVMAddon : public Napi::ObjectWrap<SSVMAddon> {
public:
  static Napi::Object Init(Napi::Env Env, Napi::Object Exports);
  SSVMAddon(const Napi::CallbackInfo &Info);
  ~SSVMAddon();

private:
  static Napi::FunctionReference Constructor;
  uint8_t *ResultData;
  SSVM::VM::Configure Configure;
  SSVM::VM::VM VM;

  void Prepare(const Napi::CallbackInfo &Info, std::vector<uint32_t> &Arguments,
               std::vector<uint8_t> &MemData, unsigned int &DataPageSize);
  SSVM::VM::ErrCode Run(const Napi::CallbackInfo &Info,
                        std::vector<uint32_t> &Arguments,
                        std::vector<uint8_t> &MemData,
                        unsigned int &DataPageSize);
  Napi::Value RunInt(const Napi::CallbackInfo &Info);
  Napi::Value RunString(const Napi::CallbackInfo &Info);
  Napi::Value RunUint8Array(const Napi::CallbackInfo &Info);
};

#endif
