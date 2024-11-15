#include "ast/section.h"
#include "ast/type.h"
#include "common/errcode.h"
#include "common/span.h"
#include "runtime/instance/memory.h"
#include "runtime/stackmgr.h"

namespace WasmEdge {
namespace Coredump {

void generateCoredump(const Runtime::StackManager &StackMgr) noexcept;
AST::CustomSection createCore();
AST::CustomSection createCoremodules();
AST::CustomSection
createCorestack(Span<const Runtime::StackManager::Frame> Frames,
                Span<const Runtime::StackManager::Value> ValueStack);
AST::GlobalSection createGlobals(
    Span<const Runtime::Instance::GlobalInstance *const> GlobalInstances);
AST::MemorySection createMemory(
    Span<const Runtime::Instance::MemoryInstance *const> MemoryInstances);
AST::DataSection
createData(Span<const Runtime::Instance::DataInstance *const> DataInstances);
AST::CustomSection createCoreinstances();
} // namespace Coredump
} // namespace WasmEdge
