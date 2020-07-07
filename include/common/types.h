// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/types.h - Types definition ------------------*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the enumerations of Wasm VM used types.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

namespace SSVM {

/// Value types enumeration class.
enum class ValType : uint8_t {
  None = 0x40,
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C
};

static inline std::unordered_map<ValType, std::string> ValTypeStr = {
    {ValType::None, "none"},
    {ValType::I32, "i32"},
    {ValType::I64, "i64"},
    {ValType::F32, "f32"},
    {ValType::F64, "f64"}};

/// Block type definition
using BlockType = std::variant<ValType, uint32_t>;

/// Element types enumeration class.
enum class ElemType : uint8_t { Func = 0x60, FuncRef = 0x70 };

/// Value mutability enumeration class.
enum class ValMut : uint8_t { Const = 0x00, Var = 0x01 };

static inline std::unordered_map<ValMut, std::string> ValMutStr = {
    {ValMut::Const, "const"}, {ValMut::Var, "var"}};

/// External type enumeration class.
enum class ExternalType : uint8_t {
  Function = 0x00U,
  Table = 0x01U,
  Memory = 0x02U,
  Global = 0x03U
};

static inline std::unordered_map<ExternalType, std::string> ExternalTypeStr = {
    {ExternalType::Function, "function"},
    {ExternalType::Table, "table"},
    {ExternalType::Memory, "memory"},
    {ExternalType::Global, "global"}};

} // namespace SSVM
