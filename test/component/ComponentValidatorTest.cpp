// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/component/component.h"
#include "ast/component/type.h"
#include "validator/validator.h"

#include <gtest/gtest.h>

namespace {

using namespace WasmEdge;
using namespace std::literals;

Configure Conf = []() {
  Configure C;
  C.addProposal(Proposal::Component);
  return C;
}();

AST::Component::Component makeImportWithVersionSuffix(std::string_view Name,
                                                      std::string_view Suffix) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  auto &Imp = ImpSec.getContent().back();
  Imp.getName() = Name;
  Imp.getVersionSuffix() = Suffix;
  Imp.setHasVersionSuffix(true);
  Imp.getDesc().setTypeBound();
  return Comp;
}

TEST(ComponentValidatorTest, VersionSuffixOnInterfaceName) {
  auto Comp =
      makeImportWithVersionSuffix("wasi:http/types@0.2"sv, ".0"sv);

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, VersionSuffixOnNonInterfaceName) {
  auto Comp = makeImportWithVersionSuffix("plain-name"sv, ".0"sv);

  Validator::Validator V(Conf);
  auto Res = V.validate(Comp);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(), ErrCode::Value::ComponentInvalidName);
}

TEST(ComponentValidatorTest, VersionSuffixProducingInvalidSemver) {
  auto Comp =
      makeImportWithVersionSuffix("wasi:http/types@0.2"sv, ".01"sv);

  Validator::Validator V(Conf);
  auto Res = V.validate(Comp);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(), ErrCode::Value::ComponentInvalidName);
}

TEST(ComponentValidatorTest, EmptyVersionSuffixOnFullCanonVersion) {
  auto Comp =
      makeImportWithVersionSuffix("wasi:http/types@0.0.0"sv, ""sv);

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, EmptyVersionSuffixWithIncompleteSemver) {
  auto Comp = makeImportWithVersionSuffix("wasi:http/types@1"sv, ""sv);

  Validator::Validator V(Conf);
  auto Res = V.validate(Comp);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(), ErrCode::Value::ComponentInvalidName);
}

TEST(ComponentValidatorTest, VersionSuffixOnCoreInlineExport) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreInstanceSection>();
  auto &InstSec =
      std::get<AST::Component::CoreInstanceSection>(Comp.getSections().back());

  AST::Component::InlineExport Export;
  Export.getName() = "plain-name";
  Export.getVersionSuffix() = ".0";
  Export.setHasVersionSuffix(true);
  Export.getSortIdx().getSort().setIsCore(true);
  Export.getSortIdx().getSort().setCoreSortType(
      AST::Component::Sort::CoreSortType::Func);
  Export.getSortIdx().setIdx(0);

  AST::Component::CoreInstance Inst;
  Inst.setInlineExports({Export});
  InstSec.getContent().push_back(std::move(Inst));

  Validator::Validator V(Conf);
  auto Res = V.validate(Comp);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error().getEnum(), ErrCode::Value::ComponentInvalidName);
}

TEST(ComponentValidatorTest, MissingArgument) {
  AST::Component::Component Comp;
  // Add both sections first, then access by index to avoid invalidation.
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ComponentSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::InstanceSection>();
  auto &CompSec =
      std::get<AST::Component::ComponentSection>(Comp.getSections()[0]);
  auto &InstSec =
      std::get<AST::Component::InstanceSection>(Comp.getSections()[1]);

  CompSec.getContent() = std::make_unique<AST::Component::Component>();
  CompSec.getContent()->getSections().emplace_back();
  CompSec.getContent()
      ->getSections()
      .back()
      .emplace<AST::Component::ImportSection>();
  auto &ImpSec = std::get<AST::Component::ImportSection>(
      CompSec.getContent()->getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "f";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  InstSec.getContent().emplace_back();
  AST::Component::InstantiateArg<AST::Component::SortIndex> Arg;
  Arg.getName() = "g";
  Arg.getIndex().getSort().setSortType(AST::Component::Sort::SortType::Func);
  Arg.getIndex().setIdx(0);
  InstSec.getContent().back().setInstantiateArgs(0U, {Arg});

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, TypeMismatch) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ComponentSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::InstanceSection>();
  auto &CompSec =
      std::get<AST::Component::ComponentSection>(Comp.getSections()[0]);
  auto &InstSec =
      std::get<AST::Component::InstanceSection>(Comp.getSections()[1]);

  CompSec.getContent() = std::make_unique<AST::Component::Component>();
  CompSec.getContent()->getSections().emplace_back();
  CompSec.getContent()
      ->getSections()
      .back()
      .emplace<AST::Component::ImportSection>();
  auto &ImpSec = std::get<AST::Component::ImportSection>(
      CompSec.getContent()->getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "f";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  InstSec.getContent().emplace_back();
  AST::Component::InstantiateArg<AST::Component::SortIndex> Arg;
  Arg.getName() = "f";
  Arg.getIndex().getSort().setSortType(
      AST::Component::Sort::SortType::Component);
  Arg.getIndex().setIdx(0);
  InstSec.getContent().back().setInstantiateArgs(0U, {Arg});

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

// =============================================================================
// DefValType validation tests
// =============================================================================

TEST(ComponentValidatorTest, OwnMustReferenceResourceType) {
  // type 0 = FuncType, type 1 = own(0) -> FAIL (own must refer to resource)
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  // type 0: FuncType (not a resource)
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(AST::Component::FuncType());

  // type 1: own(0) -- index 0 is a FuncType, not a resource
  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  DVT.setOwn(AST::Component::OwnTy{0});
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, OwnWithValidResource) {
  // type 0 = ResourceType, type 1 = own(0) -> PASS
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  // type 0: ResourceType
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setResourceType(AST::Component::ResourceType());

  // type 1: own(0) -- index 0 is a resource
  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  DVT.setOwn(AST::Component::OwnTy{0});
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, BorrowMustReferenceResourceType) {
  // type 0 = FuncType, type 1 = borrow(0) -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  // type 0: FuncType (not a resource)
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(AST::Component::FuncType());

  // type 1: borrow(0) -- index 0 is a FuncType, not a resource
  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  DVT.setBorrow(AST::Component::BorrowTy{0});
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, OwnOutOfBounds) {
  // type 0 = own(99) with no preceding types -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  DVT.setOwn(AST::Component::OwnTy{99});
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, EmptyRecordRejected) {
  // record with empty LabelTypes -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  DVT.setRecord(AST::Component::RecordTy{});
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, EmptyVariantRejected) {
  // variant with empty Cases -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  DVT.setVariant(AST::Component::VariantTy{});
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, EmptyTupleRejected) {
  // tuple with empty Types -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  DVT.setTuple(AST::Component::TupleTy{});
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, EmptyFlagsRejected) {
  // flags with empty Labels -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  DVT.setFlags(AST::Component::FlagsTy{});
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, FlagsTooManyRejected) {
  // flags with 33 labels -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::FlagsTy F;
  for (int I = 0; I < 33; ++I) {
    F.Labels.push_back("flag-" + std::to_string(I));
  }
  DVT.setFlags(std::move(F));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, EmptyEnumRejected) {
  // enum with empty Labels -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  DVT.setEnum(AST::Component::EnumTy{});
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, DuplicateRecordFieldRejected) {
  // record with two fields named "x" -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::RecordTy Rec;
  Rec.LabelTypes.emplace_back("x"s, ComponentValType(ComponentTypeCode::U32));
  Rec.LabelTypes.emplace_back("x"s, ComponentValType(ComponentTypeCode::U32));
  DVT.setRecord(std::move(Rec));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, FuncTypeDuplicateParamName) {
  // (type (func (param "x" u32) (param "x" u64)))  -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::FuncType FT;
  std::vector<AST::Component::LabelValType> Params;
  Params.emplace_back("x"s, ComponentValType(ComponentTypeCode::U32));
  Params.emplace_back("x"s, ComponentValType(ComponentTypeCode::U64));
  FT.setParamList(std::move(Params));
  TypeSec.getContent().back().setFuncType(std::move(FT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, FuncTypeParamNotKebabCase) {
  // (type (func (param "NOT_KEBAB" u32)))  -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::FuncType FT;
  std::vector<AST::Component::LabelValType> Params;
  Params.emplace_back("NOT_KEBAB"s, ComponentValType(ComponentTypeCode::U32));
  FT.setParamList(std::move(Params));
  TypeSec.getContent().back().setFuncType(std::move(FT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, FuncTypeBorrowInResultRejected) {
  // (type (resource))        ;; type 0
  // (type (func (result (borrow 0))))  -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  // Type 0: resource
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setResourceType(AST::Component::ResourceType());

  // Type 1: func with borrow in result
  TypeSec.getContent().emplace_back();
  AST::Component::FuncType FT;
  std::vector<AST::Component::LabelValType> Results;
  Results.emplace_back(ComponentValType(ComponentTypeCode::Borrow, 0));
  FT.setResultList(std::move(Results));
  TypeSec.getContent().back().setFuncType(std::move(FT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, FuncTypeValidParams) {
  // (type (func (param "name" u32) (param "age" u64) (result u32)))  -> PASS
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::FuncType FT;
  std::vector<AST::Component::LabelValType> Params;
  Params.emplace_back("name"s, ComponentValType(ComponentTypeCode::U32));
  Params.emplace_back("age"s, ComponentValType(ComponentTypeCode::U64));
  FT.setParamList(std::move(Params));
  FT.setResultList(ComponentValType(ComponentTypeCode::U32));
  TypeSec.getContent().back().setFuncType(std::move(FT));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, ResourceDestructorOutOfBounds) {
  // (type (resource (dtor (func 99))))  -- no core funcs defined -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  AST::Component::ResourceType RT;
  RT.getDestructor() = 99;
  TypeSec.getContent().back().setResourceType(std::move(RT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, ResourceNoDestructorValid) {
  // (type (resource))  -- no destructor -> PASS
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setResourceType(AST::Component::ResourceType());

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, SubResourceImportIsResourceType) {
  // (component
  //   (import "r" (type (sub resource)))   ;; type 0 = abstract resource
  //   (type (own 0))                        ;; VALID: own refs resource
  // )
  AST::Component::Component Comp;

  // Import section with (sub resource) type bound
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "r";
  ImpSec.getContent()
      .back()
      .getDesc()
      .setTypeBound(); // sub resource (Eq=false)

  // Type section with own(0)
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  DVT.setOwn(AST::Component::OwnTy{0});
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, EqTypeBoundOutOfBounds) {
  // (import "t" (type (eq 99)))   ;; no type 99 -> FAIL
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "t";
  ImpSec.getContent().back().getDesc().setTypeBound(99); // eq 99

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, EqTypeBoundPropagatesResource) {
  // (component
  //   (type (resource))                 ;; type 0 = resource
  //   (import "r" (type (eq 0)))        ;; type 1 = alias of resource
  //   (type (own 1))                    ;; VALID: own refs aliased resource
  // )

  // Type section first: define resource
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setResourceType(AST::Component::ResourceType());

  // Import section: eq alias to type 0
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "r";
  ImpSec.getContent().back().getDesc().setTypeBound(0); // eq 0

  // Another type section: own(1) -- the aliased resource
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec2 =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  TypeSec2.getContent().emplace_back();
  AST::Component::DefValType DVT;
  DVT.setOwn(AST::Component::OwnTy{1});
  TypeSec2.getContent().back().setDefValType(std::move(DVT));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

// =============================================================================
// Declaration validation tests (DECL-1 through DECL-4)
// =============================================================================

TEST(ComponentValidatorTest, InstanceTypeExportSubResourceValid) {
  // (type (instance
  //   (export "r" (type (sub resource)))   ;; type 0 = resource
  //   (type (borrow 0))                    ;; PASS: borrow refs resource
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::InstanceDecl> Decls;

  // export "r" (type (sub resource))
  AST::Component::ExportDecl Exp;
  Exp.getName() = "r";
  Exp.getExternDesc().setTypeBound(); // sub resource
  AST::Component::InstanceDecl D0;
  D0.setExport(std::move(Exp));
  Decls.push_back(std::move(D0));

  // type (borrow 0) — references the resource from export
  auto DT = std::make_unique<AST::Component::DefType>();
  AST::Component::DefValType DVT;
  DVT.setBorrow(AST::Component::BorrowTy{0});
  DT->setDefValType(std::move(DVT));
  AST::Component::InstanceDecl D1;
  D1.setType(std::move(DT));
  Decls.push_back(std::move(D1));

  AST::Component::InstanceType IT;
  IT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setInstanceType(std::move(IT));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, InstanceTypeExportEqResourcePropagates) {
  // (type (instance
  //   (export "r" (type (sub resource))) ;; type 0 = resource
  //   (export "s" (type (eq 0)))         ;; type 1 = eq resource
  //   (type (own 1))                     ;; PASS: own refs propagated resource
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::InstanceDecl> Decls;

  // export "r" (type (sub resource))
  AST::Component::ExportDecl Exp0;
  Exp0.getName() = "r";
  Exp0.getExternDesc().setTypeBound(); // sub resource
  AST::Component::InstanceDecl D0;
  D0.setExport(std::move(Exp0));
  Decls.push_back(std::move(D0));

  // export "s" (type (eq 0))
  AST::Component::ExportDecl Exp1;
  Exp1.getName() = "s";
  Exp1.getExternDesc().setTypeBound(0); // eq 0
  AST::Component::InstanceDecl D1;
  D1.setExport(std::move(Exp1));
  Decls.push_back(std::move(D1));

  // type (own 1)
  auto OwnDT = std::make_unique<AST::Component::DefType>();
  AST::Component::DefValType DVT;
  DVT.setOwn(AST::Component::OwnTy{1});
  OwnDT->setDefValType(std::move(DVT));
  AST::Component::InstanceDecl D2;
  D2.setType(std::move(OwnDT));
  Decls.push_back(std::move(D2));

  AST::Component::InstanceType IT;
  IT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setInstanceType(std::move(IT));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, InstanceTypeDuplicateExportName) {
  // (type (instance
  //   (export "x" (func))
  //   (export "x" (func))   ;; FAIL: duplicate export name
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::InstanceDecl> Decls;
  for (int I = 0; I < 2; ++I) {
    AST::Component::ExportDecl Exp;
    Exp.getName() = "x";
    Exp.getExternDesc().setFuncTypeIdx(0);
    AST::Component::InstanceDecl D;
    D.setExport(std::move(Exp));
    Decls.push_back(std::move(D));
  }

  AST::Component::InstanceType IT;
  IT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setInstanceType(std::move(IT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, InstanceTypeExportCaseFoldConflict) {
  // (type (instance
  //   (export "foo" (func))
  //   (export "foo-BAR" (func))  ;; OK: distinct
  //   (export "FOO" (func))       ;; FAIL: case-folds to existing "foo"
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::InstanceDecl> Decls;
  for (const auto *N : {"foo", "foo-BAR", "FOO"}) {
    AST::Component::ExportDecl Exp;
    Exp.getName() = N;
    Exp.getExternDesc().setFuncTypeIdx(0);
    AST::Component::InstanceDecl D;
    D.setExport(std::move(Exp));
    Decls.push_back(std::move(D));
  }

  AST::Component::InstanceType IT;
  IT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setInstanceType(std::move(IT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, InstanceTypeExportConstructorPlainAllowed) {
  // (type (instance
  //   (type (func))                         ;; type 0 for the exports below
  //   (export "foo" (func (type 0)))
  //   (export "[constructor]foo" (func (type 0)))  ;; OK: strongly-unique pair
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::InstanceDecl> Decls;
  // Define a FuncType at the instancetype's local type idx 0.
  {
    auto DT = std::make_unique<AST::Component::DefType>();
    DT->setFuncType(AST::Component::FuncType{});
    AST::Component::InstanceDecl FtDecl;
    FtDecl.setType(std::move(DT));
    Decls.push_back(std::move(FtDecl));
  }
  for (const auto *N : {"foo", "[constructor]foo"}) {
    AST::Component::ExportDecl Exp;
    Exp.getName() = N;
    Exp.getExternDesc().setFuncTypeIdx(0);
    AST::Component::InstanceDecl D;
    D.setExport(std::move(Exp));
    Decls.push_back(std::move(D));
  }

  AST::Component::InstanceType IT;
  IT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setInstanceType(std::move(IT));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, InstanceTypeExportMethodSelfDotConflict) {
  // (type (instance
  //   (export "foo" (func))
  //   (export "[method]foo.foo" (func))   ;; FAIL: L and [*]L.L conflict
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::InstanceDecl> Decls;
  for (const auto *N : {"foo", "[method]foo.foo"}) {
    AST::Component::ExportDecl Exp;
    Exp.getName() = N;
    Exp.getExternDesc().setFuncTypeIdx(0);
    AST::Component::InstanceDecl D;
    D.setExport(std::move(Exp));
    Decls.push_back(std::move(D));
  }

  AST::Component::InstanceType IT;
  IT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setInstanceType(std::move(IT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, ComponentTopLevelDuplicateExportName) {
  // (component
  //   (import "i0" (func (type 0)))
  //   (export "foo" (func 0))
  //   (export "foo" (func 0))     ;; FAIL: duplicate
  // )
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "i0";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ExportSection>();
  auto &ExpSec =
      std::get<AST::Component::ExportSection>(Comp.getSections().back());
  for (int I = 0; I < 2; ++I) {
    AST::Component::Export Ex;
    Ex.getName() = "foo";
    Ex.getSortIndex().getSort().setIsCore(false);
    Ex.getSortIndex().getSort().setSortType(
        AST::Component::Sort::SortType::Func);
    Ex.getSortIndex().setIdx(0);
    ExpSec.getContent().push_back(std::move(Ex));
  }

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, ComponentTopLevelExportCaseFoldConflict) {
  // (component
  //   (import "i0" (func (type 0)))
  //   (export "foo" (func 0))
  //   (export "FOO" (func 0))   ;; FAIL: case-folds to existing "foo"
  // )
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "i0";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ExportSection>();
  auto &ExpSec =
      std::get<AST::Component::ExportSection>(Comp.getSections().back());
  for (const auto *N : {"foo", "FOO"}) {
    AST::Component::Export Ex;
    Ex.getName() = N;
    Ex.getSortIndex().getSort().setIsCore(false);
    Ex.getSortIndex().getSort().setSortType(
        AST::Component::Sort::SortType::Func);
    Ex.getSortIndex().setIdx(0);
    ExpSec.getContent().push_back(std::move(Ex));
  }

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, ComponentTypeDuplicateImportName) {
  // (type (component
  //   (import "f" (func))
  //   (import "f" (func))   ;; FAIL: duplicate import name
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::ComponentDecl> Decls;
  for (int I = 0; I < 2; ++I) {
    AST::Component::ImportDecl Imp;
    Imp.getName() = "f";
    Imp.getExternDesc().setFuncTypeIdx(0);
    AST::Component::ComponentDecl CD;
    CD.setImport(std::move(Imp));
    Decls.push_back(std::move(CD));
  }

  AST::Component::ComponentType CT;
  CT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setComponentType(std::move(CT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, ComponentTypeImportAnnotatedNameRequiresFunc) {
  // (type (component
  //   (import "[method]r.m" (instance))   ;; FAIL: annotated name on non-func
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::ComponentDecl> Decls;
  AST::Component::ImportDecl Imp;
  Imp.getName() = "[method]r.m";
  Imp.getExternDesc().setInstanceTypeIdx(0);
  AST::Component::ComponentDecl CD;
  CD.setImport(std::move(Imp));
  Decls.push_back(std::move(CD));

  AST::Component::ComponentType CT;
  CT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setComponentType(std::move(CT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, ComponentTypeExportedInstanceAliasResolves) {
  // Regression for wit-smith–generated fuzz case: an `alias export` inside a
  // ComponentType body must see the sub-exports declared by an earlier
  // ExportDecl of an inline instance type.
  //
  // (type (component
  //   (type (;0;) (instance (export "xx" (type (sub resource)))))
  //   (export "x" (instance (type 0)))
  //   (alias export 0 "xx" (type (;1;)))    ;; needs instance 0 to expose "xx"
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::ComponentDecl> Decls;

  // type 0: (instance (export "xx" (type (sub resource))))
  auto InnerDT = std::make_unique<AST::Component::DefType>();
  {
    AST::Component::ExportDecl InnerExp;
    InnerExp.getName() = "xx";
    InnerExp.getExternDesc().setTypeBound(); // sub resource
    AST::Component::InstanceDecl InnerD;
    InnerD.setExport(std::move(InnerExp));
    std::vector<AST::Component::InstanceDecl> InnerDecls;
    InnerDecls.push_back(std::move(InnerD));
    AST::Component::InstanceType InnerIT;
    InnerIT.setDecl(std::move(InnerDecls));
    InnerDT->setInstanceType(std::move(InnerIT));
  }
  AST::Component::InstanceDecl D0;
  D0.setType(std::move(InnerDT));
  AST::Component::ComponentDecl CD0;
  CD0.setInstance(std::move(D0));
  Decls.push_back(std::move(CD0));

  // export "x" (instance (type 0))  — instance 0
  AST::Component::ExportDecl Exp;
  Exp.getName() = "x";
  Exp.getExternDesc().setInstanceTypeIdx(0);
  AST::Component::InstanceDecl D1;
  D1.setExport(std::move(Exp));
  AST::Component::ComponentDecl CD1;
  CD1.setInstance(std::move(D1));
  Decls.push_back(std::move(CD1));

  // alias export 0 "xx" (type) — consumes "xx" from instance 0
  AST::Component::Alias A;
  A.setTargetType(AST::Component::Alias::TargetType::Export);
  A.getSort().setIsCore(false);
  A.getSort().setSortType(AST::Component::Sort::SortType::Type);
  A.setExport(0, "xx");
  AST::Component::InstanceDecl D2;
  D2.setAlias(std::move(A));
  AST::Component::ComponentDecl CD2;
  CD2.setInstance(std::move(D2));
  Decls.push_back(std::move(CD2));

  AST::Component::ComponentType CT;
  CT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setComponentType(std::move(CT));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, InstanceTypeExportedInstanceAliasResolves) {
  // Same as ComponentTypeExportedInstanceAliasResolves but inside an
  // InstanceType body — the ExportDecl-populate fix must apply on both
  // componenttype and instancetype scopes.
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::InstanceDecl> Decls;

  // type 0: (instance (export "xx" (type (sub resource))))
  auto InnerDT = std::make_unique<AST::Component::DefType>();
  {
    AST::Component::ExportDecl InnerExp;
    InnerExp.getName() = "xx";
    InnerExp.getExternDesc().setTypeBound();
    AST::Component::InstanceDecl InnerD;
    InnerD.setExport(std::move(InnerExp));
    std::vector<AST::Component::InstanceDecl> InnerDecls;
    InnerDecls.push_back(std::move(InnerD));
    AST::Component::InstanceType InnerIT;
    InnerIT.setDecl(std::move(InnerDecls));
    InnerDT->setInstanceType(std::move(InnerIT));
  }
  AST::Component::InstanceDecl D0;
  D0.setType(std::move(InnerDT));
  Decls.push_back(std::move(D0));

  // export "x" (instance (type 0))
  AST::Component::ExportDecl Exp;
  Exp.getName() = "x";
  Exp.getExternDesc().setInstanceTypeIdx(0);
  AST::Component::InstanceDecl D1;
  D1.setExport(std::move(Exp));
  Decls.push_back(std::move(D1));

  // alias export 0 "xx" (type)
  AST::Component::Alias A;
  A.setTargetType(AST::Component::Alias::TargetType::Export);
  A.getSort().setIsCore(false);
  A.getSort().setSortType(AST::Component::Sort::SortType::Type);
  A.setExport(0, "xx");
  AST::Component::InstanceDecl D2;
  D2.setAlias(std::move(A));
  Decls.push_back(std::move(D2));

  AST::Component::InstanceType IT;
  IT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setInstanceType(std::move(IT));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, ComponentTypeExportedInstanceAliasMissingExport) {
  // Negative: aliasing an export that the exported instance does not expose
  // must still fail after the populate fix.
  //
  // (type (component
  //   (type (;0;) (instance (export "xx" (type (sub resource)))))
  //   (export "x" (instance (type 0)))
  //   (alias export 0 "yy" (type))   ;; FAIL: instance 0 has "xx", not "yy"
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::ComponentDecl> Decls;

  auto InnerDT = std::make_unique<AST::Component::DefType>();
  {
    AST::Component::ExportDecl InnerExp;
    InnerExp.getName() = "xx";
    InnerExp.getExternDesc().setTypeBound();
    AST::Component::InstanceDecl InnerD;
    InnerD.setExport(std::move(InnerExp));
    std::vector<AST::Component::InstanceDecl> InnerDecls;
    InnerDecls.push_back(std::move(InnerD));
    AST::Component::InstanceType InnerIT;
    InnerIT.setDecl(std::move(InnerDecls));
    InnerDT->setInstanceType(std::move(InnerIT));
  }
  AST::Component::InstanceDecl D0;
  D0.setType(std::move(InnerDT));
  AST::Component::ComponentDecl CD0;
  CD0.setInstance(std::move(D0));
  Decls.push_back(std::move(CD0));

  AST::Component::ExportDecl Exp;
  Exp.getName() = "x";
  Exp.getExternDesc().setInstanceTypeIdx(0);
  AST::Component::InstanceDecl D1;
  D1.setExport(std::move(Exp));
  AST::Component::ComponentDecl CD1;
  CD1.setInstance(std::move(D1));
  Decls.push_back(std::move(CD1));

  AST::Component::Alias A;
  A.setTargetType(AST::Component::Alias::TargetType::Export);
  A.getSort().setIsCore(false);
  A.getSort().setSortType(AST::Component::Sort::SortType::Type);
  A.setExport(0, "yy");
  AST::Component::InstanceDecl D2;
  D2.setAlias(std::move(A));
  AST::Component::ComponentDecl CD2;
  CD2.setInstance(std::move(D2));
  Decls.push_back(std::move(CD2));

  AST::Component::ComponentType CT;
  CT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setComponentType(std::move(CT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CoreModuleTypeFuncTypeOutOfBounds) {
  // (type (module
  //   (import "m" "f" (func (type 99)))   ;; FAIL: no core type 99
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreTypeSection>();
  auto &CoreTypeSec =
      std::get<AST::Component::CoreTypeSection>(Comp.getSections().back());

  std::vector<AST::Component::CoreModuleDecl> ModDecls;
  AST::Component::CoreImportDecl CImp;
  CImp.getModuleName() = "m";
  CImp.getName() = "f";
  CImp.getImportDesc().setTypeIndex(99);
  AST::Component::CoreModuleDecl MD;
  MD.setImport(std::move(CImp));
  ModDecls.push_back(std::move(MD));

  CoreTypeSec.getContent().emplace_back();
  CoreTypeSec.getContent().back().setModuleType(std::move(ModDecls));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CoreModuleTypeValidImportExport) {
  // (type (module
  //   (type (func))                       ;; core type 0
  //   (import "m" "f" (func (type 0)))    ;; core func 0
  //   (export "g" (func (type 0)))        ;; core func 1
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreTypeSection>();
  auto &CoreTypeSec =
      std::get<AST::Component::CoreTypeSection>(Comp.getSections().back());

  std::vector<AST::Component::CoreModuleDecl> ModDecls;

  // core type (func)
  auto CDT = std::make_unique<AST::Component::CoreDefType>();
  std::vector<AST::SubType> STs;
  STs.emplace_back();
  STs.back().getCompositeType().setFunctionType(AST::FunctionType());
  CDT->setSubTypes(std::move(STs));
  AST::Component::CoreModuleDecl D0;
  D0.setType(std::move(CDT));
  ModDecls.push_back(std::move(D0));

  // import "m" "f" (func (type 0))
  AST::Component::CoreImportDecl CImp;
  CImp.getModuleName() = "m";
  CImp.getName() = "f";
  CImp.getImportDesc().setTypeIndex(0);
  AST::Component::CoreModuleDecl D1;
  D1.setImport(std::move(CImp));
  ModDecls.push_back(std::move(D1));

  // export "g" (func (type 0))
  AST::Component::CoreExportDecl CExp;
  CExp.getName() = "g";
  CExp.getImportDesc().setTypeIndex(0);
  AST::Component::CoreModuleDecl D2;
  D2.setExport(std::move(CExp));
  ModDecls.push_back(std::move(D2));

  CoreTypeSec.getContent().emplace_back();
  CoreTypeSec.getContent().back().setModuleType(std::move(ModDecls));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

// =============================================================================
// Duplicate-name validation tests for instances/instantiate
// =============================================================================

TEST(ComponentValidatorTest, CoreInstanceInstantiateDuplicateArgName) {
  // (core module $M)    ;; empty core module
  // (core instance (instantiate $M (with "m" (instance 0))
  //                                (with "m" (instance 0))))  ;; FAIL: dup arg
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreModuleSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreInstanceSection>();
  auto &CoreInstSec =
      std::get<AST::Component::CoreInstanceSection>(Comp.getSections()[1]);

  CoreInstSec.getContent().emplace_back();
  AST::Component::InstantiateArg<uint32_t> Arg1;
  Arg1.getName() = "m";
  Arg1.getIndex() = 0;
  AST::Component::InstantiateArg<uint32_t> Arg2;
  Arg2.getName() = "m";
  Arg2.getIndex() = 0;
  CoreInstSec.getContent().back().setInstantiateArgs(
      0U, {std::move(Arg1), std::move(Arg2)});

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, InstanceInstantiateDuplicateArgName) {
  // (component $C (import "f" (func)))
  // (import "g" (func))
  // (instance (instantiate $C (with "f" (func 0))
  //                           (with "f" (func 0))))  ;; FAIL: dup arg
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ComponentSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::InstanceSection>();
  auto &CompSec =
      std::get<AST::Component::ComponentSection>(Comp.getSections()[0]);
  auto &ImpSec = std::get<AST::Component::ImportSection>(Comp.getSections()[1]);
  auto &InstSec =
      std::get<AST::Component::InstanceSection>(Comp.getSections()[2]);

  // Inner component with `(import "f" (func))`.
  CompSec.getContent() = std::make_unique<AST::Component::Component>();
  CompSec.getContent()->getSections().emplace_back();
  CompSec.getContent()
      ->getSections()
      .back()
      .emplace<AST::Component::ImportSection>();
  auto &InnerImpSec = std::get<AST::Component::ImportSection>(
      CompSec.getContent()->getSections().back());
  InnerImpSec.getContent().emplace_back();
  InnerImpSec.getContent().back().getName() = "f";
  InnerImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  // Outer `(import "g" (func))` -> func 0.
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "g";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  // Instantiate component 0 with two args named "f".
  InstSec.getContent().emplace_back();
  AST::Component::InstantiateArg<AST::Component::SortIndex> Arg1;
  Arg1.getName() = "f";
  Arg1.getIndex().getSort().setSortType(AST::Component::Sort::SortType::Func);
  Arg1.getIndex().setIdx(0);
  AST::Component::InstantiateArg<AST::Component::SortIndex> Arg2;
  Arg2.getName() = "f";
  Arg2.getIndex().getSort().setSortType(AST::Component::Sort::SortType::Func);
  Arg2.getIndex().setIdx(0);
  InstSec.getContent().back().setInstantiateArgs(
      0U, {std::move(Arg1), std::move(Arg2)});

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, InstanceInlineExportDuplicateName) {
  // (import "g" (func))                       ;; func 0
  // (instance (export "x" (func 0))
  //           (export "x" (func 0)))           ;; FAIL: duplicate export name
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::InstanceSection>();
  auto &ImpSec = std::get<AST::Component::ImportSection>(Comp.getSections()[0]);
  auto &InstSec =
      std::get<AST::Component::InstanceSection>(Comp.getSections()[1]);

  // import "g" (func) -> func 0
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "g";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  // Inline-export instance with two exports both named "x".
  AST::Component::InlineExport E1;
  E1.getName() = "x";
  E1.getSortIdx().getSort().setSortType(AST::Component::Sort::SortType::Func);
  E1.getSortIdx().setIdx(0);
  AST::Component::InlineExport E2;
  E2.getName() = "x";
  E2.getSortIdx().getSort().setSortType(AST::Component::Sort::SortType::Func);
  E2.getSortIdx().setIdx(0);
  InstSec.getContent().emplace_back();
  InstSec.getContent().back().setInlineExports({std::move(E1), std::move(E2)});

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

// =============================================================================
// Canonical built-in validation tests (GAP-C-1 .. GAP-C-5)
// =============================================================================

// Helper: build a Component with a single ResourceType in type index 0.
inline AST::Component::Component makeCompWithLocalResource() {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setResourceType(AST::Component::ResourceType{});
  return Comp;
}

// Helper: append a CanonSection with a single canonical to the component.
inline AST::Component::CanonSection &
appendCanonSection(AST::Component::Component &Comp) {
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CanonSection>();
  return std::get<AST::Component::CanonSection>(Comp.getSections().back());
}

inline AST::Component::CanonOpt mkOpt(ComponentCanonOptCode Code,
                                      uint32_t Idx = 0) {
  AST::Component::CanonOpt O;
  O.setCode(Code);
  O.setIndex(Idx);
  return O;
}

// Helper: build a Component with an imported component func (func index 0).
inline AST::Component::Component makeCompWithImportedFunc() {
  AST::Component::Component Comp;
  // Need a FuncType at type index 0.
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(AST::Component::FuncType());
  // Import a func of that type.
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "f";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);
  return Comp;
}

// Helper: build a Component with a resource (type 0), a FuncType (type 1),
// and a core func allocated via resource.new 0 (core func index 0).
inline AST::Component::Component makeCompWithCoreFuncAndFuncType() {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  // Type 0: ResourceType (local).
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setResourceType(AST::Component::ResourceType{});
  // Type 1: FuncType.
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(AST::Component::FuncType());
  // Canon section: allocate core func 0 via resource.new 0.
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CanonSection>();
  auto &CanonSec =
      std::get<AST::Component::CanonSection>(Comp.getSections().back());
  AST::Component::Canonical ResNew;
  ResNew.setOpCode(ComponentCanonOpCode::Resource__new);
  ResNew.setIndex(0); // resource at type 0
  CanonSec.getContent().emplace_back(std::move(ResNew));
  return Comp;
}

TEST(ComponentValidatorTest, CanonResourceNew_OnLocalResource_Passes) {
  auto Comp = makeCompWithLocalResource();
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__new);
  C.setIndex(0); // type index 0 = local resource
  CanonSec.getContent().emplace_back(std::move(C));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceNew_TypeIndexOutOfBounds_Fails) {
  AST::Component::Component Comp;
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__new);
  C.setIndex(0); // no type section → index 0 is out of bounds
  CanonSec.getContent().emplace_back(std::move(C));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceNew_TypeIsNotResource_Fails) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  // Type 0: FuncType, not a resource.
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(AST::Component::FuncType());

  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__new);
  C.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(C));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceNew_OnImportedResource_Fails) {
  // resource.new requires a LOCAL resource; imported resources are rejected.
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "r";
  ImpSec.getContent().back().getDesc().setTypeBound(); // (sub resource)
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__new);
  C.setIndex(0); // imported resource at type 0
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceRep_OnLocalResource_Passes) {
  auto Comp = makeCompWithLocalResource();
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__rep);
  C.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceRep_TypeIndexOutOfBounds_Fails) {
  AST::Component::Component Comp;
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__rep);
  C.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceRep_TypeIsNotResource_Fails) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(AST::Component::FuncType());
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__rep);
  C.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceRep_OnImportedResource_Fails) {
  // resource.rep requires a LOCAL resource; imported resources are rejected.
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "r";
  ImpSec.getContent().back().getDesc().setTypeBound(); // (sub resource)
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__rep);
  C.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceDrop_OnLocalResource_Passes) {
  auto Comp = makeCompWithLocalResource();
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__drop);
  C.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceDrop_OnImportedResource_Passes) {
  // Import a resource via a type import bound to (sub resource).
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "r";
  ImpSec.getContent().back().getDesc().setTypeBound(); // (sub resource)
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__drop);
  C.setIndex(0); // the imported type now occupies type index 0
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceDrop_TypeIsNotResource_Fails) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(AST::Component::FuncType());
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__drop);
  C.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceDrop_TypeIndexOutOfBounds_Fails) {
  AST::Component::Component Comp;
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__drop);
  C.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceDropAsync_OnLocalResource_Passes) {
  auto Comp = makeCompWithLocalResource();
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__drop_async);
  C.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceNew_RejectsOptions) {
  auto Comp = makeCompWithLocalResource();
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__new);
  C.setIndex(0);
  C.setOptions({mkOpt(ComponentCanonOptCode::Encode_UTF8)});
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonResourceDrop_RejectsOptions) {
  auto Comp = makeCompWithLocalResource();
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Resource__drop);
  C.setIndex(0);
  C.setOptions({mkOpt(ComponentCanonOptCode::Memory, 0)});
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLower_ValidFuncIndex_Passes) {
  auto Comp = makeCompWithImportedFunc();
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Lower);
  C.setIndex(0); // component func 0 exists
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLower_FuncIndexOutOfBounds_Fails) {
  AST::Component::Component Comp;
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Lower);
  C.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLower_RejectsPostReturn) {
  auto Comp = makeCompWithImportedFunc();
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Lower);
  C.setIndex(0);
  C.setOptions({mkOpt(ComponentCanonOptCode::PostReturn, 0)});
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLower_RejectsCallback) {
  auto Comp = makeCompWithImportedFunc();
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Lower);
  C.setIndex(0);
  // Async included to satisfy structural rule;
  // site-whitelist should still reject callback on Lower.
  C.setOptions({mkOpt(ComponentCanonOptCode::Async),
                mkOpt(ComponentCanonOptCode::Callback, 0)});
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLower_RejectsAlwaysTaskReturn) {
  auto Comp = makeCompWithImportedFunc();
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Lower);
  C.setIndex(0);
  // Async included to satisfy structural rule;
  // site-whitelist should still reject always-task-return on Lower.
  C.setOptions({mkOpt(ComponentCanonOptCode::Async),
                mkOpt(ComponentCanonOptCode::AlwaysTaskReturn)});
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLower_ReallocWithoutMemory_Fails) {
  auto Comp = makeCompWithImportedFunc();
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Lower);
  C.setIndex(0);
  C.setOptions({mkOpt(ComponentCanonOptCode::Realloc, 0)});
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLift_CoreFuncIndexOutOfBounds_Fails) {
  // No core funcs; index 0 is out of bounds.
  AST::Component::Component Comp;
  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical C;
  C.setOpCode(ComponentCanonOpCode::Lift);
  C.setIndex(0);
  C.setTargetIndex(0);
  CanonSec.getContent().emplace_back(std::move(C));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLift_TypeIndexOutOfBounds_Fails) {
  // Fixture provides core func 0 and types 0 (resource), 1 (func). Target
  // index 2 is out of bounds.
  auto Comp = makeCompWithCoreFuncAndFuncType();
  auto &CanonSec =
      std::get<AST::Component::CanonSection>(Comp.getSections().back());
  AST::Component::Canonical Lift;
  Lift.setOpCode(ComponentCanonOpCode::Lift);
  Lift.setIndex(0);       // core func 0 exists
  Lift.setTargetIndex(2); // no type at index 2
  CanonSec.getContent().emplace_back(std::move(Lift));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLift_TargetIsNotFuncType_Fails) {
  // Target type 0 is a ResourceType, not a FuncType.
  auto Comp = makeCompWithCoreFuncAndFuncType();
  auto &CanonSec =
      std::get<AST::Component::CanonSection>(Comp.getSections().back());
  AST::Component::Canonical Lift;
  Lift.setOpCode(ComponentCanonOpCode::Lift);
  Lift.setIndex(0);
  Lift.setTargetIndex(0); // type 0 is ResourceType
  CanonSec.getContent().emplace_back(std::move(Lift));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLift_Valid_Passes) {
  // Happy path: core func 0 + FuncType at type 1.
  auto Comp = makeCompWithCoreFuncAndFuncType();
  auto &CanonSec =
      std::get<AST::Component::CanonSection>(Comp.getSections().back());
  AST::Component::Canonical Lift;
  Lift.setOpCode(ComponentCanonOpCode::Lift);
  Lift.setIndex(0);
  Lift.setTargetIndex(1); // type 1 is FuncType
  CanonSec.getContent().emplace_back(std::move(Lift));
  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLift_WithPostReturn_Passes) {
  // post-return is a Lift-only option; exercise the happy path.
  auto Comp = makeCompWithCoreFuncAndFuncType();
  auto &CanonSec =
      std::get<AST::Component::CanonSection>(Comp.getSections().back());
  AST::Component::Canonical Lift;
  Lift.setOpCode(ComponentCanonOpCode::Lift);
  Lift.setIndex(0);
  Lift.setTargetIndex(1);
  // post-return points to core func 0 (the resource.new result from the
  // fixture).
  Lift.setOptions({mkOpt(ComponentCanonOptCode::PostReturn, 0)});
  CanonSec.getContent().emplace_back(std::move(Lift));
  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

// =============================================================================
// ExportDecl ExternDesc bounds (GAP-DECL-ED)
// =============================================================================

TEST(ComponentValidatorTest,
     InstanceTypeExportDeclFuncTypeOutOfBoundsRejected) {
  // (type (instance
  //   (export "f" (func (type 99))) ;; FAIL: no type 99 in instance scope
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::InstanceDecl> Decls;
  AST::Component::ExportDecl ExpDecl;
  ExpDecl.getName() = "f";
  ExpDecl.getExternDesc().setFuncTypeIdx(99);
  AST::Component::InstanceDecl D;
  D.setExport(std::move(ExpDecl));
  Decls.push_back(std::move(D));

  AST::Component::InstanceType IT;
  IT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setInstanceType(std::move(IT));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

// =============================================================================
// Start section validation (GAP-S-1)
// =============================================================================

TEST(ComponentValidatorTest, StartFuncIndexOutOfBoundsRejected) {
  // start (func 99) ;; FAIL — no func at index 99
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::StartSection>();
  auto &StartSec =
      std::get<AST::Component::StartSection>(Comp.getSections().back());
  StartSec.getContent().getFunctionIndex() = 99;
  StartSec.getContent().getResult() = 0;

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, StartArgArityMismatchRejected) {
  // type 0: FuncType ([] -> [])
  // import "f" (func (type 0))   ;; func 0
  // start (func 0) (arg 0) result=0   ;; FAIL — func has 0 params but 1 arg
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::StartSection>();

  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(AST::Component::FuncType());

  auto &ImpSec = std::get<AST::Component::ImportSection>(Comp.getSections()[1]);
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "f";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  auto &StartSec =
      std::get<AST::Component::StartSection>(Comp.getSections()[2]);
  StartSec.getContent().getFunctionIndex() = 0;
  StartSec.getContent().getArguments().push_back(0U);
  StartSec.getContent().getResult() = 0;

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, StartValidEmptyFuncPasses) {
  // type 0: FuncType ([] -> [])
  // import "f" (func (type 0))   ;; func 0
  // start (func 0) result=0   ;; PASS
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::StartSection>();

  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(AST::Component::FuncType());

  auto &ImpSec = std::get<AST::Component::ImportSection>(Comp.getSections()[1]);
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "f";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  auto &StartSec =
      std::get<AST::Component::StartSection>(Comp.getSections()[2]);
  StartSec.getContent().getFunctionIndex() = 0;
  StartSec.getContent().getResult() = 0;

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

// =============================================================================
// Annotated-name resource-in-scope (GAP-T-3b)
// =============================================================================

TEST(ComponentValidatorTest, AnnotatedNameMissingResourceRejected) {
  // type 0: FuncType
  // import "[constructor]missing" (func (type 0))   ;; FAIL — no resource
  // "missing"
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();

  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(AST::Component::FuncType());

  auto &ImpSec = std::get<AST::Component::ImportSection>(Comp.getSections()[1]);
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "[constructor]missing";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, AnnotatedNameResourceInScopePasses) {
  // type 0: FuncType
  // import "r" (type (sub resource))    ;; resource "r" → type 1
  // import "[method]r.f" (func (type 0)) ;; PASS — resource "r" is in scope
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();

  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(AST::Component::FuncType());

  auto &ImpSec = std::get<AST::Component::ImportSection>(Comp.getSections()[1]);
  // Resource import.
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "r";
  ImpSec.getContent().back().getDesc().setTypeBound();
  // Annotated method import referencing resource "r".
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "[method]r.f";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

// =============================================================================
// Resource destructor signature (GAP-T-5b)
// =============================================================================

TEST(ComponentValidatorTest, ResourceDestructorSignatureWrongShape) {
  // type 0: resource (no dtor)
  // canon resource.rep 0  -> core:func 0 with signature [i32] -> [i32]
  // type 1: resource (dtor = core:func 0)  ;; FAIL — dtor must be [i32] -> []
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec0 =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  // type 0: resource (no dtor)
  TypeSec0.getContent().emplace_back();
  TypeSec0.getContent().back().setResourceType(AST::Component::ResourceType{});

  // canon resource.rep on type 0 -> core:func 0, signature [i32] -> [i32].
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CanonSection>();
  auto &CanonSec =
      std::get<AST::Component::CanonSection>(Comp.getSections().back());
  AST::Component::Canonical Rep;
  Rep.setOpCode(ComponentCanonOpCode::Resource__rep);
  Rep.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(Rep));

  // type 1: resource with dtor = core:func 0.
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec1 =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  AST::Component::ResourceType BadDtor;
  BadDtor.getDestructor() = 0U;
  TypeSec1.getContent().emplace_back();
  TypeSec1.getContent().back().setResourceType(std::move(BadDtor));

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, ResourceDestructorSignatureCorrect) {
  // type 0: resource (no dtor)
  // canon resource.drop 0  -> core:func 0 with signature [i32] -> []
  // type 1: resource (dtor = core:func 0)  ;; PASS — dtor signature matches
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec0 =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  TypeSec0.getContent().emplace_back();
  TypeSec0.getContent().back().setResourceType(AST::Component::ResourceType{});

  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CanonSection>();
  auto &CanonSec =
      std::get<AST::Component::CanonSection>(Comp.getSections().back());
  AST::Component::Canonical Drop;
  Drop.setOpCode(ComponentCanonOpCode::Resource__drop);
  Drop.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(Drop));

  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec1 =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  AST::Component::ResourceType GoodDtor;
  GoodDtor.getDestructor() = 0U;
  TypeSec1.getContent().emplace_back();
  TypeSec1.getContent().back().setResourceType(std::move(GoodDtor));

  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

// =============================================================================
// Core-export alias tag support (GAP-A-2)
// =============================================================================

TEST(ComponentValidatorTest, CoreAliasCoreExportTagPasses) {
  // (core module $M
  //   (type (func))
  //   (tag (type 0))
  //   (export "t" (tag 0)))
  // (core instance $i (instantiate $M))
  // (alias core export $i "t" (core tag $t))   ;; FAIL pre-fix, PASS post-fix
  Configure ConfTag;
  ConfTag.addProposal(Proposal::Component);
  ConfTag.addProposal(Proposal::ExceptionHandling);

  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreModuleSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreInstanceSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::AliasSection>();

  // Inner core module: `(type (func)) (tag (type 0)) (export "t" (tag 0))`.
  auto &ModSec =
      std::get<AST::Component::CoreModuleSection>(Comp.getSections()[0]);
  auto &Mod = ModSec.getContent();
  // Type 0: empty FunctionType.
  AST::SubType ST;
  ST.getCompositeType().setFunctionType(AST::FunctionType());
  Mod.getTypeSection().getContent().push_back(std::move(ST));
  // Tag 0: type index 0.
  AST::TagType Tag;
  Tag.setTypeIdx(0);
  Mod.getTagSection().getContent().push_back(std::move(Tag));
  // Export "t" (tag 0).
  AST::ExportDesc ED;
  ED.setExternalName("t");
  ED.setExternalType(ExternalType::Tag);
  ED.setExternalIndex(0);
  Mod.getExportSection().getContent().push_back(std::move(ED));

  // Core instance: instantiate module 0 (no args).
  auto &CoreInstSec =
      std::get<AST::Component::CoreInstanceSection>(Comp.getSections()[1]);
  CoreInstSec.getContent().emplace_back();
  CoreInstSec.getContent().back().setInstantiateArgs(
      0U, AST::Component::CoreInstance::InstantiateArgs{});

  // Alias core:export 0 "t" with sort = core:tag.
  auto &AliasSec =
      std::get<AST::Component::AliasSection>(Comp.getSections()[2]);
  AliasSec.getContent().emplace_back();
  auto &A = AliasSec.getContent().back();
  A.setTargetType(AST::Component::Alias::TargetType::CoreExport);
  A.getSort().setIsCore(true);
  A.getSort().setCoreSortType(AST::Component::Sort::CoreSortType::Tag);
  A.setExport(0U, "t");

  Validator::Validator V(ConfTag);
  ASSERT_TRUE(V.validate(Comp));
}

// =============================================================================
// Imported component instantiation arg-checking (GAP-I-1)
// =============================================================================

TEST(ComponentValidatorTest, InstantiateImportedComponentMissingArgRejected) {
  // type 0: (component (import "x" (value bool)))
  // import "C" (component (type 0))   ;; component 0 — imported, no raw AST
  // instance (instantiate 0)          ;; FAIL — no arg supplied for "x"
  //
  // The (value ...) bound is used because it does not reference any type
  // index in the inner component-type scope (which would otherwise need
  // cross-scope resolution to validate). The point of this test is to
  // exercise GAP-I-1: an instantiate of an imported component should
  // surface MissingArgument from the ComponentType-derived import list.
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::InstanceSection>();

  // type 0: ComponentType `(component (import "x" (value bool)))`.
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);
  std::vector<AST::Component::ComponentDecl> Decls;
  AST::Component::ImportDecl ImpDecl;
  ImpDecl.getName() = "x";
  ImpDecl.getExternDesc().setValueBound(
      ComponentValType{ComponentTypeCode::Bool});
  AST::Component::ComponentDecl CD;
  CD.setImport(std::move(ImpDecl));
  Decls.push_back(std::move(CD));
  AST::Component::ComponentType CT;
  CT.setDecl(std::move(Decls));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setComponentType(std::move(CT));

  // import "C" (component (type 0)) — gives component 0.
  auto &ImpSec = std::get<AST::Component::ImportSection>(Comp.getSections()[1]);
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "C";
  ImpSec.getContent().back().getDesc().setComponentTypeIdx(0);

  // instance (instantiate 0) — no args supplied.
  auto &InstSec =
      std::get<AST::Component::InstanceSection>(Comp.getSections()[2]);
  InstSec.getContent().emplace_back();
  InstSec.getContent().back().setInstantiateArgs(
      0U, AST::Component::Instance::InstantiateArgs{});

  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

} // namespace
