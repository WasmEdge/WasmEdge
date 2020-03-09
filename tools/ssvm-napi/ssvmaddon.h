#ifndef SSVMADDON_H
#define SSVMADDON_H

#include "rapidjson/document.h"

#include <napi.h>

class MyObject : public Napi::ObjectWrap<MyObject> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  MyObject(const Napi::CallbackInfo &info);

private:
  static Napi::FunctionReference constructor;

  Napi::Value GetValue(const Napi::CallbackInfo &info);
  Napi::Value PlusOne(const Napi::CallbackInfo &info);
  Napi::Value Multiply(const Napi::CallbackInfo &info);
  Napi::Value Test(const Napi::CallbackInfo &info);

  double value_;
};

class SSVMAddon : public Napi::ObjectWrap<SSVMAddon> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  SSVMAddon(const Napi::CallbackInfo &info);

private:
  static Napi::FunctionReference constructor;
  std::string path;
  rapidjson::Document snapshot;

  Napi::Value Run(const Napi::CallbackInfo &info);
};

#endif
