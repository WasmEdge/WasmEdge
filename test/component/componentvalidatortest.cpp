#include "common/errinfo.h"
#include "validator/validator_component.h"
#include "vm/vm.h"
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

TEST(Component, LoadAndValidate_TestWasm) {
  Configure Conf{};
  Conf.addProposal(Proposal::Component);
  VM::VM VM{Conf};

  // clang-format off
  std::vector<uint8_t> Vec = {
    0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, 0x07, 0x12, 0x01, 0x42,
    0x02, 0x01, 0x40, 0x00, 0x01, 0x00, 0x04, 0x00, 0x05, 0x68, 0x65, 0x6c,
    0x6c, 0x6f, 0x01, 0x00, 0x0a, 0x11, 0x01, 0x00, 0x0c, 0x6d, 0x79, 0x3a,
    0x64, 0x65, 0x6d, 0x6f, 0x2f, 0x68, 0x6f, 0x73, 0x74, 0x05, 0x00, 0x01,
    0x57, 0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01,
    0x60, 0x00, 0x00, 0x02, 0x16, 0x01, 0x0c, 0x6d, 0x79, 0x3a, 0x64, 0x65,
    0x6d, 0x6f, 0x2f, 0x68, 0x6f, 0x73, 0x74, 0x05, 0x68, 0x65, 0x6c, 0x6c,
    0x6f, 0x00, 0x00, 0x00, 0x2f, 0x09, 0x70, 0x72, 0x6f, 0x64, 0x75, 0x63,
    0x65, 0x72, 0x73, 0x01, 0x0c, 0x70, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73,
    0x65, 0x64, 0x2d, 0x62, 0x79, 0x01, 0x0d, 0x77, 0x69, 0x74, 0x2d, 0x63,
    0x6f, 0x6d, 0x70, 0x6f, 0x6e, 0x65, 0x6e, 0x74, 0x07, 0x30, 0x2e, 0x32,
    0x32, 0x37, 0x2e, 0x31, 0x06, 0x0a, 0x01, 0x01, 0x00, 0x00, 0x05, 0x68,
    0x65, 0x6c, 0x6c, 0x6f, 0x08, 0x05, 0x01, 0x01, 0x00, 0x00, 0x00, 0x02,
    0x1d, 0x02, 0x01, 0x01, 0x05, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x0c, 0x6d, 0x79, 0x3a, 0x64, 0x65, 0x6d, 0x6f, 0x2f,
    0x68, 0x6f, 0x73, 0x74, 0x12, 0x00, 0x00, 0x2f, 0x09, 0x70, 0x72, 0x6f,
    0x64, 0x75, 0x63, 0x65, 0x72, 0x73, 0x01, 0x0c, 0x70, 0x72, 0x6f, 0x63,
    0x65, 0x73, 0x73, 0x65, 0x64, 0x2d, 0x62, 0x79, 0x01, 0x0d, 0x77, 0x69,
    0x74, 0x2d, 0x63, 0x6f, 0x6d, 0x70, 0x6f, 0x6e, 0x65, 0x6e, 0x74, 0x07,
    0x30, 0x2e, 0x32, 0x32, 0x37, 0x2e, 0x31
  };
  // clang-format on
  assertOk(VM.loadWasm(Vec), "failed to load component binary");
  assertOk(VM.validate(), "failed to validate");
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

  AST::Component::InstantiateArg InstArg;
  InstArg.getName() = "g";
  InstArg.getIndex() = SortIdx;

  std::vector<AST::Component::InstantiateArg> Args = {InstArg};
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

  AST::Component::InstantiateArg InstArg;
  InstArg.getName() = "f";
  InstArg.getIndex() = SortIdx;

  std::vector<AST::Component::InstantiateArg> Args = {InstArg};
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
