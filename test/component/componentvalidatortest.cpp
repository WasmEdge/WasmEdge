#include "common/errinfo.h"
#include "validator/validator_component.h"
#include <gtest/gtest.h>

namespace WasmEdge {
namespace Validator {

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

AST::Component::ImportSection
createImportSection(const std::vector<AST::Component::Import> &imports) {
  AST::Component::ImportSection ImportSec;
  ImportSec.getContent().insert(ImportSec.getContent().end(), imports.begin(),
                                imports.end());
  return ImportSec;
}

std::shared_ptr<AST::Component::Component>
createComponent(const AST::Component::ComponentSection &CompSec) {
  return std::make_shared<AST::Component::Component>(CompSec.getContent());
}

TEST(ComponentValidatorTest, CorrectMatching) {
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

  WasmEdge::Validator::Context Ctx;
  WasmEdge::Configure Conf;
  WasmEdge::Validator::Validator Validator(Conf);

  Ctx.addComponent(*C1);

  WasmEdge::Validator::SectionVisitor Visitor(Validator, Ctx);
  auto Result = Visitor(InstSec);
  assertOk(Result, "Validation should pass for correct matching");
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

  WasmEdge::Validator::Context Ctx;
  WasmEdge::Configure Conf;
  WasmEdge::Validator::Validator Validator(Conf);

  Ctx.addComponent(*C1);

  WasmEdge::Validator::SectionVisitor Visitor(Validator, Ctx);
  auto Result = Visitor(InstSec);
  assertFail(Result,
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

  WasmEdge::Validator::Context Ctx;
  WasmEdge::Configure Conf;
  WasmEdge::Validator::Validator Validator(Conf);

  Ctx.addComponent(*C1);

  WasmEdge::Validator::SectionVisitor Visitor(Validator, Ctx);
  auto Result = Visitor(InstSec);
  assertFail(Result, "Validation should fail due to type mismatch");
}

} // namespace Validator
} // namespace WasmEdge
