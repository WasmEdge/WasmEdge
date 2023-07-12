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
    valueStream << Value.get<uint32_t>();
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

    // TODO: DataPtrとPageLimitを読み取って、それぞれに代入する
    // char ch;
    // std::vector<Byte> byteVec(0);
    // int size = 0;
    // while (dataPtrStream.get(ch)) {
    //   byteVec.push_back((Byte)ch);
    //   size++;
    // }
    // setBytes(Span<Byte>{byteVec}, 0, 0, size);
    // 
    // Restore MemType
    std::string valueString;
    getline(valueStream, valueString);
    Value.set(valueString);

    // Close file
    valueStream.close();
    globTypeStream.close();
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
