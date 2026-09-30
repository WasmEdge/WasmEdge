// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/gc/GCOwnershipTest.cpp - GC ownership tests ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for the ownership of tables, globals and
/// modules by one GC controller, and for the refusal of foreign
/// executors.
///
//===----------------------------------------------------------------------===//

#include "gchelpers.h"

namespace {

using namespace WasmEdge;
using namespace WasmEdge::GCTest;

// ==========================================================================
// Multi-threaded GC tests
// ==========================================================================

TEST(GCThread, TableCanHoldManagedResolvedAtConstruction) {
  // The can-hold-managed bit is resolved once, at
  // ModuleInstance::addTable's call site -- the one place that has
  // both the table type and the defining module's type list -- and then just
  // read back by canHoldManaged(). All four ref types under test here are
  // abstract heap types, so AST::TypeMatcher::refTypeCanHoldGCObject never
  // needs to consult the type list; a module that defines no types is enough
  // to exercise the addTable resolution path end-to-end. Deterministic,
  // single-threaded (no concurrent GC activity is needed to exercise
  // construction-time resolution).
  class CanHoldManagedTestModule : public Runtime::Instance::ModuleInstance {
  public:
    explicit CanHoldManagedTestModule(GC::Allocator &Alloc)
        : ModuleInstance("chm") {
      addTable(Alloc, AST::TableType(TypeCode::AnyRef, 0, 1));
      addTable(Alloc, AST::TableType(TypeCode::StructRef, 0, 1));
      addTable(Alloc, AST::TableType(TypeCode::FuncRef, 0, 1));
      addTable(Alloc, AST::TableType(TypeCode::NullFuncRef, 0, 1));
    }
    uint32_t tableCount() const noexcept { return getTableNum(); }
    bool canHold(uint32_t Idx) const noexcept {
      return unsafeGetTable(Idx)->canHoldManaged();
    }
  };

  GC::Allocator Alloc;
  CanHoldManagedTestModule Mod(Alloc);
  ASSERT_EQ(Mod.tableCount(), 4u);
  EXPECT_TRUE(Mod.canHold(0)) << "anyref should be GC-managed";
  EXPECT_TRUE(Mod.canHold(1)) << "structref should be GC-managed";
  EXPECT_FALSE(Mod.canHold(2)) << "funcref should not be GC-managed";
  EXPECT_FALSE(Mod.canHold(3)) << "nullfuncref should not be GC-managed";
}

TEST(GCThread, SetAllocatorRejectsForeignGrowableTable) {
  // TableInstance::setAllocator is the single, fallible, centralized
  // owner-attach check. A second, foreign controller may not attach
  // to a table that is either managed-capable (hazard a: a ref it stores would
  // barrier against the wrong allocator and be swept while reachable) or
  // growable (hazard b: a reallocating grow would free a buffer a peer reader
  // still holds). A fixed, non-managed table carries neither hazard and stays
  // freely shareable, keeping its first owner. A same-owner re-attach is an
  // idempotent success. Deterministic and single-threaded: the two hazards are
  // structural properties of the table, so no concurrent GC activity is needed
  // to exercise the rejection decision.
  GC::Allocator AllocA;
  GC::Allocator AllocB;

  // (i) Growable funcref table (max 4 > min 0): buffer-realloc UAF (hazard b),
  // rejected for a foreign controller even though funcref is not managed.
  {
    AST::TableType TType(TypeCode::FuncRef, 0, 4);
    Runtime::Instance::TableInstance Table(TType, /*CanHoldManagedIn=*/false);
    EXPECT_TRUE(Table.setAllocator(AllocA));   // first attach: claims ownership
    EXPECT_TRUE(Table.setAllocator(AllocA));   // same owner: idempotent success
    auto Foreign = Table.setAllocator(AllocB); // foreign + growable: reject
    EXPECT_FALSE(Foreign);
    EXPECT_EQ(Foreign.error(), ErrCode::Value::IncompatibleImportType);
  }

  // (ii) Managed-capable table (structref), fixed size (max == min): rejected
  // for a foreign controller by hazard a regardless of the fixed size.
  {
    AST::TableType TType(TypeCode::StructRef, 1, 1);
    Runtime::Instance::TableInstance Table(TType, /*CanHoldManagedIn=*/true);
    EXPECT_TRUE(Table.setAllocator(AllocA));
    auto Foreign = Table.setAllocator(AllocB); // foreign + managed: reject
    EXPECT_FALSE(Foreign);
    EXPECT_EQ(Foreign.error(), ErrCode::Value::IncompatibleImportType);
  }

  // (iii) Fixed funcref table (hasMax && max == min), not managed: neither
  // hazard applies, so a foreign attach is allowed (keeps the first owner).
  {
    AST::TableType TType(TypeCode::FuncRef, 2, 2);
    Runtime::Instance::TableInstance Table(TType, /*CanHoldManagedIn=*/false);
    EXPECT_TRUE(Table.setAllocator(AllocA));
    EXPECT_TRUE(Table.setAllocator(AllocB)); // foreign but freely shareable
  }
}

// A prebuilt module whose owned tables we can pre-attach and inspect.
// addHostTable adds an owned, initially-unattached table; the executor's
// registerModule walk is what attaches it.
class NopHost : public Runtime::HostFunction<NopHost> {
public:
  Expect<void> body(const Runtime::CallingFrame &) { return {}; }
};

class RegisterWalkModule : public Runtime::Instance::ModuleInstance {
public:
  explicit RegisterWalkModule(std::string_view Name) : ModuleInstance(Name) {}
  // Both adders return null once the module is finalized (its first host
  // function call does that), when the instance was dropped, not added.
  Runtime::Instance::TableInstance *
  addOwnedTable(std::unique_ptr<Runtime::Instance::TableInstance> Tab) {
    auto *Raw = Tab.get();
    if (!addHostTable("t" + std::to_string(Count++), std::move(Tab))) {
      return nullptr;
    }
    return Raw;
  }
  Runtime::Instance::GlobalInstance *
  addOwnedGlobal(std::unique_ptr<Runtime::Instance::GlobalInstance> Glob,
                 std::string Name = {}) {
    auto *Raw = Glob.get();
    if (Name.empty()) {
      Name = "g" + std::to_string(Count++);
    }
    if (!addHostGlobal(Name, std::move(Glob))) {
      return nullptr;
    }
    return Raw;
  }
  const Runtime::Instance::FunctionInstance *
  addOwnedHostFunc(std::string Name = {}) {
    if (Name.empty()) {
      Name = "f" + std::to_string(Count++);
    }
    addHostFunc(Name, std::make_unique<NopHost>());
    return findFuncExports(Name);
  }
  // Lock seam for the claim-walk test: hold the module mutex exclusively, as
  // every index-space mutator does for the duration of its push.
  std::unique_lock<std::shared_mutex> lockExclusive() {
    return std::unique_lock(Mutex);
  }

private:
  uint32_t Count = 0;
};

TEST(GCThread, RegisterModuleRejectsForeignGrowableTable) {
  // registerModule walks a prebuilt module's owned tables/globals and attaches
  // them to the registering executor's allocator, validating ownership. A
  // managed/growable table already owned by another controller is
  // rejected; the walk is transactional (preflight-all-then-commit), so a
  // rejection reverses every attach this registration made and does not publish
  // the module. A module with only fixed non-managed tables registers into any
  // number of executors (freely shared). Deterministic and single-threaded: the
  // two hazards are structural properties of the table.
  Configure Conf = makeGCConf();

  // --- Negative + rollback ------------------------------------------------
  {
    // Executor A is the GC executor we register into; controller B already owns
    // the module's second table.
    WasmEdge::Executor::Executor ExecA(Conf);
    Runtime::StoreManager StoreA;
    GC::Controller CtrlB;

    RegisterWalkModule Mod("walk-neg");
    // Table 1: managed, fixed, initially unattached -> the walk newly claims it
    // for A. This is the attach the rollback must reverse.
    auto *Tab1 =
        Mod.addOwnedTable(std::make_unique<Runtime::Instance::TableInstance>(
            AST::TableType(TypeCode::StructRef, 1, 1),
            /*CanHoldManagedIn=*/true));
    // Table 2: growable, already owned by foreign controller B -> the walk's
    // attach to A is rejected (hazard b), aborting the whole registration.
    auto *Tab2 =
        Mod.addOwnedTable(std::make_unique<Runtime::Instance::TableInstance>(
            AST::TableType(TypeCode::FuncRef, 0, 4),
            /*CanHoldManagedIn=*/false));
    ASSERT_TRUE(Tab2->setAllocator(CtrlB.getAllocator()));

    // Preconditions: Tab1 not yet controller-owned; Tab2 owned by B (foreign to
    // A).
    ASSERT_FALSE(Tab1->isManagedByController());
    ASSERT_TRUE(Tab2->hasForeignAllocator(ExecA.getAllocator()));

    auto Res = ExecA.registerModule(StoreA, Mod);
    ASSERT_FALSE(Res);
    EXPECT_EQ(Res.error(), ErrCode::Value::IncompatibleImportType);

    // Rollback proof: Tab1 (managed) must be cleanly un-attached again, so it
    // is no longer controller-owned and is freshly claimable by any controller.
    // If rollback had failed, Tab1 would still be owned by A and this fresh
    // claim by B would be a rejected foreign-managed attach.
    EXPECT_FALSE(Tab1->isManagedByController())
        << "rollback must un-attach the table this registration newly claimed";
    EXPECT_TRUE(Tab1->setAllocator(CtrlB.getAllocator()))
        << "the rolled-back table must be cleanly unattached (claimable)";
    // Tab2 must still be owned by its first controller B (never moved to A).
    EXPECT_TRUE(Tab2->hasForeignAllocator(ExecA.getAllocator()))
        << "the foreign-owned table must stay with its first controller";
    EXPECT_FALSE(Tab2->hasForeignAllocator(CtrlB.getAllocator()));
    // Module not published.
    EXPECT_EQ(StoreA.findModule("walk-neg"), nullptr);
  }

  // --- Positive: fixed non-managed table freely shared across executors ---
  {
    WasmEdge::Executor::Executor ExecA(Conf);
    WasmEdge::Executor::Executor ExecB(Conf);
    Runtime::StoreManager StoreA;
    Runtime::StoreManager StoreB;

    RegisterWalkModule Mod("walk-pos");
    Mod.addOwnedTable(std::make_unique<Runtime::Instance::TableInstance>(
        AST::TableType(TypeCode::FuncRef, 2, 2), /*CanHoldManagedIn=*/false));

    // First registration claims the fixed funcref table for A; the second is a
    // foreign-but-freely-shareable attach that keeps A and still succeeds.
    EXPECT_TRUE(ExecA.registerModule(StoreA, Mod));
    EXPECT_TRUE(ExecB.registerModule(StoreB, Mod));
    EXPECT_NE(StoreA.findModule("walk-pos"), nullptr);
    EXPECT_NE(StoreB.findModule("walk-pos"), nullptr);
  }
}

TEST(GCThread, RegisterModuleRollsBackClaimWhenPublicationFails) {
  // Transaction boundary: the ownership claim spans the whole registration,
  // not just the attach walk. StoreManager::registerModule rejects a duplicate
  // module name after the walk has already attached the module's roots, so that
  // rejection must reverse the attach too. A leaked claim is not memory-unsafe
  // (each instance unregisters itself in its destructor), but it would leave a
  // never-published module owned by -- and scanned by -- this executor, and it
  // would make a later registration of the same module into a different
  // executor fail the foreign-hazard check, turning a recoverable name conflict
  // into a permanent cross-executor rejection. Both registerModule overloads
  // are covered. Deterministic and single-threaded.
  Configure Conf = makeGCConf();

  // --- Default-name overload ----------------------------------------------
  {
    WasmEdge::Executor::Executor ExecA(Conf);
    WasmEdge::Executor::Executor ExecC(Conf);
    Runtime::StoreManager StoreA;
    Runtime::StoreManager StoreC;

    // Occupy the name in A's store so the publication below is rejected.
    RegisterWalkModule Occupier("dup-default");
    ASSERT_TRUE(ExecA.registerModule(StoreA, Occupier));
    ASSERT_EQ(StoreA.findModule("dup-default"), &Occupier);

    // The module whose registration fails at publication (not at the walk). Its
    // managed table is unattached, so the walk newly claims it -- exactly the
    // attach the failed publication must reverse.
    RegisterWalkModule Mod("dup-default");
    auto *Tab =
        Mod.addOwnedTable(std::make_unique<Runtime::Instance::TableInstance>(
            AST::TableType(TypeCode::StructRef, 1, 1),
            /*CanHoldManagedIn=*/true));
    ASSERT_FALSE(Tab->isManagedByController());

    auto Res = ExecA.registerModule(StoreA, Mod);
    ASSERT_FALSE(Res);
    // Proves the failure came from publication, not from the ownership walk.
    EXPECT_EQ(Res.error(), ErrCode::Value::ModuleNameConflict);

    // Rollback proof 1: the newly-claimed table is un-attached again.
    EXPECT_FALSE(Tab->isManagedByController())
        << "a failed publication must reverse the attach the walk made";
    // Rollback proof 2 (the user-visible symptom): the module is still
    // registrable into a different GC executor. Without the rollback its
    // managed table would still be owned by A, so this would fail the
    // foreign-managed check with IncompatibleImportType.
    EXPECT_TRUE(ExecC.registerModule(StoreC, Mod))
        << "a rejected registration must not durably own the module's roots";
    EXPECT_EQ(StoreC.findModule("dup-default"), &Mod);
    // A's store still holds the original occupier, unchanged.
    EXPECT_EQ(StoreA.findModule("dup-default"), &Occupier);
  }

  // --- Alias-name overload -------------------------------------------------
  {
    WasmEdge::Executor::Executor ExecA(Conf);
    WasmEdge::Executor::Executor ExecC(Conf);
    Runtime::StoreManager StoreA;
    Runtime::StoreManager StoreC;

    RegisterWalkModule Occupier("occupier");
    ASSERT_TRUE(ExecA.registerModule(StoreA, Occupier, "dup-alias"));
    ASSERT_EQ(StoreA.findModule("dup-alias"), &Occupier);

    RegisterWalkModule Mod("other-name");
    auto *Tab =
        Mod.addOwnedTable(std::make_unique<Runtime::Instance::TableInstance>(
            AST::TableType(TypeCode::StructRef, 1, 1),
            /*CanHoldManagedIn=*/true));

    auto Res = ExecA.registerModule(StoreA, Mod, "dup-alias");
    ASSERT_FALSE(Res);
    EXPECT_EQ(Res.error(), ErrCode::Value::ModuleNameConflict);

    EXPECT_FALSE(Tab->isManagedByController())
        << "a failed aliased publication must reverse the attach too";
    EXPECT_TRUE(ExecC.registerModule(StoreC, Mod, "fresh-alias"))
        << "a rejected aliased registration must not durably own the roots";
    EXPECT_EQ(StoreA.findModule("dup-alias"), &Occupier);
  }
}

#ifdef WASMEDGE_USE_LLVM

// A direct Executor::invoke on a module another executor instantiated runs
// guest code whose managed roots (its anyref global here) are attached to the
// other executor's allocator: a struct.new allocated by the invoking executor
// and stored into that global is rooted by nobody the invoking collector
// scans, so its next collection frees it under the global. invoke must refuse
// a callee module whose managed-capable tables/globals belong to a foreign
// allocator, exactly as registerModule and the C-API already do
// (IncompatibleImportType), while the owner keeps working.
TEST(GCThread, InvokeRefusesModuleWithForeignManagedRoots) {
  Configure Conf = makeGCConf();
  Loader::Loader LoaderEngine(Conf, &Executor::Executor::Intrinsics);
  Validator::Validator ValidatorEngine(Conf);
  auto ModOrErr = LoaderEngine.parseModule(ForeignRootsWasm);
  ASSERT_TRUE(ModOrErr);
  std::shared_ptr<AST::Module> Mod{std::move(*ModOrErr)};
  ASSERT_TRUE(ValidatorEngine.validate(*Mod));

  Executor::Executor Owner(Conf);
  Runtime::StoreManager OwnerStore;
  auto InstOrErr = Owner.instantiateModule(OwnerStore, *Mod);
  ASSERT_TRUE(InstOrErr);
  auto Inst = std::move(*InstOrErr);
  const auto *SetFn = Inst->findFuncExports("set");
  const auto *GetFn = Inst->findFuncExports("get");
  ASSERT_NE(SetFn, nullptr);
  ASSERT_NE(GetFn, nullptr);

  // Instantiation attached every root to the owner's allocator, so the
  // module carries the owner's stamp: invoke's ownership walk is skipped for
  // it (the walk locks every managed-capable table/global, which must not be
  // paid on every call of a module this executor already owns).
  EXPECT_EQ(Inst->getGCRootsOwnerId(), Owner.getAllocator().getId());

  // The guard: a second GC executor may not run the owner's module.
  Executor::Executor Foreign(Conf);
  auto Refused = Foreign.invoke(SetFn, {}, {});
  ASSERT_FALSE(Refused);
  EXPECT_EQ(Refused.error(), ErrCode::Value::IncompatibleImportType);
  // A refused claim leaves the stamp (and the ownership) with the owner.
  EXPECT_EQ(Inst->getGCRootsOwnerId(), Owner.getAllocator().getId());

  // Control: the owner runs it, and the global it wrote is intact.
  ASSERT_TRUE(Owner.invoke(SetFn, {}, {}));
  auto Got = Owner.invoke(GetFn, {}, {});
  ASSERT_TRUE(Got);
  ASSERT_EQ(Got->size(), 1u);
  EXPECT_EQ((*Got)[0].first.get<uint32_t>(), UINT32_C(7));

  // A prebuilt module with unattached roots carries no stamp until an
  // executor claims it; the first invoke claims and stamps, so later calls
  // take the fast path.
  RegisterWalkModule Prebuilt("prebuilt-invoke");
  auto *Glob = Prebuilt.addOwnedGlobal(
      std::make_unique<Runtime::Instance::GlobalInstance>(
          AST::GlobalType(ValType(TypeCode::RefNull, TypeCode::StructRef),
                          ValMut::Var),
          /*CanHoldManagedIn=*/true,
          ValVariant(
              RefVariant(ValType(TypeCode::RefNull, TypeCode::StructRef)))));
  auto *HostFn = Prebuilt.addOwnedHostFunc();
  EXPECT_EQ(Prebuilt.getGCRootsOwnerId(), 0u);
  ASSERT_TRUE(Owner.invoke(HostFn, {}, {}));
  EXPECT_EQ(Prebuilt.getGCRootsOwnerId(), Owner.getAllocator().getId());
  EXPECT_FALSE(Glob->hasForeignAllocator(Owner.getAllocator()));
}

// The invoke fast path trusts the module's ownership stamp. The stamp names
// an allocator that is gone once its executor is destroyed, and a module that
// outlives the executor (a prebuilt host module shared by successive VMs)
// keeps the stale stamp while ~Allocator has already detached its roots. A
// new executor built in the same storage must not be mistaken for the old
// owner: its first invoke has to walk and re-attach, or the module runs with
// unattached managed roots (the #5 hole the stamp was built to keep closed).
TEST(GCThread, InvokeStampDoesNotSurviveAllocatorReuse) {
  Configure Conf = makeGCConf();
  RegisterWalkModule Prebuilt("prebuilt-reuse");
  auto *Glob = Prebuilt.addOwnedGlobal(
      std::make_unique<Runtime::Instance::GlobalInstance>(
          AST::GlobalType(ValType(TypeCode::RefNull, TypeCode::StructRef),
                          ValMut::Var),
          /*CanHoldManagedIn=*/true,
          ValVariant(
              RefVariant(ValType(TypeCode::RefNull, TypeCode::StructRef)))));
  auto *HostFn = Prebuilt.addOwnedHostFunc();

  std::optional<Executor::Executor> Exec;
  Exec.emplace(Conf);
  const auto *FirstAlloc = &Exec->getAllocator();
  ASSERT_TRUE(Exec->invoke(HostFn, {}, {}));
  EXPECT_TRUE(Glob->isManagedByController());
  // Tearing the executor down detaches the global again ...
  Exec.reset();
  EXPECT_FALSE(Glob->isManagedByController());
  // ... and the replacement lives at the very same address.
  Exec.emplace(Conf);
  ASSERT_EQ(&Exec->getAllocator(), FirstAlloc);
  ASSERT_TRUE(Exec->invoke(HostFn, {}, {}));
  EXPECT_TRUE(Glob->isManagedByController())
      << "a stale ownership stamp let invoke skip the walk";
}

// A prebuilt module can gain tables/globals after an executor claimed and
// stamped it: registerModule stamps a module that imports nothing, and the
// module stays open to addHost* until its first host-function call. The
// stamp says "every root is attached", which the addition falsifies, so it
// must be dropped: the first invoke walks and attaches the late root instead
// of running with it unattached.
TEST(GCThread, InvokeStampClearedByLaterRootAddition) {
  Configure Conf = makeGCConf();
  RegisterWalkModule Prebuilt("prebuilt-late-root");
  auto *HostFn = Prebuilt.addOwnedHostFunc();
  Executor::Executor Exec(Conf);
  Runtime::StoreManager Store;
  ASSERT_TRUE(Exec.registerModule(Store, Prebuilt));
  // No root ties the module to an allocator yet: stamped shared.
  EXPECT_EQ(Prebuilt.getGCRootsOwnerId(),
            Runtime::Instance::ModuleInstance::kSharedGCRootsOwner);

  auto *Late = Prebuilt.addOwnedGlobal(
      std::make_unique<Runtime::Instance::GlobalInstance>(
          AST::GlobalType(ValType(TypeCode::RefNull, TypeCode::StructRef),
                          ValMut::Var),
          /*CanHoldManagedIn=*/true,
          ValVariant(
              RefVariant(ValType(TypeCode::RefNull, TypeCode::StructRef)))));
  ASSERT_NE(Late, nullptr);
  EXPECT_EQ(Prebuilt.getGCRootsOwnerId(), 0u);
  EXPECT_FALSE(Late->isManagedByController());
  ASSERT_TRUE(Exec.invoke(HostFn, {}, {}));
  EXPECT_TRUE(Late->isManagedByController())
      << "the stamp survived a root addition and invoke skipped the walk";
  EXPECT_EQ(Prebuilt.getGCRootsOwnerId(), Exec.getAllocator().getId());
}

// The invoke-time ownership walk covers the invoked module's own index space
// only. The callee can still run code of another module: an imported function,
// a funcref taken from it, or a call_indirect through a table that points at
// it. That module's managed roots may belong to another executor, and a
// struct.new allocated here and stored into one of them is rooted by nobody
// this collector scans. The check therefore has to run on every module
// switch, not only at invoke.
TEST(GCThread, CalleeModuleRootsCheckedPerCall) {
  Configure Conf = makeGCConf();
  Loader::Loader LoaderEngine(Conf, &Executor::Executor::Intrinsics);
  Validator::Validator ValidatorEngine(Conf);
  auto NOrErr = LoaderEngine.parseModule(ForeignRootsWasm);
  ASSERT_TRUE(NOrErr);
  std::shared_ptr<AST::Module> NMod{std::move(*NOrErr)};
  ASSERT_TRUE(ValidatorEngine.validate(*NMod));
  auto MOrErr = LoaderEngine.parseModule(CalleeImportWasm);
  ASSERT_TRUE(MOrErr);
  std::shared_ptr<AST::Module> MMod{std::move(*MOrErr)};
  ASSERT_TRUE(ValidatorEngine.validate(*MMod));

  // The owner instantiates both: N's anyref global is its root; M has only a
  // fixed funcref table, which any executor may share.
  Executor::Executor Owner(Conf);
  Runtime::StoreManager OwnerStore;
  auto NInstOrErr = Owner.registerModule(OwnerStore, *NMod, "N");
  ASSERT_TRUE(NInstOrErr);
  auto NInst = std::move(*NInstOrErr);
  auto MInstOrErr = Owner.instantiateModule(OwnerStore, *MMod);
  ASSERT_TRUE(MInstOrErr);
  auto MInst = std::move(*MInstOrErr);
  const auto *GetFn = NInst->findFuncExports("get");
  ASSERT_NE(GetFn, nullptr);

  // The guard: every path from M into N is refused on the foreign executor,
  // and N stays with its owner.
  Executor::Executor Foreign(Conf);
  for (const char *Entry : {"run", "run_ref", "run_indirect"}) {
    const auto *Fn = MInst->findFuncExports(Entry);
    ASSERT_NE(Fn, nullptr) << Entry;
    auto Refused = Foreign.invoke(Fn, {}, {});
    ASSERT_FALSE(Refused) << Entry << " ran code of a foreign-owned module";
    EXPECT_EQ(Refused.error(), ErrCode::Value::IncompatibleImportType) << Entry;
    EXPECT_EQ(NInst->getGCRootsOwnerId(), Owner.getAllocator().getId());
  }

  // Control: the owner runs every path, and the global N wrote is intact.
  for (const char *Entry : {"run", "run_ref", "run_indirect"}) {
    ASSERT_TRUE(Owner.invoke(MInst->findFuncExports(Entry), {}, {})) << Entry;
    auto Got = Owner.invoke(GetFn, {}, {});
    ASSERT_TRUE(Got) << Entry;
    ASSERT_EQ(Got->size(), 1u);
    EXPECT_EQ((*Got)[0].first.get<uint32_t>(), UINT32_C(7)) << Entry;
  }
}

// The claim walk holds the module mutex shared, but registerModule stamps
// only after publishing the module, with the mutex long released, while every
// index-space mutator drops the stamp under it. A root added between the
// walk and the stamp store was never walked, so the stamp must not land: the
// store checks that the index space is the one the walk saw.
TEST(GCThread, StampRefusedWhenIndexSpaceMovedDuringWalk) {
  RegisterWalkModule Host("stamp-race");
  ASSERT_NE(Host.addOwnedHostFunc(), nullptr);
  const uint64_t Before = Host.getGCRootsGeneration();
  // The walk is under way (it saw generation Before); a host thread now adds
  // a managed-capable global, which drops the stamp and moves the index
  // space.
  auto *Late =
      Host.addOwnedGlobal(std::make_unique<Runtime::Instance::GlobalInstance>(
          AST::GlobalType(ValType(TypeCode::RefNull, TypeCode::StructRef),
                          ValMut::Var),
          /*CanHoldManagedIn=*/true,
          ValVariant(
              RefVariant(ValType(TypeCode::RefNull, TypeCode::StructRef)))));
  ASSERT_NE(Late, nullptr);
  EXPECT_NE(Host.getGCRootsGeneration(), Before);
  // The walk finishes and tries to stamp with what it saw: refused, and the
  // module stays unstamped, so the next call walks again and finds Late.
  EXPECT_FALSE(Host.stampGCRootsOwner(7, Before))
      << "a stamp landed over an index space the walk never saw";
  EXPECT_EQ(Host.getGCRootsOwnerId(), 0u);
  // A walk that saw the current index space stamps normally.
  EXPECT_TRUE(Host.stampGCRootsOwner(7, Host.getGCRootsGeneration()));
  EXPECT_EQ(Host.getGCRootsOwnerId(), 7u);
}

// A managed-capable global, the kind an ownership walk must claim.
std::unique_ptr<Runtime::Instance::GlobalInstance> makeManagedGlobal() {
  const ValType StructTy(TypeCode::RefNull, TypeCode::StructRef);
  return std::make_unique<Runtime::Instance::GlobalInstance>(
      AST::GlobalType(StructTy, ValMut::Var), /*CanHoldManagedIn=*/true,
      ValVariant(RefVariant(StructTy)));
}

// Records that its body ran: the observable that separates an admitted call
// from a refused one.
class FlagHost : public Runtime::HostFunction<FlagHost> {
public:
  explicit FlagHost(std::atomic<int> &C) : Calls(C) {}
  Expect<void> body(const Runtime::CallingFrame &) {
    Calls.fetch_add(1);
    return {};
  }

private:
  std::atomic<int> &Calls;
};

// (module (import "sig" "arrive" (func)) (import "locked" "target" (func))
//   (func (export "run") (call 0) (call 1)))
const std::array<WasmEdge::Byte, 69> TwoCallWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x02, 0x1e, 0x02, 0x03, 0x73, 0x69, 0x67, 0x06, 0x61, 0x72,
    0x72, 0x69, 0x76, 0x65, 0x00, 0x00, 0x06, 0x6c, 0x6f, 0x63, 0x6b, 0x65,
    0x64, 0x06, 0x74, 0x61, 0x72, 0x67, 0x65, 0x74, 0x00, 0x00, 0x03, 0x02,
    0x01, 0x00, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x02, 0x0a,
    0x08, 0x01, 0x06, 0x00, 0x10, 0x00, 0x10, 0x01, 0x0b};

// Every index-space mutator (addHostGlobal, importTable, ...) pushes into
// TabInsts/GlobInsts under the module mutex held exclusively, and the
// ownership walk iterates those same vectors. A walk that does not hold the
// mutex shared races the push (a reallocated buffer under the iterator).
// Observable without a sanitizer: while a mutator holds the mutex, a walk
// that reached the module must not have attached anything.
//
// Non-vacuous: the caller proves it reached the blocked walk. It runs a wasm
// function that first calls a host function in another module (which sets
// Arrived, and whose own claim is unaffected by the lock) and then calls into
// the locked module, whose walk is the very next thing it does. The window
// asserted below therefore starts after the caller arrived, not after an
// unsynchronized guess that it did.
TEST(GCThread, ClaimWalkWaitsForModuleMutator) {
  Configure Conf = makeGCConf();
  Executor::Executor Exec(Conf);
  GC::Allocator &Alloc = Exec.getAllocator();
  Runtime::StoreManager Store;

  std::atomic<int> Arrived{0};
  RegisterWalkModule Sig("sig");
  ASSERT_TRUE(Sig.addHostFunc("arrive", std::make_unique<FlagHost>(Arrived)));
  RegisterWalkModule Locked("locked");
  ASSERT_TRUE(Locked.addHostFunc("target", std::make_unique<NopHost>()));
  ASSERT_TRUE(Exec.registerModule(Store, Sig));
  ASSERT_TRUE(Exec.registerModule(Store, Locked));
  // Added after the registration walk: the module is still open, so the stamp
  // is dropped and the first call into it has to walk the index space again.
  auto *Glob = Locked.addOwnedGlobal(makeManagedGlobal());
  ASSERT_NE(Glob, nullptr);
  ASSERT_EQ(Locked.getGCRootsOwnerId(), 0u);
  ASSERT_FALSE(Alloc.debugHasGlobalRoot(*Glob));

  Loader::Loader LoaderEngine(Conf, &Executor::Executor::Intrinsics);
  Validator::Validator ValidatorEngine(Conf);
  auto ModOrErr = LoaderEngine.parseModule(TwoCallWasm);
  ASSERT_TRUE(ModOrErr);
  std::shared_ptr<AST::Module> Mod{std::move(*ModOrErr)};
  ASSERT_TRUE(ValidatorEngine.validate(*Mod));
  auto InstOrErr = Exec.instantiateModule(Store, *Mod);
  ASSERT_TRUE(InstOrErr);
  auto Inst = std::move(*InstOrErr);
  const auto *RunFn = Inst->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);

  // A mutator is in progress on the locked module: the walk of the first call
  // into it must wait. The observation is the allocator's root set, read
  // under its own lock, so the poll itself races nothing.
  auto Lock = Locked.lockExclusive();
  std::thread Caller([&] { EXPECT_TRUE(Exec.invoke(RunFn, {}, {})); });
  while (Arrived.load() == 0) {
    std::this_thread::yield();
  }
  const auto Deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
  while (std::chrono::steady_clock::now() < Deadline &&
         !Alloc.debugHasGlobalRoot(*Glob)) {
    std::this_thread::yield();
  }
  EXPECT_FALSE(Alloc.debugHasGlobalRoot(*Glob))
      << "the ownership walk ran without the module lock";
  Lock.unlock();
  Caller.join();
  EXPECT_TRUE(Alloc.debugHasGlobalRoot(*Glob));
  EXPECT_TRUE(Glob->isManagedByController());
  EXPECT_EQ(Locked.getGCRootsOwnerId(), Alloc.getId());
}

// Bound to LateCallWasm's "host" "store" import: allocates a managed object
// and stores it into the host module's "late" global, found by name at call
// time, so that global is the object's only root.
class StoreIntoLateGlobalHost
    : public Runtime::HostFunction<StoreIntoLateGlobalHost> {
public:
  explicit StoreIntoLateGlobalHost(Runtime::Instance::ModuleInstance &M)
      : Mod(M) {}
  Expect<void> body(const Runtime::CallingFrame &CF) {
    using RawData = Runtime::Instance::GCInstance::RawData;
    auto *Late = Mod.findGlobalExports("late");
    if (Late == nullptr) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    void *P = CF.getExecutor()->getAllocator().allocate(
        [](void *Ptr) noexcept {
          auto *Raw = static_cast<RawData *>(Ptr);
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 0;
        },
        sizeof(RawData));
    if (P == nullptr) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    const ValType StructTy(TypeCode::RefNull, TypeCode::StructRef);
    Late->setValue(ValVariant(RefVariant(StructTy, static_cast<RawData *>(P))));
    return {};
  }

private:
  Runtime::Instance::ModuleInstance &Mod;
};

// (module (import "host" "store" (func)) (func (export "run") (call 0)))
const std::array<WasmEdge::Byte, 51> LateCallWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01,
    0x60, 0x00, 0x00, 0x02, 0x0e, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74,
    0x05, 0x73, 0x74, 0x6f, 0x72, 0x65, 0x00, 0x00, 0x03, 0x02, 0x01,
    0x00, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x01, 0x0a,
    0x06, 0x01, 0x04, 0x00, 0x10, 0x00, 0x0b};

// A host module stays open to additions until its first host-function call
// finalizes it. enterFunction runs the callee's ownership check first (a
// refused call must leave the module open) and finalizes afterwards, so a
// root added in between (the module is still open) drops the stamp after
// the call was admitted: unless the check is repeated once the module is
// finalized, the call runs with that root unattached and a managed
// reference the callee stores into it is rooted by nobody this collector
// scans. Deterministic: the executor's pre-finalize hook parks the call in
// exactly that window while the test adds the root.
TEST(GCThread, LateRootAddedBeforeFirstCallIsWalkedByThatCall) {
  Configure Conf = makeGCConf();
  RegisterWalkModule Host("host");
  ASSERT_TRUE(Host.addHostFunc(
      "store", std::make_unique<StoreIntoLateGlobalHost>(Host)));
  Executor::Executor Exec(Conf);
  GC::Allocator &Alloc = Exec.getAllocator();
  GC::Controller &Ctrl = Exec.getController();
  Alloc.setManualGC(true);
  Runtime::StoreManager Store;
  // Registered import-free: the module carries a stamp and stays open.
  ASSERT_TRUE(Exec.registerModule(Store, Host));
  EXPECT_EQ(Host.getGCRootsOwnerId(),
            Runtime::Instance::ModuleInstance::kSharedGCRootsOwner);

  Loader::Loader LoaderEngine(Conf, &Executor::Executor::Intrinsics);
  Validator::Validator ValidatorEngine(Conf);
  auto ModOrErr = LoaderEngine.parseModule(LateCallWasm);
  ASSERT_TRUE(ModOrErr);
  std::shared_ptr<AST::Module> Mod{std::move(*ModOrErr)};
  ASSERT_TRUE(ValidatorEngine.validate(*Mod));
  auto InstOrErr = Exec.instantiateModule(Store, *Mod);
  ASSERT_TRUE(InstOrErr);
  auto Inst = std::move(*InstOrErr);
  const auto *RunFn = Inst->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);

  // Park the call after its ownership check, before it reads the finalized
  // state; add the root the callee will write in that window through
  // the ordinary public path (the module is still open), then let it go.
  std::atomic<bool> AtFinalize{false};
  std::atomic<bool> Go{false};
  Exec.setPreHostCallHook([&] {
    AtFinalize.store(true);
    while (!Go.load()) {
      std::this_thread::yield();
    }
  });
  std::thread Caller([&] { EXPECT_TRUE(Exec.invoke(RunFn, {}, {})); });
  while (!AtFinalize.load()) {
    std::this_thread::yield();
  }
  ASSERT_FALSE(Host.isInstantiateFinalized());
  auto *Late = Host.addOwnedGlobal(
      std::make_unique<Runtime::Instance::GlobalInstance>(
          AST::GlobalType(ValType(TypeCode::RefNull, TypeCode::StructRef),
                          ValMut::Var),
          /*CanHoldManagedIn=*/true,
          ValVariant(
              RefVariant(ValType(TypeCode::RefNull, TypeCode::StructRef)))),
      "late");
  ASSERT_NE(Late, nullptr);
  EXPECT_EQ(Host.getGCRootsOwnerId(), 0u);
  Go.store(true);
  Caller.join();
  Exec.setPreHostCallHook(nullptr);
  EXPECT_TRUE(Host.isInstantiateFinalized());
  EXPECT_TRUE(Alloc.debugHasGlobalRoot(*Late))
      << "the call ran with a root its ownership walk never saw";
  EXPECT_EQ(Host.getGCRootsOwnerId(), Alloc.getId());
  // The object's only root is the late global: two cycles (newborn grace,
  // then a real reclaim) keep it only if that global is scanned.
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_GT(Alloc.getMemoryUsage(), 0u)
      << "the object stored into the late root was collected";
}

// The ownership check of a callee runs before its host module is finalized,
// so a call refused because the module's roots belong to another executor
// leaves the module exactly as it was: still open, so its owner can go on
// building it. Finalizing first would turn a wrong-executor call into a
// permanent WrongVMWorkflow for every later addHost* by the owner.
TEST(GCThread, RefusedCalleeClaimLeavesHostModuleOpen) {
  Configure Conf = makeGCConf();
  RegisterWalkModule Host("host");
  ASSERT_TRUE(Host.addHostFunc("store", std::make_unique<NopHost>()));

  // Executor B registers the (still root-less) host module and instantiates
  // a caller that imports its function.
  Executor::Executor B(Conf);
  Runtime::StoreManager StoreB;
  ASSERT_TRUE(B.registerModule(StoreB, Host));
  Loader::Loader LoaderEngine(Conf, &Executor::Executor::Intrinsics);
  Validator::Validator ValidatorEngine(Conf);
  auto ModOrErr = LoaderEngine.parseModule(LateCallWasm);
  ASSERT_TRUE(ModOrErr);
  std::shared_ptr<AST::Module> Mod{std::move(*ModOrErr)};
  ASSERT_TRUE(ValidatorEngine.validate(*Mod));
  auto InstOrErr = B.instantiateModule(StoreB, *Mod);
  ASSERT_TRUE(InstOrErr);
  auto Inst = std::move(*InstOrErr);
  const auto *RunFn = Inst->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);

  // The owner, executor A, then adds a managed global and claims it.
  auto *Glob =
      Host.addOwnedGlobal(std::make_unique<Runtime::Instance::GlobalInstance>(
          AST::GlobalType(ValType(TypeCode::RefNull, TypeCode::StructRef),
                          ValMut::Var),
          /*CanHoldManagedIn=*/true,
          ValVariant(
              RefVariant(ValType(TypeCode::RefNull, TypeCode::StructRef)))));
  ASSERT_NE(Glob, nullptr);
  Executor::Executor A(Conf);
  Runtime::StoreManager StoreA;
  ASSERT_TRUE(A.registerModule(StoreA, Host));
  EXPECT_EQ(Host.getGCRootsOwnerId(), A.getAllocator().getId());

  // B's call into the host function is refused (the global is A's) ...
  auto Refused = B.invoke(RunFn, {}, {});
  ASSERT_FALSE(Refused);
  EXPECT_EQ(Refused.error(), ErrCode::Value::IncompatibleImportType);
  // ... and the module is still open to its owner.
  EXPECT_FALSE(Host.isInstantiateFinalized())
      << "a refused call finalized the host module";
  EXPECT_NE(Host.addOwnedHostFunc(), nullptr)
      << "the owner can no longer add to its host module";
  EXPECT_EQ(Host.getGCRootsOwnerId(), A.getAllocator().getId());
}

// Two executors can reach a still-open host module's first call at the same
// time, and only one of them finalizes it. The ownership check that admits
// the call runs before finalization, so it can describe an index space the
// module no longer has; the check after finalization is the decisive one and
// must not be skipped by the caller that did not finalize. Deterministic: the
// pre-host-call seam parks A between its check and its finalized-state read,
// and B does the whole first call in that window.
TEST(GCThread, ConcurrentFirstHostCallRevalidatesAfterAnotherFinalizes) {
  Configure Conf = makeGCConf();
  Executor::Executor A(Conf);
  Executor::Executor B(Conf);
  std::atomic<int> Calls{0};
  // Root-less while A checks it: the walk stamps it shared, so A is admitted.
  RegisterWalkModule Host("open-host");
  ASSERT_TRUE(Host.addHostFunc("target", std::make_unique<FlagHost>(Calls)));
  const auto *TargetFn = Host.findFuncExports("target");
  ASSERT_NE(TargetFn, nullptr);

  std::atomic<bool> AtSeam{false};
  std::atomic<bool> Go{false};
  A.setPreHostCallHook([&] {
    AtSeam.store(true);
    while (!Go.load()) {
      std::this_thread::yield();
    }
  });
  bool AAdmitted = true;
  ErrCode AErr = ErrCode::Value::Success;
  std::thread Caller([&] {
    auto R = A.invoke(TargetFn, {}, {});
    AAdmitted = static_cast<bool>(R);
    if (!R) {
      AErr = R.error();
    }
  });
  while (!AtSeam.load()) {
    std::this_thread::yield();
  }
  // No fatal assertion from here to the join: the caller is parked in the
  // seam and only Go releases it.
  EXPECT_FALSE(Host.isInstantiateFinalized());
  EXPECT_EQ(Host.getGCRootsOwnerId(),
            Runtime::Instance::ModuleInstance::kSharedGCRootsOwner);

  // In A's window the module gains a managed root (it is still open), and B
  // makes the whole first call: B claims that root, finalizes the module and
  // stamps it with B's id.
  auto *Glob =
      Host.addOwnedGlobal(std::make_unique<Runtime::Instance::GlobalInstance>(
          AST::GlobalType(ValType(TypeCode::RefNull, TypeCode::StructRef),
                          ValMut::Var),
          /*CanHoldManagedIn=*/true,
          ValVariant(
              RefVariant(ValType(TypeCode::RefNull, TypeCode::StructRef)))));
  EXPECT_NE(Glob, nullptr);
  EXPECT_TRUE(B.invoke(TargetFn, {}, {}));
  EXPECT_TRUE(Host.isInstantiateFinalized());
  EXPECT_EQ(Host.getGCRootsOwnerId(), B.getAllocator().getId());
  if (Glob != nullptr) {
    EXPECT_TRUE(B.getAllocator().debugHasGlobalRoot(*Glob));
  }
  EXPECT_EQ(Calls.load(), 1);

  Go.store(true);
  Caller.join();
  A.setPreHostCallHook(nullptr);
  // A finds the module finalized by someone else. Its own check is stale: the
  // module is B's now, and a managed reference A stores into that global is
  // barriered against the wrong allocator and scanned by nobody A collects.
  EXPECT_FALSE(AAdmitted) << "a call ran on a module another executor owns";
  if (!AAdmitted) {
    EXPECT_EQ(AErr, ErrCode::Value::IncompatibleImportType);
  }
  EXPECT_EQ(Calls.load(), 1);
  EXPECT_EQ(Host.getGCRootsOwnerId(), B.getAllocator().getId());
}

// registerModule decides whether to stamp from the module's import counts.
// Those live in the same vectors and fields the index-space mutators write
// under the module mutex, so the decision must come from the locked walk,
// not from a later unlocked read. A concurrent mutator makes the unlocked
// read a data race that ThreadSanitizer reports; on the fixed side this
// loop is clean under it, and the final registration stamps normally.
TEST(GCThread, RegisterModuleStampGateReadUnderModuleLock) {
  Configure Conf = makeGCConf();
  RegisterWalkModule Host("gate-race");
  ASSERT_NE(Host.addOwnedHostFunc(), nullptr);
  Executor::Executor Exec(Conf);
  Runtime::StoreManager Store;
  constexpr int Rounds = 64;

  std::atomic<bool> Go{false};
  std::thread Mutator([&] {
    while (!Go.load()) {
      std::this_thread::yield();
    }
    for (int I = 0; I < Rounds; ++I) {
      // Fixed, non-managed: freely shareable, so every registration below
      // keeps succeeding while the index space keeps growing under it.
      EXPECT_NE(
          Host.addOwnedTable(std::make_unique<Runtime::Instance::TableInstance>(
              AST::TableType(TypeCode::FuncRef, 1, 1),
              /*CanHoldManagedIn=*/false)),
          nullptr);
    }
  });
  Go.store(true);
  for (int I = 0; I < Rounds; ++I) {
    ASSERT_TRUE(Exec.registerModule(Store, Host, "alias" + std::to_string(I)));
  }
  Mutator.join();

  // Quiescent: one more registration sees an import-free module whose roots
  // tie it to no allocator.
  ASSERT_TRUE(Exec.registerModule(Store, Host, "final"));
  EXPECT_EQ(Host.getGCRootsOwnerId(),
            Runtime::Instance::ModuleInstance::kSharedGCRootsOwner);
}

// Two registrations of one module (two stores, or two alias names) walk the
// same roots at the same time, and only one of them turns a global from
// unattached into this allocator's; the other finds it already attached. The
// attachment is therefore held by both, and the registration that fails to
// publish must release only its own hold. Reversing the attachment outright
// detaches a root the other, successful registration published and stamped.
TEST(GC, ReversedClaimKeepsARootAnotherClaimHolds) {
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  auto Glob = makeManagedGlobal();
  auto First = Glob->claimForRegister(Alloc);
  ASSERT_TRUE(First);
  EXPECT_TRUE(*First) << "the claim that attached the root recorded nothing";
  auto Second = Glob->claimForRegister(Alloc);
  ASSERT_TRUE(Second);
  EXPECT_TRUE(*Second) << "a walk took no hold on a root it goes on to rely on";
  ASSERT_TRUE(Alloc.debugHasGlobalRoot(*Glob));
  // The second registration published and committed, so its hold is now
  // permanent; the first failed and reverses exactly what it recorded.
  Glob->detachAllocator(Alloc);
  EXPECT_TRUE(Alloc.debugHasGlobalRoot(*Glob))
      << "a reversed claim detached a root another claim holds";
  EXPECT_TRUE(Glob->isManagedByController());
}

// The same hazard end to end. One registration loses on the alias name and
// reverses its claim; the other publishes the module and stamps it, after
// which no call walks its roots again. Deterministic: the loser starts alone
// and is the one that attaches the observed global, and the long tail of
// managed globals behind it keeps its walk running while the winner's walk
// passes that global and publishes.
TEST(GCThread, FailedRegistrationKeepsRootsOfConcurrentSuccess) {
  Configure Conf = makeGCConf();
  Executor::Executor Exec(Conf);
  GC::Allocator &Alloc = Exec.getAllocator();
  Runtime::StoreManager Store;
  RegisterWalkModule Occupier("occupier");
  ASSERT_TRUE(Store.registerModule(&Occupier, "taken"));

  RegisterWalkModule Host("two-registrations");
  auto *Glob = Host.addOwnedGlobal(makeManagedGlobal());
  ASSERT_NE(Glob, nullptr);
  for (int I = 0; I < 256; ++I) {
    ASSERT_NE(Host.addOwnedGlobal(makeManagedGlobal()), nullptr);
  }

  std::thread Loser([&] {
    auto Res = Exec.registerModule(Store, Host, "taken");
    EXPECT_FALSE(Res) << "the alias was taken, this registration must fail";
  });
  const auto Deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (!Alloc.debugHasGlobalRoot(*Glob) &&
         std::chrono::steady_clock::now() < Deadline) {
    std::this_thread::yield();
  }
  EXPECT_TRUE(Alloc.debugHasGlobalRoot(*Glob))
      << "the losing registration never attached the observed root";
  // Not ASSERT_*: the losing thread is still joinable here.
  EXPECT_TRUE(Exec.registerModule(Store, Host, "ok"));
  Loser.join();

  EXPECT_TRUE(Alloc.debugHasGlobalRoot(*Glob))
      << "a failed registration detached a root the published module holds";
  EXPECT_TRUE(Glob->isManagedByController());
  // The published module carries the stamp, so no later call walks it again:
  // a detached root here stays detached for good.
  EXPECT_TRUE(Host.gcRootsOwnedBy(Alloc.getId()));
}

// A module whose roots tie it to no allocator (numeric globals, fixed
// non-managed tables, host functions) is stamped shared by the first walk
// and then run by every executor without another walk: two executors
// alternating on it must not re-stamp and re-walk on every switch. The
// stamp is dropped like any other by a root addition, and the next walk,
// which finds an allocator-specific root, stamps the specific id.
TEST(GCThread, ModuleWithoutAllocatorSpecificRootsIsSharedAcrossExecutors) {
  Configure Conf = makeGCConf();
  RegisterWalkModule Shared("shared-roots");
  ASSERT_NE(
      Shared.addOwnedGlobal(std::make_unique<Runtime::Instance::GlobalInstance>(
          AST::GlobalType(ValType(TypeCode::I32), ValMut::Var),
          /*CanHoldManagedIn=*/false, ValVariant(UINT32_C(1)))),
      nullptr);
  ASSERT_NE(
      Shared.addOwnedTable(std::make_unique<Runtime::Instance::TableInstance>(
          AST::TableType(TypeCode::FuncRef, 1, 1),
          /*CanHoldManagedIn=*/false)),
      nullptr);
  auto *HostFn = Shared.addOwnedHostFunc();
  ASSERT_NE(HostFn, nullptr);
  constexpr uint64_t kShared =
      Runtime::Instance::ModuleInstance::kSharedGCRootsOwner;

  Executor::Executor A(Conf);
  Executor::Executor B(Conf);
  ASSERT_TRUE(A.invoke(HostFn, {}, {}));
  EXPECT_EQ(Shared.getGCRootsOwnerId(), kShared);
  ASSERT_TRUE(B.invoke(HostFn, {}, {}));
  EXPECT_EQ(Shared.getGCRootsOwnerId(), kShared)
      << "the second executor re-stamped a module it does not need to own";
  ASSERT_TRUE(A.invoke(HostFn, {}, {}));
  EXPECT_EQ(Shared.getGCRootsOwnerId(), kShared);
  // Both executors admit it without a walk.
  EXPECT_TRUE(Shared.gcRootsOwnedBy(A.getAllocator().getId()));
  EXPECT_TRUE(Shared.gcRootsOwnedBy(B.getAllocator().getId()));

  // A managed-capable root makes the module allocator-specific again. The
  // module is finalized by now, so the root goes into a fresh one.
  RegisterWalkModule Specific("specific-roots");
  ASSERT_NE(
      Specific.addOwnedGlobal(
          std::make_unique<Runtime::Instance::GlobalInstance>(
              AST::GlobalType(ValType(TypeCode::RefNull, TypeCode::StructRef),
                              ValMut::Var),
              /*CanHoldManagedIn=*/true,
              ValVariant(RefVariant(
                  ValType(TypeCode::RefNull, TypeCode::StructRef))))),
      nullptr);
  auto *SpecificFn = Specific.addOwnedHostFunc();
  ASSERT_TRUE(A.invoke(SpecificFn, {}, {}));
  EXPECT_EQ(Specific.getGCRootsOwnerId(), A.getAllocator().getId());
  EXPECT_FALSE(Specific.gcRootsOwnedBy(B.getAllocator().getId()));
  auto Refused = B.invoke(SpecificFn, {}, {});
  ASSERT_FALSE(Refused);
  EXPECT_EQ(Refused.error(), ErrCode::Value::IncompatibleImportType);
}

// The positive side of the same check: a callee module nobody owns yet (a
// prebuilt host module with an unattached managed global, published straight
// into the store) is claimed by the executor that first calls into it, so the
// global becomes a root of that executor's collector before any guest code of
// the module can write it.
TEST(GCThread, CalleeModuleClaimedOnFirstCall) {
  Configure Conf = makeGCConf();
  RegisterWalkModule Host("N");
  auto *Glob =
      Host.addOwnedGlobal(std::make_unique<Runtime::Instance::GlobalInstance>(
          AST::GlobalType(ValType(TypeCode::RefNull, TypeCode::StructRef),
                          ValMut::Var),
          /*CanHoldManagedIn=*/true,
          ValVariant(
              RefVariant(ValType(TypeCode::RefNull, TypeCode::StructRef)))));
  ASSERT_NE(Glob, nullptr);
  ASSERT_NE(Host.addOwnedHostFunc("set"), nullptr);
  Runtime::StoreManager Store;
  // Published without an executor: no claim, no stamp.
  ASSERT_TRUE(Store.registerModule(&Host));
  ASSERT_EQ(Host.getGCRootsOwnerId(), 0u);

  Loader::Loader LoaderEngine(Conf, &Executor::Executor::Intrinsics);
  Validator::Validator ValidatorEngine(Conf);
  auto MOrErr = LoaderEngine.parseModule(CalleeImportWasm);
  ASSERT_TRUE(MOrErr);
  std::shared_ptr<AST::Module> MMod{std::move(*MOrErr)};
  ASSERT_TRUE(ValidatorEngine.validate(*MMod));
  Executor::Executor Exec(Conf);
  auto MInstOrErr = Exec.instantiateModule(Store, *MMod);
  ASSERT_TRUE(MInstOrErr);
  auto MInst = std::move(*MInstOrErr);
  EXPECT_FALSE(Glob->isManagedByController());

  ASSERT_TRUE(Exec.invoke(MInst->findFuncExports("run"), {}, {}));
  EXPECT_TRUE(Glob->isManagedByController())
      << "the first call into the host module did not claim its global";
  EXPECT_EQ(Host.getGCRootsOwnerId(), Exec.getAllocator().getId());
}

#endif // WASMEDGE_USE_LLVM

} // namespace
