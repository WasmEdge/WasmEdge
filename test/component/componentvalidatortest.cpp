#include "common/errinfo.h"
#include "validator/validator_component.h"
#include <gtest/gtest.h>

namespace WasmEdge {
namespace Validator {

static AST::Component::ImportSection
createImportSection(const std::vector<AST::Component::Import> &imports);

static std::shared_ptr<AST::Component::Component>
createComponent(const AST::Component::ComponentSection &CompSec);

template <typename T> void assertOk(Expect<T> Res, const char *Message) {
  if (!Res) {
    EXPECT_TRUE(false) << Message;
  }
}

template <typename T> void assertFail(Expect<T> Res, const char *Message) {
  if (Res) {
    FAIL() << Message;
  }
}

static AST::Component::ImportSection
createImportSection(const std::vector<AST::Component::Import> &imports) {
  AST::Component::ImportSection ImportSec;
  ImportSec.getContent().insert(ImportSec.getContent().end(), imports.begin(),
                                imports.end());
  return ImportSec;
}

static std::shared_ptr<AST::Component::Component>
createComponent(const AST::Component::ComponentSection &CompSec) {
  return std::make_shared<AST::Component::Component>(CompSec.getContent());
}
TEST(ComponentValidatorTest, CorrectMatchingMultipleComponents) {
  AST::Component::DescTypeIndex Desc;
  Desc.getIndex() = 0;
  Desc.getKind() = AST::Component::IndexKind::FuncType;

  AST::Component::Import ImportF;
  ImportF.getName() = "f";
  ImportF.getDesc() = Desc;

  AST::Component::Import ImportG;
  ImportG.getName() = "g";
  ImportG.getDesc() = Desc;

  auto ImportSec0 = createImportSection({ImportF});
  auto ImportSec1 = createImportSection({ImportG});

  AST::Component::ComponentSection CompSec0;
  CompSec0.getContent() = std::make_shared<AST::Component::Component>();
  CompSec0.getContent()->getSections().push_back(ImportSec0);

  AST::Component::ComponentSection CompSec1;
  CompSec1.getContent() = std::make_shared<AST::Component::Component>();
  CompSec1.getContent()->getSections().push_back(ImportSec1);

  AST::Component::SortIndex<AST::Component::Sort> SortIdx0;
  SortIdx0.getSort() = AST::Component::SortCase::Func;
  SortIdx0.getSortIdx() = 0;

  AST::Component::InstantiateArg<
      AST::Component::SortIndex<AST::Component::Sort>>
      InstArg0;
  InstArg0.getName() = "f";
  InstArg0.getIndex() = SortIdx0;

  AST::Component::Instantiate Inst0(0, {InstArg0});

  AST::Component::SortIndex<AST::Component::Sort> SortIdx1;
  SortIdx1.getSort() = AST::Component::SortCase::Func;
  SortIdx1.getSortIdx() = 0;

  AST::Component::InstantiateArg<
      AST::Component::SortIndex<AST::Component::Sort>>
      InstArg1;
  InstArg1.getName() = "g";
  InstArg1.getIndex() = SortIdx1;

  AST::Component::Instantiate Inst1(1, {InstArg1});

  AST::Component::InstanceSection InstSec;
  InstSec.getContent().push_back(Inst0);
  InstSec.getContent().push_back(Inst1);

  AST::Component::ComponentSection OuterCompSec;
  OuterCompSec.getContent() = std::make_shared<AST::Component::Component>();
  OuterCompSec.getContent()->getSections().push_back(CompSec0);
  OuterCompSec.getContent()->getSections().push_back(CompSec1);
  OuterCompSec.getContent()->getSections().push_back(InstSec);

  auto C0 = createComponent(OuterCompSec);

  WasmEdge::Configure Conf;
  WasmEdge::Validator::Validator Validator(Conf);

  assertOk(Validator.validate(*C0),
           "Validation should pass for multiple correctly matched components");
}

TEST(ComponentValidatorTest, NestedCorrectMatching) {
  AST::Component::DescTypeIndex Desc;
  Desc.getIndex() = 0;
  Desc.getKind() = AST::Component::IndexKind::FuncType;

  AST::Component::Import ImportF;
  ImportF.getName() = "f";
  ImportF.getDesc() = Desc;

  auto ImportSec = createImportSection({ImportF});

  AST::Component::ComponentSection CompSec;
  CompSec.getContent() = std::make_shared<AST::Component::Component>();
  CompSec.getContent()->getSections().push_back(ImportSec);

  AST::Component::SortIndex<AST::Component::Sort> SortIdx;
  SortIdx.getSort() = AST::Component::SortCase::Func;
  SortIdx.getSortIdx() = 0;

  AST::Component::InstantiateArg<
      AST::Component::SortIndex<AST::Component::Sort>>
      InstArg;
  InstArg.getName() = "f";
  InstArg.getIndex() = SortIdx;

  std::vector<AST::Component::InstantiateArg<
      AST::Component::SortIndex<AST::Component::Sort>>>
      Args = {InstArg};
  AST::Component::Instantiate Inst(0, std::move(Args));

  AST::Component::InstanceSection InstSec;
  InstSec.getContent().push_back(Inst);

  AST::Component::ComponentSection OuterCompSec;
  OuterCompSec.getContent() = std::make_shared<AST::Component::Component>();
  OuterCompSec.getContent()->getSections().push_back(CompSec);
  OuterCompSec.getContent()->getSections().push_back(InstSec);

  auto C0 = createComponent(OuterCompSec);

  WasmEdge::Configure Conf;
  WasmEdge::Validator::Validator Validator(Conf);

  assertOk(Validator.validate(*C0),
           "Validation should pass for correct matching");
}

TEST(ComponentValidatorTest, MissingArgument) {
  AST::Component::DescTypeIndex Desc;
  Desc.getIndex() = 0;
  Desc.getKind() = AST::Component::IndexKind::FuncType;

  AST::Component::Import ImportF;
  ImportF.getName() = "f";
  ImportF.getDesc() = Desc;

  auto ImportSec = createImportSection({ImportF});

  AST::Component::ComponentSection CompSec;
  CompSec.getContent() = std::make_shared<AST::Component::Component>();
  CompSec.getContent()->getSections().push_back(ImportSec);

  auto C1 = createComponent(CompSec);

  AST::Component::SortIndex<AST::Component::Sort> SortIdx;
  SortIdx.getSort() = AST::Component::SortCase::Func;
  SortIdx.getSortIdx() = 0;

  AST::Component::InstantiateArg<
      AST::Component::SortIndex<AST::Component::Sort>>
      InstArg;
  InstArg.getName() = "g";
  InstArg.getIndex() = SortIdx;

  std::vector<AST::Component::InstantiateArg<
      AST::Component::SortIndex<AST::Component::Sort>>>
      Args = {InstArg};
  AST::Component::Instantiate Inst(0, std::move(Args));

  AST::Component::InstanceSection InstSec;
  InstSec.getContent().push_back(Inst);

  AST::Component::ComponentSection OuterCompSec;
  OuterCompSec.getContent() = std::make_shared<AST::Component::Component>();
  OuterCompSec.getContent()->getSections().push_back(CompSec);
  OuterCompSec.getContent()->getSections().push_back(InstSec);

  auto C0 = createComponent(OuterCompSec);

  WasmEdge::Configure Conf;
  WasmEdge::Validator::Validator Validator(Conf);

  assertFail(Validator.validate(*C0),
             "Validation should fail due to missing argument 'f' but got 'g'");
}

TEST(ComponentValidatorTest, TypeMismatch) {
  AST::Component::DescTypeIndex Desc;
  Desc.getIndex() = 0;
  Desc.getKind() = AST::Component::IndexKind::FuncType;

  AST::Component::Import ImportF;
  ImportF.getName() = "f";
  ImportF.getDesc() = Desc;

  auto ImportSec = createImportSection({ImportF});

  AST::Component::ComponentSection CompSec;
  CompSec.getContent() = std::make_shared<AST::Component::Component>();
  CompSec.getContent()->getSections().push_back(ImportSec);

  auto C1 = createComponent(CompSec);

  AST::Component::SortIndex<AST::Component::Sort> SortIdx;
  SortIdx.getSort() = AST::Component::SortCase::Component;
  SortIdx.getSortIdx() = 0;

  AST::Component::InstantiateArg<
      AST::Component::SortIndex<AST::Component::Sort>>
      InstArg;
  InstArg.getName() = "f";
  InstArg.getIndex() = SortIdx;

  std::vector<AST::Component::InstantiateArg<
      AST::Component::SortIndex<AST::Component::Sort>>>
      Args = {InstArg};
  AST::Component::Instantiate Inst(0, std::move(Args));

  AST::Component::InstanceSection InstSec;
  InstSec.getContent().push_back(Inst);

  AST::Component::ComponentSection OuterCompSec;
  OuterCompSec.getContent() = std::make_shared<AST::Component::Component>();
  OuterCompSec.getContent()->getSections().push_back(CompSec);
  OuterCompSec.getContent()->getSections().push_back(InstSec);

  auto C0 = createComponent(OuterCompSec);

  WasmEdge::Configure Conf;
  WasmEdge::Validator::Validator Validator(Conf);

  assertFail(Validator.validate(*C0),
             "Validation should fail due to type mismatch");
}

} // namespace Validator
} // namespace WasmEdge
