// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/gc/GCHostRootsTest.cpp - GC host root tests ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for GC references that the host holds:
/// allocator host roots, executor retain and release, the C API
/// getters and mutators, and element-segment and exception roots.
///
//===----------------------------------------------------------------------===//

#include "gchelpers.h"

namespace {

using namespace WasmEdge;
using namespace WasmEdge::GCTest;

// Returns an externalized gc struct to the host (regression for the host-root
// retention of externalized refs):
//   (func (export "make_ext") (param i32) (result externref)
//     (extern.convert_any (struct.new $s (local.get 0))))
const std::array<WasmEdge::Byte, 64> MakeExtWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x02,
    0x5f, 0x01, 0x7f, 0x01, 0x60, 0x01, 0x7f, 0x01, 0x6f, 0x03, 0x02,
    0x01, 0x01, 0x07, 0x0c, 0x01, 0x08, 0x6d, 0x61, 0x6b, 0x65, 0x5f,
    0x65, 0x78, 0x74, 0x00, 0x00, 0x0a, 0x0b, 0x01, 0x09, 0x00, 0x20,
    0x00, 0xfb, 0x00, 0x00, 0xfb, 0x1b, 0x0b, 0x00, 0x0b, 0x04, 0x6e,
    0x61, 0x6d, 0x65, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

// A passive element segment whose init expr allocates a gc struct (regression
// for element-segment refs being scanned as roots). After two collections the
// segment holds the only reference; the test materializes it via array.new_elem
// and reads the field, garbage if the segment was not scanned.
//   (type $s (struct (field i32))) (type $a (array (ref null $s)))
//   (elem $e (ref null $s) (item (struct.new $s (i32.const 42))))
//   (func (export "test") coll; coll;
//     check (struct.get $s 0 (ref.cast (ref $s)
//       (array.get $a (array.new_elem $a $e 0 1) 0))))
const std::array<WasmEdge::Byte, 164> ElemRootWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x10, 0x04, 0x5f,
    0x01, 0x7f, 0x00, 0x5e, 0x63, 0x00, 0x00, 0x60, 0x00, 0x00, 0x60, 0x01,
    0x7f, 0x00, 0x02, 0x16, 0x02, 0x02, 0x67, 0x63, 0x04, 0x63, 0x6f, 0x6c,
    0x6c, 0x00, 0x02, 0x02, 0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b,
    0x00, 0x03, 0x03, 0x02, 0x01, 0x02, 0x07, 0x08, 0x01, 0x04, 0x74, 0x65,
    0x73, 0x74, 0x00, 0x02, 0x09, 0x0b, 0x01, 0x05, 0x63, 0x00, 0x01, 0x41,
    0x2a, 0xfb, 0x00, 0x00, 0x0b, 0x0a, 0x25, 0x01, 0x23, 0x01, 0x01, 0x63,
    0x01, 0x10, 0x00, 0x10, 0x00, 0x41, 0x00, 0x41, 0x01, 0xfb, 0x0a, 0x01,
    0x00, 0x21, 0x00, 0x20, 0x00, 0x41, 0x00, 0xfb, 0x0b, 0x01, 0xfb, 0x16,
    0x00, 0xfb, 0x02, 0x00, 0x00, 0x10, 0x01, 0x0b, 0x00, 0x2e, 0x04, 0x6e,
    0x61, 0x6d, 0x65, 0x01, 0x0e, 0x02, 0x00, 0x04, 0x63, 0x6f, 0x6c, 0x6c,
    0x01, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x02, 0x08, 0x01, 0x02, 0x01,
    0x00, 0x03, 0x61, 0x72, 0x72, 0x04, 0x07, 0x02, 0x00, 0x01, 0x73, 0x01,
    0x01, 0x61, 0x08, 0x04, 0x01, 0x00, 0x01, 0x65};

// Exports a global and a table each holding a genuinely concrete (ref $s)
// managed struct reference, produced by a constant struct.new initializer.
// Unlike every other GC-suite fixture (which hand-stamps an abstract StructRef
// ref, reads an i31 slot, or reads an already-normalized invoke result), the
// slot RefVariant here is stamped ValType(TypeCode::Ref, <type index>) by
// Executor::structNew -- the concrete
// form that Executor::expandGCRefType must normalize to structref. Used by
// GC.ExpandGCRefTypeNormalizesConcreteSlotRef.
//   (type $s (struct (field i32)))
//   (global (export "g") (ref $s) (struct.new $s (i32.const 42)))
//   (table (export "t") 1 1 (ref $s) (struct.new $s (i32.const 7)))
const std::array<WasmEdge::Byte, 67> ConcreteGlobalTableWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x5f,
    0x01, 0x7f, 0x00, 0x04, 0x0e, 0x01, 0x40, 0x00, 0x64, 0x00, 0x01, 0x01,
    0x01, 0x41, 0x07, 0xfb, 0x00, 0x00, 0x0b, 0x06, 0x0a, 0x01, 0x64, 0x00,
    0x00, 0x41, 0x2a, 0xfb, 0x00, 0x00, 0x0b, 0x07, 0x09, 0x02, 0x01, 0x67,
    0x03, 0x00, 0x01, 0x74, 0x01, 0x00, 0x00, 0x0b, 0x04, 0x6e, 0x61, 0x6d,
    0x65, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

// Allocate one zero-child GC object directly through the allocator, wrap it as
// a RefVariant, and exercise retain/collect/release. Zero-Length RawData so the
// collector's child-scan reads no garbage children.
TEST(GC, AllocatorHostRootsRetainRelease) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Allocator Alloc;

  void *P = Alloc.allocate(
      [](void *Ptr) noexcept {
        auto *Raw = static_cast<RawData *>(Ptr);
        Raw->ModInst = nullptr;
        Raw->TypeIdx = 0;
        Raw->Length = 0;
      },
      sizeof(RawData));
  ASSERT_NE(P, nullptr);
  RefVariant Ref(ValType(TypeCode::Ref, TypeCode::StructRef),
                 static_cast<RawData *>(P));

  Alloc.retainResult(Ref);

  EXPECT_TRUE(Alloc.manualCollect()); // new object: gray -> black -> white
  EXPECT_GT(Alloc.getMemoryUsage(), 0u);
  EXPECT_TRUE(Alloc.manualCollect()); // retained: re-grayed, survives the sweep
  EXPECT_GT(Alloc.getMemoryUsage(), 0u);

  Alloc.releaseRef(Ref);
  EXPECT_TRUE(Alloc.manualCollect()); // unrooted now -> swept
  EXPECT_EQ(Alloc.getMemoryUsage(), 0u);
}

TEST(GC, ReleaseAllRefsDoesNotClearScopedRoots) {
  // Scoped BoundaryRoots must survive a public releaseAllRefs()
  // (WasmEdge_{VM,Executor}ReleaseAllRefs) -- they live in a separate store the
  // host-facing release-all cannot consume, and are still scanned as roots.
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Allocator Alloc;
  Alloc.setManualGC(true);

  void *P = Alloc.allocate(
      [](void *Ptr) noexcept {
        auto *Raw = static_cast<RawData *>(Ptr);
        Raw->ModInst = nullptr;
        Raw->TypeIdx = 0;
        Raw->Length = 0;
      },
      sizeof(RawData));
  ASSERT_NE(P, nullptr);
  RefVariant Ref(ValType(TypeCode::Ref, TypeCode::StructRef),
                 static_cast<RawData *>(P));

  {
    GC::BoundaryRoots BR(Alloc);
    BR.pin(Ref); // scoped root only

    // A newly-allocated object is born gray and gets traced on its very
    // first collect regardless of rootedness (see
    // GC.AllocatorHostRootsRetainRelease), so this first collect is not yet
    // a meaningful check of the scoped root -- it just clears that grace
    // period before the real assertion below.
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_GT(Alloc.getMemoryUsage(), 0u);

    // Host-facing release-all must not drop the scoped root.
    Alloc.releaseAllRefs();

    // Forced collection: a scoped root that leaked into HostRoots (pre-fix)
    // would have been cleared by releaseAllRefs above and the object swept
    // here.
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_GT(Alloc.getMemoryUsage(), 0u); // survived: still a scoped root

    // BR goes out of scope -> releaseScopedRef drops the last root.
  }

  // Now unrooted: the object is reclaimed.
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_EQ(Alloc.getMemoryUsage(), 0u);
}

TEST(GC, ExecutorRetainAndRelease) {
  Configure Conf = makeGCConf();
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  // autoCollect off: only explicit manualCollect() runs, else asserts flaky.
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  auto Res = VM.execute("make", std::initializer_list<ValVariant>{UINT32_C(42)},
                        {ValType(TypeCode::I32)});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1u);
  const RefVariant Ref = (*Res)[0].first.get<RefVariant>();
  EXPECT_GT(Alloc.getMemoryUsage(), Before); // struct allocated

  EXPECT_TRUE(Alloc.manualCollect()); // new -> white
  EXPECT_TRUE(Alloc.manualCollect()); // retained by HostRoots -> survives
  EXPECT_GT(Alloc.getMemoryUsage(), Before);

  VM.getExecutor().releaseRef(Ref);
  EXPECT_TRUE(Alloc.manualCollect()); // unrooted -> reclaimed
  EXPECT_EQ(Alloc.getMemoryUsage(), Before);
}

// Load-bearing check: Executor::expandGCRefType
// (include/executor/executor.h) must normalize a genuinely concrete
// (ref $t) slot ref into its abstract composite kind (structref) so the
// producer-bearing retained C-API getters
// (WasmEdge_GlobalInstanceGetValueRetained /
// WasmEdge_TableInstanceGetDataRetained) recognize it as GC-retainable and
// pin it before handing it to the host. ValType::isHostRetainedRefType() is
// false for a concrete type-index ref, so if expandGCRefType ever returned the
// concrete type unchanged the getter would skip retention and hand back a
// dangling reference -- a use-after-free. Every other GC-suite test hand-stamps
// an already-abstract StructRef, reads an i31 (abstract) slot, or reads an
// invoke result (Executor::invoke already normalizes returned refs to
// abstract), so none falsifies the concrete->abstract transform. This one does:
// it instantiates a module whose exported global and table hold concrete (ref
// $s) refs produced by a constant struct.new, reads each slot exactly as the
// getter does (getValue()/getRefAddr()), asserts the ref is genuinely concrete
// (not abstract, not already GC-ref), then asserts expandGCRefType turns it
// into a GC-ref.
TEST(GC, ExpandGCRefTypeNormalizesConcreteSlotRef) {
  Configure Conf = makeGCConf();
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(ConcreteGlobalTableWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  const auto *ModInst = VM.getActiveModule();
  ASSERT_NE(ModInst, nullptr);
  auto &Exec = VM.getExecutor();

  // ---- Concrete ref in an exported global slot (populated by struct.new) ----
  {
    // findGlobalExports is exactly what WasmEdge_ModuleInstanceFindGlobal (the
    // C-API path feeding the retained getter) uses to reach the slot.
    const auto *Glob = ModInst->findGlobalExports("g");
    ASSERT_NE(Glob, nullptr);
    const RefVariant SlotRef = Glob->getValue().get<RefVariant>();
    ASSERT_FALSE(SlotRef.isNull());

    // The slot ref is genuinely concrete: a type-index heap type, not one of
    // the abstract heap types. If this fails the fixture is wrong (the slot
    // was normalized on the way in).
    EXPECT_FALSE(SlotRef.getType().isAbsHeapType());
    // A concrete type-index ref is not by-value releasable:
    // isHostRetainedRefType() (structref/arrayref only) is what the getter keys
    // retention off of.
    EXPECT_FALSE(SlotRef.getType().isHostRetainedRefType());
    // The load-bearing normalization: concrete (ref $s) -> abstract structref,
    // which is a GC-ref, so the getter retains it (no UAF).
    EXPECT_TRUE(Exec.expandGCRefType(SlotRef).isHostRetainedRefType());
  }

  // ---- Concrete ref in an exported table slot (populated by struct.new) ----
  {
    const auto *Tab = ModInst->findTableExports("t");
    ASSERT_NE(Tab, nullptr);
    auto AddrRes = Tab->getRefAddr(0); // exactly what the getter reads
    ASSERT_TRUE(AddrRes);
    const RefVariant SlotRef = *AddrRes;
    ASSERT_FALSE(SlotRef.isNull());

    EXPECT_FALSE(SlotRef.getType().isAbsHeapType());
    EXPECT_FALSE(SlotRef.getType().isHostRetainedRefType());
    EXPECT_TRUE(Exec.expandGCRefType(SlotRef).isHostRetainedRefType());
  }
}

TEST(GCThread, ControllerOwnedTableRejectsRawCApiMutation) {
  // The direct C-API table mutators (WasmEdge_TableInstanceSetData /
  // WasmEdge_TableInstanceGrow) refuse a raw mutation on a table a GC
  // controller manages -- the C-API cannot prove the caller's controller
  // matches the owner. The gate is TableInstance::isManagedByController(); the
  // C-API wrappers do exactly `if (isManagedByController()) reject; else
  // mutate`. The GC suite cannot link libwasmedge, so the gate decision and the
  // still-open paths are exercised here directly against real TableInstances.
  // Deterministic and single-threaded.
  GC::Controller Ctrl;
  GC::Allocator &Alloc = Ctrl.getAllocator();
  const RefVariant Null(ValType(TypeCode::RefNull, TypeCode::NullFuncRef));

  // (a) Managed table attached to a controller-backed allocator: a raw C-API
  // setData/grow is rejected (provenance unprovable).
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::StructRef, 1, 4), /*CanHoldManagedIn=*/true);
    ASSERT_TRUE(Table.setAllocator(Alloc));
    EXPECT_TRUE(Table.isManagedByController())
        << "managed + controller-attached -> raw C-API mutation rejected";
  }

  // (b) Non-managed table attached to the same controller-backed allocator: not
  // managed, so the raw C-API path stays open (this is the existing
  // instantiated externref-table C-API behavior, kept working).
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::FuncRef, 2, 2), /*CanHoldManagedIn=*/false);
    ASSERT_TRUE(Table.setAllocator(Alloc));
    EXPECT_FALSE(Table.isManagedByController())
        << "attached but non-managed -> C-API mutation still allowed";
    EXPECT_TRUE(Table.setRefAddr(0, Null)); // allowed path really mutates
  }

  // (b2) externref table attached to the same controller: canHoldManaged() is
  // true (extern.convert_any can wrap a GC object), but externref is the host-
  // facing reference primitive -- the standard C-API workflow stores host
  // references through it, so the gate must not fire and it stays mutable.
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::ExternRef, 1, 4), /*CanHoldManagedIn=*/true);
    ASSERT_TRUE(Table.setAllocator(Alloc));
    EXPECT_FALSE(Table.isManagedByController())
        << "externref table stays C-API-mutable despite canHoldManaged()";
    EXPECT_TRUE(Table.setRefAddr(0, Null));
  }

  // (c) Standalone, unattached table (as WasmEdge_TableInstanceCreate builds,
  // even a managed one): no controller owns it, so it mutates freely as before.
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::StructRef, 1, 4), /*CanHoldManagedIn=*/true);
    EXPECT_FALSE(Table.isManagedByController())
        << "standalone table -> C-API mutation allowed";
    EXPECT_TRUE(Table.growTable(1)); // allowed path really mutates
  }
}

TEST(GCThread, LegacyManagedRefGettersReturnSentinel) {
  // WasmEdge_GlobalInstanceGetValue / WasmEdge_TableInstanceGetData restrict a
  // managed-ref slot's legacy borrowed getter to a typed-null sentinel plus a
  // warning, instead of handing out a live reference that is not in HostRoots
  // and could be reclaimed while the host still holds it. The gate is
  // `CanHoldManaged() && !RefType.isExternRefType()`
  // (isRestrictedManagedRefGetterType, shared with wasmedge.cpp via
  // include/api/internal/managed_ref_getter.h) and the sentinel is
  // `RefVariant(RefType)`. externref is excluded: it is the host-facing
  // reference primitive whose host-pointer round trip must keep working.
  //
  // The GC suite cannot link libwasmedge, so this test calls the real gate
  // predicate against real GlobalInstance/TableInstance objects rather than the
  // exported C symbols -- a mis-wiring in wasmedge.cpp would fail here too. It
  // also confirms the underlying getValue()/getRefAddr() is unchanged and still
  // returns the live borrowed value, and that the sentinel is null and distinct
  // from it. APIUnitTest.cpp drives the exported symbols for the non-managed
  // paths the C-API can construct. Deterministic and single-threaded.

  // A live (non-null) i31 ref: i31 needs no heap allocation (RefI31Op simply
  // tags a small integer as a pointer -- see Executor::runRefI31Op), so it
  // exercises the "managed ref" path without a GC allocator/controller.
  const ValType I31Ty(TypeCode::I31Ref);
  const RefVariant LiveI31(
      I31Ty, reinterpret_cast<void *>(static_cast<uintptr_t>(0x80000001U)));
  ASSERT_FALSE(LiveI31.isNull());
  // Arbitrary non-null host address for the externref cases below (must be a
  // mutable object -- RefVariant's pointer-typed constructor forwards it
  // as-is, so a `const` source would not bind to `void *`).
  int HostDummy = 0;
  void *const HostPtr = &HostDummy;

  // (a) Managed-ref global (i31ref, CanHoldManaged=true, not externref): the
  // gate fires -- WasmEdge_GlobalInstanceGetValue must substitute the
  // sentinel instead of this live value.
  {
    Runtime::Instance::GlobalInstance Glob(AST::GlobalType(I31Ty, ValMut::Var),
                                           /*CanHoldManagedIn=*/true,
                                           ValVariant(LiveI31));
    EXPECT_TRUE(Glob.canHoldManaged());
    EXPECT_FALSE(Glob.getGlobalType().getValType().isExternRefType());
    // Assert against the real shared predicate (the exact function
    // lib/api/wasmedge.cpp calls), not a hand-rederived copy of the boolean.
    EXPECT_TRUE(isRestrictedManagedRefGetterType(
        Glob.canHoldManaged(), Glob.getGlobalType().getValType()))
        << "gate condition: canHoldManaged() && !isExternRefType() -> "
           "restricted";
    // getValue() itself is left untouched and still returns the live,
    // non-null borrowed ref -- exactly what the restricted C-API getter must
    // not hand back.
    EXPECT_FALSE(Glob.getValue().get<RefVariant>().isNull());
    const RefVariant Sentinel(Glob.getGlobalType().getValType());
    EXPECT_TRUE(Sentinel.isNull())
        << "the documented sentinel is a null ref of the slot's type";
    EXPECT_NE(Glob.getValue().get<RefVariant>().getPtr<void>(),
              Sentinel.getPtr<void>())
        << "the restricted getter's sentinel must differ from the live "
           "borrowed value it replaces";
  }

  // (b) funcref global (CanHoldManaged=false): the gate does not fire --
  // unaffected, returns the value as before.
  {
    Runtime::Instance::GlobalInstance Glob(
        AST::GlobalType(ValType(TypeCode::FuncRef), ValMut::Var),
        /*CanHoldManagedIn=*/false);
    EXPECT_FALSE(Glob.canHoldManaged());
    EXPECT_FALSE(isRestrictedManagedRefGetterType(
        Glob.canHoldManaged(), Glob.getGlobalType().getValType()))
        << "gate condition false -> funcref getter unaffected";
  }

  // (c) externref global: canHoldManaged() is true (extern.convert_any can
  // wrap a GC object), but the scoping decision excludes externref from the
  // sentinel restriction so the standard host-pointer round trip keeps
  // working -- the gate must not fire despite CanHoldManaged().
  {
    Runtime::Instance::GlobalInstance Glob(
        AST::GlobalType(ValType(TypeCode::ExternRef), ValMut::Var),
        /*CanHoldManagedIn=*/true, ValVariant(RefVariant(HostPtr)));
    EXPECT_TRUE(Glob.canHoldManaged());
    EXPECT_TRUE(Glob.getGlobalType().getValType().isExternRefType());
    EXPECT_FALSE(isRestrictedManagedRefGetterType(
        Glob.canHoldManaged(), Glob.getGlobalType().getValType()))
        << "gate condition: canHoldManaged() && !isExternRefType() -> false "
           "-> externref getter unaffected (externref exclusion)";
    EXPECT_EQ(Glob.getValue().get<RefVariant>().getPtr<void>(), HostPtr)
        << "externref getter must still return the live host pointer, not "
           "a sentinel";
  }

  // (d) i32 (non-ref) global: never managed, unaffected.
  {
    Runtime::Instance::GlobalInstance Glob(
        AST::GlobalType(ValType(TypeCode::I32), ValMut::Var),
        /*CanHoldManagedIn=*/false, ValVariant(UINT32_C(42)));
    EXPECT_FALSE(Glob.canHoldManaged());
    EXPECT_FALSE(isRestrictedManagedRefGetterType(
        Glob.canHoldManaged(), Glob.getGlobalType().getValType()));
    EXPECT_EQ(Glob.getValue().get<uint32_t>(), 42U);
  }

  // (e) Managed-ref table (i31ref element, CanHoldManaged=true): same gate,
  // mirrored for WasmEdge_TableInstanceGetData / TableInstance::getRefAddr.
  {
    Runtime::Instance::TableInstance Table(AST::TableType(I31Ty, 1, 1),
                                           /*CanHoldManagedIn=*/true);
    ASSERT_TRUE(Table.setRefAddr(0, LiveI31));
    EXPECT_TRUE(Table.canHoldManaged());
    EXPECT_FALSE(Table.getTableType().getRefType().isExternRefType());
    EXPECT_TRUE(isRestrictedManagedRefGetterType(
        Table.canHoldManaged(), Table.getTableType().getRefType()))
        << "gate condition: canHoldManaged() && !isExternRefType() -> "
           "restricted";
    auto Res = Table.getRefAddr(0);
    ASSERT_TRUE(Res);
    EXPECT_FALSE(Res->isNull())
        << "getRefAddr() itself is unchanged and still returns the live "
           "borrowed ref";
    const RefVariant Sentinel(Table.getTableType().getRefType());
    EXPECT_TRUE(Sentinel.isNull());
    EXPECT_NE(Res->getPtr<void>(), Sentinel.getPtr<void>());
  }

  // (f) externref table: canHoldManaged() true, but excluded -- unaffected
  // (this is the shipped tab-ext workflow exercised in APIUnitTest.cpp's
  // WasmEdge_TableInstanceSetData/GetData externref round trip).
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::ExternRef, 1, 1), /*CanHoldManagedIn=*/true);
    ASSERT_TRUE(Table.setRefAddr(0, RefVariant(HostPtr)));
    EXPECT_TRUE(Table.canHoldManaged());
    EXPECT_TRUE(Table.getTableType().getRefType().isExternRefType());
    EXPECT_FALSE(isRestrictedManagedRefGetterType(
        Table.canHoldManaged(), Table.getTableType().getRefType()))
        << "gate condition false -> externref table getter unaffected";
    auto Res = Table.getRefAddr(0);
    ASSERT_TRUE(Res);
    EXPECT_EQ(Res->getPtr<void>(), HostPtr);
  }

  // (g) funcref table: never managed, unaffected.
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(TypeCode::FuncRef, 1, 1), /*CanHoldManagedIn=*/false);
    EXPECT_FALSE(Table.canHoldManaged());
    EXPECT_FALSE(isRestrictedManagedRefGetterType(
        Table.canHoldManaged(), Table.getTableType().getRefType()));
  }

  // (h) non-nullable managed-ref global (i31ref, not nullable,
  // CanHoldManaged=true): the gate still fires -- this documents that the
  // typed-null sentinel is a deliberate choice for this case, not an
  // oversight. A non-nullable managed-ref slot is a real, constructible
  // instance (GlobalInstance/TableInstance require
  // isNullableRefType() || !Val.isNull() at construction, so it must be
  // seeded with a live, non-null value), yet there is no valid *non-null*
  // sentinel we could synthesize -- we cannot fabricate a live managed
  // object out of thin air. Typed-null is therefore the only representable
  // sentinel even though it is not a valid value of the slot's own
  // (non-nullable) type; callers must treat it purely as an
  // error-indicator, never round-trip it back into a constructor. See
  // genManagedRefGetterSentinel in lib/api/wasmedge.cpp.
  {
    const ValType NonNullI31Ty(TypeCode::Ref, TypeCode::I31Ref);
    Runtime::Instance::GlobalInstance Glob(
        AST::GlobalType(NonNullI31Ty, ValMut::Var), /*CanHoldManagedIn=*/true,
        ValVariant(LiveI31));
    EXPECT_TRUE(Glob.canHoldManaged());
    EXPECT_FALSE(Glob.getGlobalType().getValType().isNullableRefType());
    EXPECT_TRUE(isRestrictedManagedRefGetterType(
        Glob.canHoldManaged(), Glob.getGlobalType().getValType()))
        << "the gate fires for a non-nullable managed slot exactly as it "
           "does for a nullable one -- the sentinel is a documented error "
           "indicator, not intended to be a round-trippable non-null value";
  }
}

TEST(GCThread, RetainedManagedRefGetterSurvivesCollection) {
  // The producer-bearing retained getters
  // (WasmEdge_GlobalInstanceGetValueRetained /
  // WasmEdge_TableInstanceGetDataRetained, lib/api/wasmedge.cpp) are the safe
  // alternative to the legacy borrowed getters: they root a managed
  // struct/array reference handed back to the host by pinning it through
  // Executor::getAllocator().retainResult(ref) -- the exact call
  // Executor::invoke makes for a returned GC ref -- so a concurrent collection
  // cannot reclaim it until the host releases it (WasmEdge_ExecutorReleaseRef
  // -> Executor::releaseRef -> Allocator::releaseRef).
  //
  // The GC suite cannot link libwasmedge (see
  // GCThread.ControllerOwnedTableRejectsRawCApiMutation), so it cannot call
  // the exported symbols, and the C-API has no public force-collect. So this
  // test proves the survives-a-real-collection property the getter relies on
  // by driving the exact underlying calls the getter makes: read the live ref
  // from a real attached GlobalInstance / TableInstance slot
  // (getValue / getRefAddr), retainResult it, drop the slot's own reference,
  // then force a full collection -- and assert the object survives purely
  // because of the retention, then becomes collectable after releaseRef.
  // test/api/APIUnitTest.cpp
  // (APICoreTest.RetainedManagedRefGettersRootAndRelease) separately drives
  // the exported C symbols for foreign rejection, the retain->release round
  // trip, and non-managed passthrough. Documented split, dictated by what each
  // suite can construct. Deterministic and single-threaded.
  using RawData = Runtime::Instance::GCInstance::RawData;
  Configure Conf = makeGCConf();
  WasmEdge::Executor::Executor Exec(Conf);
  // Executor::getAllocator() forwards to its controller's allocator -- the
  // same object the retained getters reach via
  // fromExecutorCxt(Cxt)->getAllocator().
  GC::Allocator &Alloc = Exec.getAllocator();
  Alloc.setManualGC(true); // only explicit manualCollect() runs (no flaky auto)

  // Allocate one zero-child GC struct directly through the allocator (Length 0
  // so the collector's child-scan reads no garbage). Mirrors
  // GC.AllocatorHostRootsRetainRelease.
  auto AllocStruct = [&]() noexcept -> RefVariant {
    void *P = Alloc.allocate(
        [](void *Ptr) noexcept {
          auto *Raw = static_cast<RawData *>(Ptr);
          Raw->ModInst = nullptr;
          Raw->TypeIdx = 0;
          Raw->Length = 0;
        },
        sizeof(RawData));
    return RefVariant(ValType(TypeCode::Ref, TypeCode::StructRef),
                      static_cast<RawData *>(P));
  };
  // A null struct ref to overwrite the slot with (the "drop the source root"
  // step). setValue/setRefAddr's write barrier is a no-op while the collector
  // is Idle (Allocator::writeBarrier), which it always is here between the
  // synchronous manualCollect() calls -- so overwriting does not shade the old
  // struct and cannot mask the retention we are testing.
  const RefVariant NullStruct(ValType(TypeCode::RefNull, TypeCode::StructRef));

  // ---- Managed global slot ----
  {
    Runtime::Instance::GlobalInstance Glob(
        AST::GlobalType(ValType(TypeCode::RefNull, TypeCode::StructRef),
                        ValMut::Var),
        /*CanHoldManagedIn=*/true, ValVariant(AllocStruct()));
    ASSERT_TRUE(Glob.setAllocator(Alloc)); // attach: the slot is now a GC root

    // Read the live ref exactly as the getter does, then retain it exactly as
    // the getter does.
    const RefVariant Ref = Glob.getValue().get<RefVariant>();
    ASSERT_FALSE(Ref.isNull());
    Alloc.retainResult(Ref);

    // Clear the newborn grace period: a just-allocated object survives its very
    // first collect regardless of rootedness (see
    // GC.AllocatorHostRootsRetainRelease). It is still rooted by both the
    // global slot and the retention here.
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_GT(Alloc.getMemoryUsage(), 0u);

    // Drop the slot's reference: the retention is now the only root.
    Glob.setValue(ValVariant(NullStruct));
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_GT(Alloc.getMemoryUsage(), 0u)
        << "the retained managed ref must survive a collection after the "
           "global slot that produced it is cleared";

    // Release the retention -> unrooted -> reclaimed on the next collection.
    Alloc.releaseRef(Ref);
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_EQ(Alloc.getMemoryUsage(), 0u)
        << "after the release path's Allocator::releaseRef, the ref is "
           "collectable";
  }

  // ---- Managed table slot ----
  {
    Runtime::Instance::TableInstance Table(
        AST::TableType(ValType(TypeCode::RefNull, TypeCode::StructRef), 1, 1),
        /*CanHoldManagedIn=*/true);
    ASSERT_TRUE(Table.setAllocator(Alloc));
    ASSERT_TRUE(Table.setRefAddr(0, AllocStruct()));

    auto Res = Table.getRefAddr(0); // exactly what the getter reads
    ASSERT_TRUE(Res);
    const RefVariant Ref = *Res;
    ASSERT_FALSE(Ref.isNull());
    Alloc.retainResult(Ref);

    EXPECT_TRUE(Alloc.manualCollect()); // clear the newborn grace period
    EXPECT_GT(Alloc.getMemoryUsage(), 0u);

    ASSERT_TRUE(Table.setRefAddr(0, NullStruct)); // drop the slot's root
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_GT(Alloc.getMemoryUsage(), 0u)
        << "the retained managed table ref must survive a collection after "
           "the table slot that produced it is cleared";

    Alloc.releaseRef(Ref);
    EXPECT_TRUE(Alloc.manualCollect());
    EXPECT_EQ(Alloc.getMemoryUsage(), 0u);
  }
}

// The retained getters read the slot, expand the reference's type (a payload
// dereference) and only then retain it. The calling host thread is not a
// registered mutator, so nothing parks it: a guest clearing the slot and
// forcing a full collection in that window would free the object under the
// getter, which then roots a dangling pointer. The shared helper must hold the
// controller's exclusive token across read -> expand -> retain so no cycle can
// start (or be in flight) inside the window. Driven through the exact helper
// the C-API calls (retainManagedRefFromSlot, include/api/internal/
// managed_ref_getter.h) with a slot read that pauses inside the window while
// another thread clears the slot and requests a collection.
TEST(GCThread, RetainedGetterPinsAcrossCollectionInsideReadWindow) {
  using RawData = Runtime::Instance::GCInstance::RawData;
  Configure Conf = makeGCConf();
  WasmEdge::Executor::Executor Exec(Conf);
  GC::Allocator &Alloc = Exec.getAllocator();
  GC::Controller &Ctrl = Exec.getController();
  Alloc.setManualGC(true);

  void *P = Alloc.allocate(
      [](void *Ptr) noexcept {
        auto *Raw = static_cast<RawData *>(Ptr);
        Raw->ModInst = nullptr;
        Raw->TypeIdx = 0;
        Raw->Length = 0;
      },
      sizeof(RawData));
  ASSERT_NE(P, nullptr);
  const ValType StructTy(TypeCode::RefNull, TypeCode::StructRef);
  const RefVariant Live(StructTy, static_cast<RawData *>(P));
  Runtime::Instance::GlobalInstance Glob(AST::GlobalType(StructTy, ValMut::Var),
                                         /*CanHoldManagedIn=*/true,
                                         ValVariant(Live));
  ASSERT_TRUE(Glob.setAllocator(Alloc));
  // Clear the newborn grace period; the global slot keeps the object alive.
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_TRUE(Ctrl.collect(true, false));
  const uint64_t UsageWithObject = Alloc.getMemoryUsage();
  ASSERT_GT(UsageWithObject, 0u);

  std::atomic<bool> InWindow{false};
  std::atomic<bool> Go{false};
  std::atomic<bool> CollectedInWindow{false};
  Expect<std::pair<RefVariant, ValType>> Retained =
      Unexpect(ErrCode::Value::Interrupted);
  std::thread Host([&] {
    Retained =
        retainManagedRefFromSlot(Exec, StructTy, [&]() -> Expect<RefVariant> {
          const RefVariant R = Glob.getValue().get<RefVariant>();
          InWindow.store(true);
          while (!Go.load()) {
            std::this_thread::yield();
          }
          return R;
        });
  });
  while (!InWindow.load()) {
    std::this_thread::yield();
  }
  // The guest drops the slot's root and forces a collection while the host is
  // between its read and its retain. With the token held by the getter the
  // collection must be refused (collect() returns false); without the pin it
  // runs and frees the object the host is about to root.
  Glob.setValue(ValVariant(RefVariant(StructTy)));
  CollectedInWindow.store(Ctrl.collect(true, false));
  Go.store(true);
  Host.join();

  EXPECT_FALSE(CollectedInWindow.load())
      << "a collection must not run while a retained getter is inside its "
         "read -> retain window";
  ASSERT_TRUE(Retained);
  EXPECT_FALSE(Retained->first.isNull());
  // The retention is now the only root: the object must still be there.
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_EQ(Alloc.getMemoryUsage(), UsageWithObject)
      << "the object was freed under the getter and a dangling ref rooted";
  Alloc.releaseRef(Retained->first);
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_TRUE(Ctrl.collect(true, false));
  EXPECT_LT(Alloc.getMemoryUsage(), UsageWithObject);
}

TEST(GC, VMReleaseAllRefsArray) {
  Configure Conf = makeGCConf();
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  // autoCollect off: only explicit manualCollect() runs, else asserts flaky.
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  // Allocate two arrays (length 4) and keep both refs alive.
  auto R1 =
      VM.execute("make_arr", std::initializer_list<ValVariant>{UINT32_C(4)},
                 {ValType(TypeCode::I32)});
  auto R2 =
      VM.execute("make_arr", std::initializer_list<ValVariant>{UINT32_C(4)},
                 {ValType(TypeCode::I32)});
  ASSERT_TRUE(R1);
  ASSERT_TRUE(R2);
  EXPECT_GT(Alloc.getMemoryUsage(), Before);

  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_GT(Alloc.getMemoryUsage(), Before); // both retained survive

  VM.releaseAllRefs();
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_EQ(Alloc.getMemoryUsage(), Before); // both reclaimed
}

TEST(GC, VMReleaseRefsBatch) {
  Configure Conf = makeGCConf();
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  // autoCollect off: only explicit manualCollect() runs, else asserts flaky.
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  auto R1 = VM.execute("make", std::initializer_list<ValVariant>{UINT32_C(1)},
                       {ValType(TypeCode::I32)});
  auto R2 = VM.execute("make", std::initializer_list<ValVariant>{UINT32_C(2)},
                       {ValType(TypeCode::I32)});
  ASSERT_TRUE(R1);
  ASSERT_TRUE(R2);
  ASSERT_EQ(R1->size(), 1u);
  ASSERT_EQ(R2->size(), 1u);
  std::array<RefVariant, 2> Refs{(*R1)[0].first.get<RefVariant>(),
                                 (*R2)[0].first.get<RefVariant>()};

  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_GT(Alloc.getMemoryUsage(), Before);

  VM.releaseRefs(Refs);
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_EQ(Alloc.getMemoryUsage(), Before);
}

TEST(GC, NonGCResultNotRetained) {
  Configure Conf = makeGCConf();
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostRetentionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  // autoCollect off: only explicit manualCollect() runs, else asserts flaky.
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  // Returns an i31 (non-heap) after allocating and dropping a struct; neither
  // the i31 nor the dropped struct should be retained.
  auto Res = VM.execute("drop_return_i31");
  ASSERT_TRUE(Res);
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_EQ(Alloc.getMemoryUsage(),
            Before); // dropped struct reclaimed, i31 not retained
}

TEST(GC, ExternalizedRefRetained) {
  // An externalized gc struct (extern.convert_any) returned to the host must be
  // retained as a host root. It is typed externref but still points to a
  // collectible object; before the fix it escaped the struct/array retain check
  // because the type was folded to externref first.
  Configure Conf = makeGCConf();
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(MakeExtWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto &Alloc = VM.getExecutor().getAllocator();
  Alloc.setManualGC(true);
  const uint64_t Before = Alloc.getMemoryUsage();

  auto Res =
      VM.execute("make_ext", std::initializer_list<ValVariant>{UINT32_C(42)},
                 {ValType(TypeCode::I32)});
  ASSERT_TRUE(Res);
  EXPECT_GT(Alloc.getMemoryUsage(), Before); // externalized struct allocated

  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_GT(Alloc.getMemoryUsage(), Before); // retained across collections

  VM.releaseAllRefs();
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_EQ(Alloc.getMemoryUsage(), Before); // reclaimed once released
}

TEST(GC, ElemSegmentRoot) {
  // A passive element segment holds the only reference to a struct created in
  // its init expression; it must be scanned as a GC root or the struct is swept
  // and array.new_elem later reads a dangling pointer.
  Configure Conf = makeGCConf();
  // Host modules must outlive the VM that terminates them; declare first.
  auto GCMod = std::make_unique<Runtime::Instance::ModuleInstance>("gc");
  GCMod->addHostFunc("coll", std::make_unique<Collect>());
  auto CP = std::make_unique<Check>();
  auto *C = CP.get();
  GCMod->addHostFunc("check", std::move(CP));
  VM::VM VM(Conf);
  ASSERT_NO_FATAL_FAILURE(setUpManualGCVM(VM, *GCMod, ElemRootWasm));
  ASSERT_TRUE(VM.execute("test"));

  auto Values = C->getValues();
  ASSERT_EQ(Values.size(), 1u);
  // 42 == the struct field read back after two collections; garbage/crash if
  // the element segment was not scanned and the struct was reclaimed.
  EXPECT_EQ(Values[0], 42u);
}

TEST(GC, ExceptionPayloadRoot) {
  // A struct reference captured in an exception payload survives only via the
  // ExceptionInstance once the on-stack copies are consumed; the payload must
  // be scanned as a GC root (throw_ref re-pushes it). Construct the instance
  // directly to isolate the root-scanning behavior.
  using RawData = Runtime::Instance::GCInstance::RawData;
  GC::Allocator Alloc;
  void *P = Alloc.allocate(
      [](void *Ptr) noexcept {
        auto *Raw = static_cast<RawData *>(Ptr);
        Raw->ModInst = nullptr;
        Raw->TypeIdx = 0;
        Raw->Length = 0;
      },
      sizeof(RawData));
  ASSERT_NE(P, nullptr);
  RefVariant Ref(ValType(TypeCode::Ref, TypeCode::StructRef),
                 static_cast<RawData *>(P));

  AST::TagType DummyTag;
  Runtime::Instance::TagInstance Tag(DummyTag, nullptr);
  std::vector<ValVariant> Payload;
  Payload.emplace_back(Ref);
  Runtime::Instance::ExceptionInstance Exc(&Tag, std::move(Payload));
  Exc.setAllocator(Alloc);

  EXPECT_TRUE(Alloc.manualCollect()); // payload scanned -> struct survives
  EXPECT_GT(Alloc.getMemoryUsage(), 0u);
  EXPECT_TRUE(Alloc.manualCollect());
  EXPECT_GT(Alloc.getMemoryUsage(), 0u);
}

} // namespace
