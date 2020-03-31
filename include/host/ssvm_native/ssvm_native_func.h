// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"
#include "ssvm_native_base.h"

namespace SSVM {
namespace Host {

class SSVMNativeStorageCreateUUID
    : public SSVMNative<SSVMNativeStorageCreateUUID> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint64_t &UUID) {
    /// TODO: Function body.
    return ErrCode::Success;
  }
};

class SSVMNativeStorageBeginStoreTx
    : public SSVMNative<SSVMNativeStorageBeginStoreTx> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint64_t NewKey) {
    /// TODO: Function body.
    return ErrCode::Success;
  }
};

class SSVMNativeStorageBeginLoadTx
    : public SSVMNative<SSVMNativeStorageBeginLoadTx> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint64_t NewKey) {
    /// TODO: Function body.
    return ErrCode::Success;
  }
};

class SSVMNativeStorageStoreI32 : public SSVMNative<SSVMNativeStorageStoreI32> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t Value) {
    /// TODO: Function body.
    return ErrCode::Success;
  }
};

class SSVMNativeStorageLoadI32 : public SSVMNative<SSVMNativeStorageLoadI32> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &Value) {
    /// TODO: Function body.
    return ErrCode::Success;
  }
};

class SSVMNativeStorageStoreI64 : public SSVMNative<SSVMNativeStorageStoreI64> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint64_t Value) {
    /// TODO: Function body.
    return ErrCode::Success;
  }
};

class SSVMNativeStorageLoadI64 : public SSVMNative<SSVMNativeStorageLoadI64> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint64_t &Value) {
    /// TODO: Function body.
    return ErrCode::Success;
  }
};

class SSVMNativeStorageEndStoreTx
    : public SSVMNative<SSVMNativeStorageEndStoreTx> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst) {
    /// TODO: Function body.
    return ErrCode::Success;
  }
};

class SSVMNativeStorageEndLoadTx
    : public SSVMNative<SSVMNativeStorageEndLoadTx> {
public:
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst) {
    /// TODO: Function body.
    return ErrCode::Success;
  }
};

} // namespace Host
} // namespace SSVM