// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "DebugInfoTestUtils.h"

namespace {

using namespace WasmEdge;
using namespace WasmEdge::DebugInfoTest;
using namespace std::literals;

struct TypeFixture {
  Configure Conf;
  std::unique_ptr<AST::Module> Mod;
  std::unique_ptr<LLVM::DebugInfo::DwarfReader> Reader;
  llvm::LLVMContext Ctx;
  std::unique_ptr<llvm::Module> M;
  std::unique_ptr<LLVM::DebugInfo::ModuleDebugInfo> DI;
  bool load(std::string_view Name, std::string_view Triple = {}) {
    return adopt(loadFixture(Conf, Name), Triple);
  }
  bool adopt(std::unique_ptr<AST::Module> Module,
             std::string_view Triple = {}) {
    Mod = std::move(Module);
    if (!Mod) {
      return false;
    }
    Reader = LLVM::DebugInfo::DwarfReader::create(*Mod);
    M = std::make_unique<llvm::Module>("t", Ctx);
    if (!Triple.empty()) {
#if LLVM_VERSION_MAJOR < 21
      M->setTargetTriple(llvm::StringRef(Triple.data(), Triple.size()));
#else
      M->setTargetTriple(
          llvm::Triple(llvm::StringRef(Triple.data(), Triple.size())));
#endif
    }
    DI = LLVM::DebugInfo::ModuleDebugInfo::create(
        *Mod, llvm::wrap(M.get()), LLVM::DebugInfo::ModuleDebugInfo::Part::All);
    return Reader && DI;
  }
};

TEST(DebugInfoTypes, RecursiveStructUsesWasmPointer) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_basic_O0.wasm"sv));
  auto NodeDie = findType(F.Reader->getContext(),
                          llvm::dwarf::DW_TAG_structure_type, "Node"sv);
  ASSERT_TRUE(NodeDie);
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(NodeDie.getOffset())));
  ASSERT_EQ(T->getElements().size(), 2U);
  auto *Next = llvm::cast<llvm::DIDerivedType>(T->getElements()[1]);
  EXPECT_EQ(Next->getName(), "Next");
  EXPECT_EQ(Next->getOffsetInBits(), 32U);
  auto *Ptr = llvm::cast<llvm::DICompositeType>(Next->getBaseType());
  EXPECT_EQ(Ptr->getName(), "__wasm_ptr<Node>");
  EXPECT_EQ(Ptr->getSizeInBits(), 32U);
  auto *Pointee = llvm::cast<llvm::DIDerivedType>(Ptr->getElements()[1]);
  auto *Arr = llvm::cast<llvm::DICompositeType>(Pointee->getBaseType());
  auto *Elem = llvm::cast<llvm::DIDerivedType>(Arr->getBaseType());
  EXPECT_EQ(Elem->getTag(), llvm::dwarf::DW_TAG_pointer_type);
  EXPECT_EQ(Elem->getBaseType(), T);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, RustEnumHasVariantPart) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_enum_O0.wasm"sv));
  auto ShapeDie = findType(F.Reader->getContext(),
                           llvm::dwarf::DW_TAG_structure_type, "Shape"sv);
  ASSERT_TRUE(ShapeDie);
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(ShapeDie.getOffset())));
  bool HasVariantPart = false;
  for (auto *E : T->getElements()) {
    if (auto *C = llvm::dyn_cast<llvm::DICompositeType>(E)) {
      HasVariantPart |= C->getTag() == llvm::dwarf::DW_TAG_variant_part;
    }
  }
  EXPECT_TRUE(HasVariantPart);
  auto *NS = llvm::dyn_cast_or_null<llvm::DINamespace>(T->getScope());
  ASSERT_NE(NS, nullptr);
  EXPECT_EQ(NS->getName(), "dwarf_enum");
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, NestedTypeFirstKeepsEnclosingMembers) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_nested_O0.wasm"sv));
  auto KindDie = findType(F.Reader->getContext(),
                          llvm::dwarf::DW_TAG_enumeration_type, "Kind"sv);
  auto OuterDie = findType(F.Reader->getContext(),
                           llvm::dwarf::DW_TAG_structure_type, "Outer"sv);
  ASSERT_TRUE(KindDie);
  ASSERT_TRUE(OuterDie);
  auto *Kind = llvm::dyn_cast_or_null<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(KindDie.getOffset())));
  ASSERT_NE(Kind, nullptr);
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(OuterDie.getOffset())));
  EXPECT_EQ(Kind->getScope(), T);
  ASSERT_EQ(T->getElements().size(), 4U);
  auto *K = llvm::cast<llvm::DIDerivedType>(T->getElements()[0]);
  EXPECT_EQ(K->getName(), "K");
  EXPECT_EQ(K->getBaseType(), Kind);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, Dwarf2MemberLayout) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_nested_O0.wasm"sv));
  auto OuterDie = findType(F.Reader->getContext(),
                           llvm::dwarf::DW_TAG_structure_type, "Outer"sv);
  ASSERT_TRUE(OuterDie);
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(OuterDie.getOffset())));
  ASSERT_EQ(T->getElements().size(), 4U);
  auto *Low = llvm::cast<llvm::DIDerivedType>(T->getElements()[1]);
  auto *High = llvm::cast<llvm::DIDerivedType>(T->getElements()[2]);
  auto *Tail = llvm::cast<llvm::DIDerivedType>(T->getElements()[3]);
  EXPECT_EQ(Low->getName(), "Low");
  EXPECT_EQ(Low->getOffsetInBits(), 32U);
  EXPECT_EQ(Low->getSizeInBits(), 3U);
  EXPECT_EQ(High->getName(), "High");
  EXPECT_EQ(High->getOffsetInBits(), 35U);
  EXPECT_EQ(High->getSizeInBits(), 5U);
  EXPECT_EQ(Tail->getName(), "Tail");
  EXPECT_EQ(Tail->getOffsetInBits(), 64U);
  EXPECT_EQ(Tail->getSizeInBits(), 32U);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, BitFieldBeforeTheParentIsDropped) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_nested_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  uint64_t Code = 0;
  uint64_t ValueAt = 0;
  {
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    auto Outer = findType(R->getContext(), llvm::dwarf::DW_TAG_structure_type,
                          "Outer"sv);
    ASSERT_TRUE(Outer);
    for (auto C : Outer.children()) {
      const char *Name = C.getName(llvm::DINameKind::ShortName);
      if (!Name || "Low"sv != Name) {
        continue;
      }
      for (const auto &A : C.attributes()) {
        if (A.Attr == llvm::dwarf::DW_AT_bit_offset &&
            A.Value.getForm() == llvm::dwarf::DW_FORM_data1) {
          Code = C.getAbbreviationDeclarationPtr()->getCode();
          ValueAt = A.Offset;
        }
      }
    }
  }
  ASSERT_NE(Code, 0U);
  for (auto &Custom : Mod->getCustomSections()) {
    if (Custom.getName() == ".debug_info"sv) {
      Custom.getContent()[ValueAt] = 96;
    }
  }
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  llvm::LLVMContext Ctx;
  llvm::Module M("t", Ctx);
  auto DI = LLVM::DebugInfo::ModuleDebugInfo::create(
      *Mod, llvm::wrap(&M), LLVM::DebugInfo::ModuleDebugInfo::Part::All);
  ASSERT_NE(DI, nullptr);
  auto OuterDie =
      findType(R->getContext(), llvm::dwarf::DW_TAG_structure_type, "Outer"sv);
  ASSERT_TRUE(OuterDie);
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(DI->translateType(OuterDie.getOffset())));
  bool HasHigh = false;
  for (auto *E : T->getElements()) {
    const auto Name = llvm::cast<llvm::DIDerivedType>(E)->getName();
    EXPECT_NE(Name, "Low");
    HasHigh |= Name == "High";
  }
  EXPECT_TRUE(HasHigh);
  DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(M, &llvm::errs()));
}

TEST(DebugInfoTypes, NamespacedStaticGetsGlobalVariable) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_enum_O0.wasm"sv));
  auto *MB = F.M->getGlobalVariable("__wasmedge_debug_membase");
  ASSERT_NE(MB, nullptr);
  llvm::SmallVector<llvm::DIGlobalVariableExpression *, 4> GVEs;
  MB->getDebugInfo(GVEs);
  const llvm::DIGlobalVariable *Calls = nullptr;
  for (auto *GVE : GVEs) {
    if (GVE->getVariable()->getName() == "CALLS") {
      Calls = GVE->getVariable();
    }
  }
  ASSERT_NE(Calls, nullptr);
  auto *NS = llvm::dyn_cast_or_null<llvm::DINamespace>(Calls->getScope());
  ASSERT_NE(NS, nullptr);
  EXPECT_EQ(NS->getName(), "dwarf_enum");
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, RustEnumKeepsSignedValues) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_enum_O0.wasm"sv));
  auto &Ctx = F.Reader->getContext();
  auto SignDie = findType(Ctx, llvm::dwarf::DW_TAG_enumeration_type, "Sign"sv);
  ASSERT_TRUE(SignDie);
  auto *Sign = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(SignDie.getOffset())));
  std::map<std::string, int64_t> Enumerators;
  for (auto *E : Sign->getElements()) {
    auto *En = llvm::cast<llvm::DIEnumerator>(E);
    EXPECT_FALSE(En->isUnsigned());
    Enumerators[En->getName().str()] = En->getValue().getSExtValue();
  }
  EXPECT_EQ(Enumerators["Minus"], -2);
  EXPECT_EQ(Enumerators["Plus"], 1);

  auto TaggedDie =
      findType(Ctx, llvm::dwarf::DW_TAG_structure_type, "Tagged"sv);
  ASSERT_TRUE(TaggedDie);
  auto *Tagged = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(TaggedDie.getOffset())));
  std::map<std::string, int64_t> Discriminants;
  for (auto *E : Tagged->getElements()) {
    auto *Part = llvm::dyn_cast<llvm::DICompositeType>(E);
    if (!Part || Part->getTag() != llvm::dwarf::DW_TAG_variant_part) {
      continue;
    }
    for (auto *V : Part->getElements()) {
      auto *Member = llvm::cast<llvm::DIDerivedType>(V);
      if (auto *C = llvm::dyn_cast_or_null<llvm::ConstantInt>(
              Member->getDiscriminantValue())) {
        Discriminants[Member->getName().str()] = C->getSExtValue();
      }
    }
  }
  EXPECT_EQ(Discriminants["Low"], -3);
  EXPECT_EQ(Discriminants["High"], 5);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, PackedBitFieldCrossesItsStorageUnit) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_nested_O0.wasm"sv));
  auto PackedDie = findType(F.Reader->getContext(),
                            llvm::dwarf::DW_TAG_structure_type, "Packed"sv);
  ASSERT_TRUE(PackedDie);
  bool Negative = false;
  for (auto C : PackedDie.children()) {
    if (auto V = C.find(llvm::dwarf::DW_AT_bit_offset)) {
      const auto Signed = V->getAsSignedConstant();
      Negative |= Signed && *Signed < 0;
    }
  }
  ASSERT_TRUE(Negative);
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(PackedDie.getOffset())));
  const llvm::DIDerivedType *Cross = nullptr;
  for (auto *E : T->getElements()) {
    auto *M = llvm::cast<llvm::DIDerivedType>(E);
    if (M->getName() == "Cross") {
      Cross = M;
    }
  }
  ASSERT_NE(Cross, nullptr);
  EXPECT_EQ(Cross->getOffsetInBits(), 8U);
  EXPECT_EQ(Cross->getSizeInBits(), 30U);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, VirtualBaseIsOmitted) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_nested_O0.wasm"sv));
  auto translate = [&](std::string_view Name) {
    auto Die = findType(F.Reader->getContext(),
                        llvm::dwarf::DW_TAG_structure_type, Name);
    EXPECT_TRUE(Die);
    return llvm::cast<llvm::DICompositeType>(
        llvm::unwrap(F.DI->translateType(Die.getOffset())));
  };
  auto *Plain = translate("Plain"sv);
  ASSERT_EQ(Plain->getElements().size(), 2U);
  auto *Inherit = llvm::cast<llvm::DIDerivedType>(Plain->getElements()[0]);
  EXPECT_EQ(Inherit->getTag(), llvm::dwarf::DW_TAG_inheritance);
  EXPECT_EQ(Inherit->getOffsetInBits(), 0U);
  auto *Shared = translate("Shared"sv);
  for (const auto *E : Shared->getElements()) {
    auto *Member = llvm::cast<llvm::DIDerivedType>(E);
    EXPECT_NE(Member->getTag(), llvm::dwarf::DW_TAG_inheritance);
    if (Member->getName() == "Extra") {
      EXPECT_EQ(Member->getOffsetInBits(), 32U);
    }
  }
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, FunctionPointerThroughTypedefIsFuncRef) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_data_O0.wasm"sv));
  for (auto Name : {"Plain"sv, "Aliased"sv}) {
    llvm::DWARFDie Var;
    for (const auto &CU : F.Reader->getContext().compile_units()) {
      for (const auto &Entry : CU->dies()) {
        llvm::DWARFDie D(CU.get(), &Entry);
        const char *N = D.getName(llvm::DINameKind::ShortName);
        if (D.getTag() == llvm::dwarf::DW_TAG_variable && N && Name == N) {
          Var = D;
        }
      }
    }
    ASSERT_TRUE(Var) << Name;
    auto Type = Var.getAttributeValueAsReferencedDie(llvm::dwarf::DW_AT_type);
    ASSERT_TRUE(Type) << Name;
    auto *T = llvm::dyn_cast_or_null<llvm::DICompositeType>(
        llvm::unwrap(F.DI->translateType(Type.getOffset())));
    ASSERT_NE(T, nullptr) << Name;
    EXPECT_EQ(T->getName(), "__wasm_funcref") << Name;
  }
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, TypeUnitsKeepTheirOwnTypes) {
  namespace dwarf = llvm::dwarf;
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_types_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  llvm::DWARFDie Var;
  for (const auto &CU : R->getContext().compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie D(CU.get(), &Entry);
      const char *N = D.getName(llvm::DINameKind::ShortName);
      if (D.getTag() == dwarf::DW_TAG_variable && N && "P"sv == N) {
        Var = D;
      }
    }
  }
  ASSERT_TRUE(Var);
  const auto Decl = Var.getAttributeValueAsReferencedDie(dwarf::DW_AT_type);
  ASSERT_TRUE(Decl.find(dwarf::DW_AT_signature));
  const auto Pair = LLVM::DebugInfo::ref(Var);
  if (!Pair || Pair == Decl) {
    GTEST_SKIP() << "this LLVM cannot follow DW_AT_signature";
  }
  ASSERT_TRUE(Pair.getDwarfUnit()->isTypeUnit());
  const auto Point = LLVM::DebugInfo::ref(Pair.getFirstChild());
  ASSERT_TRUE(Point);
  const auto Int = LLVM::DebugInfo::ref(Point.getFirstChild());
  ASSERT_TRUE(Int);
  ASSERT_EQ(Int.getTag(), dwarf::DW_TAG_base_type);
  ASSERT_EQ(Int.getOffset(), Decl.getOffset());

  for (const bool TypeUnitFirst : {true, false}) {
    llvm::LLVMContext Ctx;
    llvm::Module M("t", Ctx);
    LLVM::DebugInfo::UnitState S;
    S.DB = std::make_unique<llvm::DIBuilder>(M);
    S.CU = S.DB->createCompileUnit(
#if LLVM_VERSION_MAJOR < 22
        dwarf::DW_LANG_C_plus_plus,
#else
        llvm::DISourceLanguageName(dwarf::DW_LANG_C_plus_plus),
#endif
        S.DB->createFile("t.cpp", ""), "test", false, "", 0);
    LLVM::DebugInfo::TypeTranslator T(Ctx, 64);
    llvm::DIType *FromTypeUnit = nullptr;
    llvm::DIType *FromInfo = nullptr;
    if (TypeUnitFirst) {
      FromTypeUnit = T.type(S, Int);
      FromInfo = T.type(S, Decl);
    } else {
      FromInfo = T.type(S, Decl);
      FromTypeUnit = T.type(S, Int);
    }
    ASSERT_NE(FromTypeUnit, nullptr) << TypeUnitFirst;
    ASSERT_NE(FromInfo, nullptr) << TypeUnitFirst;
    EXPECT_EQ(FromTypeUnit->getName(), "int") << TypeUnitFirst;
    EXPECT_EQ(FromInfo->getName(), "Pair") << TypeUnitFirst;
    auto *Defined = llvm::dyn_cast_or_null<llvm::DICompositeType>(
        T.type(S, LLVM::DebugInfo::ref(Var)));
    ASSERT_NE(Defined, nullptr) << TypeUnitFirst;
    EXPECT_EQ(Defined->getName(), "Pair") << TypeUnitFirst;
    EXPECT_EQ(Defined->getElements().size(), 1U) << TypeUnitFirst;
    S.DB->finalize();
  }
}

TEST(DebugInfoTypes, NativePointersUseTheTargetWidth) {
  for (auto [Triple, Bits] : {std::pair{"i686-unknown-linux-gnu"sv, 32U},
                              std::pair{"x86_64-unknown-linux-gnu"sv, 64U}}) {
    TypeFixture F;
    ASSERT_TRUE(F.load("dwarf_basic_O0.wasm"sv, Triple));
    auto NodeDie = findType(F.Reader->getContext(),
                            llvm::dwarf::DW_TAG_structure_type, "Node"sv);
    ASSERT_TRUE(NodeDie);
    auto *T = llvm::cast<llvm::DICompositeType>(
        llvm::unwrap(F.DI->translateType(NodeDie.getOffset())));
    auto *Next = llvm::cast<llvm::DIDerivedType>(T->getElements()[1]);
    auto *Ptr = llvm::cast<llvm::DICompositeType>(Next->getBaseType());
    auto *Pointee = llvm::cast<llvm::DIDerivedType>(Ptr->getElements()[1]);
    auto *Array = llvm::cast<llvm::DICompositeType>(Pointee->getBaseType());
    EXPECT_EQ(Array->getBaseType()->getSizeInBits(), Bits) << Triple;
    auto *MB = F.M->getGlobalVariable("__wasmedge_debug_membase");
    ASSERT_NE(MB, nullptr);
    llvm::SmallVector<llvm::DIGlobalVariableExpression *, 4> GVEs;
    MB->getDebugInfo(GVEs);
    const auto It = std::find_if(GVEs.begin(), GVEs.end(), [](auto *GVE) {
      return GVE->getVariable()->getName() == "__wasmedge_debug_membase";
    });
    ASSERT_NE(It, GVEs.end());
    EXPECT_EQ((*It)->getVariable()->getType()->getSizeInBits(), Bits) << Triple;
  }
}

TEST(DebugInfoTypes, BigEndianTargetGetsOnlyLines) {
  for (const auto [Triple, Variables] :
       {std::pair{"x86_64-unknown-linux-gnu"sv, true},
        std::pair{"s390x-unknown-linux-gnu"sv, false}}) {
    TypeFixture F;
    ASSERT_TRUE(F.load("dwarf_basic_O0.wasm"sv, Triple));
    EXPECT_EQ(F.DI->getMemoryBaseGlobal() != nullptr, Variables) << Triple;
  }
}

TEST(DebugInfoTypes, StaticInAFunctionBlockIsAGlobal) {
  namespace dwarf = llvm::dwarf;
  TestDie Unit{
      dwarf::DW_TAG_compile_unit,
      {strAttr(dwarf::DW_AT_producer, "clang"),
       {dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_C11}},
      {{dwarf::DW_TAG_base_type,
        {strAttr(dwarf::DW_AT_name, "int"),
         {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_signed},
         {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 4}},
        {},
        1},
       {dwarf::DW_TAG_subprogram,
        {strAttr(dwarf::DW_AT_name, "count")},
        {{dwarf::DW_TAG_lexical_block,
          {},
          {{dwarf::DW_TAG_variable,
            {strAttr(dwarf::DW_AT_name, "Calls"),
             refAttr(dwarf::DW_AT_type, 1),
             {dwarf::DW_AT_location,
              dwarf::DW_FORM_exprloc,
              0,
              {dwarf::DW_OP_addr, 0x10, 0, 0, 0}}}}}}}}}};
  TypeFixture F;
  ASSERT_TRUE(F.adopt(moduleWithDwarf(F.Conf, writeDwarfUnit(4, Unit))));
  EXPECT_TRUE(F.Reader->hasGlobalVariables());
  EXPECT_NE(F.M->getGlobalVariable("__wasmedge_debug_membase"), nullptr);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, DiscardedGlobalKeepsItsName) {
  namespace dwarf = llvm::dwarf;
  TestDie Unit{
      dwarf::DW_TAG_compile_unit,
      {strAttr(dwarf::DW_AT_producer, "clang"),
       {dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_C11}},
      {{dwarf::DW_TAG_base_type,
        {strAttr(dwarf::DW_AT_name, "int"),
         {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_signed},
         {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 4}},
        {},
        1},
       {dwarf::DW_TAG_variable,
        {strAttr(dwarf::DW_AT_name, "Dropped"),
         refAttr(dwarf::DW_AT_type, 1),
         {dwarf::DW_AT_location,
          dwarf::DW_FORM_exprloc,
          0,
          {dwarf::DW_OP_addr, 0xFF, 0xFF, 0xFF, 0xFF}}}}}};
  TypeFixture F;
  ASSERT_TRUE(F.adopt(moduleWithDwarf(F.Conf, writeDwarfUnit(4, Unit))));
  EXPECT_FALSE(F.Reader->hasGlobalVariables());
  EXPECT_EQ(F.M->getGlobalVariable("__wasmedge_debug_membase"), nullptr);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
  bool Found = false;
  for (const auto *CU : F.M->debug_compile_units()) {
    for (const auto *GVE : CU->getGlobalVariables()) {
      Found |= GVE->getVariable()->getName() == "Dropped";
    }
  }
  EXPECT_TRUE(Found);
}

TEST(DebugInfoTypes, NoGlobalsMeansNoMemoryBaseGlobal) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_scope_Og.wasm"sv));
  EXPECT_FALSE(F.Reader->hasGlobalVariables());
  EXPECT_EQ(F.M->getGlobalVariable("__wasmedge_debug_membase"), nullptr);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, OneBasedLanguageArrayStartsAtOne) {
  TypeFixture F;
  Configure Conf;
  F.Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(F.Mod, nullptr);
  uint64_t LanguageOffset = 0;
  {
    auto R = LLVM::DebugInfo::DwarfReader::create(*F.Mod);
    ASSERT_NE(R, nullptr);
    auto CU = R->getContext().compile_units().begin()->get()->getUnitDIE();
    for (const auto &A : CU.attributes()) {
      if (A.Attr == llvm::dwarf::DW_AT_language &&
          A.Value.getForm() == llvm::dwarf::DW_FORM_data2) {
        LanguageOffset = A.Offset;
      }
    }
  }
  ASSERT_NE(LanguageOffset, 0U);
  for (auto &Custom : F.Mod->getCustomSections()) {
    if (Custom.getName() == ".debug_info"sv) {
      Custom.getContent()[LanguageOffset] = llvm::dwarf::DW_LANG_Fortran90;
      Custom.getContent()[LanguageOffset + 1] = 0;
    }
  }
  F.Reader = LLVM::DebugInfo::DwarfReader::create(*F.Mod);
  ASSERT_NE(F.Reader, nullptr);
  F.M = std::make_unique<llvm::Module>("t", F.Ctx);
  F.DI = LLVM::DebugInfo::ModuleDebugInfo::create(
      *F.Mod, llvm::wrap(F.M.get()),
      LLVM::DebugInfo::ModuleDebugInfo::Part::All);
  ASSERT_NE(F.DI, nullptr);
  llvm::DWARFDie ArrayDie;
  for (const auto &CU : F.Reader->getContext().compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie D(CU.get(), &Entry);
      if (!ArrayDie && D.getTag() == llvm::dwarf::DW_TAG_array_type) {
        ArrayDie = D;
      }
    }
  }
  ASSERT_TRUE(ArrayDie);
  auto *Arr = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(ArrayDie.getOffset())));
  ASSERT_EQ(Arr->getElements().size(), 1U);
  auto *Sub = llvm::cast<llvm::DISubrange>(Arr->getElements()[0]);
#if LLVM_VERSION_MAJOR < 11
  EXPECT_EQ(Sub->getLowerBound(), 1);
#elif LLVM_VERSION_MAJOR < 17
  auto *Lower = Sub->getLowerBound().dyn_cast<llvm::ConstantInt *>();
  ASSERT_NE(Lower, nullptr);
  EXPECT_EQ(Lower->getSExtValue(), 1);
#else
  auto *Lower =
      llvm::dyn_cast_if_present<llvm::ConstantInt *>(Sub->getLowerBound());
  ASSERT_NE(Lower, nullptr);
  EXPECT_EQ(Lower->getSExtValue(), 1);
#endif
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, UnknownTagBecomesOpaqueBytes) {
  TypeFixture F;
  ASSERT_TRUE(F.load("dwarf_basic_O0.wasm"sv));
  auto CUDie =
      F.Reader->getContext().compile_units().begin()->get()->getUnitDIE();
  auto *T = llvm::unwrap(F.DI->translateType(CUDie.getOffset()));
  auto *Arr = llvm::dyn_cast_or_null<llvm::DICompositeType>(T);
  ASSERT_NE(Arr, nullptr);
  EXPECT_EQ(Arr->getTag(), llvm::dwarf::DW_TAG_array_type);
}

TEST(DebugInfoTypes, MemberOffsetForms) {
  namespace dwarf = llvm::dwarf;
  using LLVM::DebugInfo::memberOffset;
  const auto Data4 =
      llvm::DWARFFormValue::createFromUValue(dwarf::DW_FORM_data4, 8);
  const auto Data1 =
      llvm::DWARFFormValue::createFromUValue(dwarf::DW_FORM_data1, 4);
  const auto Negative =
      llvm::DWARFFormValue::createFromSValue(dwarf::DW_FORM_sdata, -4);
  const auto SecOffset =
      llvm::DWARFFormValue::createFromUValue(dwarf::DW_FORM_sec_offset, 8);
  EXPECT_FALSE(memberOffset(Data4, 3));
  EXPECT_EQ(memberOffset(Data4, 4), std::optional<uint64_t>(8));
  EXPECT_EQ(memberOffset(Data1, 2), std::optional<uint64_t>(4));
  EXPECT_FALSE(memberOffset(Negative, 5));
  EXPECT_FALSE(memberOffset(SecOffset, 5));
}

TEST(DebugInfoTypes, Dwarf3LocationListIsNotAMemberOffset) {
  namespace dwarf = llvm::dwarf;
  for (const uint16_t Version : {3, 4}) {
    TestDie Unit{
        dwarf::DW_TAG_compile_unit,
        {strAttr(dwarf::DW_AT_producer, "clang"),
         {dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_C11}},
        {{dwarf::DW_TAG_base_type,
          {strAttr(dwarf::DW_AT_name, "int"),
           {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_signed},
           {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 4}},
          {},
          1},
         {dwarf::DW_TAG_structure_type,
          {strAttr(dwarf::DW_AT_name, "S"),
           {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 8}},
          {{dwarf::DW_TAG_member,
            {strAttr(dwarf::DW_AT_name, "First"),
             refAttr(dwarf::DW_AT_type, 1),
             {dwarf::DW_AT_data_member_location, dwarf::DW_FORM_data1, 0}}},
           {dwarf::DW_TAG_member,
            {strAttr(dwarf::DW_AT_name, "Second"),
             refAttr(dwarf::DW_AT_type, 1),
             {dwarf::DW_AT_data_member_location, dwarf::DW_FORM_data4, 4}}}},
          2},
         {dwarf::DW_TAG_structure_type,
          {strAttr(dwarf::DW_AT_name, "D"),
           {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 8}},
          {{dwarf::DW_TAG_inheritance,
            {refAttr(dwarf::DW_AT_type, 2),
             {dwarf::DW_AT_data_member_location, dwarf::DW_FORM_data4, 0}}}},
          3}}};
    const auto Dwarf = writeDwarfUnit(Version, Unit);
    TypeFixture F;
    ASSERT_TRUE(F.adopt(moduleWithDwarf(F.Conf, Dwarf))) << Version;
    auto *S = llvm::cast<llvm::DICompositeType>(
        llvm::unwrap(F.DI->translateType(Dwarf.Offsets.at(2))));
    auto *D = llvm::cast<llvm::DICompositeType>(
        llvm::unwrap(F.DI->translateType(Dwarf.Offsets.at(3))));
    if (Version < 4) {
      ASSERT_EQ(S->getElements().size(), 1U);
      EXPECT_EQ(llvm::cast<llvm::DIDerivedType>(S->getElements()[0])->getName(),
                "First");
      EXPECT_EQ(D->getElements().size(), 0U);
    } else {
      ASSERT_EQ(S->getElements().size(), 2U);
      auto *Second = llvm::cast<llvm::DIDerivedType>(S->getElements()[1]);
      EXPECT_EQ(Second->getName(), "Second");
      EXPECT_EQ(Second->getOffsetInBits(), 32U);
      EXPECT_EQ(D->getElements().size(), 1U);
    }
    F.DI->finalize();
    EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
  }
}

TEST(DebugInfoTypes, ArrayStrideKeepsTheLayoutOrOmitsIt) {
  namespace dwarf = llvm::dwarf;
  using Attr = TestDie::Attribute;
  const Attr NoStride{dwarf::DW_AT_name, dwarf::DW_FORM_string, 0, {'a'}};
  auto ByteStride = [](uint64_t V) {
    return Attr{dwarf::DW_AT_byte_stride, dwarf::DW_FORM_data1, V};
  };
  auto BitStride = [](uint64_t V) {
    return Attr{dwarf::DW_AT_bit_stride, dwarf::DW_FORM_data1, V};
  };
  auto Count = [](uint64_t V) {
    return Attr{dwarf::DW_AT_count, dwarf::DW_FORM_data1, V};
  };
  struct Case {
    Attr OnArray;
    std::vector<std::vector<Attr>> Subranges;
    bool KeepsLayout;
  };
  const Case Cases[] = {
      {NoStride, {{Count(8)}}, true},
      {ByteStride(4), {{Count(8)}}, true},
      {ByteStride(8), {{Count(4)}}, false},
      {BitStride(32), {{Count(8)}}, true},
      {BitStride(33), {{Count(8)}}, false},
      {NoStride, {{Count(8), ByteStride(8)}}, false},
      {NoStride, {{Count(2), ByteStride(16)}, {Count(4), ByteStride(4)}}, true},
      {NoStride, {{Count(2), ByteStride(20)}, {Count(4)}}, false},
      {NoStride, {{Count(2), ByteStride(16)}, {}}, false},
  };
  for (size_t I = 0; I < std::size(Cases); ++I) {
    const auto &C = Cases[I];
    TestDie Array{dwarf::DW_TAG_array_type,
                  {refAttr(dwarf::DW_AT_type, 1),
                   {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 32},
                   C.OnArray},
                  {},
                  2};
    for (const auto &Sub : C.Subranges) {
      Array.Children.push_back({dwarf::DW_TAG_subrange_type, Sub});
    }
    TestDie Unit{
        dwarf::DW_TAG_compile_unit,
        {strAttr(dwarf::DW_AT_producer, "clang"),
         {dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_C11}},
        {{dwarf::DW_TAG_base_type,
          {strAttr(dwarf::DW_AT_name, "int"),
           {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_signed},
           {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 4}},
          {},
          1},
         Array}};
    const auto Dwarf = writeDwarfUnit(4, Unit);
    TypeFixture F;
    ASSERT_TRUE(F.adopt(moduleWithDwarf(F.Conf, Dwarf))) << I;
    auto *T = llvm::cast<llvm::DICompositeType>(
        llvm::unwrap(F.DI->translateType(Dwarf.Offsets.at(2))));
    auto *Base = llvm::cast<llvm::DIBasicType>(T->getBaseType());
    EXPECT_EQ(Base->getName() == "int", C.KeepsLayout) << I;
    if (!C.KeepsLayout) {
      EXPECT_EQ(T->getSizeInBits(), 256U) << I;
    }
    F.DI->finalize();
    EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs())) << I;
  }
}

TEST(DebugInfoTypes, EachVariantKeepsItsFields) {
  namespace dwarf = llvm::dwarf;
  auto Field = [](std::string_view Name, uint32_t Type, uint64_t Offset) {
    return TestDie{
        dwarf::DW_TAG_member,
        {strAttr(dwarf::DW_AT_name, Name),
         refAttr(dwarf::DW_AT_type, Type),
         {dwarf::DW_AT_data_member_location, dwarf::DW_FORM_data1, Offset}}};
  };
  auto Variant = [](uint64_t Value, std::vector<TestDie> Fields) {
    return TestDie{dwarf::DW_TAG_variant,
                   {{dwarf::DW_AT_discr_value, dwarf::DW_FORM_data1, Value}},
                   std::move(Fields)};
  };
  auto Base = [](std::string_view Name, uint64_t Size, uint32_t Id) {
    return TestDie{
        dwarf::DW_TAG_base_type,
        {strAttr(dwarf::DW_AT_name, Name),
         {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_unsigned},
         {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, Size}},
        {},
        Id};
  };
  TestDie Part{dwarf::DW_TAG_variant_part,
               {refAttr(dwarf::DW_AT_discr, 10)},
               {Field("tag", 1, 0),
                Variant(0, {Field("a", 2, 2), Field("b", 2, 4)}),
                Variant(1, {}), Variant(2, {Field("c", 3, 4)})}};
  Part.Children[0].Id = 10;
  TestDie Unit{
      dwarf::DW_TAG_compile_unit,
      {strAttr(dwarf::DW_AT_producer, "clang LLVM (rustc version 1.98.1)"),
       {dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_Rust}},
      {Base("u8", 1, 1),
       Base("u16", 2, 2),
       Base("u32", 4, 3),
       {dwarf::DW_TAG_structure_type,
        {strAttr(dwarf::DW_AT_name, "V"),
         {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 8}},
        {Part},
        4}}};
  const auto Dwarf = writeDwarfUnit(4, Unit);
  TypeFixture F;
  ASSERT_TRUE(F.adopt(moduleWithDwarf(F.Conf, Dwarf)));
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(Dwarf.Offsets.at(4))));
  ASSERT_EQ(T->getElements().size(), 1U);
  auto *VP = llvm::cast<llvm::DICompositeType>(T->getElements()[0]);
  EXPECT_EQ(VP->getTag(), dwarf::DW_TAG_variant_part);
  ASSERT_EQ(VP->getElements().size(), 3U);
  std::vector<size_t> FieldCounts;
  for (size_t I = 0; I < 3; ++I) {
    auto *M = llvm::cast<llvm::DIDerivedType>(VP->getElements()[I]);
    auto *Value =
        llvm::dyn_cast_or_null<llvm::ConstantInt>(M->getDiscriminantValue());
    ASSERT_NE(Value, nullptr) << I;
    EXPECT_EQ(Value->getZExtValue(), I);
    if (auto *Group = llvm::dyn_cast<llvm::DICompositeType>(M->getBaseType());
        Group && M->getName().empty()) {
      FieldCounts.push_back(Group->getElements().size());
    } else {
      EXPECT_EQ(M->getName(), "c");
      EXPECT_EQ(M->getOffsetInBits(), 32U);
      FieldCounts.push_back(1);
    }
  }
  EXPECT_EQ(FieldCounts, (std::vector<size_t>{2, 0, 1}));
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, LoneBitFieldOfAVariantKeepsItsBits) {
  namespace dwarf = llvm::dwarf;
  auto Base = [](std::string_view Name, uint64_t Size, uint32_t Id) {
    return TestDie{
        dwarf::DW_TAG_base_type,
        {strAttr(dwarf::DW_AT_name, Name),
         {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_unsigned},
         {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, Size}},
        {},
        Id};
  };
  TestDie Tag{dwarf::DW_TAG_member,
              {strAttr(dwarf::DW_AT_name, "tag"),
               refAttr(dwarf::DW_AT_type, 1),
               {dwarf::DW_AT_data_member_location, dwarf::DW_FORM_data1, 0}},
              {},
              10};
  TestDie Flag{dwarf::DW_TAG_member,
               {strAttr(dwarf::DW_AT_name, "flag"),
                refAttr(dwarf::DW_AT_type, 2),
                {dwarf::DW_AT_bit_size, dwarf::DW_FORM_data1, 3},
                {dwarf::DW_AT_data_bit_offset, dwarf::DW_FORM_data1, 17}}};
  TestDie Unit{
      dwarf::DW_TAG_compile_unit,
      {strAttr(dwarf::DW_AT_producer, "clang LLVM (rustc version 1.98.1)"),
       {dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_Rust}},
      {Base("u8", 1, 1),
       Base("u32", 4, 2),
       {dwarf::DW_TAG_structure_type,
        {strAttr(dwarf::DW_AT_name, "V"),
         {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 8}},
        {{dwarf::DW_TAG_variant_part,
          {refAttr(dwarf::DW_AT_discr, 10)},
          {Tag,
           {dwarf::DW_TAG_variant,
            {{dwarf::DW_AT_discr_value, dwarf::DW_FORM_data1, 0}},
            {Flag}}}}},
        4}}};
  const auto Dwarf = writeDwarfUnit(4, Unit);
  TypeFixture F;
  ASSERT_TRUE(F.adopt(moduleWithDwarf(F.Conf, Dwarf)));
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(Dwarf.Offsets.at(4))));
  ASSERT_EQ(T->getElements().size(), 1U);
  auto *VP = llvm::cast<llvm::DICompositeType>(T->getElements()[0]);
  ASSERT_EQ(VP->getElements().size(), 1U);
  auto *V = llvm::cast<llvm::DIDerivedType>(VP->getElements()[0]);
  auto *Group = llvm::dyn_cast<llvm::DICompositeType>(V->getBaseType());
  ASSERT_NE(Group, nullptr);
  ASSERT_EQ(Group->getElements().size(), 1U);
  auto *Field = llvm::cast<llvm::DIDerivedType>(Group->getElements()[0]);
  EXPECT_EQ(Field->getName(), "flag");
  EXPECT_TRUE(Field->isBitField());
  EXPECT_EQ(Field->getSizeInBits(), 3U);
  EXPECT_EQ(Field->getOffsetInBits(), 17U);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, NestedArrayKeepsAnExplicitStride) {
  namespace dwarf = llvm::dwarf;
  TestDie Unit{
      dwarf::DW_TAG_compile_unit,
      {strAttr(dwarf::DW_AT_producer, "clang"),
       {dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_C11}},
      {{dwarf::DW_TAG_base_type,
        {strAttr(dwarf::DW_AT_name, "int"),
         {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_signed},
         {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 4}},
        {},
        1},
       {dwarf::DW_TAG_array_type,
        {refAttr(dwarf::DW_AT_type, 1)},
        {{dwarf::DW_TAG_subrange_type,
          {{dwarf::DW_AT_count, dwarf::DW_FORM_data1, 4}}}},
        2},
       {dwarf::DW_TAG_array_type,
        {refAttr(dwarf::DW_AT_type, 2),
         {dwarf::DW_AT_byte_stride, dwarf::DW_FORM_data1, 16}},
        {{dwarf::DW_TAG_subrange_type,
          {{dwarf::DW_AT_count, dwarf::DW_FORM_data1, 2}}}},
        3}}};
  const auto Dwarf = writeDwarfUnit(4, Unit);
  TypeFixture F;
  ASSERT_TRUE(F.adopt(moduleWithDwarf(F.Conf, Dwarf)));
  auto *Inner = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(Dwarf.Offsets.at(2))));
  EXPECT_EQ(Inner->getSizeInBits(), 128U);
  auto *Outer = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(Dwarf.Offsets.at(3))));
  EXPECT_EQ(Outer->getBaseType(), Inner);
  EXPECT_EQ(Outer->getSizeInBits(), 256U);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, VariantWithAnUnreadableFieldIsOmitted) {
  namespace dwarf = llvm::dwarf;
  auto Base = [](std::string_view Name, uint64_t Size, uint32_t Id) {
    return TestDie{
        dwarf::DW_TAG_base_type,
        {strAttr(dwarf::DW_AT_name, Name),
         {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_unsigned},
         {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, Size}},
        {},
        Id};
  };
  TestDie Tag{dwarf::DW_TAG_member,
              {strAttr(dwarf::DW_AT_name, "tag"),
               refAttr(dwarf::DW_AT_type, 1),
               {dwarf::DW_AT_data_member_location, dwarf::DW_FORM_data1, 0}},
              {},
              10};
  TestDie Good{dwarf::DW_TAG_member,
               {strAttr(dwarf::DW_AT_name, "a"),
                refAttr(dwarf::DW_AT_type, 2),
                {dwarf::DW_AT_data_member_location, dwarf::DW_FORM_data1, 2}}};
  TestDie Unreadable{dwarf::DW_TAG_member,
                     {strAttr(dwarf::DW_AT_name, "b"),
                      refAttr(dwarf::DW_AT_type, 2),
                      {dwarf::DW_AT_data_member_location,
                       dwarf::DW_FORM_exprloc,
                       0,
                       {dwarf::DW_OP_lit4}}}};
  TestDie Unit{
      dwarf::DW_TAG_compile_unit,
      {strAttr(dwarf::DW_AT_producer, "clang LLVM (rustc version 1.98.1)"),
       {dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_Rust}},
      {Base("u8", 1, 1),
       Base("u16", 2, 2),
       {dwarf::DW_TAG_structure_type,
        {strAttr(dwarf::DW_AT_name, "V"),
         {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 8}},
        {{dwarf::DW_TAG_variant_part,
          {refAttr(dwarf::DW_AT_discr, 10)},
          {Tag,
           {dwarf::DW_TAG_variant,
            {{dwarf::DW_AT_discr_value, dwarf::DW_FORM_data1, 0}},
            {Good, Unreadable}}}}},
        4}}};
  const auto Dwarf = writeDwarfUnit(4, Unit);
  TypeFixture F;
  ASSERT_TRUE(F.adopt(moduleWithDwarf(F.Conf, Dwarf)));
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(Dwarf.Offsets.at(4))));
  EXPECT_TRUE(T->getElements().empty());
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, GlobalsWithoutAPlainAddressKeepTheirNames) {
#if LLVM_VERSION_MAJOR < 12
  GTEST_SKIP() << "the test writes a DWARF 5 location list";
#else
  namespace dwarf = llvm::dwarf;
  std::vector<uint8_t> LocLists = {0, 0, 0, 0, 5, 0, 4, 0, 0, 0, 0, 0};
  auto AddressEntry = [&LocLists](uint8_t Kind, std::vector<uint8_t> Range,
                                  uint32_t Address) {
    LocLists.push_back(Kind);
    LocLists.insert(LocLists.end(), Range.begin(), Range.end());
    LocLists.insert(LocLists.end(),
                    {5, dwarf::DW_OP_addr, static_cast<uint8_t>(Address),
                     static_cast<uint8_t>(Address >> 8), 0, 0});
  };
  AddressEntry(dwarf::DW_LLE_default_location, {}, 0x400);
  LocLists.push_back(dwarf::DW_LLE_end_of_list);
  const uint64_t GapList = LocLists.size();
  AddressEntry(dwarf::DW_LLE_start_end, {0x10, 0, 0, 0, 0x20, 0, 0, 0}, 0x500);
  AddressEntry(dwarf::DW_LLE_start_end, {0x30, 0, 0, 0, 0x40, 0, 0, 0}, 0x500);
  LocLists.push_back(dwarf::DW_LLE_end_of_list);
  LocLists[0] = static_cast<uint8_t>(LocLists.size() - 4);
  TestDie Unit{
      dwarf::DW_TAG_compile_unit,
      {strAttr(dwarf::DW_AT_producer, "clang"),
       {dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_C11}},
      {{dwarf::DW_TAG_base_type,
        {strAttr(dwarf::DW_AT_name, "int"),
         {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_signed},
         {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 4}},
        {},
        1},
       {dwarf::DW_TAG_variable,
        {strAttr(dwarf::DW_AT_name, "Limit"),
         refAttr(dwarf::DW_AT_type, 1),
         {dwarf::DW_AT_const_value, dwarf::DW_FORM_sdata, 7}}},
       {dwarf::DW_TAG_variable,
        {strAttr(dwarf::DW_AT_name, "Base"),
         refAttr(dwarf::DW_AT_type, 1),
         {dwarf::DW_AT_location, dwarf::DW_FORM_sec_offset, 12}}},
       {dwarf::DW_TAG_variable,
        {strAttr(dwarf::DW_AT_name, "Gap"),
         refAttr(dwarf::DW_AT_type, 1),
         {dwarf::DW_AT_location, dwarf::DW_FORM_sec_offset, GapList}}}}};
  const auto Dwarf = writeDwarfUnit(5, Unit);
  TypeFixture F;
  ASSERT_TRUE(
      F.adopt(moduleWithDwarf(F.Conf, Dwarf, {{".debug_loclists", LocLists}})));
  EXPECT_TRUE(F.Reader->hasGlobalVariables());
  F.DI->finalize();
  std::map<std::string, std::vector<uint64_t>> Expressions;
  for (const auto *CU : F.M->debug_compile_units()) {
    for (const auto *GVE : CU->getGlobalVariables()) {
      const auto *E = GVE->getExpression();
      Expressions[GVE->getVariable()->getName().str()] = {E->elements_begin(),
                                                          E->elements_end()};
    }
  }
  EXPECT_EQ(Expressions["Limit"],
            (std::vector<uint64_t>{dwarf::DW_OP_constu, 7,
                                   dwarf::DW_OP_stack_value}));
  EXPECT_EQ(Expressions["Base"],
            (std::vector<uint64_t>{dwarf::DW_OP_deref, dwarf::DW_OP_plus_uconst,
                                   0x400}));
  EXPECT_EQ(Expressions.count("Gap"), 0U);
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
#endif
}

TEST(DebugInfoTypes, ArrayStorageOrderMatchesTheStrides) {
  namespace dwarf = llvm::dwarf;
  auto Subrange = [](uint64_t Count, uint64_t Stride) {
    return TestDie{dwarf::DW_TAG_subrange_type,
                   {{dwarf::DW_AT_count, dwarf::DW_FORM_data1, Count},
                    {dwarf::DW_AT_byte_stride, dwarf::DW_FORM_data1, Stride}}};
  };
  struct Case {
    uint64_t Language;
    std::optional<uint64_t> Ordering;
    std::vector<TestDie> Subranges;
    bool KeepsLayout;
  };
  const Case Cases[] = {
      {dwarf::DW_LANG_C11,
       std::nullopt,
       {Subrange(2, 12), Subrange(3, 4)},
       true},
      {dwarf::DW_LANG_C11,
       std::nullopt,
       {Subrange(2, 4), Subrange(3, 8)},
       false},
      {dwarf::DW_LANG_Fortran90,
       std::nullopt,
       {Subrange(2, 4), Subrange(3, 8)},
       true},
      {dwarf::DW_LANG_Fortran90,
       dwarf::DW_ORD_col_major,
       {Subrange(2, 4), Subrange(3, 8)},
       true},
      {dwarf::DW_LANG_C11,
       dwarf::DW_ORD_col_major,
       {Subrange(2, 4), Subrange(3, 8)},
       false},
      {dwarf::DW_LANG_Fortran90,
       dwarf::DW_ORD_row_major,
       {Subrange(2, 12), Subrange(3, 4)},
       false},
#if LLVM_VERSION_MAJOR < 16
#else
      {dwarf::DW_LANG_Fortran18,
       std::nullopt,
       {Subrange(2, 4), Subrange(3, 8)},
       true},
      {dwarf::DW_LANG_Fortran18,
       dwarf::DW_ORD_col_major,
       {Subrange(2, 4), Subrange(3, 8)},
       true},
      {dwarf::DW_LANG_Fortran18,
       std::nullopt,
       {Subrange(2, 12), Subrange(3, 4)},
       false},
#endif
  };
  for (size_t I = 0; I < std::size(Cases); ++I) {
    const auto &C = Cases[I];
    TestDie Array{dwarf::DW_TAG_array_type,
                  {refAttr(dwarf::DW_AT_type, 1),
                   {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 24}},
                  C.Subranges,
                  2};
    if (C.Ordering) {
      Array.Attributes.push_back(
          {dwarf::DW_AT_ordering, dwarf::DW_FORM_data1, *C.Ordering});
    }
    TestDie Unit{
        dwarf::DW_TAG_compile_unit,
        {strAttr(dwarf::DW_AT_producer, "clang"),
         {dwarf::DW_AT_language, dwarf::DW_FORM_data2, C.Language}},
        {{dwarf::DW_TAG_base_type,
          {strAttr(dwarf::DW_AT_name, "int"),
           {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_signed},
           {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 4}},
          {},
          1},
         Array}};
    const auto Dwarf = writeDwarfUnit(4, Unit);
    TypeFixture F;
    ASSERT_TRUE(F.adopt(moduleWithDwarf(F.Conf, Dwarf))) << I;
    auto *T = llvm::cast<llvm::DICompositeType>(
        llvm::unwrap(F.DI->translateType(Dwarf.Offsets.at(2))));
    auto *Base = llvm::cast<llvm::DIBasicType>(T->getBaseType());
    EXPECT_EQ(Base->getName() == "int", C.KeepsLayout) << I;
    F.DI->finalize();
    EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs())) << I;
  }
}

TEST(DebugInfoTypes, DataBitOffsetPlacesAnOrdinaryMember) {
  namespace dwarf = llvm::dwarf;
  TestDie Int{
      dwarf::DW_TAG_base_type,
      {strAttr(dwarf::DW_AT_name, "int"),
       {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_signed},
       {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 4}},
      {},
      1};
  TestDie Struct{
      dwarf::DW_TAG_structure_type,
      {strAttr(dwarf::DW_AT_name, "S"),
       {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 8}},
      {{dwarf::DW_TAG_member,
        {strAttr(dwarf::DW_AT_name, "First"), refAttr(dwarf::DW_AT_type, 1)}},
       {dwarf::DW_TAG_member,
        {strAttr(dwarf::DW_AT_name, "Second"),
         refAttr(dwarf::DW_AT_type, 1),
         {dwarf::DW_AT_data_bit_offset, dwarf::DW_FORM_data1, 32}}}},
      2};
  TestDie Unit{
      dwarf::DW_TAG_compile_unit,
      {strAttr(dwarf::DW_AT_producer, "clang"),
       {dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_C11}},
      {Int, Struct}};
  const auto Dwarf = writeDwarfUnit(5, Unit);
  TypeFixture F;
  ASSERT_TRUE(F.adopt(moduleWithDwarf(F.Conf, Dwarf)));
  auto *T = llvm::cast<llvm::DICompositeType>(
      llvm::unwrap(F.DI->translateType(Dwarf.Offsets.at(2))));
  ASSERT_EQ(T->getElements().size(), 2U);
  EXPECT_EQ(
      llvm::cast<llvm::DIDerivedType>(T->getElements()[0])->getOffsetInBits(),
      0U);
  EXPECT_EQ(
      llvm::cast<llvm::DIDerivedType>(T->getElements()[1])->getOffsetInBits(),
      32U);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

TEST(DebugInfoTypes, VariadicSignatureKeepsItsEllipsis) {
  namespace dwarf = llvm::dwarf;
  TestDie Unit{
      dwarf::DW_TAG_compile_unit,
      {strAttr(dwarf::DW_AT_producer, "clang"),
       {dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_C11}},
      {{dwarf::DW_TAG_base_type,
        {strAttr(dwarf::DW_AT_name, "int"),
         {dwarf::DW_AT_encoding, dwarf::DW_FORM_data1, dwarf::DW_ATE_signed},
         {dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, 4}},
        {},
        1},
       {dwarf::DW_TAG_subroutine_type,
        {refAttr(dwarf::DW_AT_type, 1)},
        {{dwarf::DW_TAG_formal_parameter, {refAttr(dwarf::DW_AT_type, 1)}},
         {dwarf::DW_TAG_unspecified_parameters}},
        2}}};
  const auto Dwarf = writeDwarfUnit(5, Unit);
  TypeFixture F;
  ASSERT_TRUE(F.adopt(moduleWithDwarf(F.Conf, Dwarf)));
  auto *T = llvm::cast<llvm::DISubroutineType>(
      llvm::unwrap(F.DI->translateType(Dwarf.Offsets.at(2))));
  const auto Types = T->getTypeArray();
  ASSERT_EQ(Types.size(), 3U);
  EXPECT_NE(Types[1], nullptr);
  EXPECT_EQ(Types[2], nullptr);
  F.DI->finalize();
  EXPECT_FALSE(llvm::verifyModule(*F.M, &llvm::errs()));
}

} // namespace
