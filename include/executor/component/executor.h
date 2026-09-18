// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/executor/component/executor.h - Component executor -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component-model executor. It instantiates
/// components, runs the canonical built-ins, and schedules the tasks and
/// threads of the calls in progress, on top of the core executor.
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
#include "runtime/instance/module.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

/// The component-model executor. It instantiates components section by
/// section and is the only one that resumes the threads of the tasks.
class ComponentExecutor {
  /// What an asynchronous built-in returns in place of a block.
  static inline constexpr const uint32_t TransmitBlocked = 0xffffffffU;
  /// The bytes a sink takes per read.
  static inline constexpr const uint32_t SinkChunkSize = 65536;

  /// The event a wait, a poll, or a callback delivers: its code, the handle
  /// it concerns, and its payload.
  enum class EventCode : uint32_t {
    None = 0,
    Subtask = 1,
    StreamRead = 2,
    StreamWrite = 3,
    FutureRead = 4,
    FutureWrite = 5,
    TaskCancelled = 6,
  };
  struct Event {
    EventCode Code = EventCode::None;
    uint32_t Idx = 0;
    uint32_t Payload = 0;
  };

public:
  explicit ComponentExecutor(Executor &CoreExec) noexcept
      : Core(CoreExec),
        WakeUp(std::make_shared<Runtime::Component::Thread::Signal>()) {}
  ~ComponentExecutor() noexcept { cleanup(); }
  ComponentExecutor(const ComponentExecutor &) = delete;
  ComponentExecutor &operator=(const ComponentExecutor &) = delete;

  /// Instantiate a component as an anonymous component instance. The
  /// instance copies the definitions it keeps, as a core module instance
  /// copies its types and its code, so Comp is read only here.
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

  /// Whether a code point is a Unicode scalar value.
  static bool isValidChar(uint32_t CodePoint) noexcept;

  /// \name The ends of streams and futures the host holds.
  /// @{
  /// A stream or future of the element type Elem, read against the instance
  /// of the activation; the host keeps the writable end.
  std::shared_ptr<Runtime::Instance::Component::Stream>
  newHostStream(const Runtime::Component::CallingFrame &Frame, bool IsFuture,
                std::optional<ComponentValType> Elem);
  /// Write Data through the writable end of S, parking until the guest read
  /// all of it or dropped its end; the outcome and the elements moved.
  Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
  write(Runtime::Component::CallingFrame &Frame,
        Runtime::Instance::Component::Stream &S, Span<const uint8_t> Data);
  Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
  write(Runtime::Component::CallingFrame &Frame,
        Runtime::Instance::Component::Stream &S,
        std::vector<ComponentValVariant> Vals);
  /// Read up to Max elements through the readable end of S, parking until
  /// the guest wrote or dropped; the outcome and the elements moved.
  Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
  read(Runtime::Component::CallingFrame &Frame,
       Runtime::Instance::Component::Stream &S, std::vector<uint8_t> &Out,
       uint32_t Max);
  Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
  read(Runtime::Component::CallingFrame &Frame,
       Runtime::Instance::Component::Stream &S,
       std::vector<ComponentValVariant> &Out, uint32_t Max);
  /// Drop the end E of S: a parked peer learns of it at once.
  void drop(Runtime::Instance::Component::Stream &S,
            Runtime::Instance::Component::Stream::End E);
  /// Write one value through S from a detached host task, which then drops
  /// the writable end.
  void writeDetached(Runtime::Component::CallingFrame &Frame,
                     std::shared_ptr<Runtime::Instance::Component::Stream> S,
                     ComponentValVariant Val);
  /// Consume every guest write to S from a detached host task until Consume
  /// returns false; OnDone(Frame, Failed) runs in that task at the end.
  void
  sink(Runtime::Component::CallingFrame &Frame,
       std::shared_ptr<Runtime::Instance::Component::Stream> S,
       std::function<bool(Span<const uint8_t>)> Consume,
       std::function<void(Runtime::Component::CallingFrame &, bool)> OnDone);
  /// @}

private:
  /// \name Functions for instantiation.
  /// @{
  /// Instantiation of a root component instance: the imports resolve from
  /// the store, and the instance is registered under Name if given.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  instantiate(Runtime::Component::StoreManager &StoreMgr,
              const AST::Component::Component &Comp,
              std::optional<std::string_view> Name = std::nullopt);

  /// Instantiation of a nested component instance: the imports resolve from
  /// the exports of Provider, the instance of the instantiation arguments.
  Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
  instantiate(const Runtime::Instance::ComponentInstance &Provider,
              const AST::Component::Component &Comp,
              Runtime::Instance::ComponentInstance *Parent);

  /// The section walk shared by both instantiation entries: the imports
  /// resolve from the store at the root, else from Provider.
  Expect<void>
  instantiate(Runtime::Instance::ComponentInstance &CompInst,
              const AST::Component::Component &Comp,
              Runtime::Component::StoreManager *StoreMgr,
              const Runtime::Instance::ComponentInstance *Provider);

  /// \name Copies of the AST definitions a root instance owns. One per node
  /// type, like the loader's readers but reading a source node instead of the
  /// file; a root instance walks its own copy, so no instance reads an AST
  /// the embedder may free.
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

  /// Instantiation of a core module of CompInst with the instantiation
  /// arguments Args, each an index in the core instance index space.
  Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
  instantiateModule(Runtime::Instance::ComponentInstance &CompInst,
                    const AST::Module &Mod,
                    Span<const AST::Component::InstantiateArg<uint32_t>> Args);

  /// A synthesized core module instance exporting the named items of the
  /// core index spaces of CompInst.
  Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
  instantiateModule(Runtime::Instance::ComponentInstance &CompInst,
                    Span<const AST::Component::InlineExport> Exports);

  /// Add the item of the sort index space at Idx of CompInst to the exports
  /// of the destination instance under Name.
  Expect<void> addExport(Runtime::Instance::ComponentInstance &CompInst,
                         Runtime::Instance::ComponentInstance &DstInst,
                         std::string_view Name,
                         const AST::Component::SortIndex &SortIdx);

  /// Whether the exports of Inst provide the instance type Ty; the shape of
  /// a root import resolved from the store. Outer type aliases of the
  /// instance type resolve through Importing.
  Expect<void>
  matchInstanceType(const Runtime::Instance::ComponentInstance &Inst,
                    const AST::Component::InstanceType &Ty,
                    const Runtime::Instance::ComponentInstance &Importing);

  /// The canonical options of a canon section entry, resolved in CompInst.
  Expect<Runtime::Component::CanonOptions>
  getCanonOptions(const Runtime::Instance::ComponentInstance &CompInst,
                  const AST::Component::Canonical &Canon);

  /// The core type of the lower of FuncType under Opts, per the flattening
  /// rules of the specification.
  Expect<AST::FunctionType>
  flattenFuncType(const Runtime::Instance::ComponentInstance &TypeInst,
                  const AST::Component::FuncType &FuncType,
                  const Runtime::Component::CanonOptions &Opts);
  /// @}

  /// \name Canonical ABI: the layout of a value type.
  /// @{
  static inline constexpr const uint32_t MaxFlatParams = 16;
  static inline constexpr const uint32_t MaxFlatAsyncParams = 4;
  static inline constexpr const uint32_t MaxFlatResults = 1;
  /// The flag of a UTF-16 string in a `latin1+utf16` packed length.
  static inline constexpr const uint64_t UTF16Tag = UINT64_C(1) << 31;
  /// The most elements one stream copy may carry.
  static inline constexpr const uint32_t MaxCopyLength = (1U << 28) - 1;

  /// The defined value type behind a type index, and the instance whose
  /// index space its nested indices read against.
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
  Expect<void> flattenType(const Runtime::Instance::ComponentInstance &TypeInst,
                           const ComponentValType &Ty, bool IsMem64,
                           std::vector<ValType> &Out);
  Expect<void> flattenType(const Runtime::Instance::ComponentInstance &TypeInst,
                           const AST::Component::DefValType &Ty, bool IsMem64,
                           std::vector<ValType> &Out);
  static uint32_t getDiscriminantSize(uint64_t NumCases) noexcept;
  static uint64_t alignTo(uint64_t Ptr, uint32_t Align) noexcept;
  /// The cases of a variant-like type, each an optional payload type.
  static std::vector<std::optional<ComponentValType>>
  getVariantCases(const AST::Component::DefValType &Ty) noexcept;
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
  /// Load a string: the pointer and packed length as the encoding lays them
  /// out, decoded to UTF-8.
  Expect<std::string> loadString(const Runtime::Component::CanonOptions &Opts,
                                 uint64_t Ptr, uint64_t PackedLen);
  /// Store a string with the encoding of the options; the pointer and the
  /// packed length the guest reads.
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
  /// Store the elements of a list; the pointer and length the guest reads.
  Expect<std::pair<uint64_t, uint64_t>>
  storeList(const Runtime::Component::CanonOptions &Opts,
            const Runtime::Instance::ComponentInstance &TypeInst,
            const ComponentValType &ElemTy,
            Span<const ComponentValVariant> Elems);
  /// Call the realloc of the options; the returned pointer is aligned and
  /// in bounds.
  Expect<uint64_t> invokeRealloc(const Runtime::Component::CanonOptions &Opts,
                                 uint64_t OldPtr, uint64_t OldSize,
                                 uint32_t Align, uint64_t NewSize);
  /// Check that Ptr is a multiple of Align.
  Expect<void> checkAligned(uint64_t Ptr, uint32_t Align);
  /// Check that Count elements of Size bytes stay within the canonical ABI
  /// maximum byte length.
  Expect<void> checkByteLength(uint64_t Count, uint64_t Size);
  /// Check that Len bytes at Ptr lie in the memory of the options.
  Expect<void> checkInBounds(const Runtime::Component::CanonOptions &Opts,
                             uint64_t Ptr, uint64_t Len);
  /// Find the case a host value names: its label first, else its index.
  Expect<uint32_t> findCaseIdx(uint32_t Case, std::string_view Label,
                               Span<const std::string> Labels);
  /// @}

  /// \name Canonical ABI: lifting and lowering through flat core values.
  /// @{
  Expect<ComponentValVariant>
  lift(const Runtime::Component::CanonOptions &Opts,
       const Runtime::Instance::ComponentInstance &TypeInst,
       const ComponentValType &Ty, Span<const ValVariant> Flat, size_t &Pos);
  Expect<ComponentValVariant>
  lift(const Runtime::Component::CanonOptions &Opts,
       const Runtime::Instance::ComponentInstance &TypeInst,
       const AST::Component::DefValType &Ty, Span<const ValVariant> Flat,
       size_t &Pos);
  Expect<void> lower(const Runtime::Component::CanonOptions &Opts,
                     const Runtime::Instance::ComponentInstance &TypeInst,
                     const ComponentValType &Ty, const ComponentValVariant &Val,
                     std::vector<ValVariant> &Flat);
  Expect<void> lower(const Runtime::Component::CanonOptions &Opts,
                     const Runtime::Instance::ComponentInstance &TypeInst,
                     const AST::Component::DefValType &Ty,
                     const ComponentValVariant &Val,
                     std::vector<ValVariant> &Flat);
  /// Lift the values of Types from Flat, through memory past MaxFlat.
  Expect<std::vector<ComponentValVariant>>
  liftValues(const Runtime::Component::CanonOptions &Opts,
             const Runtime::Instance::ComponentInstance &TypeInst,
             Span<const ComponentValType> Types, Span<const ValVariant> Flat,
             uint32_t MaxFlat);
  /// Lower Vals of Types into Flat, through memory past MaxFlat; a lower
  /// past MaxFlat writes at OutPtr when given, else allocates.
  Expect<void> lowerValues(const Runtime::Component::CanonOptions &Opts,
                           const Runtime::Instance::ComponentInstance &TypeInst,
                           Span<const ComponentValType> Types,
                           Span<const ComponentValVariant> Vals,
                           uint32_t MaxFlat, std::vector<ValVariant> &Flat,
                           std::optional<uint64_t> OutPtr = std::nullopt);
  /// Whether a host value has the shape of Ty.
  Expect<void> checkValue(const Runtime::Instance::ComponentInstance &TypeInst,
                          const ComponentValType &Ty,
                          const ComponentValVariant &Val);
  /// Whether two value types are structurally equal; resources by identity.
  Expect<bool> matchValType(const Runtime::Instance::ComponentInstance &AInst,
                            const ComponentValType &A,
                            const Runtime::Instance::ComponentInstance &BInst,
                            const ComponentValType &B);
  /// Whether values of Ty copy between memories byte for byte: only numbers
  /// and aggregates of numbers need no lifting on the way.
  Expect<bool>
  isRawCopyable(const Runtime::Instance::ComponentInstance &TypeInst,
                const ComponentValType &Ty);
  /// A flat slot as a bit pattern, and back: what coerces a variant payload
  /// into the joined slot type.
  static uint64_t getSlotBits(const ValVariant &Val,
                              const ValType &Ty) noexcept;
  static ValVariant newSlot(const ValType &Ty, uint64_t Bits) noexcept;
  /// A NaN crosses the boundary in its canonical form.
  static float canonicalizeNaN(float F) noexcept;
  static double canonicalizeNaN(double D) noexcept;
  /// @}

  /// \name Canonical ABI: handles and transferred ends.
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
  /// The resource entry at Idx of the handles table of Inst, of the resource
  /// type ResType when given.
  Expect<Runtime::Instance::ComponentInstance::ResourceHandle *>
  getHandle(const Runtime::Instance::ComponentInstance &Inst, uint32_t Idx,
            const Runtime::Instance::Component::ResourceTypeInstance *ResType);
  /// The subtask at Idx of the handles table of Inst: the callee task.
  Expect<Runtime::Component::Task *>
  getSubtask(const Runtime::Instance::ComponentInstance &Inst, uint32_t Idx);
  /// The stream whose end E, of a future when IsFuture, sits at Idx of the
  /// handles table of Inst.
  Expect<Runtime::Instance::Component::Stream *>
  getStreamEnd(const Runtime::Instance::ComponentInstance &Inst, uint32_t Idx,
               bool IsFuture, Runtime::Instance::Component::Stream::End E);
  /// Whether the waitable at Idx has an event pending, and the event: the
  /// progress of a subtask or the outcome of a copy.
  bool hasPendingEvent(const Runtime::Instance::ComponentInstance &Inst,
                       uint32_t Idx) const noexcept;
  Event takePendingEvent(const Runtime::Instance::ComponentInstance &Inst,
                         uint32_t Idx) noexcept;
  /// Collect the outcome of the copy on E as the payload of its event.
  static uint32_t
  takeOutcome(Runtime::Instance::Component::Stream &S,
              Runtime::Instance::Component::Stream::End E) noexcept;
  /// Move the waitable at Idx into the set at SetIdx, or out of every set
  /// when SetIdx is 0.
  Expect<void> joinWaitableSet(const Runtime::Instance::ComponentInstance &Inst,
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

  /// Write the event a wait or poll delivered into the guest memory at Ptr
  /// and return its code.
  Expect<uint32_t> storeEvent(const Component::CanonFunction &Canon,
                              const Event &Pending, uint64_t Ptr);
  /// The trap of an async built-in a synchronous task may not block in.
  Expect<void> checkMayBlock(Runtime::Component::Task &T,
                             Runtime::Component::Thread &Cur);
  /// Start the copy the end E of S loaded: the rendezvous decides, and the
  /// elements it names move between the buffers.
  Expect<void> copy(Runtime::Instance::Component::Stream &S,
                    Runtime::Instance::Component::Stream::End E);
  /// Move Count elements from the writable buffer of S to its readable one.
  Expect<void> copyElements(Runtime::Instance::Component::Stream &S,
                            uint32_t Count);
  /// The outcome of a copy built-in on the end E of S: collected, BLOCKED
  /// for an asynchronous copy, or awaited on the current thread.
  Expect<uint32_t> takeCopyOutcome(Runtime::Instance::Component::Stream &S,
                                   Runtime::Instance::Component::Stream::End E,
                                   bool IsAsync);
  /// @}

  /// \name The tasks and threads.
  /// @{
  /// Build and run the task of a lifted component function for Caller: a
  /// synchronous one on the stack of its caller, else on a stack of its own.
  Expect<Runtime::Component::Task *>
  liftCall(const Runtime::Instance::ComponentFunctionInstance *FuncInst,
           Runtime::Component::Task::OnStartCallback OnStart,
           Runtime::Component::Task::OnResolveCallback OnResolve,
           Runtime::Component::Task *Caller);

  /// Run the body of a lifted guest task on the thread Cur.
  Expect<void> runTask(Runtime::Component::Task &T,
                       Runtime::Component::Thread &Cur);
  /// Run the callback loop of an async callback lift from the packed Code.
  Expect<void> runCallbackLoop(Runtime::Component::Task &T,
                               Runtime::Component::Thread &Cur, uint32_t Code);
  /// Run the body of a host task on the thread Cur.
  Expect<void> runHostTask(Runtime::Component::Task &T,
                           Runtime::Component::Thread &Cur);
  /// Run the body of a detached host task the host spawned.
  Expect<void> runSpawnedTask(Runtime::Component::Task &T,
                              Runtime::Component::Thread &Cur,
                              const Runtime::Component::Task::SpawnBody &Body);
  /// Wait on the waitable set at SetIdx of Inst for T on Cur: the event
  /// collected, or the cancellation delivered at this suspension point.
  Expect<Event> waitOnSet(Runtime::Component::Task &T,
                          Runtime::Component::Thread &Cur,
                          const Runtime::Instance::ComponentInstance &Inst,
                          uint32_t SetIdx, bool Cancellable);
  /// Run a resource destructor as an implicit sync task of its instance.
  Expect<void> runResourceDtor(const Runtime::Instance::ComponentInstance *Impl,
                               Runtime::Instance::FunctionInstance *Dtor,
                               uint64_t Rep);
  /// Run Body as an implicit synchronous task of Inst: nested on the current
  /// stack, or pumped on a stack of its own from the embedder thread.
  Expect<void> runImplicitTask(const Runtime::Instance::ComponentInstance *Inst,
                               const std::function<Expect<void>()> &Body);
  /// Run Body as one embedder entry. The outermost entry drains the ended
  /// tasks afterwards; a trap aborts everything and is what it returns.
  Expect<void> runEntry(const std::function<Expect<void>()> &Body);
  /// Call the core function Func with Args on the current stack.
  Expect<std::vector<ValVariant>>
  invokeCore(const Runtime::Instance::FunctionInstance *Func,
             Span<const ValVariant> Args);

  /// Add a task to the tasks the executor owns.
  Runtime::Component::Task *
  addTask(std::shared_ptr<Runtime::Component::Task> T);
  /// Find the owning handle of a task the executor holds.
  std::shared_ptr<Runtime::Component::Task>
  findTask(Runtime::Component::Task *T) const noexcept;
  /// A thread of T on a stack of its own, running Body once resumed.
  Runtime::Component::Thread *newThread(Runtime::Component::Task &T,
                                        Runtime::Component::Thread::Body Body);
  /// A thread of T on the stack of the current thread.
  Runtime::Component::Thread *newNestedThread(Runtime::Component::Task &T);
  /// The innermost thread on the stack in hand; null on the embedder thread.
  Runtime::Component::Thread *getCurrentThread() const noexcept {
    return Current == nullptr ? nullptr : Current->getInnermost();
  }
  /// The task of the current thread.
  Runtime::Component::Task *getCurrentTask() const noexcept {
    auto *Cur = getCurrentThread();
    return Cur == nullptr ? nullptr : &Cur->getOwner();
  }
  /// @}

  /// \name The copies through host storage.
  /// @{
  /// Load the host storage of the end E of S for a copy of Length elements.
  void loadHostBuffer(const Runtime::Component::CallingFrame &Frame,
                      Runtime::Instance::Component::Stream &S,
                      Runtime::Instance::Component::Stream::End E,
                      uint32_t Length) noexcept;
  /// One copy on the end E of S, parked until its outcome.
  Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
  copyHostBuffer(Runtime::Component::CallingFrame &Frame,
                 Runtime::Instance::Component::Stream &S,
                 Runtime::Instance::Component::Stream::End E);
  /// Copies of the loaded host storage until Total elements moved.
  Expect<std::pair<Runtime::Instance::Component::Stream::Result, uint32_t>>
  writeHostBuffer(Runtime::Component::CallingFrame &Frame,
                  Runtime::Instance::Component::Stream &S, uint32_t Total);
  /// @}

  /// \name The scheduler.
  /// @{
  /// Hand the stack to Target until the threads it names park or end.
  void resume(Runtime::Component::Thread &Target,
              Runtime::Component::Thread::Reason Wake);
  /// Resume ready parked threads until Done(); traps on a deadlock. A
  /// non-async-typed task parked on Pin restricts what may take the stack.
  Expect<void> pump(const std::function<bool()> &Done,
                    Runtime::Component::Thread *Pin = nullptr);
  /// Start the detached host tasks Spawner asked for on their own stacks.
  void startSpawns(Runtime::Component::Task &Spawner);
  /// The first waiting thread that is ready and that Eligible accepts.
  Runtime::Component::Thread *findReadyThread(
      const std::function<bool(const Runtime::Component::Thread &)> &Eligible)
      const noexcept;
  /// Record the first trap and poison the instance tree containing Inst.
  void setTrap(ErrCode Err,
               const Runtime::Instance::ComponentInstance *Inst) noexcept;
  /// Abort and join every live stack, then drop every task and thread.
  void cleanup() noexcept;
  /// Drop the tasks and threads that have ended.
  void drainEndedTasks() noexcept;
  /// @}

  /// \name Data of component executor.
  /// @{
  /// The core executor below this one.
  Executor &Core;
  /// The wake-up the scheduler sleeps on when only a host operation on
  /// another OS thread or a deadline can make progress.
  std::shared_ptr<Runtime::Component::Thread::Signal> WakeUp;
  /// A task is shared with the subtask handle naming it.
  std::vector<std::shared_ptr<Runtime::Component::Task>> OwnedTasks;
  std::vector<std::unique_ptr<Runtime::Component::Thread>> OwnedThreads;
  /// The parked threads with a readiness predicate, in park order.
  std::vector<Runtime::Component::Thread *> Waiting;
  /// The thread the stack was last handed to; null while the embedder
  /// thread pumps.
  Runtime::Component::Thread *Current = nullptr;
  /// The first trap of the entry in progress.
  std::optional<ErrCode> Trap;
  /// Depth of the embedder entries in progress.
  uint32_t EntryDepth = 0;
  /// @}
};

} // namespace Executor
} // namespace WasmEdge
