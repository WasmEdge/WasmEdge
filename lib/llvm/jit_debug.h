// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/defines.h"

#include <llvm/Config/llvm-config.h>
#include <llvm/Support/Error.h>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace llvm::orc {
class LLJIT;
} // namespace llvm::orc

namespace WasmEdge::LLVM {

/// True when the JIT can give a debugger the debug info of its code. The
/// compiler emits debug info for the JIT, and the JIT compiles at O0 for a
/// debugger, only when this is true.
constexpr bool jitDebuggerSupported() noexcept {
  return !WASMEDGE_OS_WINDOWS &&
         !(WASMEDGE_OS_MACOS && LLVM_VERSION_MAJOR < 15);
}

/// Lets a debugger read the code of the JIT through the GDB JIT interface.
/// The debugger forgets the code again before the JIT frees it.
llvm::Error enableJITDebugger(llvm::orc::LLJIT &J);

#if LLVM_VERSION_MAJOR < 11
/// Tells the debugger that the JIT frees all of its code.
void releaseJITDebugger(const llvm::orc::LLJIT &J);
#endif

#if LLVM_VERSION_MAJOR < 15 || WASMEDGE_OS_WINDOWS
#else
/// An ID for a debugger entry whose debug object comes later.
uint64_t reserveJITDebugEntry();

/// Adds the debugger entry with Id for the debug object at [Start, Start +
/// Size), unless removeJITDebugEntries already removed the ID.
void addJITDebugEntry(uint64_t Id, uint64_t Start, uint64_t Size);

/// Removes the debugger entries with these IDs, or cancels their adds.
void removeJITDebugEntries(const std::vector<uint64_t> &Ids);

/// The number of reserved IDs whose debug objects did not come yet.
size_t pendingJITDebugEntries();
#endif

/// The number of debug objects in the list of the GDB JIT interface.
size_t countJITDebugObjects() noexcept;

/// The debug objects of each JIT resource key, by the ID of the debugger
/// entry of each debug object.
class DebugObjectOwners {
public:
  void add(uintptr_t Key, const std::vector<uint64_t> &Ids);
  std::vector<uint64_t> take(uintptr_t Key);
  void transfer(uintptr_t DstKey, uintptr_t SrcKey);
  std::vector<uint64_t> takeAll();

private:
  std::mutex Mutex;
  std::unordered_map<uintptr_t, std::vector<uint64_t>> Owned;
};

} // namespace WasmEdge::LLVM
