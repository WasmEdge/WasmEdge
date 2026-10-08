// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "jit_debug.h"

#include "common/defines.h"

#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#if LLVM_VERSION_MAJOR < 15
#else
#include <llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/Shared/ExecutorAddress.h>
#include <llvm/ExecutionEngine/Orc/TargetProcess/JITLoaderGDB.h>
#if LLVM_VERSION_MAJOR < 18
#include <llvm/ExecutionEngine/Orc/DebuggerSupportPlugin.h>
#else
#include <llvm/ExecutionEngine/Orc/Debugging/DebuggerSupportPlugin.h>
#endif
#if LLVM_VERSION_MAJOR < 17
#else
#include <llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h>
#endif
#endif
#if LLVM_VERSION_MAJOR < 15
#elif LLVM_VERSION_MAJOR < 22
#include <llvm/ExecutionEngine/Orc/DebugObjectManagerPlugin.h>
#include <llvm/ExecutionEngine/Orc/EPCDebugObjectRegistrar.h>
#else
#include <llvm/ExecutionEngine/Orc/Debugging/ELFDebugObjectPlugin.h>
#endif

#include <atomic>
#include <unordered_set>
#include <utility>

#if WASMEDGE_OS_WINDOWS
#else
extern "C" {
#if LLVM_VERSION_MAJOR < 18
struct jit_code_entry {
  struct jit_code_entry *next_entry;
  struct jit_code_entry *prev_entry;
  const char *symfile_addr;
  uint64_t symfile_size;
};

struct jit_descriptor {
  uint32_t version;
  uint32_t action_flag;
  struct jit_code_entry *relevant_entry;
  struct jit_code_entry *first_entry;
};
#endif

extern struct jit_descriptor __jit_debug_descriptor;
void __jit_debug_register_code();
}
#endif

namespace WasmEdge::LLVM {

void DebugObjectOwners::add(uintptr_t Key, const std::vector<uint64_t> &Ids) {
  if (Ids.empty()) {
    return;
  }
  std::lock_guard Lock(Mutex);
  auto &List = Owned[Key];
  List.insert(List.end(), Ids.begin(), Ids.end());
}

std::vector<uint64_t> DebugObjectOwners::take(uintptr_t Key) {
  std::lock_guard Lock(Mutex);
  auto It = Owned.find(Key);
  if (It == Owned.end()) {
    return {};
  }
  auto Ids = std::move(It->second);
  Owned.erase(It);
  return Ids;
}

void DebugObjectOwners::transfer(uintptr_t DstKey, uintptr_t SrcKey) {
  if (DstKey == SrcKey) {
    return;
  }
  auto Ids = take(SrcKey);
  add(DstKey, Ids);
}

std::vector<uint64_t> DebugObjectOwners::takeAll() {
  std::lock_guard Lock(Mutex);
  std::vector<uint64_t> Ids;
  for (auto &[Key, List] : Owned) {
    Ids.insert(Ids.end(), List.begin(), List.end());
  }
  Owned.clear();
  return Ids;
}

size_t countJITDebugObjects() noexcept {
#if WASMEDGE_OS_WINDOWS
  return 0;
#else
  size_t Count = 0;
  for (auto *E = __jit_debug_descriptor.first_entry; E; E = E->next_entry) {
    ++Count;
  }
  return Count;
#endif
}

#if LLVM_VERSION_MAJOR < 11
namespace {
struct GDBRegistrations {
  std::mutex Mutex;
  uint64_t LastKey = 0;
  std::unordered_map<const llvm::orc::LLJIT *, std::vector<uint64_t>> Keys;
};

GDBRegistrations &gdbRegistrations() noexcept {
  static GDBRegistrations R;
  return R;
}
} // namespace

void releaseJITDebugger(const llvm::orc::LLJIT &J) {
  auto &R = gdbRegistrations();
  std::unique_lock Lock(R.Mutex);
  if (auto It = R.Keys.find(&J); It != R.Keys.end()) {
    auto *Listener = llvm::JITEventListener::createGDBRegistrationListener();
    for (const auto Key : It->second) {
      Listener->notifyFreeingObject(Key);
    }
    R.Keys.erase(It);
  }
}
#endif

#if LLVM_VERSION_MAJOR < 15 || WASMEDGE_OS_WINDOWS
#else
namespace {

/// The GDB JIT entries that WasmEdge adds. Each entry has an ID that no
/// other entry of the process gets, so removing an entry by its ID never
/// removes a later entry for a debug object at the same address.
class DebugObjectRegistry {
public:
  static DebugObjectRegistry &get() noexcept {
    static auto *Registry = new DebugObjectRegistry();
    return *Registry;
  }

  /// An ID for a later add. The ID is pending until then.
  uint64_t reserve() {
    const auto Id = NextId.fetch_add(1, std::memory_order_relaxed) + 1;
    std::lock_guard Lock(Mutex);
    Pending.insert(Id);
    return Id;
  }

  /// Adds the entry with Id for the debug object in Mem, unless a removal
  /// cancelled the pending ID. An entry of a debug object at the same address
  /// is stale, because its memory was freed, so it goes away.
  void add(uint64_t Id, llvm::orc::ExecutorAddrRange Mem,
           bool AutoRegisterCode) {
    std::lock_guard Lock(Mutex);
    if (Pending.erase(Id) == 0) {
      return;
    }
    auto *E =
        new jit_code_entry{nullptr, nullptr,
                           reinterpret_cast<const char *>(
                               static_cast<uintptr_t>(Mem.Start.getValue())),
                           Mem.End.getValue() - Mem.Start.getValue()};
    if (auto It = IdByStart.find(Mem.Start.getValue()); It != IdByStart.end()) {
      removeLocked(It->second);
    }
    E->next_entry = __jit_debug_descriptor.first_entry;
    if (E->next_entry) {
      E->next_entry->prev_entry = E;
    }
    __jit_debug_descriptor.first_entry = E;
    __jit_debug_descriptor.relevant_entry = E;
    __jit_debug_descriptor.action_flag = RegisterFn;
    if (AutoRegisterCode) {
      __jit_debug_register_code();
    }
    ById.emplace(Id, E);
    IdByStart[Mem.Start.getValue()] = Id;
  }

  size_t pendingCount() {
    std::lock_guard Lock(Mutex);
    return Pending.size();
  }

  /// Removes the entries with these IDs. A pending ID is cancelled, so its
  /// debug object never gets an entry.
  void remove(const std::vector<uint64_t> &Ids) {
    std::lock_guard Lock(Mutex);
    for (const auto Id : Ids) {
      removeLocked(Id);
    }
  }

private:
  static constexpr uint32_t RegisterFn = 1;
  static constexpr uint32_t UnregisterFn = 2;

  void removeLocked(uint64_t Id) {
    const auto It = ById.find(Id);
    if (It == ById.end()) {
      Pending.erase(Id);
      return;
    }
    auto *E = It->second;
    ById.erase(It);
    const auto Start =
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(E->symfile_addr));
    if (auto ByStart = IdByStart.find(Start);
        ByStart != IdByStart.end() && ByStart->second == Id) {
      IdByStart.erase(ByStart);
    }
    unlink(E);
  }

  /// Removes the entry from the list, tells the debugger, and frees it.
  static void unlink(jit_code_entry *E) {
    if (E->prev_entry) {
      E->prev_entry->next_entry = E->next_entry;
    } else {
      __jit_debug_descriptor.first_entry = E->next_entry;
    }
    if (E->next_entry) {
      E->next_entry->prev_entry = E->prev_entry;
    }
    __jit_debug_descriptor.relevant_entry = E;
    __jit_debug_descriptor.action_flag = UnregisterFn;
    __jit_debug_register_code();
    delete E;
  }

  std::atomic<uint64_t> NextId = 0;
  std::mutex Mutex;
  std::unordered_map<uint64_t, jit_code_entry *> ById;
  std::unordered_map<uint64_t, uint64_t> IdByStart;
  std::unordered_set<uint64_t> Pending;
};

/// Removes the debugger entries of a resource before Base frees the debug
/// objects of that resource.
template <typename Base> class EntryReleasingPlugin : public Base {
public:
  using Base::Base;

  ~EntryReleasingPlugin() override {
    DebugObjectRegistry::get().remove(Owners.takeAll());
  }

#if LLVM_VERSION_MAJOR < 16
  llvm::Error notifyRemovingResources(llvm::orc::ResourceKey K) override {
    DebugObjectRegistry::get().remove(Owners.take(K));
    return Base::notifyRemovingResources(K);
  }

  void notifyTransferringResources(llvm::orc::ResourceKey DstKey,
                                   llvm::orc::ResourceKey SrcKey) override {
    Owners.transfer(DstKey, SrcKey);
    Base::notifyTransferringResources(DstKey, SrcKey);
  }
#else
  llvm::Error notifyRemovingResources(llvm::orc::JITDylib &JD,
                                      llvm::orc::ResourceKey K) override {
    DebugObjectRegistry::get().remove(Owners.take(K));
    return Base::notifyRemovingResources(JD, K);
  }

  void notifyTransferringResources(llvm::orc::JITDylib &JD,
                                   llvm::orc::ResourceKey DstKey,
                                   llvm::orc::ResourceKey SrcKey) override {
    Owners.transfer(DstKey, SrcKey);
    Base::notifyTransferringResources(JD, DstKey, SrcKey);
  }
#endif

protected:
  DebugObjectOwners Owners;
};

/// Forwards every event to another plugin.
class ForwardingPlugin : public llvm::orc::ObjectLinkingLayer::Plugin {
public:
  explicit ForwardingPlugin(
      std::unique_ptr<llvm::orc::ObjectLinkingLayer::Plugin> Inner) noexcept
      : Inner(std::move(Inner)) {}

  void modifyPassConfig(llvm::orc::MaterializationResponsibility &MR,
                        llvm::jitlink::LinkGraph &G,
                        llvm::jitlink::PassConfiguration &Config) override {
    Inner->modifyPassConfig(MR, G, Config);
  }

  void notifyMaterializing(llvm::orc::MaterializationResponsibility &MR,
                           llvm::jitlink::LinkGraph &G,
                           llvm::jitlink::JITLinkContext &Ctx,
                           llvm::MemoryBufferRef InputObject) override {
    Inner->notifyMaterializing(MR, G, Ctx, InputObject);
  }

#if LLVM_VERSION_MAJOR < 21
  void notifyLoaded(llvm::orc::MaterializationResponsibility &MR) override {
    Inner->notifyLoaded(MR);
  }
#endif

  llvm::Error
  notifyEmitted(llvm::orc::MaterializationResponsibility &MR) override {
    return Inner->notifyEmitted(MR);
  }

  llvm::Error
  notifyFailed(llvm::orc::MaterializationResponsibility &MR) override {
    return Inner->notifyFailed(MR);
  }

#if LLVM_VERSION_MAJOR < 16
  llvm::Error notifyRemovingResources(llvm::orc::ResourceKey K) override {
    return Inner->notifyRemovingResources(K);
  }

  void notifyTransferringResources(llvm::orc::ResourceKey DstKey,
                                   llvm::orc::ResourceKey SrcKey) override {
    Inner->notifyTransferringResources(DstKey, SrcKey);
  }
#else
  llvm::Error notifyRemovingResources(llvm::orc::JITDylib &JD,
                                      llvm::orc::ResourceKey K) override {
    return Inner->notifyRemovingResources(JD, K);
  }

  void notifyTransferringResources(llvm::orc::JITDylib &JD,
                                   llvm::orc::ResourceKey DstKey,
                                   llvm::orc::ResourceKey SrcKey) override {
    Inner->notifyTransferringResources(JD, DstKey, SrcKey);
  }
#endif

private:
  std::unique_ptr<llvm::orc::ObjectLinkingLayer::Plugin> Inner;
};

using AllocActionResult =
    decltype(llvm_orc_registerJITLoaderGDBAllocAction(nullptr, 0));

AllocActionResult registerDebugObjectAction(const char *ArgData,
                                            size_t ArgSize) {
  using namespace llvm::orc::shared;
  return WrapperFunction<SPSError(uint64_t, SPSExecutorAddrRange, bool)>::
      handle(ArgData, ArgSize,
             [](uint64_t Id, llvm::orc::ExecutorAddrRange Mem,
                bool AutoRegisterCode) {
               DebugObjectRegistry::get().add(Id, Mem, AutoRegisterCode);
               return llvm::Error::success();
             })
          .release();
}

AllocActionResult deregisterDebugObjectAction(const char *ArgData,
                                              size_t ArgSize) {
  using namespace llvm::orc::shared;
  return WrapperFunction<SPSError(uint64_t)>::handle(
             ArgData, ArgSize,
             [](uint64_t Id) {
               DebugObjectRegistry::get().remove({Id});
               return llvm::Error::success();
             })
      .release();
}

/// LLVM registers debug objects with an allocation action that has no
/// deallocation action, so the debugger keeps entries for freed memory. This
/// plugin wraps the LLVM plugin and replaces each such action with a pair
/// that also removes the entry.
///
/// The LLVM 22 ELF plugin also expects a pending debug object in
/// notifyFailed, but it drops that object when it adds the registration
/// action. This plugin forwards a failure only while the wrapped plugin
/// still holds the object: the wrapped plugin added link passes, and the
/// registration action does not exist yet.
class DebugObjectDeregistrationPlugin final
    : public EntryReleasingPlugin<ForwardingPlugin> {
public:
  using EntryReleasingPlugin::EntryReleasingPlugin;

  void modifyPassConfig(llvm::orc::MaterializationResponsibility &MR,
                        llvm::jitlink::LinkGraph &G,
                        llvm::jitlink::PassConfiguration &Config) override {
    const auto Before = passCount(Config);
    EntryReleasingPlugin::modifyPassConfig(MR, G, Config);
    if (passCount(Config) != Before) {
      std::lock_guard Lock(PendingMutex);
      Pending.insert(&MR);
    }
    Config.PostFixupPasses.push_back(
        [this, &MR](llvm::jitlink::LinkGraph &Graph) {
          return pairActions(MR, Graph);
        });
  }

  llvm::Error
  notifyEmitted(llvm::orc::MaterializationResponsibility &MR) override {
    {
      std::lock_guard Lock(PendingMutex);
      Pending.erase(&MR);
      InFlight.erase(&MR);
    }
    return EntryReleasingPlugin::notifyEmitted(MR);
  }

  llvm::Error
  notifyFailed(llvm::orc::MaterializationResponsibility &MR) override {
    std::vector<uint64_t> Ids;
    bool Forward = false;
    {
      std::lock_guard Lock(PendingMutex);
      Forward = Pending.erase(&MR) != 0;
      if (auto It = InFlight.find(&MR); It != InFlight.end()) {
        Ids = std::move(It->second);
        InFlight.erase(It);
      }
    }
    DebugObjectRegistry::get().remove(Ids);
    if (!Forward) {
      return llvm::Error::success();
    }
    return EntryReleasingPlugin::notifyFailed(MR);
  }

private:
  static size_t passCount(const llvm::jitlink::PassConfiguration &C) noexcept {
    return C.PrePrunePasses.size() + C.PostPrunePasses.size() +
           C.PostAllocationPasses.size() + C.PreFixupPasses.size() +
           C.PostFixupPasses.size();
  }

  llvm::Error pairActions(llvm::orc::MaterializationResponsibility &MR,
                          llvm::jitlink::LinkGraph &G) {
    using namespace llvm::orc::shared;
    const auto Register = llvm::orc::ExecutorAddr::fromPtr(
        &llvm_orc_registerJITLoaderGDBAllocAction);
    std::vector<uint64_t> Ids;
    auto RollBack = [&Ids](llvm::Error Err) {
      DebugObjectRegistry::get().remove(Ids);
      return Err;
    };
    for (auto &A : G.allocActions()) {
      if (A.Finalize.getCallee() != Register || A.Dealloc) {
        continue;
      }
      const auto &Args = A.Finalize.getArgData();
      SPSInputBuffer IB(Args.data(), Args.size());
      llvm::orc::ExecutorAddrRange Mem;
      if (!SPSArgList<SPSExecutorAddrRange>::deserialize(IB, Mem)) {
        return RollBack(llvm::make_error<llvm::StringError>(
            "cannot read the debug object range",
            llvm::inconvertibleErrorCode()));
      }
      bool AutoRegisterCode = true;
      if (!SPSArgList<bool>::deserialize(IB, AutoRegisterCode)) {
        AutoRegisterCode = true;
      }
      const auto Id = DebugObjectRegistry::get().reserve();
      A.Finalize =
          llvm::cantFail(WrapperFunctionCall::Create<
                         SPSArgList<uint64_t, SPSExecutorAddrRange, bool>>(
              llvm::orc::ExecutorAddr::fromPtr(&registerDebugObjectAction), Id,
              Mem, AutoRegisterCode));
      A.Dealloc =
          llvm::cantFail(WrapperFunctionCall::Create<SPSArgList<uint64_t>>(
              llvm::orc::ExecutorAddr::fromPtr(&deregisterDebugObjectAction),
              Id));
      Ids.push_back(Id);
    }
    if (Ids.empty()) {
      return llvm::Error::success();
    }
    {
      std::lock_guard Lock(PendingMutex);
      Pending.erase(&MR);
    }
    if (auto Err = MR.withResourceKeyDo(
            [&](llvm::orc::ResourceKey K) { Owners.add(K, Ids); })) {
      return RollBack(std::move(Err));
    }
    std::lock_guard Lock(PendingMutex);
    InFlight[&MR] = std::move(Ids);
    return llvm::Error::success();
  }

  std::mutex PendingMutex;
  std::unordered_set<const llvm::orc::MaterializationResponsibility *> Pending;
  /// The entry IDs of each materialization between its fixups and its end,
  /// so a failure cancels them.
  std::unordered_map<const llvm::orc::MaterializationResponsibility *,
                     std::vector<uint64_t>>
      InFlight;
};

#if LLVM_VERSION_MAJOR < 22
/// Registers debug objects with the GDB JIT interface and keeps their start
/// addresses, so that the plugin below can give them to a resource key.
class DebugObjectEntries final : public llvm::orc::DebugObjectRegistrar {
public:
#if LLVM_VERSION_MAJOR < 17
  llvm::Error registerDebugObject(llvm::orc::ExecutorAddrRange Mem) override {
    add(Mem, true);
    return llvm::Error::success();
  }
#else
  llvm::Error registerDebugObject(llvm::orc::ExecutorAddrRange Mem,
                                  bool AutoRegisterCode) override {
    add(Mem, AutoRegisterCode);
    return llvm::Error::success();
  }
#endif

  std::vector<uint64_t> takeAdded() noexcept {
    std::lock_guard Lock(Mutex);
    return std::exchange(Added, {});
  }

private:
  void add(llvm::orc::ExecutorAddrRange Mem, bool AutoRegisterCode) {
    auto &Registry = DebugObjectRegistry::get();
    const auto Id = Registry.reserve();
    Registry.add(Id, Mem, AutoRegisterCode);
    std::lock_guard Lock(Mutex);
    Added.push_back(Id);
  }

  std::mutex Mutex;
  std::vector<uint64_t> Added;
};

/// LLVM's DebugObjectManagerPlugin frees a debug object when its resource
/// goes away, but never tells the debugger. This plugin gives the entries of
/// each emitted object to its resource key, so they go away first.
class UnregisteringDebugObjectPlugin final
    : public EntryReleasingPlugin<llvm::orc::DebugObjectManagerPlugin> {
public:
  UnregisteringDebugObjectPlugin(llvm::orc::ExecutionSession &ES,
                                 std::unique_ptr<DebugObjectEntries> Target,
                                 DebugObjectEntries &Entries)
#if LLVM_VERSION_MAJOR < 17
      : EntryReleasingPlugin(ES, std::move(Target)),
#else
      : EntryReleasingPlugin(ES, std::move(Target), true, true),
#endif
        Entries(Entries) {
  }

  llvm::Error
  notifyEmitted(llvm::orc::MaterializationResponsibility &MR) override {
    std::lock_guard EmitLock(EmitMutex);
    auto Err = EntryReleasingPlugin::notifyEmitted(MR);
    auto Added = Entries.takeAdded();
    if (Added.empty()) {
      return Err;
    }
    bool Owned = false;
    auto KeyErr = MR.withResourceKeyDo([&](llvm::orc::ResourceKey K) {
      Owners.add(K, Added);
      Owned = true;
    });
    if (!Owned) {
      DebugObjectRegistry::get().remove(Added);
    }
    return llvm::joinErrors(std::move(Err), std::move(KeyErr));
  }

#if LLVM_VERSION_MAJOR < 16
  llvm::Error notifyRemovingResources(llvm::orc::ResourceKey K) override {
    std::lock_guard EmitLock(EmitMutex);
    return EntryReleasingPlugin::notifyRemovingResources(K);
  }
#else
  llvm::Error notifyRemovingResources(llvm::orc::JITDylib &JD,
                                      llvm::orc::ResourceKey K) override {
    std::lock_guard EmitLock(EmitMutex);
    return EntryReleasingPlugin::notifyRemovingResources(JD, K);
  }
#endif

private:
  DebugObjectEntries &Entries;
  /// Held while LLVM registers the objects of an emission and they get their
  /// resource key, and while a resource goes away, so a removal always finds
  /// the entries of the objects that it frees.
  std::mutex EmitMutex;
};
#endif

llvm::Error addMachODebuggerSupport(llvm::orc::LLJIT &J,
                                    llvm::orc::ObjectLinkingLayer &Linking) {
  auto &ES = J.getExecutionSession();
  auto &ProcessJD = ES.createBareJITDylib("<wasmedge-debugger-support>");
#if LLVM_VERSION_MAJOR < 17
  const llvm::JITEvaluatedSymbol AllocAction(
      llvm::pointerToJITTargetAddress(
          &llvm_orc_registerJITLoaderGDBAllocAction),
      llvm::JITSymbolFlags::Exported);
#else
  const llvm::orc::ExecutorSymbolDef AllocAction(
      llvm::orc::ExecutorAddr::fromPtr(
          &llvm_orc_registerJITLoaderGDBAllocAction),
      llvm::JITSymbolFlags::Exported);
#endif
  if (auto Err = ProcessJD.define(llvm::orc::absoluteSymbols(
          {{J.mangleAndIntern("llvm_orc_registerJITLoaderGDBAllocAction"),
            AllocAction}}))) {
    return Err;
  }
  auto Plugin = llvm::orc::GDBJITDebugInfoRegistrationPlugin::Create(
      ES, ProcessJD, J.getTargetTriple());
  if (!Plugin) {
    return Plugin.takeError();
  }
  Linking.addPlugin(
      std::make_unique<DebugObjectDeregistrationPlugin>(std::move(*Plugin)));
  return llvm::Error::success();
}

} // namespace
#endif

#if LLVM_VERSION_MAJOR < 15 || WASMEDGE_OS_WINDOWS
#else
uint64_t reserveJITDebugEntry() { return DebugObjectRegistry::get().reserve(); }

void addJITDebugEntry(uint64_t Id, uint64_t Start, uint64_t Size) {
  DebugObjectRegistry::get().add(
      Id,
      llvm::orc::ExecutorAddrRange(llvm::orc::ExecutorAddr(Start),
                                   llvm::orc::ExecutorAddr(Start + Size)),
      false);
}

void removeJITDebugEntries(const std::vector<uint64_t> &Ids) {
  DebugObjectRegistry::get().remove(Ids);
}

size_t pendingJITDebugEntries() {
  return DebugObjectRegistry::get().pendingCount();
}
#endif

llvm::Error enableJITDebugger(llvm::orc::LLJIT &J) {
#if WASMEDGE_OS_WINDOWS
  static_cast<void>(J);
  return llvm::make_error<llvm::StringError>("not supported on Windows",
                                             llvm::inconvertibleErrorCode());
#elif WASMEDGE_OS_MACOS && LLVM_VERSION_MAJOR < 13
  static_cast<void>(J);
  return llvm::make_error<llvm::StringError>("unknown JIT object layer",
                                             llvm::inconvertibleErrorCode());
#else
  auto &Layer = J.getObjLinkingLayer();
#if LLVM_VERSION_MAJOR < 11
  static_cast<llvm::orc::RTDyldObjectLinkingLayer &>(Layer).setNotifyLoaded(
      [JIT = &J](llvm::orc::VModuleKey, const llvm::object::ObjectFile &Obj,
                 const llvm::RuntimeDyld::LoadedObjectInfo &Info) {
        auto &R = gdbRegistrations();
        std::unique_lock Lock(R.Mutex);
        const auto Key = ++R.LastKey;
        R.Keys[JIT].push_back(Key);
        llvm::JITEventListener::createGDBRegistrationListener()
            ->notifyObjectLoaded(Key, Obj, Info);
      });
  return llvm::Error::success();
#elif LLVM_VERSION_MAJOR < 13
  static_cast<llvm::orc::RTDyldObjectLinkingLayer &>(Layer)
      .registerJITEventListener(
          *llvm::JITEventListener::createGDBRegistrationListener());
  return llvm::Error::success();
#else
  if (auto *RTDyld =
          llvm::dyn_cast<llvm::orc::RTDyldObjectLinkingLayer>(&Layer)) {
    RTDyld->registerJITEventListener(
        *llvm::JITEventListener::createGDBRegistrationListener());
    return llvm::Error::success();
  }
#if LLVM_VERSION_MAJOR < 15
  return llvm::make_error<llvm::StringError>("unknown JIT object layer",
                                             llvm::inconvertibleErrorCode());
#else
  auto *Linking = llvm::dyn_cast<llvm::orc::ObjectLinkingLayer>(&Layer);
  if (!Linking) {
    return llvm::make_error<llvm::StringError>("unknown JIT object layer",
                                               llvm::inconvertibleErrorCode());
  }
  if (J.getTargetTriple().isOSBinFormatMachO()) {
    return addMachODebuggerSupport(J, *Linking);
  }
#if LLVM_VERSION_MAJOR < 22
  auto Entries = std::make_unique<DebugObjectEntries>();
  auto &EntriesRef = *Entries;
  Linking->addPlugin(std::make_unique<UnregisteringDebugObjectPlugin>(
      J.getExecutionSession(), std::move(Entries), EntriesRef));
  return llvm::Error::success();
#else
  llvm::Error Err = llvm::Error::success();
  auto Plugin = std::make_unique<llvm::orc::ELFDebugObjectPlugin>(
      J.getExecutionSession(), false, true, Err);
  if (Err) {
    return Err;
  }
  Linking->addPlugin(
      std::make_unique<DebugObjectDeregistrationPlugin>(std::move(Plugin)));
  return llvm::Error::success();
#endif
#endif
#endif
#endif
}

} // namespace WasmEdge::LLVM
