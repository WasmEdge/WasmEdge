// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/component/component.h"
#include "ast/component/type.h"
#include "common/configure.h"
#include "common/enum_types.hpp"
#include "common/types.h"
#include "validator/validator.h"

#include <gtest/gtest.h>
#include <vector>

namespace {

using namespace WasmEdge;
using namespace std::literals;

TEST(ComponentValidatorTest, ConstructorAndMethodTypes) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Val(Conf);

  AST::Component::Component Comp;
  AST::Component::TypeSection TypeSec;

  // 1. Define Resource Import FIRST to get Type Index 0
  AST::Component::ImportSection ImportSec;
  {
    AST::Component::Import Im;
    Im.getName() = "my-res";
    AST::Component::ExternDesc Desc;
    // TypeBound (sub resource) -> Index 0
    Desc.setTypeBound();
    Im.getDesc() = std::move(Desc);
    ImportSec.getContent().push_back(std::move(Im));
  }
  Comp.getSections().push_back(std::move(ImportSec));

  // 2. Define Func Types (using Type Index 0 for resource)

  // Func Type for Constructor: (func (result (own 0))) (Index 1)
  {
    WasmEdge::ComponentValType ValTy(ComponentTypeCode::Own, 0);

    AST::Component::FuncType FuncTy;
    FuncTy.setResultList(ValTy);

    AST::Component::DefType DT;
    DT.setFuncType(std::move(FuncTy));
    TypeSec.getContent().push_back(std::move(DT));
  }

  // Func Type for Method: (func (param "self" (borrow 0))) (Index 2)
  {
    WasmEdge::ComponentValType ValTy(ComponentTypeCode::Borrow, 0);
    AST::Component::LabelValType Param("self"s, ValTy);

    AST::Component::FuncType FuncTy;
    std::vector<AST::Component::LabelValType> Params;
    Params.push_back(Param);
    FuncTy.setParamList(std::move(Params));

    AST::Component::DefType DT;
    DT.setFuncType(std::move(FuncTy));
    TypeSec.getContent().push_back(std::move(DT));
  }
  Comp.getSections().push_back(std::move(TypeSec));

  // 3. Define Constructor and Method Imports
  AST::Component::ImportSection ImportSec2;
  // Import 2: The Constructor "[constructor]my-res"
  {
    AST::Component::Import Im;
    Im.getName() = "[constructor]my-res";
    AST::Component::ExternDesc Desc;
    Desc.setFuncTypeIdx(1); // Points to (func (result (own 0)))
    Im.getDesc() = std::move(Desc);
    ImportSec2.getContent().push_back(std::move(Im));
  }

  // Import 3: The Method "[method]my-res.foo"
  {
    AST::Component::Import Im;
    Im.getName() = "[method]my-res.foo";
    AST::Component::ExternDesc Desc;
    Desc.setFuncTypeIdx(2); // Points to (func (param "self" (borrow 0)))
    Im.getDesc() = std::move(Desc);
    ImportSec2.getContent().push_back(std::move(Im));
  }
  Comp.getSections().push_back(std::move(ImportSec2));

  // Validate
  EXPECT_TRUE(Val.validate(Comp));
}

TEST(ComponentValidatorTest, InvalidConstructorType) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Val(Conf);

  AST::Component::Component Comp;
  AST::Component::TypeSection TypeSec;

  // 1. Define Resource Type (Index 0)
  {
    AST::Component::DefType DT;
    DT.setResourceType(AST::Component::ResourceType());
    TypeSec.getContent().push_back(std::move(DT));
  }

  // 2. Define BAD Func Type: (func (result s32)) (Index 1)
  {
    WasmEdge::ComponentValType ValTy(ComponentTypeCode::S32, 0);
    AST::Component::FuncType FuncTy;
    FuncTy.setResultList(ValTy);

    AST::Component::DefType DT;
    DT.setFuncType(std::move(FuncTy));
    TypeSec.getContent().push_back(std::move(DT));
  }

  Comp.getSections().push_back(std::move(TypeSec));

  // Imports
  AST::Component::ImportSection ImportSec;
  // Resource
  {
    AST::Component::Import Im;
    Im.getName() = "my-res";
    AST::Component::ExternDesc Desc;
    Desc.setTypeBound(0);
    Im.getDesc() = std::move(Desc);
    ImportSec.getContent().push_back(std::move(Im));
  }
  // BAD Constructor
  {
    AST::Component::Import Im;
    Im.getName() = "[constructor]my-res";
    AST::Component::ExternDesc Desc;
    Desc.setFuncTypeIdx(1);
    Im.getDesc() = std::move(Desc);
    ImportSec.getContent().push_back(std::move(Im));
  }

  Comp.getSections().push_back(std::move(ImportSec));

  EXPECT_FALSE(Val.validate(Comp));
}

TEST(ComponentValidatorTest, DuplicateMethodName) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Validator::Validator Val(Conf);

  AST::Component::Component Comp;

  // 1. Define Resource Import
  AST::Component::ImportSection ImportSec;
  {
    AST::Component::Import Im;
    Im.getName() = "my-res";
    AST::Component::ExternDesc Desc;
    Desc.setTypeBound();
    Im.getDesc() = std::move(Desc);
    ImportSec.getContent().push_back(std::move(Im));
  }
  Comp.getSections().push_back(std::move(ImportSec));

  // 2. Define Func Types
  AST::Component::TypeSection TypeSec;
  // Method type: (func (param "self" (borrow 0)))
  {
    WasmEdge::ComponentValType ValTy(ComponentTypeCode::Borrow, 0);
    AST::Component::LabelValType Param("self"s, ValTy);
    AST::Component::FuncType FuncTy;
    std::vector<AST::Component::LabelValType> Params;
    Params.push_back(Param);
    FuncTy.setParamList(std::move(Params));
    AST::Component::DefType DT;
    DT.setFuncType(std::move(FuncTy));
    TypeSec.getContent().push_back(std::move(DT));
  }
  // Static type: (func) - index 1

  {
    AST::Component::FuncType FuncTy; // Empty params/results
    AST::Component::DefType DT;
    DT.setFuncType(std::move(FuncTy));
    TypeSec.getContent().push_back(std::move(DT));
  }
  Comp.getSections().push_back(std::move(TypeSec));

  // 3. Imports with COLLISION
  AST::Component::ImportSection ImportSec2;
  // [method]my-res.foo
  {
    AST::Component::Import Im;
    Im.getName() = "[method]my-res.foo";
    AST::Component::ExternDesc Desc;
    Desc.setFuncTypeIdx(1);
    Im.getDesc() = std::move(Desc);
    ImportSec2.getContent().push_back(std::move(Im));
  }
  // [static]my-res.foo -> Should collide with above because both map to
  // "my-res.foo" checking
  {
    AST::Component::Import Im;
    Im.getName() = "[static]my-res.foo";
    AST::Component::ExternDesc Desc;
    Desc.setFuncTypeIdx(2);
    Im.getDesc() = std::move(Desc);
    ImportSec2.getContent().push_back(std::move(Im));
  }
  Comp.getSections().push_back(std::move(ImportSec2));

  EXPECT_FALSE(Val.validate(Comp));
}

} // namespace
