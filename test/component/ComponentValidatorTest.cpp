// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "ast/component/component.h"
#include "ast/component/type.h"
#include "validator/validator.h"
#include "vm/vm.h"

#include <gtest/gtest.h>

namespace {

using namespace WasmEdge;
using namespace std::literals;

Configure Conf = []() {
  Configure C;
  C.addProposal(Proposal::Component);
  return C;
}();

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

  // Type 1: (borrow 0)
  TypeSec.getContent().emplace_back();
  AST::Component::DefValType BorrowDVT;
  BorrowDVT.setBorrow(AST::Component::BorrowTy{0});
  TypeSec.getContent().back().setDefValType(std::move(BorrowDVT));

  // Type 2: func with borrow in result
  TypeSec.getContent().emplace_back();
  AST::Component::FuncType FT;
  std::vector<AST::Component::LabelValType> Results;
  Results.emplace_back(ComponentValType(1));
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
  //   (export "foo" (type (sub resource)))       ;; local type 0
  //   (type (own 0))                             ;; local type 1
  //   (type (func (result (own 0))))             ;; local type 2
  //   (export "[constructor]foo" (func (type 2))) ;; OK: strongly-unique pair
  // ))
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());

  std::vector<AST::Component::InstanceDecl> Decls;
  {
    AST::Component::ExportDecl Exp;
    Exp.getName() = "foo";
    Exp.getExternDesc().setTypeBound();
    AST::Component::InstanceDecl D;
    D.setExport(std::move(Exp));
    Decls.push_back(std::move(D));
  }
  {
    auto DT = std::make_unique<AST::Component::DefType>();
    AST::Component::DefValType OwnDVT;
    OwnDVT.setOwn(AST::Component::OwnTy{0});
    DT->setDefValType(std::move(OwnDVT));
    AST::Component::InstanceDecl D;
    D.setType(std::move(DT));
    Decls.push_back(std::move(D));
  }
  {
    auto DT = std::make_unique<AST::Component::DefType>();
    AST::Component::FuncType FT;
    FT.setResultList(ComponentValType(1));
    DT->setFuncType(std::move(FT));
    AST::Component::InstanceDecl D;
    D.setType(std::move(DT));
    Decls.push_back(std::move(D));
  }
  {
    AST::Component::ExportDecl Exp;
    Exp.getName() = "[constructor]foo";
    Exp.getExternDesc().setFuncTypeIdx(2);
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
  // Type 1: FuncType (param "p" u32) (result u32). Its lift flattening
  // (flatten_functype($opts, $ft, 'lift'), CanonicalABI.md L3562) is
  // [i32] -> [i32], matching the resource.new core func used as the lift
  // $callee below.
  AST::Component::FuncType FT;
  std::vector<AST::Component::LabelValType> Params;
  Params.emplace_back("p", ComponentValType(ComponentTypeCode::U32));
  FT.setParamList(std::move(Params));
  FT.setResultList(ComponentValType(ComponentTypeCode::U32));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(std::move(FT));
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
  // post-return is a Lift-only option; exercise the happy path. Target type 1
  // lift-flattens to [i32] -> [i32], so spec L3564 requires post-return to have
  // type (func (param i32)), i.e. [i32] -> []. resource.drop 0 produces exactly
  // such a core func at index 1.
  auto Comp = makeCompWithCoreFuncAndFuncType();
  auto &CanonSec =
      std::get<AST::Component::CanonSection>(Comp.getSections().back());
  AST::Component::Canonical Drop;
  Drop.setOpCode(ComponentCanonOpCode::Resource__drop);
  Drop.setIndex(0); // resource at type 0 -> core func 1, sig [i32] -> []
  CanonSec.getContent().emplace_back(std::move(Drop));
  AST::Component::Canonical Lift;
  Lift.setOpCode(ComponentCanonOpCode::Lift);
  Lift.setIndex(0); // $callee = core func 0 (resource.new), sig [i32] -> [i32]
  Lift.setTargetIndex(1);
  Lift.setOptions({mkOpt(ComponentCanonOptCode::PostReturn, 1)});
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
  // import "r" (type (sub resource))          ;; resource "r" → type 0
  // type 1: (borrow 0)
  // type 2: (func (param "self" (borrow 0)))
  // import "[method]r.f" (func (type 2))      ;; PASS — proper method shape
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();

  auto &ImpSec0 =
      std::get<AST::Component::ImportSection>(Comp.getSections()[0]);
  ImpSec0.getContent().emplace_back();
  ImpSec0.getContent().back().getName() = "r";
  ImpSec0.getContent().back().getDesc().setTypeBound();

  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[1]);
  TypeSec.getContent().emplace_back();
  AST::Component::DefValType BorrowDVT;
  BorrowDVT.setBorrow(AST::Component::BorrowTy{0});
  TypeSec.getContent().back().setDefValType(std::move(BorrowDVT));
  TypeSec.getContent().emplace_back();
  AST::Component::FuncType MethodFT;
  std::vector<AST::Component::LabelValType> Params;
  Params.emplace_back("self"s, ComponentValType(1));
  MethodFT.setParamList(std::move(Params));
  TypeSec.getContent().back().setFuncType(std::move(MethodFT));

  auto &ImpSec1 =
      std::get<AST::Component::ImportSection>(Comp.getSections()[2]);
  ImpSec1.getContent().emplace_back();
  ImpSec1.getContent().back().getName() = "[method]r.f";
  ImpSec1.getContent().back().getDesc().setFuncTypeIdx(2);

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
  // surface the missing-argument diagnostic from the ComponentType-derived
  // import list.
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

// =============================================================================
// Core instance memory index-type checking on instantiation (GAP-CI-1)
// =============================================================================

namespace {
// Builds:
//   (core module $A (import "" "" (memory 1)))   ;; imports a 32-bit memory
//   (core module $B (memory (export "") <mem>))  ;; exports `Mem`
//   (core instance $b (instantiate $B))
//   (core instance $a (instantiate $A (with "" (instance $b))))
// so the provided memory's index type is checked against $A's import.
AST::Component::Component buildMemoryLinkComponent(const AST::MemoryType &Mem) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreModuleSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreModuleSection>();
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreInstanceSection>();

  // Module 0 ($A): import a 32-bit memory.
  auto &ModA =
      std::get<AST::Component::CoreModuleSection>(Comp.getSections()[0])
          .getContent();
  AST::ImportDesc ImpA;
  ImpA.setModuleName("");
  ImpA.setExternalName("");
  ImpA.setExternalType(ExternalType::Memory);
  ImpA.getExternalMemoryType() = AST::MemoryType(AST::Limit(1));
  ModA.getImportSection().getContent().push_back(std::move(ImpA));

  // Module 1 ($B): define and export `Mem`.
  auto &ModB =
      std::get<AST::Component::CoreModuleSection>(Comp.getSections()[1])
          .getContent();
  ModB.getMemorySection().getContent().push_back(Mem);
  AST::ExportDesc EDB;
  EDB.setExternalName("");
  EDB.setExternalType(ExternalType::Memory);
  EDB.setExternalIndex(0);
  ModB.getExportSection().getContent().push_back(std::move(EDB));

  // Core instance 0: instantiate module 1 ($B).
  auto &CoreInstSec =
      std::get<AST::Component::CoreInstanceSection>(Comp.getSections()[2]);
  CoreInstSec.getContent().emplace_back();
  CoreInstSec.getContent().back().setInstantiateArgs(
      1U, AST::Component::CoreInstance::InstantiateArgs{});
  // Core instance 1: instantiate module 0 ($A) with instance 0 as arg "".
  AST::Component::InstantiateArg<uint32_t> Arg;
  Arg.getName() = "";
  Arg.getIndex() = 0U;
  CoreInstSec.getContent().emplace_back();
  CoreInstSec.getContent().back().setInstantiateArgs(0U, {Arg});
  return Comp;
}
} // namespace

TEST(ComponentValidatorTest, CoreInstanceMemoryIndexTypeMismatchRejected) {
  // Provide a 64-bit memory where a 32-bit memory is imported -> reject.
  auto Comp = buildMemoryLinkComponent(AST::MemoryType(AST::Limit(1, true)));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CoreInstanceMemoryIndexTypeMatchAccepted) {
  // Provide a 32-bit memory matching the 32-bit import -> accept.
  auto Comp = buildMemoryLinkComponent(AST::MemoryType(AST::Limit(1)));
  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

// Helper: build a Component whose type 1 is `(func (param "s" string))` —
// triggers the spec's `lift(T)` and `lower(T)` realloc/memory rules
// (CanonicalABI.md L3273-3277).
inline AST::Component::Component makeCompWithStringParamFunc() {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  // Type 0: ResourceType (local) — used to allocate core func 0.
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setResourceType(AST::Component::ResourceType{});
  // Type 1: FuncType with a string param.
  AST::Component::FuncType FT;
  std::vector<AST::Component::LabelValType> Params;
  Params.emplace_back("s", ComponentValType(ComponentTypeCode::String));
  FT.setParamList(std::move(Params));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(std::move(FT));
  // Canon section: register core func 0 via resource.new 0 so canon lift has
  // a target.
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CanonSection>();
  auto &CanonSec =
      std::get<AST::Component::CanonSection>(Comp.getSections().back());
  AST::Component::Canonical ResNew;
  ResNew.setOpCode(ComponentCanonOpCode::Resource__new);
  ResNew.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(ResNew));
  return Comp;
}

TEST(ComponentValidatorTest, CanonLift_StringParamRequiresRealloc_Fails) {
  // Spec L3293: `lift(param)` for a list/string-containing T requires
  // 'realloc'. The lift below omits realloc and so must be rejected by the
  // validator (previously only caught at instantiate time).
  auto Comp = makeCompWithStringParamFunc();
  auto &CanonSec =
      std::get<AST::Component::CanonSection>(Comp.getSections().back());
  AST::Component::Canonical Lift;
  Lift.setOpCode(ComponentCanonOpCode::Lift);
  Lift.setIndex(0);
  Lift.setTargetIndex(1);
  CanonSec.getContent().emplace_back(std::move(Lift));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CanonLower_StringParamRequiresMemory_Fails) {
  // Spec L3520: `lower(param)` for a list/string-containing T requires
  // 'memory'. Set up an imported component func with a string param and
  // canon-lower it without supplying 'memory'.
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  AST::Component::FuncType FT;
  std::vector<AST::Component::LabelValType> Params;
  Params.emplace_back("s", ComponentValType(ComponentTypeCode::String));
  FT.setParamList(std::move(Params));
  TypeSec.getContent().emplace_back();
  TypeSec.getContent().back().setFuncType(std::move(FT));
  // Import the func at index 0 so getFunc(0) returns the FuncType.
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "f";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  auto &CanonSec = appendCanonSection(Comp);
  AST::Component::Canonical Lower;
  Lower.setOpCode(ComponentCanonOpCode::Lower);
  Lower.setIndex(0);
  CanonSec.getContent().emplace_back(std::move(Lower));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

// =============================================================================
// End-to-end coverage tests for the validator's canon-lift option checks.
// Existing AST-based tests above hit the validator logic directly; these
// load real binaries through the loader so the loader→validator integration
// is exercised as well. .wat sources embedded as comments.

// === canon lift result spills but no memory ===
// (component
//   ;; (canon lift) with a result type that needs memory (tuple of two u32 →
//   ;; indirect-return) but the lift omits the `(memory ...)` option. The
//   ;; validator's flatten-derived check (component_validator.cpp L1412) must
//   ;; reject this with InvalidCanonOption.
//   (core module $m
//     (memory (export "mem") 1)
//     (func (export "g") (result i32) i32.const 16)
//     (func (export "realloc") (param i32 i32 i32 i32) (result i32) i32.const
//     256))
//   (core instance $i (instantiate $m))
//   (alias core export $i "realloc" (core func $r))
//   (alias core export $i "g" (core func $g))
//   (type $tup (tuple u32 u32))
//   (type $ft (func (result $tup)))
//   (func (export "g") (type $ft)
//     (canon lift (core func $g) (realloc $r))))
static const std::vector<uint8_t> validator_no_memory_wasm = {
    0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, 0x01, 0x51, 0x00, 0x61,
    0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0d, 0x02, 0x60, 0x00, 0x01,
    0x7f, 0x60, 0x04, 0x7f, 0x7f, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x03, 0x02,
    0x00, 0x01, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07, 0x15, 0x03, 0x03, 0x6d,
    0x65, 0x6d, 0x02, 0x00, 0x01, 0x67, 0x00, 0x00, 0x07, 0x72, 0x65, 0x61,
    0x6c, 0x6c, 0x6f, 0x63, 0x00, 0x01, 0x0a, 0x0c, 0x02, 0x04, 0x00, 0x41,
    0x10, 0x0b, 0x05, 0x00, 0x41, 0x80, 0x02, 0x0b, 0x00, 0x09, 0x04, 0x6e,
    0x61, 0x6d, 0x65, 0x00, 0x02, 0x01, 0x6d, 0x02, 0x04, 0x01, 0x00, 0x00,
    0x00, 0x06, 0x13, 0x02, 0x00, 0x00, 0x01, 0x00, 0x07, 0x72, 0x65, 0x61,
    0x6c, 0x6c, 0x6f, 0x63, 0x00, 0x00, 0x01, 0x00, 0x01, 0x67, 0x07, 0x09,
    0x02, 0x6f, 0x02, 0x79, 0x79, 0x40, 0x00, 0x00, 0x00, 0x08, 0x08, 0x01,
    0x00, 0x00, 0x01, 0x01, 0x04, 0x00, 0x01, 0x0b, 0x07, 0x01, 0x00, 0x01,
    0x67, 0x01, 0x00, 0x00, 0x00, 0x37, 0x0e, 0x63, 0x6f, 0x6d, 0x70, 0x6f,
    0x6e, 0x65, 0x6e, 0x74, 0x2d, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x09, 0x00,
    0x00, 0x02, 0x00, 0x01, 0x72, 0x01, 0x01, 0x67, 0x01, 0x06, 0x00, 0x11,
    0x01, 0x00, 0x01, 0x6d, 0x01, 0x06, 0x00, 0x12, 0x01, 0x00, 0x01, 0x69,
    0x01, 0x0b, 0x03, 0x02, 0x00, 0x03, 0x74, 0x75, 0x70, 0x01, 0x02, 0x66,
    0x74,
};

// === canon lift string param but no realloc ===
// (component
//   ;; Symmetric to validator_no_memory.wat — a (canon lift) whose param
//   ;; type contains a string (forcing the spill to realloc) but omits the
//   ;; `(realloc ...)` option. Should be rejected with InvalidCanonOption.
//   (core module $m
//     (memory (export "mem") 1)
//     (func (export "sink") (param i32 i32) (result i32) local.get 1)
//     (func (export "realloc") (param i32 i32 i32 i32) (result i32) i32.const
//     256))
//   (core instance $i (instantiate $m))
//   (alias core export $i "mem" (core memory $mem))
//   (alias core export $i "sink" (core func $g))
//   (type $ft (func (param "s" string) (result u32)))
//   (func (export "sink") (type $ft)
//     (canon lift (core func $g) (memory $mem))))
static const std::vector<uint8_t> validator_no_realloc_wasm = {
    0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, 0x01, 0x56, 0x00, 0x61,
    0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x02, 0x60, 0x02, 0x7f,
    0x7f, 0x01, 0x7f, 0x60, 0x04, 0x7f, 0x7f, 0x7f, 0x7f, 0x01, 0x7f, 0x03,
    0x03, 0x02, 0x00, 0x01, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07, 0x18, 0x03,
    0x03, 0x6d, 0x65, 0x6d, 0x02, 0x00, 0x04, 0x73, 0x69, 0x6e, 0x6b, 0x00,
    0x00, 0x07, 0x72, 0x65, 0x61, 0x6c, 0x6c, 0x6f, 0x63, 0x00, 0x01, 0x0a,
    0x0c, 0x02, 0x04, 0x00, 0x20, 0x01, 0x0b, 0x05, 0x00, 0x41, 0x80, 0x02,
    0x0b, 0x00, 0x09, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x00, 0x02, 0x01, 0x6d,
    0x02, 0x04, 0x01, 0x00, 0x00, 0x00, 0x06, 0x12, 0x02, 0x00, 0x02, 0x01,
    0x00, 0x03, 0x6d, 0x65, 0x6d, 0x00, 0x00, 0x01, 0x00, 0x04, 0x73, 0x69,
    0x6e, 0x6b, 0x07, 0x08, 0x01, 0x40, 0x01, 0x01, 0x73, 0x73, 0x00, 0x79,
    0x08, 0x08, 0x01, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x0b, 0x0a,
    0x01, 0x00, 0x04, 0x73, 0x69, 0x6e, 0x6b, 0x01, 0x00, 0x00, 0x00, 0x39,
    0x0e, 0x63, 0x6f, 0x6d, 0x70, 0x6f, 0x6e, 0x65, 0x6e, 0x74, 0x2d, 0x6e,
    0x61, 0x6d, 0x65, 0x01, 0x06, 0x00, 0x00, 0x01, 0x00, 0x01, 0x67, 0x01,
    0x08, 0x00, 0x02, 0x01, 0x00, 0x03, 0x6d, 0x65, 0x6d, 0x01, 0x06, 0x00,
    0x11, 0x01, 0x00, 0x01, 0x6d, 0x01, 0x06, 0x00, 0x12, 0x01, 0x00, 0x01,
    0x69, 0x01, 0x06, 0x03, 0x01, 0x00, 0x02, 0x66, 0x74,
};

// canon lift whose result spills into the return area but omits 'memory'.
// The validator must reject this end-to-end.
TEST(ComponentValidatorTest, EndToEnd_CanonLift_NoMemoryRejected) {
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(validator_no_memory_wasm));
  EXPECT_FALSE(VM.validate());
}

// canon lift whose string param forces a realloc but omits 'realloc'.
TEST(ComponentValidatorTest, EndToEnd_CanonLift_NoReallocRejected) {
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(validator_no_realloc_wasm));
  EXPECT_FALSE(VM.validate());
}

// =============================================================================
// Core instance global/table/memory checking via an imported core MODULE TYPE
// (GAP-CI-1). Unlike the raw-module tests above, these drive the imported
// module-type subtype path: the instantiated module's core externs come from a
// CoreModuleType (not an inline AST::Module).
// =============================================================================

namespace {
// Builds a component that links two imported core modules by type:
//   (core type (module (export "g" <ExpDesc>)))      ;; core:type 0 (provider)
//   (core type (module (import "provider" "g" <ImpDesc>)))  ;; core:type 1
//   (import "provider-mod" (core module (type 0)))    ;; core:module 0
//   (import "consumer-mod" (core module (type 1)))    ;; core:module 1
//   (core instance (instantiate 0))                   ;; core instance 0
//   (core instance (instantiate 1 (with "provider" (instance 0))))
// so the provider's exported core extern is subtype-checked against the
// consumer's import on instantiation.
AST::Component::Component
buildCoreModuleLinkComponent(AST::Component::CoreImportDesc ExpDesc,
                             AST::Component::CoreImportDesc ImpDesc) {
  AST::Component::Component Comp;

  // CoreTypeSection with the two module types.
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreTypeSection>();
  auto &CoreTypeSec =
      std::get<AST::Component::CoreTypeSection>(Comp.getSections().back());

  // core:type 0 (provider): (module (export "g" <ExpDesc>))
  std::vector<AST::Component::CoreModuleDecl> Decls0;
  AST::Component::CoreExportDecl CExp;
  CExp.getName() = "g";
  CExp.getImportDesc() = std::move(ExpDesc);
  AST::Component::CoreModuleDecl DE;
  DE.setExport(std::move(CExp));
  Decls0.push_back(std::move(DE));
  CoreTypeSec.getContent().emplace_back();
  CoreTypeSec.getContent().back().setModuleType(std::move(Decls0));

  // core:type 1 (consumer): (module (import "provider" "g" <ImpDesc>))
  std::vector<AST::Component::CoreModuleDecl> Decls1;
  AST::Component::CoreImportDecl CImp;
  CImp.getModuleName() = "provider";
  CImp.getName() = "g";
  CImp.getImportDesc() = std::move(ImpDesc);
  AST::Component::CoreModuleDecl DI;
  DI.setImport(std::move(CImp));
  Decls1.push_back(std::move(DI));
  CoreTypeSec.getContent().emplace_back();
  CoreTypeSec.getContent().back().setModuleType(std::move(Decls1));

  // ImportSection: import the two core modules ascribed to those types.
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "provider-mod";
  ImpSec.getContent().back().getDesc().setCoreTypeIdx(0);
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "consumer-mod";
  ImpSec.getContent().back().getDesc().setCoreTypeIdx(1);

  // CoreInstanceSection: instantiate provider then consumer.
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::CoreInstanceSection>();
  auto &CoreInstSec =
      std::get<AST::Component::CoreInstanceSection>(Comp.getSections().back());
  // Core instance 0: instantiate core module 0 (provider), no args.
  CoreInstSec.getContent().emplace_back();
  CoreInstSec.getContent().back().setInstantiateArgs(
      0U, AST::Component::CoreInstance::InstantiateArgs{});
  // Core instance 1: instantiate core module 1 (consumer) with the provider.
  AST::Component::InstantiateArg<uint32_t> Arg;
  Arg.getName() = "provider";
  Arg.getIndex() = 0U;
  CoreInstSec.getContent().emplace_back();
  CoreInstSec.getContent().back().setInstantiateArgs(1U, {Arg});
  return Comp;
}

// Helper: build a CoreImportDesc carrying a global type.
AST::Component::CoreImportDesc mkGlobalDesc(ValType VT, ValMut Mut) {
  AST::Component::CoreImportDesc D;
  D.setGlobalType(AST::GlobalType(VT, Mut));
  return D;
}

// Helper: build a CoreImportDesc carrying a table type.
AST::Component::CoreImportDesc mkTableDesc(ValType RefT, uint64_t Min) {
  AST::Component::CoreImportDesc D;
  D.setTableType(AST::TableType(RefT, Min));
  return D;
}

// Helper: build a CoreImportDesc carrying a memory type.
AST::Component::CoreImportDesc mkMemoryDesc(const AST::Limit &L) {
  AST::Component::CoreImportDesc D;
  D.setMemoryType(AST::MemoryType(L));
  return D;
}
} // namespace

TEST(ComponentValidatorTest, CoreInstanceImportGlobalTypeMismatchRejected) {
  // Provider exports (global (mut i64)); consumer imports (global (mut i32)).
  auto Comp =
      buildCoreModuleLinkComponent(mkGlobalDesc(TypeCode::I64, ValMut::Var),
                                   mkGlobalDesc(TypeCode::I32, ValMut::Var));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest,
     CoreInstanceImportGlobalMutabilityMismatchRejected) {
  // Same value type but mutability differs (var vs const) -> reject.
  auto Comp =
      buildCoreModuleLinkComponent(mkGlobalDesc(TypeCode::I32, ValMut::Const),
                                   mkGlobalDesc(TypeCode::I32, ValMut::Var));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CoreInstanceImportGlobalTypeMatchAccepted) {
  // Provider and consumer agree on (global (mut i64)) -> accept.
  auto Comp =
      buildCoreModuleLinkComponent(mkGlobalDesc(TypeCode::I64, ValMut::Var),
                                   mkGlobalDesc(TypeCode::I64, ValMut::Var));
  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CoreInstanceImportTableElemTypeMismatchRejected) {
  // Provider exports a funcref table; consumer imports an externref table.
  auto Comp = buildCoreModuleLinkComponent(mkTableDesc(TypeCode::FuncRef, 1),
                                           mkTableDesc(TypeCode::ExternRef, 1));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CoreInstanceImportTableLimitsMismatchRejected) {
  // Consumer requires min 5; provider only offers min 1 -> reject.
  auto Comp = buildCoreModuleLinkComponent(mkTableDesc(TypeCode::FuncRef, 1),
                                           mkTableDesc(TypeCode::FuncRef, 5));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, CoreInstanceImportTableTypeMatchAccepted) {
  // Provider offers min 5 where consumer requires min 1 -> accept.
  auto Comp = buildCoreModuleLinkComponent(mkTableDesc(TypeCode::FuncRef, 5),
                                           mkTableDesc(TypeCode::FuncRef, 1));
  Validator::Validator V(Conf);
  ASSERT_TRUE(V.validate(Comp));
}

TEST(ComponentValidatorTest,
     CoreInstanceImportMemoryIndexTypeMismatchRejected) {
  // Provider exports a 32-bit memory; consumer imports a 64-bit memory.
  auto Comp = buildCoreModuleLinkComponent(mkMemoryDesc(AST::Limit(1)),
                                           mkMemoryDesc(AST::Limit(1, true)));
  Validator::Validator V(Conf);
  ASSERT_FALSE(V.validate(Comp));
}

TEST(ComponentValidatorTest, ValueNotConsumedAtEndOfComponent) {
  // Import a value, never consume it -> reject
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "val0";
  ImpSec.getContent().back().getDesc().setValueBound(
      ComponentValType(ComponentTypeCode::U32));

  Validator::Validator V(Conf);
  auto Res = V.validate(Comp);
  EXPECT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::ComponentValueNotConsumed);
}

TEST(ComponentValidatorTest, ValueConsumedExactlyOnceByExport) {
  // Import a value, export it once -> accept
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "val0";
  ImpSec.getContent().back().getDesc().setValueBound(
      ComponentValType(ComponentTypeCode::U32));

  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ExportSection>();
  auto &ExpSec =
      std::get<AST::Component::ExportSection>(Comp.getSections().back());
  ExpSec.getContent().emplace_back();
  ExpSec.getContent().back().getName() = "exp0";
  ExpSec.getContent().back().getSortIndex().getSort().setIsCore(false);
  ExpSec.getContent().back().getSortIndex().getSort().setSortType(
      AST::Component::Sort::SortType::Value);
  ExpSec.getContent().back().getSortIndex().setIdx(0);

  Validator::Validator V(Conf);
  auto Res = V.validate(Comp);
  EXPECT_TRUE(Res);
}

TEST(ComponentValidatorTest, ExportConsumesSameValueTwice) {
  // Import a value, export it twice -> reject
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "val0";
  ImpSec.getContent().back().getDesc().setValueBound(
      ComponentValType(ComponentTypeCode::U32));

  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ExportSection>();
  auto &ExpSec =
      std::get<AST::Component::ExportSection>(Comp.getSections().back());

  ExpSec.getContent().emplace_back();
  ExpSec.getContent().back().getName() = "exp0";
  ExpSec.getContent().back().getSortIndex().getSort().setIsCore(false);
  ExpSec.getContent().back().getSortIndex().getSort().setSortType(
      AST::Component::Sort::SortType::Value);
  ExpSec.getContent().back().getSortIndex().setIdx(0);

  ExpSec.getContent().emplace_back();
  ExpSec.getContent().back().getName() = "exp1";
  ExpSec.getContent().back().getSortIndex().getSort().setIsCore(false);
  ExpSec.getContent().back().getSortIndex().getSort().setSortType(
      AST::Component::Sort::SortType::Value);
  ExpSec.getContent().back().getSortIndex().setIdx(0);

  Validator::Validator V(Conf);
  auto Res = V.validate(Comp);
  EXPECT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::ComponentValueAlreadyConsumed);
}

TEST(ComponentValidatorTest, ValueConsumedExactlyOnceByStart) {
  // Import a value, call a start function with that value -> accept
  AST::Component::Component Comp;

  // 1. Define a function type (param: u32, result: none)
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  TypeSec.getContent().emplace_back();
  AST::Component::FuncType FT;
  FT.setParamList({AST::Component::LabelValType(
      "arg0"s, ComponentValType(ComponentTypeCode::U32))});
  TypeSec.getContent().back().setFuncType(std::move(FT));

  // 2. Import section: first import the value, then the function using type 0
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());

  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "val0";
  ImpSec.getContent().back().getDesc().setValueBound(
      ComponentValType(ComponentTypeCode::U32));

  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "func0";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  // 3. Start section calling func0 (funcidx 0) with val0 (valueidx 0)
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::StartSection>();
  auto &StartSec =
      std::get<AST::Component::StartSection>(Comp.getSections().back());
  StartSec.getContent().getFunctionIndex() = 0;
  StartSec.getContent().getArguments() = {0};
  StartSec.getContent().getResult() = 0;

  Validator::Validator V(Conf);
  auto Res = V.validate(Comp);
  EXPECT_TRUE(Res);
}

TEST(ComponentValidatorTest, StartConsumesSameValueTwice) {
  // Import a value, call a start function with two params using same value
  // twice
  // -> reject
  AST::Component::Component Comp;

  // 1. Define a function type (param: u32, u32, result: none)
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  TypeSec.getContent().emplace_back();
  AST::Component::FuncType FT;
  FT.setParamList({AST::Component::LabelValType(
                       "arg0"s, ComponentValType(ComponentTypeCode::U32)),
                   AST::Component::LabelValType(
                       "arg1"s, ComponentValType(ComponentTypeCode::U32))});
  TypeSec.getContent().back().setFuncType(std::move(FT));

  // 2. Import section: first import the value, then the function using type 0
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());

  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "val0";
  ImpSec.getContent().back().getDesc().setValueBound(
      ComponentValType(ComponentTypeCode::U32));

  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "func0";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  // 3. Start section calling func0 (funcidx 0) with args {0, 0}
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::StartSection>();
  auto &StartSec =
      std::get<AST::Component::StartSection>(Comp.getSections().back());
  StartSec.getContent().getFunctionIndex() = 0;
  StartSec.getContent().getArguments() = {0, 0};
  StartSec.getContent().getResult() = 0;

  Validator::Validator V(Conf);
  auto Res = V.validate(Comp);
  EXPECT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::ComponentValueAlreadyConsumed);
}

TEST(ComponentValidatorTest, StartArgumentValueIndexOutOfBounds) {
  // Call a start function whose argument references a value index that does not
  // exist in the (empty) value index space -> reject with InvalidIndex.
  AST::Component::Component Comp;

  // 1. Define a function type (param: u32, result: none).
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec =
      std::get<AST::Component::TypeSection>(Comp.getSections().back());
  TypeSec.getContent().emplace_back();
  AST::Component::FuncType FT;
  FT.setParamList({AST::Component::LabelValType(
      "arg0"s, ComponentValType(ComponentTypeCode::U32))});
  TypeSec.getContent().back().setFuncType(std::move(FT));

  // 2. Import only the function (no value is imported, so the value index
  // space stays empty).
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::ImportSection>();
  auto &ImpSec =
      std::get<AST::Component::ImportSection>(Comp.getSections().back());
  ImpSec.getContent().emplace_back();
  ImpSec.getContent().back().getName() = "func0";
  ImpSec.getContent().back().getDesc().setFuncTypeIdx(0);

  // 3. Start section calling func0 with value index 0, which is out of bounds.
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::StartSection>();
  auto &StartSec =
      std::get<AST::Component::StartSection>(Comp.getSections().back());
  StartSec.getContent().getFunctionIndex() = 0;
  StartSec.getContent().getArguments() = {0};
  StartSec.getContent().getResult() = 0;

  Validator::Validator V(Conf);
  auto Res = V.validate(Comp);
  EXPECT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::InvalidIndex);
}

} // namespace
