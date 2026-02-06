// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/errinfo.h"
#include "validator/validator.h"
#include "vm/vm.h"

#include <gtest/gtest.h>

namespace {

using namespace WasmEdge;

TEST(Component, LoadAndValidate_TestWasm) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM VM(Conf);

  std::vector<uint8_t> Vec = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, // WASM preamble

      0x07, 0x12, 0x01,             // Type section: size 0x12, vector size 1
      0x42, 0x02,                   // TypeSec[0]: instance type, vector size 2
      0x01, 0x40, 0x00, 0x01, 0x00, // InstType[0]: type: functype {}->{}
      0x04,                         // InstType[1]: exportdecl
      0x00, 0x05, 0x68, 0x65, 0x6c, 0x6c, 0x6f, // exportdecl: name "hello"
      0x01, 0x00, // exportdecl: externdesc: func[0]

      0x0a, 0x11, 0x01, // Import section: size 0x11, vector size 1
      0x00, 0x0c,       // ImportSec[0]: import name "my:demo/host"
      0x6d, 0x79, 0x3a, 0x64, 0x65, 0x6d, 0x6f, 0x2f, 0x68, 0x6f, 0x73, 0x74,
      0x05, 0x00, // import externdesc: instance[0]

      0x01, 0x57, // CoreModule section: size 0x57
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, // WASM header
      0x01, 0x04, 0x01, 0x60, 0x00, 0x00, // Type section: {func{}->{}}
      0x02, 0x16, 0x01, 0x0c,             // Import section: vector size 1
      0x6d, 0x79, 0x3a, 0x64, 0x65, 0x6d, 0x6f, 0x2f, 0x68, 0x6f, 0x73, 0x74,
      0x05, 0x68, 0x65, 0x6c, 0x6c, 0x6f, // "my:demo/host" "hello"
      0x00, 0x00,                         // import func[0]
      0x00, 0x2f, 0x09, 0x70, 0x72, 0x6f, 0x64, 0x75, 0x63, 0x65, 0x72, 0x73,
      0x01, 0x0c, 0x70, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x65, 0x64, 0x2d,
      0x62, 0x79, 0x01, 0x0d, 0x77, 0x69, 0x74, 0x2d, 0x63, 0x6f, 0x6d, 0x70,
      0x6f, 0x6e, 0x65, 0x6e, 0x74, 0x07, 0x30, 0x2e, 0x32, 0x32, 0x37, 0x2e,
      0x31, // Custom section

      0x06, 0x0a, 0x01, // Alias section: size 0x0a, vector size 1
      0x01,             // sort: func
      0x00, 0x00,       // target: instance[0]
      0x05, 0x68, 0x65, 0x6c, 0x6c, 0x6f, // target: name "hello"

      0x08, 0x05, 0x01,       // Canon section: size 0x05, vector size 1
      0x01, 0x00, 0x00, 0x00, // canon lower func[0]

      0x02, 0x1d, 0x02, // CoreInstance section: size 0x1d, vector size 2
      0x01, 0x01,       // CoreInstSec[0]: inlineexport
      0x05, 0x68, 0x65, 0x6c, 0x6c, 0x6f, // export name "hello"
      0x00, 0x00,                         // export sort func[0]
      0x00, 0x00, // CoreInstSec[1]: instantiate module[0]
      0x01, 0x0c, 0x6d, 0x79, 0x3a, 0x64, 0x65, 0x6d, 0x6f, 0x2f, 0x68, 0x6f,
      0x73, 0x74, // module name "my:demo/host"
      0x12, 0x00, // instance index 0

      0x00, 0x2f, 0x09, 0x70, 0x72, 0x6f, 0x64, 0x75, 0x63, 0x65, 0x72, 0x73,
      0x01, 0x0c, 0x70, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x65, 0x64, 0x2d,
      0x62, 0x79, 0x01, 0x0d, 0x77, 0x69, 0x74, 0x2d, 0x63, 0x6f, 0x6d, 0x70,
      0x6f, 0x6e, 0x65, 0x6e, 0x74, 0x07, 0x30, 0x2e, 0x32, 0x32, 0x37, 0x2e,
      0x31, // Custom section
  };

  ASSERT_TRUE(VM.loadWasm(Vec));
  ASSERT_TRUE(VM.validate());
}

TEST(ComponentValidatorTest, MissingArgument) {
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
  Arg.getName() = "g";
  Arg.getIndex().getSort().setSortType(AST::Component::Sort::SortType::Func);
  Arg.getIndex().setIdx(0);
  InstSec.getContent().back().setInstantiateArgs(0U, {Arg});

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
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

  WasmEdge::Configure Conf;
  WasmEdge::Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

// The following tests correspond to `assert_invalid` cases from
// `https://github.com/WebAssembly/component-model/blob/main/test/wasm-tools/definedtypes.wast`
// in the official Component Model spec tests. They are expressed using the
// WasmEdge AST to test validator logic directly.

TEST(ComponentValidatorTest, EmptyRecordFieldName) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::RecordTy Record;

  // Add a field with an empty name
  ComponentValType ValTy(ComponentTypeCode::S32);
  AST::Component::LabelValType Field("", ValTy);
  Record.LabelTypes.push_back(Field);

  DVT.setRecord(std::move(Record));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, EmptyVariantCaseName) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::VariantTy Variant;

  ComponentValType ValTy(ComponentTypeCode::S32);

  // Add a case with an empty name
  Variant.Cases.push_back(
      std::make_pair(std::string(""), std::optional<ComponentValType>(ValTy)));

  DVT.setVariant(std::move(Variant));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, EmptyFlagName) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::FlagsTy Flags;

  // Add an empty flag name
  Flags.Labels.push_back("");

  DVT.setFlags(std::move(Flags));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, EmptyEnumTagName) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::EnumTy Enum;

  // Add an empty enum tag name
  Enum.Labels.push_back("");

  DVT.setEnum(std::move(Enum));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, OptionTypeIndexOutOfBounds) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::OptionTy Option;

  // Use type index 0, which is out of bounds (no types defined yet)
  ComponentValType ValTy(0);
  Option.ValTy = ValTy;

  DVT.setOption(std::move(Option));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, ListTypeIndexOutOfBounds) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::ListTy List;

  // Use type index 0, which is out of bounds (no types defined yet)
  ComponentValType ValTy(0);
  List.ValTy = ValTy;

  DVT.setList(std::move(List));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, RecordFieldTypeIndexOutOfBounds) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::RecordTy Record;

  // Use type index 0, which is out of bounds (no types defined yet)
  ComponentValType ValTy(0);
  AST::Component::LabelValType Field("x", ValTy);
  Record.LabelTypes.push_back(Field);

  DVT.setRecord(std::move(Record));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, VariantCaseTypeIndexOutOfBounds) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::VariantTy Variant;

  // Use type index 0, which is out of bounds (no types defined yet)
  ComponentValType ValTy(0);

  Variant.Cases.push_back(
      std::make_pair(std::string("x"), std::optional<ComponentValType>(ValTy)));

  DVT.setVariant(std::move(Variant));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, ResultTypeIndexOutOfBounds) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::ResultTy Result;

  // Use type indices 0 and 1, which are out of bounds (no types defined yet)
  ComponentValType ValTy(0);
  ComponentValType ErrTy(1);
  Result.ValTy = ValTy;
  Result.ErrTy = ErrTy;

  DVT.setResult(std::move(Result));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, TupleTypeIndexOutOfBounds) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::TupleTy Tuple;

  // Use type index 0, which is out of bounds (no types defined yet)
  ComponentValType ValTy(0);
  Tuple.Types.push_back(ValTy);

  DVT.setTuple(std::move(Tuple));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, FuncParamTypeNotDefValType) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  // First, add a func type (type $t (func))
  TypeSec.getContent().emplace_back();
  AST::Component::FuncType FType;
  TypeSec.getContent()[0].setFuncType(std::move(FType));

  // Then, add another func type that tries to use type index 0 as a param
  // (type (func (param "t" $t)))
  TypeSec.getContent().emplace_back();
  AST::Component::FuncType FType2;

  // Use type index 0, which refers to a func type, not a defvaltype
  ComponentValType ValTy(0);
  AST::Component::LabelValType Param("t", ValTy);
  std::vector<AST::Component::LabelValType> Params;
  Params.push_back(Param);
  FType2.setParamList(std::move(Params));

  TypeSec.getContent()[1].setFuncType(std::move(FType2));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, FuncResultTypeNotDefValType) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  // add an instance type (type $t (instance))
  TypeSec.getContent().emplace_back();
  AST::Component::InstanceType IType;
  TypeSec.getContent()[0].setInstanceType(std::move(IType));

  // add a func type that tries to use type index 0 as a result
  // (type (func (result $t)))
  TypeSec.getContent().emplace_back();
  AST::Component::FuncType FType;

  // Use type index 0, which refers to an instance type, not a defvaltype
  ComponentValType ValTy(0);
  AST::Component::LabelValType Result("", ValTy);
  std::vector<AST::Component::LabelValType> Results;
  Results.push_back(Result);
  FType.setResultList(std::move(Results));

  TypeSec.getContent()[1].setFuncType(std::move(FType));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, OptionTypeNotDefValType) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  // First, add a component type (type $t (component))
  TypeSec.getContent().emplace_back();
  AST::Component::ComponentType CType;
  TypeSec.getContent()[0].setComponentType(std::move(CType));

  // Then, add an option type that tries to use type index 0
  // (type (option $t))
  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::OptionTy Option;

  // Use type index 0, which refers to a component type, not a defvaltype
  ComponentValType ValTy(0);
  Option.ValTy = ValTy;

  DVT.setOption(std::move(Option));
  TypeSec.getContent()[1].setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, EmptyVariant) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::VariantTy Variant;

  // Variant.Cases is left empty

  DVT.setVariant(std::move(Variant));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, RecordFieldNameConflictCaseInsensitive) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::RecordTy Record;

  // Add field "a-B-c-D" with type string
  ComponentValType ValTy1(ComponentTypeCode::String);
  AST::Component::LabelValType Field1("a-B-c-D", ValTy1);
  Record.LabelTypes.push_back(Field1);

  // Add field "A-b-C-d" with type u8 (conflicts with "a-B-c-D")
  ComponentValType ValTy2(ComponentTypeCode::U8);
  AST::Component::LabelValType Field2("A-b-C-d", ValTy2);
  Record.LabelTypes.push_back(Field2);

  DVT.setRecord(std::move(Record));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, VariantCaseNameDuplicate) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::VariantTy Variant;

  ComponentValType ValTy(ComponentTypeCode::S64);

  // Add case "x"
  Variant.Cases.push_back(
      std::make_pair(std::string("x"), std::optional<ComponentValType>(ValTy)));

  // Add duplicate case "x"
  Variant.Cases.push_back(
      std::make_pair(std::string("x"), std::optional<ComponentValType>(ValTy)));

  DVT.setVariant(std::move(Variant));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, FlagNameConflictCaseInsensitive) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::FlagsTy Flags;

  // Add flags "x", "y", "X" (where "X" conflicts with "x")
  Flags.Labels.push_back("x");
  Flags.Labels.push_back("y");
  Flags.Labels.push_back("X"); // Conflicts with "x" (case-insensitive)

  DVT.setFlags(std::move(Flags));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

TEST(ComponentValidatorTest, EnumTagNameConflictCaseInsensitive) {
  AST::Component::Component Comp;
  Comp.getSections().emplace_back();
  Comp.getSections().back().emplace<AST::Component::TypeSection>();
  auto &TypeSec = std::get<AST::Component::TypeSection>(Comp.getSections()[0]);

  TypeSec.getContent().emplace_back();
  AST::Component::DefValType DVT;
  AST::Component::EnumTy Enum;

  // Add enum tags "x", "y", "X" (where "X" conflicts with "x")
  Enum.Labels.push_back("x");
  Enum.Labels.push_back("y");
  Enum.Labels.push_back("X"); // Conflicts with "x" (case-insensitive)

  DVT.setEnum(std::move(Enum));
  TypeSec.getContent().back().setDefValType(std::move(DVT));

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Validator(Conf);
  ASSERT_FALSE(Validator.validate(Comp));
}

// Derived from:
// https://github.com/WebAssembly/component-model/blob/main/test/wasm-tools/definedtypes.wast
//
// Converted with wasm-tools into a component binary and embedded here to
// exercise loader + validator end-to-end.

TEST(ComponentValidatorBinaryTest, ValidDefValTypes_FromWasmBinary) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM VM(Conf);

  std::vector<uint8_t> Binary = {
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, 0x07, 0x76, 0x24, 0x7f,
      0x7d, 0x7e, 0x7b, 0x7c, 0x79, 0x7a, 0x77, 0x78, 0x76, 0x75, 0x76, 0x75,
      0x74, 0x73, 0x6f, 0x01, 0x74, 0x72, 0x01, 0x01, 0x78, 0x0f, 0x72, 0x01,
      0x01, 0x78, 0x00, 0x71, 0x01, 0x01, 0x78, 0x00, 0x00, 0x71, 0x01, 0x01,
      0x78, 0x01, 0x00, 0x00, 0x71, 0x03, 0x01, 0x78, 0x00, 0x00, 0x01, 0x79,
      0x01, 0x73, 0x00, 0x01, 0x7a, 0x01, 0x73, 0x00, 0x71, 0x03, 0x01, 0x78,
      0x00, 0x00, 0x01, 0x79, 0x01, 0x73, 0x00, 0x01, 0x7a, 0x01, 0x73, 0x00,
      0x6f, 0x01, 0x7d, 0x70, 0x16, 0x70, 0x02, 0x6f, 0x01, 0x7d, 0x6f, 0x01,
      0x03, 0x6e, 0x01, 0x01, 0x78, 0x6d, 0x01, 0x01, 0x78, 0x6f, 0x01, 0x79,
      0x6b, 0x1d, 0x6b, 0x05, 0x6a, 0x00, 0x00, 0x6a, 0x01, 0x06, 0x00, 0x6a,
      0x00, 0x01, 0x07, 0x6a, 0x01, 0x08, 0x01, 0x09, 0x00, 0xca, 0x01, 0x0e,
      0x63, 0x6f, 0x6d, 0x70, 0x6f, 0x6e, 0x65, 0x6e, 0x74, 0x2d, 0x6e, 0x61,
      0x6d, 0x65, 0x00, 0x02, 0x01, 0x43, 0x01, 0xb4, 0x01, 0x03, 0x21, 0x00,
      0x02, 0x41, 0x31, 0x01, 0x02, 0x41, 0x32, 0x02, 0x02, 0x41, 0x33, 0x03,
      0x02, 0x41, 0x34, 0x04, 0x02, 0x41, 0x35, 0x05, 0x02, 0x41, 0x36, 0x06,
      0x02, 0x41, 0x37, 0x07, 0x02, 0x41, 0x38, 0x08, 0x02, 0x41, 0x39, 0x09,
      0x04, 0x41, 0x31, 0x30, 0x61, 0x0a, 0x04, 0x41, 0x31, 0x31, 0x61, 0x0b,
      0x04, 0x41, 0x31, 0x30, 0x62, 0x0c, 0x04, 0x41, 0x31, 0x31, 0x62, 0x0d,
      0x03, 0x41, 0x31, 0x32, 0x0e, 0x03, 0x41, 0x31, 0x33, 0x10, 0x04, 0x41,
      0x31, 0x34, 0x62, 0x11, 0x04, 0x41, 0x31, 0x34, 0x63, 0x12, 0x04, 0x41,
      0x31, 0x35, 0x61, 0x13, 0x04, 0x41, 0x31, 0x35, 0x62, 0x14, 0x04, 0x41,
      0x31, 0x35, 0x63, 0x15, 0x04, 0x41, 0x31, 0x35, 0x64, 0x17, 0x04, 0x41,
      0x31, 0x36, 0x61, 0x18, 0x04, 0x41, 0x31, 0x36, 0x62, 0x19, 0x04, 0x41,
      0x31, 0x37, 0x61, 0x1a, 0x04, 0x41, 0x31, 0x37, 0x62, 0x1b, 0x04, 0x41,
      0x31, 0x38, 0x62, 0x1c, 0x04, 0x41, 0x31, 0x39, 0x62, 0x1e, 0x04, 0x41,
      0x32, 0x31, 0x61, 0x1f, 0x04, 0x41, 0x32, 0x31, 0x62, 0x20, 0x04, 0x41,
      0x32, 0x32, 0x61, 0x21, 0x04, 0x41, 0x32, 0x32, 0x62, 0x22, 0x04, 0x41,
      0x32, 0x32, 0x63, 0x23, 0x04, 0x41, 0x32, 0x32, 0x64};

  ASSERT_TRUE(VM.loadWasm(Binary));
  ASSERT_TRUE(VM.validate());
}

} // namespace
