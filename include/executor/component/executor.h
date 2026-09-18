// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/executor/component/executor.h - Component executor -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component-model executor.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/component.h"
#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include "executor/component/canon.h"
#include "executor/executor.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/storemgr.h"
#include "runtime/component/task.h"
#include "runtime/component/thread.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/component/resource.h"
#include "runtime/instance/component/stream.h"
#include "runtime/instance/module.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

namespace Component {

/// The event a wait, a poll, or a callback delivers.
enum class EventCode : uint32_t {
  None = 0,
  Subtask = 1,
  StreamRead = 2,
  StreamWrite = 3,
  FutureRead = 4,
  FutureWrite = 5,
  TaskCancelled = 6,
};

/// One event: its code, the waitable index and the payload.
struct WaitableEvent {
  EventCode Code = EventCode::None;
  uint32_t Idx = 0;
  uint32_t Payload = 0;
};

} // namespace Component

/// The component-model executor.
class ComponentExecutor {
public:
  explicit ComponentExecutor(Executor &CoreExec) noexcept : Core(CoreExec) {}
  ComponentExecutor(const ComponentExecutor &) = delete;
  ComponentExecutor &operator=(const ComponentExecutor &) = delete;

  /// Instantiate a component as an anonymous component instance.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  instantiateComponent(Runtime::Component::StoreManager &StoreMgr,
                       const AST::Component::Component &Comp);

  /// Instantiate and register a component as a named component instance.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  registerComponent(Runtime::Component::StoreManager &StoreMgr,
                    const AST::Component::Component &Comp,
                    std::string_view Name);

  /// Register an instantiated component under its own name.
  Expect<void>
  registerComponent(Runtime::Component::StoreManager &StoreMgr,
                    const Runtime::Instance::ComponentInstance &CompInst);

  /// Invoke a component function by function instance.
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
  invoke(const Runtime::Instance::ComponentFunctionInstance *FuncInst,
         Span<const ComponentValVariant> Params,
         Span<const ComponentValType> ParamTypes);

  /// Run a canonical built-in; the canonical function calls back here.
  Expect<void> runCanon(const Component::CanonFunction &Canon,
                        Span<const ValVariant> Args, Span<ValVariant> Rets);

  /// \name Host operations on streams and futures.
  /// @{
  /// A new stream or future of the host of Frame; returns the handles of its
  /// writable and readable ends.
  std::pair<uint32_t, uint32_t>
  newHostStream(const Runtime::Component::CallingFrame &Frame, bool IsFuture,
                std::optional<ComponentValType> Elem);
  /// Write Data as the writer, blocking until it is all read or dropped.
  Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                   uint32_t>>
  writeStream(Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx,
              Span<const uint8_t> Data);
  Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                   uint32_t>>
  writeStream(Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx,
              std::vector<ComponentValVariant> Vals);
  /// Read up to Max elements as the reader, blocking until written or dropped.
  Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                   uint32_t>>
  readStream(Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx,
             std::vector<uint8_t> &Out, uint32_t Max);
  Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                   uint32_t>>
  readStream(Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx,
             std::vector<ComponentValVariant> &Out, uint32_t Max);
  /// Drop the end behind a handle.
  void dropStreamHandle(const Runtime::Component::CallingFrame &Frame,
                        uint32_t HandleIdx);
  /// Write one value from a detached host task, then drop the writer handle.
  void spawnStreamWrite(Runtime::Component::CallingFrame &Frame,
                        uint32_t HandleIdx, ComponentValVariant Val);
  /// Read as the reader until Consume fails or the writer drops, then drop
  /// the reader handle; true when it stopped before the writer dropped.
  Expect<bool>
  drainStream(Runtime::Component::CallingFrame &Frame, uint32_t HandleIdx,
              const std::function<bool(Span<const uint8_t>)> &Consume);
  /// @}

private:
  /// The flat value limits before values go through memory.
  static inline constexpr const uint32_t MaxFlatParams = 16;
  static inline constexpr const uint32_t MaxFlatAsyncParams = 4;
  static inline constexpr const uint32_t MaxFlatResults = 1;
  /// The flag of a UTF-16 string in a `latin1+utf16` packed length.
  static inline constexpr const uint64_t UTF16Tag = UINT64_C(1) << 31;
  /// The most elements one stream copy may carry.
  static inline constexpr const uint32_t MaxCopyLength = (1U << 28) - 1;
  /// What an asynchronous built-in returns in place of a block.
  static inline constexpr const uint32_t CopyBlocked = 0xffffffffU;
  /// The bytes a sink takes per read.
  static inline constexpr const uint32_t SinkChunkSize = 65536;

  /// What a callback asks for next: the low 4 bits of its packed result.
  enum class CallbackCode : uint32_t {
    Exit = 0,
    Yield = 1,
    Wait = 2,
  };

  /// \name Copies of the AST definitions a root instance owns.
  /// @{
  void copyComponent(const AST::Component::Component &Src,
                     AST::Component::Component &Comp) noexcept;
  void copySection(const AST::Component::CoreTypeSection &Src,
                   AST::Component::CoreTypeSection &Sec) noexcept;
  void copySection(const AST::Component::ComponentSection &Src,
                   AST::Component::ComponentSection &Sec) noexcept;
  void copySection(const AST::Component::TypeSection &Src,
                   AST::Component::TypeSection &Sec) noexcept;
  void copyType(const AST::Component::CoreDefType &Src,
                AST::Component::CoreDefType &Ty) noexcept;
  void copyType(const AST::Component::DefType &Src,
                AST::Component::DefType &Ty) noexcept;
  void copyType(const AST::Component::ComponentType &Src,
                AST::Component::ComponentType &Ty) noexcept;
  void copyType(const AST::Component::InstanceType &Src,
                AST::Component::InstanceType &Ty) noexcept;
  void copyDecl(const AST::Component::CoreModuleDecl &Src,
                AST::Component::CoreModuleDecl &Decl) noexcept;
  void copyDecl(const AST::Component::InstanceDecl &Src,
                AST::Component::InstanceDecl &Decl) noexcept;
  void copyDecl(const AST::Component::ComponentDecl &Src,
                AST::Component::ComponentDecl &Decl) noexcept;
  /// @}

  /// \name Functions for instantiation.
  /// @{
  /// Instantiation of a root component instance, importing from the store.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  instantiate(Runtime::Component::StoreManager &StoreMgr,
              const AST::Component::Component &Comp,
              std::optional<std::string_view> Name = std::nullopt);

  /// Instantiation of a nested component instance, importing from Provider.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  instantiate(const Runtime::Instance::ComponentInstance &Provider,
              const AST::Component::Component &Comp,
              Runtime::Instance::ComponentInstance *Parent);

  /// The section walk shared by both instantiation entries.
  Expect<void>
  instantiate(Runtime::Instance::ComponentInstance &CompInst,
              const AST::Component::Component &Comp,
              Runtime::Component::StoreManager *StoreMgr,
              const Runtime::Instance::ComponentInstance *Provider);

  /// Instantiation of Core Module Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::CoreModuleSection &CoreModSec);

  /// Instantiation of Core Instance Section.
  Expect<void>
  instantiate(Runtime::Instance::ComponentInstance &CompInst,
              const AST::Component::CoreInstanceSection &CoreInstSec);

  /// Instantiation of Core Type Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::CoreTypeSection &CoreTypeSec);

  /// Instantiation of Component Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ComponentSection &CompSec);

  /// Instantiation of Instance Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::InstanceSection &InstSec);

  /// Instantiation of Alias Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::AliasSection &AliasSec);

  /// Instantiation of Type Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::TypeSection &TypeSec);

  /// Instantiation of Canonical Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::CanonSection &CanonSec);

  /// Instantiation of Start Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::StartSection &StartSec);

  /// Instantiation of Value Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ValueSection &ValSec);

  /// Instantiation of Import Section from the store.
  Expect<void> instantiate(Runtime::Component::StoreManager &StoreMgr,
                           Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ImportSection &ImportSec);

  /// Instantiation of Import Section from the exports of Provider.
  Expect<void> instantiate(const Runtime::Instance::ComponentInstance &Provider,
                           Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ImportSection &ImportSec);

  /// Instantiation of Export Section.
  Expect<void> instantiate(Runtime::Instance::ComponentInstance &CompInst,
                           const AST::Component::ExportSection &ExportSec);

  /// Instantiation of a core module of CompInst with the arguments Args.
  Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
  instantiateModule(Runtime::Instance::ComponentInstance &CompInst,
                    const AST::Module &Mod,
                    Span<const AST::Component::InstantiateArg<uint32_t>> Args);

  /// A synthesized core module instance exporting items of CompInst.
  Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
  instantiateInlineExports(Runtime::Instance::ComponentInstance &CompInst,
                           Span<const AST::Component::InlineExport> Exports);

  /// Log a name of Sort that Provider does not export.
  Expect<void> logUnknownError(std::string_view Sort, std::string_view Name,
                               std::string_view Provider, ASTNodeAttr Node);

  /// Export the item at SortIdx of CompInst from DstInst under Name.
  Expect<void> addExport(Runtime::Instance::ComponentInstance &CompInst,
                         Runtime::Instance::ComponentInstance &DstInst,
                         std::string_view Name,
                         const AST::Component::SortIndex &SortIdx);

  /// Whether the exports of Inst provide the instance type Ty.
  Expect<void>
  matchInstanceType(const Runtime::Instance::ComponentInstance &Inst,
                    const AST::Component::InstanceType &Ty,
                    const Runtime::Instance::ComponentInstance &Importing);

  /// The canonical options of a canon section entry, resolved in CompInst.
  Expect<Runtime::Component::CanonOptions>
  getCanonOptions(Runtime::Instance::ComponentInstance &CompInst,
                  const AST::Component::Canonical &Canon);

  /// The flattened core type of FuncType under Opts.
  Expect<AST::FunctionType>
  flattenFuncType(const Runtime::Instance::ComponentInstance &TypeInst,
                  const AST::Component::FuncType &FuncType,
                  const Runtime::Component::CanonOptions &Opts);
  /// @}

  /// \name Canonical ABI: the layout of a value type.
  /// @{
  /// The defined value type behind Ty and the instance it reads against.
  Expect<std::pair<const AST::Component::DefValType *,
                   const Runtime::Instance::ComponentInstance *>>
  getDefValType(const Runtime::Instance::ComponentInstance &TypeInst,
                const ComponentValType &Ty);

  Expect<uint32_t>
  getAlignment(const Runtime::Instance::ComponentInstance &TypeInst,
               const ComponentValType &Ty, bool IsMem64);
  Expect<uint32_t>
  getAlignment(const Runtime::Instance::ComponentInstance &TypeInst,
               const AST::Component::DefValType &Ty, bool IsMem64);
  Expect<uint64_t>
  getElemSize(const Runtime::Instance::ComponentInstance &TypeInst,
              const ComponentValType &Ty, bool IsMem64);
  Expect<uint64_t>
  getElemSize(const Runtime::Instance::ComponentInstance &TypeInst,
              const AST::Component::DefValType &Ty, bool IsMem64);
  /// The error of a type the validator guarantees: an instantiation error
  /// while the sections are walked, else a trap.
  ErrCode::Value getInvalidTypeCode(
      const Runtime::Instance::ComponentInstance &TypeInst) const noexcept {
    const auto *Root = TypeInst.getEntryRoot();
    return Root->Entry == Runtime::Instance::ComponentInstance::EntryKind::
                              Instantiate &&
                   Root->Current == nullptr
               ? ErrCode::Value::ComponentUnexpectedType
               : ErrCode::Value::ComponentTrap;
  }
  Expect<void> flattenType(const Runtime::Instance::ComponentInstance &TypeInst,
                           const ComponentValType &Ty, bool IsMem64,
                           std::vector<ValType> &Out);
  Expect<void> flattenType(const Runtime::Instance::ComponentInstance &TypeInst,
                           const AST::Component::DefValType &Ty, bool IsMem64,
                           std::vector<ValType> &Out);
  uint32_t getDiscriminantSize(uint64_t NumCases) const noexcept;
  /// The cases of a variant-like type, each an optional payload type.
  std::vector<std::optional<ComponentValType>>
  getVariantCases(const AST::Component::DefValType &Ty) const noexcept;
  /// @}

  /// \name Canonical ABI: loading and storing values in linear memory.
  /// @{
  Expect<ComponentValVariant>
  load(const Runtime::Component::CanonOptions &Opts,
       const Runtime::Instance::ComponentInstance &TypeInst,
       const ComponentValType &Ty, uint64_t Ptr);
  Expect<ComponentValVariant>
  load(const Runtime::Component::CanonOptions &Opts,
       const Runtime::Instance::ComponentInstance &TypeInst,
       const AST::Component::DefValType &Ty, uint64_t Ptr);
  Expect<void> store(const Runtime::Component::CanonOptions &Opts,
                     const Runtime::Instance::ComponentInstance &TypeInst,
                     const ComponentValType &Ty, const ComponentValVariant &Val,
                     uint64_t Ptr);
  Expect<void> store(const Runtime::Component::CanonOptions &Opts,
                     const Runtime::Instance::ComponentInstance &TypeInst,
                     const AST::Component::DefValType &Ty,
                     const ComponentValVariant &Val, uint64_t Ptr);
  Expect<uint64_t> loadPtr(const Runtime::Component::CanonOptions &Opts,
                           uint64_t Ptr);
  Expect<void> storePtr(const Runtime::Component::CanonOptions &Opts,
                        uint64_t Ptr, uint64_t Val);
  /// Load a string in the encoding of the options, decoded to UTF-8.
  Expect<std::string> loadString(const Runtime::Component::CanonOptions &Opts,
                                 uint64_t Ptr, uint64_t PackedLen);
  /// Store a string in the encoding of the options; the pointer and length.
  Expect<std::pair<uint64_t, uint64_t>>
  storeString(const Runtime::Component::CanonOptions &Opts,
              std::string_view Str);
  /// Load a list of Len elements at Ptr.
  Expect<ComponentValVariant>
  loadList(const Runtime::Component::CanonOptions &Opts,
           const Runtime::Instance::ComponentInstance &TypeInst,
           const ComponentValType &ElemTy, uint64_t Ptr, uint64_t Len);
  Expect<ComponentValVariant>
  loadList(const Runtime::Component::CanonOptions &Opts,
           const Runtime::Instance::ComponentInstance &TypeInst,
           const AST::Component::DefValType &ElemTy, uint64_t Ptr,
           uint64_t Len);
  /// Store a list; the pointer and length.
  Expect<std::pair<uint64_t, uint64_t>>
  storeList(const Runtime::Component::CanonOptions &Opts,
            const Runtime::Instance::ComponentInstance &TypeInst,
            const ComponentValType &ElemTy,
            Span<const ComponentValVariant> Elems);
  /// Call the realloc of the options; the result is aligned and in bounds.
  Expect<uint64_t> invokeRealloc(const Runtime::Component::CanonOptions &Opts,
                                 uint64_t OldPtr, uint64_t OldSize,
                                 uint32_t Align, uint64_t NewSize);
  /// Check that Ptr is a multiple of Align.
  Expect<void> checkAligned(uint64_t Ptr, uint32_t Align);
  /// Check that Count elements of Size bytes stay within the maximum length.
  Expect<void> checkByteLength(uint64_t Count, uint64_t Size);
  /// Check that Len bytes at Ptr lie in the memory of the options.
  Expect<void> checkInBounds(const Runtime::Component::CanonOptions &Opts,
                             uint64_t Ptr, uint64_t Len, ErrCode::Value Code);
  /// Whether a code point is a Unicode scalar value.
  bool isValidChar(uint32_t CodePoint) const noexcept;
  /// Find the case a host value names: its label first, else its index.
  Expect<uint32_t> findCaseIdx(uint32_t Case, std::string_view Label,
                               Span<const std::string> Labels);
  /// @}

  /// \name Canonical ABI: lifting and lowering through flat core values.
  /// @{
  Expect<ComponentValVariant>
  liftFlat(const Runtime::Component::CanonOptions &Opts,
           const Runtime::Instance::ComponentInstance &TypeInst,
           const ComponentValType &Ty, Span<const ValVariant> Flat,
           size_t &Pos);
  Expect<ComponentValVariant>
  liftFlat(const Runtime::Component::CanonOptions &Opts,
           const Runtime::Instance::ComponentInstance &TypeInst,
           const AST::Component::DefValType &Ty, Span<const ValVariant> Flat,
           size_t &Pos);
  Expect<void> lowerFlat(const Runtime::Component::CanonOptions &Opts,
                         const Runtime::Instance::ComponentInstance &TypeInst,
                         const ComponentValType &Ty,
                         const ComponentValVariant &Val,
                         std::vector<ValVariant> &Flat);
  Expect<void> lowerFlat(const Runtime::Component::CanonOptions &Opts,
                         const Runtime::Instance::ComponentInstance &TypeInst,
                         const AST::Component::DefValType &Ty,
                         const ComponentValVariant &Val,
                         std::vector<ValVariant> &Flat);
  /// Lift the values of Types from Flat, through memory past MaxFlat.
  Expect<std::vector<ComponentValVariant>>
  liftFlatValues(const Runtime::Component::CanonOptions &Opts,
                 const Runtime::Instance::ComponentInstance &TypeInst,
                 Span<const ComponentValType> Types,
                 Span<const ValVariant> Flat, uint32_t MaxFlat);
  /// Lower Vals of Types into Flat, through memory past MaxFlat.
  Expect<void>
  lowerFlatValues(const Runtime::Component::CanonOptions &Opts,
                  const Runtime::Instance::ComponentInstance &TypeInst,
                  Span<const ComponentValType> Types,
                  Span<const ComponentValVariant> Vals, uint32_t MaxFlat,
                  std::vector<ValVariant> &Flat,
                  std::optional<uint64_t> OutPtr = std::nullopt);
  /// Whether a host value has the shape of Ty.
  Expect<void>
  checkHostValue(const Runtime::Instance::ComponentInstance &TypeInst,
                 const ComponentValType &Ty, const ComponentValVariant &Val);
  /// Whether two value types are structurally equal; resources by identity.
  Expect<bool> matchValType(const Runtime::Instance::ComponentInstance &AInst,
                            const ComponentValType &A,
                            const Runtime::Instance::ComponentInstance &BInst,
                            const ComponentValType &B);
  /// Whether values of Ty copy between memories byte for byte.
  Expect<bool>
  isRawCopyable(const Runtime::Instance::ComponentInstance &TypeInst,
                const ComponentValType &Ty);
  /// A NaN crosses the boundary in its canonical form.
  float canonicalizeNaN(float F) const noexcept;
  double canonicalizeNaN(double D) const noexcept;
  /// @}

  /// \name Canonical ABI: handles and transferred streams.
  /// @{
  Expect<uint32_t>
  lowerOwn(const Runtime::Component::CanonOptions &Opts,
           const Runtime::Instance::ComponentInstance &TypeInst,
           uint32_t TypeIdx, uint64_t Rep);
  Expect<ValVariant>
  lowerBorrow(const Runtime::Component::CanonOptions &Opts,
              const Runtime::Instance::ComponentInstance &TypeInst,
              uint32_t TypeIdx, uint64_t Rep);
  Expect<uint64_t> liftOwn(const Runtime::Component::CanonOptions &Opts,
                           const Runtime::Instance::ComponentInstance &TypeInst,
                           uint32_t TypeIdx, uint32_t Handle);
  Expect<uint64_t>
  liftBorrow(const Runtime::Component::CanonOptions &Opts,
             const Runtime::Instance::ComponentInstance &TypeInst,
             uint32_t TypeIdx, uint32_t Handle);
  Expect<uint32_t> lowerStream(const Runtime::Component::CanonOptions &Opts,
                               const ComponentValVariant &Val);
  Expect<ComponentValVariant>
  liftStream(const Runtime::Component::CanonOptions &Opts, uint32_t Handle,
             bool IsStream);
  /// The resource handle at Idx of Inst, of the type ResType when given.
  Expect<Runtime::Instance::Component::ResourceHandle *> getResourceHandle(
      Runtime::Instance::ComponentInstance &Inst, uint32_t Idx,
      const Runtime::Instance::Component::ResourceTypeInstance *ResType);
  /// The subtask at Idx of Inst.
  Expect<Runtime::Component::Task *>
  getSubtask(const Runtime::Instance::ComponentInstance &Inst, uint32_t Idx);
  /// The stream behind the handle of the role Role at Idx of Inst.
  Expect<Runtime::Instance::Component::StreamInstance *>
  getStream(const Runtime::Instance::ComponentInstance &Inst, uint32_t Idx,
            bool IsFuture,
            Runtime::Instance::Component::StreamInstance::Role Role);
  /// Whether the waitable at Idx has an event pending, and the event.
  bool hasPendingEvent(const Runtime::Instance::ComponentInstance &Inst,
                       uint32_t Idx) const noexcept;
  Component::WaitableEvent
  takePendingEvent(const Runtime::Instance::ComponentInstance &Inst,
                   uint32_t Idx) noexcept;
  /// Collect the result of the copy on Role as the payload of its event.
  uint32_t takeCopyResult(
      Runtime::Instance::Component::StreamInstance &S,
      Runtime::Instance::Component::StreamInstance::Role Role) noexcept;
  /// Move the waitable at Idx into the set at SetIdx; 0 for no set.
  Expect<void> joinWaitableSet(Runtime::Instance::ComponentInstance &Inst,
                               uint32_t Idx, uint32_t SetIdx);
  /// @}

  /// \name The canonical built-ins.
  /// @{
  Expect<void> runCanonLower(const Component::CanonFunction &Canon,
                             Span<const ValVariant> Args,
                             Span<ValVariant> Rets);
  Expect<void> runCanonResourceNew(const Component::CanonFunction &Canon,
                                   Span<const ValVariant> Args,
                                   Span<ValVariant> Rets);
  Expect<void> runCanonResourceRep(const Component::CanonFunction &Canon,
                                   Span<const ValVariant> Args,
                                   Span<ValVariant> Rets);
  Expect<void> runCanonResourceDrop(const Component::CanonFunction &Canon,
                                    Span<const ValVariant> Args);
  Expect<void> runCanonBackpressure(const Component::CanonFunction &Canon,
                                    bool Inc);
  Expect<void> runCanonTaskReturn(const Component::CanonFunction &Canon,
                                  Span<const ValVariant> Args);
  Expect<void> runCanonTaskCancel(const Component::CanonFunction &Canon);
  Expect<void> runCanonContextGet(const Component::CanonFunction &Canon,
                                  Span<ValVariant> Rets);
  Expect<void> runCanonContextSet(const Component::CanonFunction &Canon,
                                  Span<const ValVariant> Args);
  Expect<void> runCanonYield(const Component::CanonFunction &Canon,
                             Span<ValVariant> Rets);
  Expect<void> runCanonSubtaskCancel(const Component::CanonFunction &Canon,
                                     Span<const ValVariant> Args,
                                     Span<ValVariant> Rets);
  Expect<void> runCanonSubtaskDrop(const Component::CanonFunction &Canon,
                                   Span<const ValVariant> Args);
  Expect<void> runCanonStreamNew(const Component::CanonFunction &Canon,
                                 Span<ValVariant> Rets, bool IsStream);
  Expect<void> runCanonStreamCopy(const Component::CanonFunction &Canon,
                                  Span<const ValVariant> Args,
                                  Span<ValVariant> Rets, bool IsStream,
                                  bool IsRead);
  Expect<void> runCanonStreamCancel(const Component::CanonFunction &Canon,
                                    Span<const ValVariant> Args,
                                    Span<ValVariant> Rets, bool IsStream,
                                    bool IsRead);
  Expect<void> runCanonStreamDrop(const Component::CanonFunction &Canon,
                                  Span<const ValVariant> Args, bool IsStream,
                                  bool IsRead);
  Expect<void> runCanonStreamForward(const Component::CanonFunction &Canon,
                                     Span<const ValVariant> Args,
                                     bool IsStream);
  Expect<void> runCanonErrorContextNew(const Component::CanonFunction &Canon,
                                       Span<const ValVariant> Args,
                                       Span<ValVariant> Rets);
  Expect<void>
  runCanonErrorContextDebugMessage(const Component::CanonFunction &Canon,
                                   Span<const ValVariant> Args);
  Expect<void> runCanonErrorContextDrop(const Component::CanonFunction &Canon,
                                        Span<const ValVariant> Args);
  Expect<void> runCanonWaitableSetNew(const Component::CanonFunction &Canon,
                                      Span<ValVariant> Rets);
  Expect<void> runCanonWaitableSetWait(const Component::CanonFunction &Canon,
                                       Span<const ValVariant> Args,
                                       Span<ValVariant> Rets, bool IsPoll);
  Expect<void> runCanonWaitableSetDrop(const Component::CanonFunction &Canon,
                                       Span<const ValVariant> Args);
  Expect<void> runCanonWaitableJoin(const Component::CanonFunction &Canon,
                                    Span<const ValVariant> Args);
  Expect<void> runCanonThreadIndex(const Component::CanonFunction &Canon,
                                   Span<ValVariant> Rets);
  Expect<void> runCanonThreadNewIndirect(const Component::CanonFunction &Canon,
                                         Span<const ValVariant> Args,
                                         Span<ValVariant> Rets);
  Expect<void> runCanonThreadResumeLater(const Component::CanonFunction &Canon,
                                         Span<const ValVariant> Args);
  Expect<void> runCanonThreadSuspend(const Component::CanonFunction &Canon,
                                     Span<ValVariant> Rets);
  Expect<void> runCanonThreadSwitch(const Component::CanonFunction &Canon,
                                    Span<const ValVariant> Args,
                                    Span<ValVariant> Rets, bool IsYield,
                                    bool IsPromote);

  /// Store an event at Ptr and return its code.
  Expect<uint32_t> storeEvent(const Component::CanonFunction &Canon,
                              const Component::WaitableEvent &Pending,
                              uint64_t Ptr);
  /// Start the copy loaded on the end Role of S, moving elements to or from
  /// the buffer of its peer when both wait.
  Expect<void>
  startStreamCopy(Runtime::Instance::Component::StreamInstance &S,
                  Runtime::Instance::Component::StreamInstance::Role Role);
  /// Move Count elements from the buffer of a writer to that of a reader.
  Expect<void>
  moveElements(Runtime::Instance::Component::StreamBuffer &SrcBuffer,
               Runtime::Instance::Component::StreamBuffer &DstBuffer,
               uint32_t Count);
  /// Trap unless the end Role of S at Idx may be forwarded by Canon.
  Expect<void>
  checkForwardEnd(const Component::CanonFunction &Canon,
                  const Runtime::Instance::Component::StreamInstance &S,
                  Runtime::Instance::Component::StreamInstance::Role Role,
                  uint32_t Idx, bool IsStream);
  /// The result of a copy built-in; BLOCKED for an asynchronous copy.
  Expect<uint32_t>
  waitCopyResult(const Runtime::Instance::ComponentInstance &Inst,
                 Runtime::Instance::Component::StreamInstance &S,
                 Runtime::Instance::Component::StreamInstance::Role Role,
                 bool IsAsync);
  /// @}

  /// \name The tasks and threads.
  /// @{
  /// Build and run the task of a lifted component function for Caller.
  Expect<Runtime::Component::Task *>
  liftCall(const Runtime::Instance::ComponentFunctionInstance *FuncInst,
           Runtime::Component::Task::OnStartCallback OnStart,
           Runtime::Component::Task::OnResolveCallback OnResolve,
           Runtime::Component::Task *Caller,
           Runtime::Component::Thread *CallerThread);

  /// Run the body of a lifted guest task on the thread Cur.
  Expect<void> runGuestTask(Runtime::Component::Task &T,
                            Runtime::Component::Thread &Cur);
  /// Run the callback loop of an async callback lift.
  Expect<void> runCallbackLoop(Runtime::Component::Task &T,
                               Runtime::Component::Thread &Cur, uint32_t Code);
  /// Run the body of a host task on the thread Cur.
  Expect<void> runHostTask(Runtime::Component::Task &T,
                           Runtime::Component::Thread &Cur);
  /// Run the body of a detached host task the host spawned.
  Expect<void> runSpawnedTask(Runtime::Component::Task &T,
                              Runtime::Component::Thread &Cur,
                              const Runtime::Component::Task::SpawnBody &Body);
  /// Wait on the waitable set at SetIdx of Inst; between two callbacks when
  /// BetweenCallbacks, where a pending cancellation also ends the wait.
  Expect<Component::WaitableEvent>
  waitOnWaitableSet(Runtime::Component::Task &T,
                    Runtime::Component::Thread &Cur,
                    const Runtime::Instance::ComponentInstance &Inst,
                    uint32_t SetIdx, bool BetweenCallbacks);
  /// Run a resource destructor as an implicit sync task of its instance, on
  /// the stack of Cur; null from the embedder thread.
  Expect<void> runResourceDtor(Runtime::Instance::ComponentInstance *Impl,
                               Runtime::Component::Thread *Cur,
                               Runtime::Instance::FunctionInstance *Dtor,
                               uint64_t Rep);
  /// Run Body as an implicit synchronous task of Inst, on the stack of Cur;
  /// null from the embedder thread.
  Expect<void> runImplicitTask(Runtime::Instance::ComponentInstance *Inst,
                               Runtime::Component::Thread *Cur,
                               const std::function<Expect<void>()> &Body);
  /// Run Body as one embedder entry into the store of Root.
  Expect<void> runEntry(Runtime::Instance::ComponentInstance &Root,
                        Runtime::Instance::ComponentInstance::EntryKind Kind,
                        const std::function<Expect<void>()> &Body);
  /// A new root thread of T in Store, running Body.
  Runtime::Component::Thread *
  newRootThread(Runtime::Instance::ComponentInstance &Store,
                Runtime::Component::Task &T, std::function<void()> Body);
  /// Call the core function Func with Args on the current stack.
  Expect<std::vector<ValVariant>>
  invokeCore(const Runtime::Instance::FunctionInstance *Func,
             Span<const ValVariant> Args);

  /// The innermost thread on the stack the store of Inst runs; null on the
  /// embedder thread.
  Runtime::Component::Thread *getCurrentThread(
      const Runtime::Instance::ComponentInstance &Inst) const noexcept {
    const auto *Cur = Inst.getEntryRoot()->Current;
    return Cur == nullptr ? nullptr : Cur->getInnermost();
  }
  /// The task of the current thread.
  Runtime::Component::Task *getCurrentTask(
      const Runtime::Instance::ComponentInstance &Inst) const noexcept {
    auto *Cur = getCurrentThread(Inst);
    return Cur == nullptr ? nullptr : &Cur->getOwner();
  }
  /// @}

  /// \name The streams and the copies through host storage.
  /// @{
  /// The stream behind a handle of the host of Frame, held as Role.
  Expect<Runtime::Instance::Component::StreamInstance *>
  getHostStream(const Runtime::Component::CallingFrame &Frame,
                uint32_t HandleIdx,
                Runtime::Instance::Component::StreamInstance::Role Role);
  /// The ends Val carries enter the handle table of HostRoot.
  Expect<void> lowerHostStreams(Runtime::Instance::ComponentInstance &HostRoot,
                                ComponentValVariant &Val);
  /// The ends Val names by handle leave the handle table of HostRoot.
  Expect<void> liftHostStreams(Runtime::Instance::ComponentInstance &HostRoot,
                               ComponentValVariant &Val);
  /// Load the host buffer of the role Role of S for Length elements.
  void loadHostBuffer(const Runtime::Component::CallingFrame &Frame,
                      Runtime::Instance::Component::StreamInstance &S,
                      Runtime::Instance::Component::StreamInstance::Role Role,
                      uint32_t Length) noexcept;
  /// One copy on the role Role of S, blocking until its result.
  Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                   uint32_t>>
  copyHostBuffer(Runtime::Component::CallingFrame &Frame,
                 Runtime::Instance::Component::StreamInstance &S,
                 Runtime::Instance::Component::StreamInstance::Role Role);
  /// Copy the loaded host buffer until Total elements moved.
  Expect<std::pair<Runtime::Instance::Component::StreamInstance::CopyResult,
                   uint32_t>>
  writeHostBuffer(Runtime::Component::CallingFrame &Frame,
                  Runtime::Instance::Component::StreamInstance &S,
                  uint32_t Total);
  /// @}

  /// \name The scheduler.
  /// @{
  /// Hand the stack to Target until it parks or ends; a parked non-async-typed
  /// guest call then runs its loop unless its stack is pinned already.
  void resumeThread(Runtime::Component::Thread &Target,
                    Runtime::Component::Thread::WakeReason Wake);
  /// Resume the ready threads of the store of Root and of the stores it
  /// imports from until Done(); a sync call on Pin limits them to its instance.
  Expect<void> pump(Runtime::Instance::ComponentInstance &Root,
                    const std::function<bool()> &Done,
                    Runtime::Component::Thread *Pin = nullptr);
  /// Mark the waiting threads of Roots whose OS handle turned ready. Blocking
  /// waits for one or for the earliest sleep; false when none waits on either.
  bool pollWaiting(Span<Runtime::Instance::ComponentInstance *const> Roots,
                   bool Block);
  /// Start the detached host tasks Spawner asked for.
  void startSpawnedTasks(Runtime::Component::Task &Spawner);
  /// The first waiting thread that is ready and that Eligible accepts.
  Runtime::Component::Thread *
  findReadyThread(Span<Runtime::Instance::ComponentInstance *const> Roots,
                  const std::function<bool(const Runtime::Component::Thread &)>
                      &Eligible) const noexcept;
  /// The first trap latched in Roots.
  std::optional<ErrCode> getTrap(
      Span<Runtime::Instance::ComponentInstance *const> Roots) const noexcept;
  /// Latch the first trap in the store of Inst.
  void setTrap(ErrCode Err,
               const Runtime::Instance::ComponentInstance &Inst) noexcept;
  /// @}

  /// \name Data of component executor.
  /// @{
  Executor &Core;
  /// Whether the calling OS thread runs an entry.
  static thread_local bool InEntry;
  /// @}
};

} // namespace Executor
} // namespace WasmEdge
