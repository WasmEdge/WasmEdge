// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/runtime/instance/global.h - Global Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the global instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"

#include <fstream>
#include <cstring>
#include <iostream>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class GlobalInstance {
public:
  GlobalInstance() = delete;
  GlobalInstance(const AST::GlobalType &GType,
                 ValVariant Val = uint32_t(0)) noexcept
      : GlobType(GType), Value(Val) {}

  /// Getter of global type.
  const AST::GlobalType &getGlobalType() const noexcept { return GlobType; }

  /// Getter of value.
  const ValVariant &getValue() const noexcept { return Value; }

  /// Getter of value.
  ValVariant &getValue() noexcept { return Value; }

  // Migration functions
  // filename = globinst_{i}
  void dump(std::string filename) const noexcept {
    std::ofstream valueStream;

    // Open file
    std::string valueFile = filename + "_value.img";
    valueStream.open(valueFile, std::ios::trunc);

    // TODO: uint32_t型以外の場合どうなるのか(int32_tとかは同じ値が出力された)
    
    // valueStream << (int)GlobType.getValType() << std::endl;
    switch (GlobType.getValType()) {
      case ValType::I32: {
        valueStream << Value.get<uint32_t>();
        break;
      }
      case ValType::I64: {
        valueStream << Value.get<uint64_t>();
        break;
      }
      case ValType::F32: {
        valueStream << Value.get<float>();
        break;
      }
      case ValType::F64: {
        valueStream << Value.get<double>();
        break;
      }
      /// TODO: V128の処理についてもう少し精査する
      case ValType::V128: {
        valueStream << Value.get<uint128_t>();
        break;
      }
      /// TODO: FuncRef and ExternRef
      default: {
        assert(-1);
        break;
      }
    }
    // valueStream << Value.get<int32_t>();

    // Close file
    valueStream.close();
  }

  void restore(std::string filename)  noexcept {
    // restoreFileをparseする
    std::ifstream valueStream;

    // Open file
    std::string valueFile = filename + "_value.img";
    valueStream.open(valueFile);

    // Restore Value
    std::string valueString;
    // TODO: getlineじゃなくてファイルの中身全部とってくるようにする
    getline(valueStream, valueString);
    switch (GlobType.getValType()) {
      case ValType::I32: {
        Value = static_cast<uint32_t>(std::stoul(valueString));
        break;
      }
      case ValType::I64: {
        Value = static_cast<uint64_t>(std::stoul(valueString));
        break;
      }
      case ValType::F32: {
        Value = static_cast<float>(std::stof(valueString));
        break;
      }
      case ValType::F64: {
        Value = static_cast<double>(std::stod(valueString));
        break;
      }
      /// TODO: V128の処理についてもう少し精査する
      case ValType::V128: {
        Value = static_cast<uint128_t>(std::stoul(valueString));
        break;
      }
      /// TODO: FuncRef and ExternRef
      default: {
        assert(-1);
        break;
      }
    }

    // Close file
    valueStream.close();
  }
private:
  /// \name Data of global instance.
  /// @{
  AST::GlobalType GlobType;
  ValVariant Value;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
