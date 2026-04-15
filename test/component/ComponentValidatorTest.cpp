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
// DefValType validation tests (GAP-T-1, GAP-T-2)
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

} // namespace
