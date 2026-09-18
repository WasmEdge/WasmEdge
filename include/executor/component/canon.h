// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/executor/component/canon.h - Canonical function ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the canonical function definition.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/component_valtype.h"
#include "common/enum_ast.hpp"
#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/component/resource.h"
#include "runtime/instance/table.h"

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

class ComponentExecutor;

namespace Component {

/// A canonical built-in as a core function, with its immediates.
class CanonFunction : public Runtime::HostFunctionBase {
public:
  CanonFunction(ComponentExecutor &CompExec, ComponentCanonOpCode Code,
                const Runtime::Component::CanonOptions &CanonOpts,
                AST::FunctionType CoreType) noexcept
      : Runtime::HostFunctionBase(0), Exec(CompExec), OpCode(Code),
        Opts(CanonOpts) {
    DefType.getCompositeType().getFuncType() = std::move(CoreType);
  }

  /// Run the built-in through the executor.
  Expect<void> run(const Runtime::CallingFrame &Frame,
                   Span<const ValVariant> Args, Span<ValVariant> Rets) override;

  /// Getter of the opcode.
  ComponentCanonOpCode getOpCode() const noexcept { return OpCode; }

  /// Getter of the canonical options and the instance they belong to.
  const Runtime::Component::CanonOptions &getOptions() const noexcept {
    return Opts;
  }
  Runtime::Instance::ComponentInstance *getInstance() const noexcept {
    return Opts.Inst;
  }

  /// Getter of the instance the value type immediate reads against.
  const Runtime::Instance::ComponentInstance *getTypeInstance() const noexcept {
    return TypeInst != nullptr ? TypeInst : Opts.Inst;
  }

  /// \name The immediates.
  /// @{
  /// The component function a `canon lower` calls.
  const Runtime::Instance::ComponentFunctionInstance *
  getCallee() const noexcept {
    return Callee;
  }

  /// The resource type of a `resource.*` built-in.
  const Runtime::Instance::Component::ResourceTypeInstance *
  getResource() const noexcept {
    return Resource;
  }

  /// The value type of a `stream.*` or `future.*` built-in.
  const std::optional<ComponentValType> &getValType() const noexcept {
    return ValType;
  }

  /// The result types of a `task.return` built-in.
  Span<const ComponentValType> getResultTypes() const noexcept {
    return ResultTypes;
  }

  /// The table a `thread.new-indirect` calls through.
  Runtime::Instance::TableInstance *getTable() const noexcept { return Table; }

  /// The slot of `context.*` or the type index of `thread.new-indirect`.
  uint32_t getConstVal() const noexcept { return ConstVal; }

  /// The `async` or `cancellable` immediate of an asynchronous built-in.
  bool getFlag() const noexcept { return Flag; }

  /// Whether the built-in reads its address arguments as 64-bit pointers.
  bool isMemory64() const noexcept { return Opts.isMemory64(); }
  /// @}

protected:
  friend class WasmEdge::Executor::ComponentExecutor;

  /// \name Setters of the immediates.
  /// @{
  void
  setTypeInstance(const Runtime::Instance::ComponentInstance *Inst) noexcept {
    TypeInst = Inst;
  }
  void setCallee(
      const Runtime::Instance::ComponentFunctionInstance *FuncInst) noexcept {
    Callee = FuncInst;
  }
  void setResource(const Runtime::Instance::Component::ResourceTypeInstance
                       *ResType) noexcept {
    Resource = ResType;
  }
  void setValType(std::optional<ComponentValType> Type) noexcept {
    ValType = Type;
  }
  void setResultTypes(std::vector<ComponentValType> Types) noexcept {
    ResultTypes = std::move(Types);
  }
  void setTable(Runtime::Instance::TableInstance *TabInst) noexcept {
    Table = TabInst;
  }
  void setConstVal(uint32_t Val) noexcept { ConstVal = Val; }
  void setFlag(bool Val) noexcept { Flag = Val; }
  /// @}

private:
  /// \name Data of canonical function.
  /// @{
  ComponentExecutor &Exec;
  ComponentCanonOpCode OpCode;
  Runtime::Component::CanonOptions Opts;
  const Runtime::Instance::ComponentInstance *TypeInst = nullptr;
  const Runtime::Instance::ComponentFunctionInstance *Callee = nullptr;
  const Runtime::Instance::Component::ResourceTypeInstance *Resource = nullptr;
  std::optional<ComponentValType> ValType;
  std::vector<ComponentValType> ResultTypes;
  Runtime::Instance::TableInstance *Table = nullptr;
  uint32_t ConstVal = 0;
  bool Flag = false;
  /// @}
};

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
