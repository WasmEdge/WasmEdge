// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "ast/section.h"
#include "ast/type.h"
#include "common/errcode.h"
#include "common/span.h"
#include "loader/serialize.h"
#include "runtime/instance/memory.h"
#include "runtime/stackmgr.h"

namespace WasmEdge {
namespace Coredump {

void generateCoredump(const Runtime::StackManager &StackMgr,
                      bool ForWasmgdb) noexcept;
AST::CustomSection createCore();
AST::CustomSection createCoremodules(
    Loader::Serializer &Ser,
    Span<const Runtime::Instance::ModuleInstance *const> ModuleInstances);
AST::CustomSection createCorestack(
    Loader::Serializer &Ser, Span<const Runtime::StackManager::Frame> Frames,
    Span<const Runtime::StackManager::Value> ValueStack, bool ForWasmgdb);
AST::CustomSection createCoreinstances(
    Span<const Runtime::Instance::ModuleInstance *const> ModuleInstances);
AST::MemorySection createMemory(
    Span<const Runtime::Instance::MemoryInstance *const> MemoryInstances);
AST::GlobalSection createGlobals(
    Span<const Runtime::Instance::GlobalInstance *const> GlobalInstances);
AST::DataSection
createData(Span<const Runtime::Instance::DataInstance *const> DataInstances);
} // namespace Coredump
} // namespace WasmEdge
