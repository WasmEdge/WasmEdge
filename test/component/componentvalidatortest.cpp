// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/errinfo.h"
#include "validator/validator.h"
#include "vm/vm.h"

#include <gtest/gtest.h>

namespace {

using namespace WasmEdge;

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

TEST(Component, LoadAndValidate_TestWasm) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM VM{Conf};

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

  assertOk(VM.loadWasm(Vec), "failed to load component binary");

  // TODO: Fix this for the validator.
  // assertOk(VM.validate(), "failed to validate");
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
  assertFail(Validator.validate(Comp),
             "Validation should fail due to missing argument 'f'");
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
  assertFail(Validator.validate(Comp),
             "Validation should fail due to type mismatch");
}

} // namespace
