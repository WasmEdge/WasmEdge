// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/executor/CoredumpTest.cpp - Coredump tests ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of the generated coredump.
///
//===----------------------------------------------------------------------===//

#include "executor/coredump.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "executor/restore.h"
#include "loader/serialize.h"
#include "vm/vm.h"

#include "../spec/hostfunc.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;

// Reader of the coredump custom section content.
class CoredumpReader {
public:
  CoredumpReader(WasmEdge::Span<const WasmEdge::Byte> C) noexcept
      : Content(C) {}

  uint8_t readByte() noexcept {
    if (Pos >= Content.size()) {
      Failed = true;
      return 0;
    }
    return Content[Pos++];
  }

  uint32_t readU32() noexcept {
    uint32_t Result = 0;
    uint32_t Shift = 0;
    while (Pos < Content.size()) {
      const uint8_t B = Content[Pos++];
      Result |= static_cast<uint32_t>(B & 0x7FU) << Shift;
      if ((B & 0x80U) == 0) {
        return Result;
      }
      Shift += 7;
    }
    Failed = true;
    return 0;
  }

  uint64_t readU64() noexcept {
    uint64_t Result = 0;
    for (uint32_t Shift = 0; Shift < 64; Shift += 7) {
      const auto B = readByte();
      if (Failed || (Shift == 63 && B > 1)) {
        Failed = true;
        return 0;
      }
      Result |= static_cast<uint64_t>(B & 0x7FU) << Shift;
      if ((B & 0x80U) == 0) {
        return Result;
      }
    }
    Failed = true;
    return 0;
  }

  template <typename T> void expectType(const T &Type) {
    std::vector<Byte> Bytes;
    Loader::Serializer().serializeType(Type, Bytes);
    for (const auto B : Bytes) {
      EXPECT_EQ(readByte(), B);
    }
  }

  std::array<uint32_t, 3> readRef() noexcept {
    const auto Tag = readByte();
    if (Tag == 1) {
      const auto Instance = readU32();
      return {Tag, Instance, readU32()};
    }
    if (Tag > 2) {
      Failed = true;
    }
    return {Tag, 0, 0};
  }

  const Runtime::Instance::FunctionInstance *readFunction(
      Span<const Runtime::Instance::ModuleInstance *const> Modules) noexcept {
    const auto Ref = readRef();
    if (Ref[0] == 0) {
      return nullptr;
    }
    if (Ref[0] != 1 || Ref[1] >= Modules.size()) {
      Failed = true;
      return nullptr;
    }
    const auto Functions = Modules[Ref[1]]->getFunctionInstances();
    if (Ref[2] >= Functions.size()) {
      Failed = true;
      return nullptr;
    }
    return Functions[Ref[2]];
  }

  std::string readName() noexcept {
    const uint32_t Size = readU32();
    if (Pos + Size > Content.size()) {
      Failed = true;
      return {};
    }
    std::string Result(reinterpret_cast<const char *>(&Content[Pos]), Size);
    Pos += Size;
    return Result;
  }

  // Read a fixed width little endian integer of the wasmgdb dialect.
  template <typename T> T readFixed() noexcept {
    static_assert(std::is_integral_v<T>);
    if (Pos + sizeof(T) > Content.size()) {
      Failed = true;
      return 0;
    }
    T Result = 0;
    std::memcpy(&Result, &Content[Pos], sizeof(Result));
    Pos += sizeof(T);
    return Result;
  }

  int64_t readS64() noexcept {
    int64_t Result = 0;
    uint32_t Shift = 0;
    while (Pos < Content.size()) {
      const uint8_t B = Content[Pos++];
      Result |= static_cast<int64_t>(B & 0x7FU) << Shift;
      Shift += 7;
      if ((B & 0x80U) == 0) {
        if (Shift < 64 && (B & 0x40U) != 0) {
          Result |= -(INT64_C(1) << Shift);
        }
        return Result;
      }
    }
    Failed = true;
    return 0;
  }

  // Read one `value` and return its type tag. The canonical encoding of the
  // format uses LEB128 integers, wasmgdb expects fixed width integers.
  uint8_t readValue(bool ForWasmgdb, int64_t &Out) noexcept {
    const uint8_t Type = readByte();
    switch (Type) {
    case 0x01:
      Out = 0;
      return Type;
    case 0x7F:
      Out = ForWasmgdb ? readFixed<int32_t>() : readS64();
      return Type;
    case 0x7E:
      Out = ForWasmgdb ? readFixed<int64_t>() : readS64();
      return Type;
    case 0x7D:
      readFixed<int32_t>();
      Out = 0;
      return Type;
    case 0x7C:
      readFixed<int64_t>();
      Out = 0;
      return Type;
    default:
      Failed = true;
      return Type;
    }
  }

  bool isFailed() const noexcept { return Failed; }
  bool isEnd() const noexcept { return Pos == Content.size(); }

private:
  WasmEdge::Span<const WasmEdge::Byte> Content;
  size_t Pos = 0;
  bool Failed = false;
};

const AST::CustomSection *findCustomSection(const AST::Module &Mod,
                                            std::string_view Name) noexcept {
  for (const auto &Sec : Mod.getCustomSections()) {
    if (Sec.getName() == Name) {
      return &Sec;
    }
  }
  return nullptr;
}

// Restore the working directory when leaving the scope.
class ScopedWorkingDirectory {
public:
  ScopedWorkingDirectory(const std::filesystem::path &Path)
      : Origin(std::filesystem::current_path()) {
    std::filesystem::current_path(Path);
  }
  ~ScopedWorkingDirectory() {
    std::error_code Error;
    std::filesystem::current_path(Origin, Error);
  }

private:
  std::filesystem::path Origin;
};

// (module
//   (memory (export "mem") 1)
//   (global $g (mut i32) (i32.const 42))
//   (global $h f64 (f64.const 3.5))
//   (func $inner (param i32) (result i32)
//     (local i64)
//     i32.const 7
//     local.get 0
//     i32.const 0
//     i32.store
//     drop
//     local.get 0)
//   (func (export "access_out_of_bounds") (result i32)
//     i32.const 16
//     i32.const 0xcafe
//     i32.store
//     i32.const 0x4522f0
//     call $inner))
std::array<WasmEdge::Byte, 147> CoredumpWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x02, 0x60,
    0x01, 0x7f, 0x01, 0x7f, 0x60, 0x00, 0x01, 0x7f, 0x03, 0x03, 0x02, 0x00,
    0x01, 0x05, 0x03, 0x01, 0x00, 0x01, 0x06, 0x12, 0x02, 0x7f, 0x01, 0x41,
    0x2a, 0x0b, 0x7c, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c,
    0x40, 0x0b, 0x07, 0x1e, 0x02, 0x03, 0x6d, 0x65, 0x6d, 0x02, 0x00, 0x14,
    0x61, 0x63, 0x63, 0x65, 0x73, 0x73, 0x5f, 0x6f, 0x75, 0x74, 0x5f, 0x6f,
    0x66, 0x5f, 0x62, 0x6f, 0x75, 0x6e, 0x64, 0x73, 0x00, 0x01, 0x0a, 0x25,
    0x02, 0x10, 0x01, 0x01, 0x7e, 0x41, 0x07, 0x20, 0x00, 0x41, 0x00, 0x36,
    0x02, 0x00, 0x1a, 0x20, 0x00, 0x0b, 0x12, 0x00, 0x41, 0x10, 0x41, 0xfe,
    0x95, 0x03, 0x36, 0x02, 0x00, 0x41, 0xf0, 0xc5, 0x94, 0x02, 0x10, 0x00,
    0x0b, 0x00, 0x18, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x01, 0x00,
    0x05, 0x69, 0x6e, 0x6e, 0x65, 0x72, 0x07, 0x07, 0x02, 0x00, 0x01, 0x67,
    0x01, 0x01, 0x68};

// Trap the module in an empty directory and parse the generated coredump.
void runCoredump(bool ForWasmgdb, AST::Module &Output) {
  const auto TempDir =
      std::filesystem::temp_directory_path() /
      u8path("wasmedge-coredump-"s + std::to_string(ForWasmgdb));
  std::error_code Error;
  std::filesystem::remove_all(TempDir, Error);
  ASSERT_TRUE(std::filesystem::create_directories(TempDir, Error));

  {
    ScopedWorkingDirectory Guard(TempDir);
    WasmEdge::Configure Conf;
    Conf.getRuntimeConfigure().setEnableCoredump(true);
    Conf.getRuntimeConfigure().setCoredumpWasmgdb(ForWasmgdb);
    WasmEdge::VM::VM VM(Conf);
    ASSERT_TRUE(VM.loadWasm(CoredumpWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    const auto Result = VM.execute("access_out_of_bounds"sv);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }

  std::vector<std::filesystem::path> Dumps;
  for (const auto &Entry : std::filesystem::directory_iterator(TempDir)) {
    if (Entry.path().filename().string().rfind("coredump."s, 0) == 0) {
      Dumps.push_back(Entry.path());
    }
  }
  ASSERT_EQ(Dumps.size(), 1U);

  // The coredump must be a well-formed WASM module.
  WasmEdge::Configure LoadConf;
  WasmEdge::Loader::Loader LoadEngine(LoadConf);
  auto Mod = LoadEngine.parseModule(u8string(Dumps[0]));
  ASSERT_TRUE(Mod);
  Output = std::move(**Mod);
  std::filesystem::remove_all(TempDir, Error);
}

TEST(CoredumpTest, Sections) {
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(runCoredump(false, Mod));

  ASSERT_EQ(Mod.getCustomSections().size(), 7U);
  for (const auto Name : {"wasmedge.tables"sv, "wasmedge.globals"sv}) {
    const auto *Section = findCustomSection(Mod, Name);
    ASSERT_NE(Section, nullptr);
    CoredumpReader Reader(Section->getContent());
    EXPECT_EQ(Reader.readU32(), 1U);
    EXPECT_EQ(Reader.readU32(), 0U);
    EXPECT_FALSE(Reader.isFailed());
    EXPECT_TRUE(Reader.isEnd());
  }
  const auto *Extensions = findCustomSection(Mod, "wasmedge.instances"sv);
  ASSERT_NE(Extensions, nullptr);
  CoredumpReader ExtensionReader(Extensions->getContent());
  EXPECT_EQ(ExtensionReader.readU32(), 1U);
  EXPECT_EQ(ExtensionReader.readU32(), 1U);
  for (unsigned I = 0; I < 3; ++I) {
    EXPECT_EQ(ExtensionReader.readU32(), 0U);
  }
  EXPECT_FALSE(ExtensionReader.isFailed());
  EXPECT_TRUE(ExtensionReader.isEnd());

  ASSERT_EQ(Mod.getMemorySection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getMemorySection().getContent()[0].getLimit().getMin(), 1U);

  // The runs of zeros of the linear memory are skipped, only the marker
  // written by the module is dumped.
  ASSERT_EQ(Mod.getDataSection().getContent().size(), 1U);
  const auto &Seg = Mod.getDataSection().getContent()[0];
  EXPECT_EQ(Seg.getMode(), AST::DataSegment::DataMode::Active);
  EXPECT_EQ(Seg.getIdx(), 0U);
  ASSERT_EQ(Seg.getExpr().getInstrs().size(), 2U);
  EXPECT_EQ(Seg.getExpr().getInstrs()[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(Seg.getExpr().getInstrs()[0].getNum().get<int32_t>(), 16);
  ASSERT_EQ(Seg.getData().size(), 2U);
  EXPECT_EQ(Seg.getData()[0], 0xFE);
  EXPECT_EQ(Seg.getData()[1], 0xCA);

  // The globals are dumped as constant globals holding their current values.
  ASSERT_EQ(Mod.getGlobalSection().getContent().size(), 2U);
  const auto &GlobI32 = Mod.getGlobalSection().getContent()[0];
  EXPECT_EQ(GlobI32.getGlobalType().getValMut(), ValMut::Const);
  EXPECT_EQ(GlobI32.getGlobalType().getValType().getCode(), TypeCode::I32);
  ASSERT_EQ(GlobI32.getExpr().getInstrs().size(), 2U);
  EXPECT_EQ(GlobI32.getExpr().getInstrs()[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(GlobI32.getExpr().getInstrs()[0].getNum().get<int32_t>(), 42);
  const auto &GlobF64 = Mod.getGlobalSection().getContent()[1];
  EXPECT_EQ(GlobF64.getGlobalType().getValMut(), ValMut::Const);
  ASSERT_EQ(GlobF64.getExpr().getInstrs().size(), 2U);
  EXPECT_EQ(GlobF64.getExpr().getInstrs()[0].getOpCode(), OpCode::F64__const);
  EXPECT_DOUBLE_EQ(GlobF64.getExpr().getInstrs()[0].getNum().get<double>(),
                   3.5);

  // core ::= customsec(0x0 executable-name:name)
  const auto *Core = findCustomSection(Mod, "core"sv);
  ASSERT_NE(Core, nullptr);
  CoredumpReader CoreReader(Core->getContent());
  EXPECT_EQ(CoreReader.readByte(), 0x00);
  CoreReader.readName();
  EXPECT_FALSE(CoreReader.isFailed());
  EXPECT_TRUE(CoreReader.isEnd());

  // coremodules ::= customsec(vec(0x0 module-name:name))
  const auto *CoreModules = findCustomSection(Mod, "coremodules"sv);
  ASSERT_NE(CoreModules, nullptr);
  CoredumpReader ModuleReader(CoreModules->getContent());
  ASSERT_EQ(ModuleReader.readU32(), 1U);
  EXPECT_EQ(ModuleReader.readByte(), 0x00);
  ModuleReader.readName();
  EXPECT_FALSE(ModuleReader.isFailed());
  EXPECT_TRUE(ModuleReader.isEnd());

  // coreinstances ::= customsec(vec(0x0 moduleidx memories:vec globals:vec))
  const auto *CoreInstances = findCustomSection(Mod, "coreinstances"sv);
  ASSERT_NE(CoreInstances, nullptr);
  CoredumpReader InstanceReader(CoreInstances->getContent());
  ASSERT_EQ(InstanceReader.readU32(), 1U);
  EXPECT_EQ(InstanceReader.readByte(), 0x00);
  EXPECT_EQ(InstanceReader.readU32(), 0U);
  ASSERT_EQ(InstanceReader.readU32(), 1U);
  EXPECT_EQ(InstanceReader.readU32(), 0U);
  ASSERT_EQ(InstanceReader.readU32(), 2U);
  EXPECT_EQ(InstanceReader.readU32(), 0U);
  EXPECT_EQ(InstanceReader.readU32(), 1U);
  EXPECT_FALSE(InstanceReader.isFailed());
  EXPECT_TRUE(InstanceReader.isEnd());
}

TEST(CoredumpTest, StackFrames) {
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(runCoredump(false, Mod));

  const auto *CoreStack = findCustomSection(Mod, "corestack"sv);
  ASSERT_NE(CoreStack, nullptr);
  CoredumpReader Reader(CoreStack->getContent());
  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readName(), "main"s);

  // The frames are listed from the youngest one to the oldest one, therefore
  // the trapping `$inner` comes before its caller.
  ASSERT_EQ(Reader.readU32(), 2U);

  int64_t Value = 0;
  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 0U);
  // The code offset is relative to the first instruction of the function body,
  // the trapping `i32.store` of `$inner` follows three 2 byte instructions.
  EXPECT_EQ(Reader.readU32(), 6U);
  // `$inner` has one i32 parameter and one i64 local.
  ASSERT_EQ(Reader.readU32(), 2U);
  EXPECT_EQ(Reader.readValue(false, Value), 0x7F);
  EXPECT_EQ(Value, INT64_C(0x4522f0));
  EXPECT_EQ(Reader.readValue(false, Value), 0x7E);
  EXPECT_EQ(Value, INT64_C(0));
  // The operand stack values are untyped in the interpreter, hence they are
  // dumped as missing values.
  ASSERT_EQ(Reader.readU32(), 1U);
  EXPECT_EQ(Reader.readValue(false, Value), 0x01);

  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 1U);
  // The `call` of the caller follows a 2 byte, a 4 byte, a 3 byte and a 5 byte
  // instruction.
  EXPECT_EQ(Reader.readU32(), 14U);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 0U);

  EXPECT_FALSE(Reader.isFailed());
  EXPECT_TRUE(Reader.isEnd());
}

TEST(CoredumpTest, Wasmgdb) {
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(runCoredump(true, Mod));
  EXPECT_TRUE(Mod.getTableSection().getContent().empty());
  for (const auto &Section : Mod.getCustomSections()) {
    EXPECT_NE(Section.getName().find("wasmedge."sv), 0U);
  }

  // The wasmgdb parser only reads the first data segment and pads it with the
  // offset, hence the whole memory must be dumped as a single segment.
  ASSERT_EQ(Mod.getDataSection().getContent().size(), 1U);
  const auto &Seg = Mod.getDataSection().getContent()[0];
  EXPECT_EQ(Seg.getExpr().getInstrs()[0].getNum().get<int32_t>(), 0);
  ASSERT_EQ(Seg.getData().size(), UINT32_C(65536));
  EXPECT_EQ(Seg.getData()[16], 0xFE);
  EXPECT_EQ(Seg.getData()[17], 0xCA);

  const auto *CoreStack = findCustomSection(Mod, "corestack"sv);
  ASSERT_NE(CoreStack, nullptr);
  CoredumpReader Reader(CoreStack->getContent());
  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readName(), "main"s);
  ASSERT_EQ(Reader.readU32(), 2U);

  // The wasmgdb parser expects both vector sizes before the values and never
  // reads the operand stack, hence the operand stack must be empty.
  int64_t Value = 0;
  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 6U);
  ASSERT_EQ(Reader.readU32(), 2U);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readValue(true, Value), 0x7F);
  EXPECT_EQ(Value, INT64_C(0x4522f0));
  EXPECT_EQ(Reader.readValue(true, Value), 0x7E);
  EXPECT_EQ(Value, INT64_C(0));

  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 1U);
  EXPECT_EQ(Reader.readU32(), 14U);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 0U);

  EXPECT_FALSE(Reader.isFailed());
  EXPECT_TRUE(Reader.isEnd());
}

// (module
//   (memory (;0;) 1)
//   (global (;0;) (mut i32) i32.const 4660)
//   (data (;0;) (i32.const 0) "B")
// )
const std::array<WasmEdge::Byte, 31> CoredumpModuleB{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x05, 0x03, 0x01,
    0x00, 0x01, 0x06, 0x07, 0x01, 0x7f, 0x01, 0x41, 0xb4, 0x24, 0x0b,
    0x0b, 0x07, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x01, 0x42};

// (module
//   (table 2 10 funcref) (memory 1)
//   (global (mut funcref) (ref.null func)) (global i32 (i32.const 7))
//   (func $f0) (func $f1) (func $f2)
//   (elem (i32.const 0) $f0)
//   (elem funcref (ref.func $f1) (ref.func $f2))
//   (elem funcref (ref.func $f0))
//   (data "ab") (data "cd")
//   (func (export "setup")
//     (table.set (i32.const 1) (ref.func $f2))
//     (drop (table.grow (ref.null func) (i32.const 2)))
//     (table.init 1 (i32.const 2) (i32.const 0) (i32.const 2))
//     (elem.drop 2) (data.drop 1)
//     (i32.store (i32.const 1024) (i32.const 0xCAFE))
//     (global.set 0 (ref.func $f0))))
const std::array<WasmEdge::Byte, 154> CoredumpReadBackWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x05, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x05, 0x01,
    0x70, 0x01, 0x02, 0x0a, 0x05, 0x03, 0x01, 0x00, 0x01, 0x06, 0x0b, 0x02,
    0x70, 0x01, 0xd0, 0x70, 0x0b, 0x7f, 0x00, 0x41, 0x07, 0x0b, 0x07, 0x09,
    0x01, 0x05, 0x73, 0x65, 0x74, 0x75, 0x70, 0x00, 0x03, 0x09, 0x16, 0x03,
    0x00, 0x41, 0x00, 0x0b, 0x01, 0x00, 0x05, 0x70, 0x02, 0xd2, 0x01, 0x0b,
    0xd2, 0x02, 0x0b, 0x05, 0x70, 0x01, 0xd2, 0x00, 0x0b, 0x0c, 0x01, 0x02,
    0x0a, 0x39, 0x04, 0x02, 0x00, 0x0b, 0x02, 0x00, 0x0b, 0x02, 0x00, 0x0b,
    0x2e, 0x00, 0x41, 0x01, 0xd2, 0x02, 0x26, 0x00, 0xd0, 0x70, 0x41, 0x02,
    0xfc, 0x0f, 0x00, 0x1a, 0x41, 0x02, 0x41, 0x00, 0x41, 0x02, 0xfc, 0x0c,
    0x01, 0x00, 0xfc, 0x0d, 0x02, 0xfc, 0x09, 0x01, 0x41, 0x80, 0x08, 0x41,
    0xfe, 0x95, 0x03, 0x36, 0x02, 0x00, 0xd2, 0x00, 0x24, 0x00, 0x0b, 0x0b,
    0x09, 0x02, 0x01, 0x02, 0x61, 0x62, 0x01, 0x02, 0x63, 0x64};

// (module
//   (type $f (func))
//   (func $f0)
//   (table (export "t") 1 (ref null $f))
//   (global (export "g") (mut (ref null $f)) (ref.null $f))
//   (elem declare func $f0)
//   (func (export "setup")
//     (table.set 0 (i32.const 0) (ref.func $f0))
//     (global.set 0 (ref.func $f0))))
const std::array<WasmEdge::Byte, 80> CoredumpTypeOwnerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x03, 0x02, 0x00, 0x00, 0x04, 0x05, 0x01, 0x63, 0x00,
    0x00, 0x01, 0x06, 0x07, 0x01, 0x63, 0x00, 0x01, 0xd0, 0x00, 0x0b, 0x07,
    0x11, 0x03, 0x01, 0x74, 0x01, 0x00, 0x01, 0x67, 0x03, 0x00, 0x05, 0x73,
    0x65, 0x74, 0x75, 0x70, 0x00, 0x01, 0x09, 0x05, 0x01, 0x03, 0x00, 0x01,
    0x00, 0x0a, 0x11, 0x02, 0x02, 0x00, 0x0b, 0x0c, 0x00, 0x41, 0x00, 0xd2,
    0x00, 0x26, 0x00, 0xd2, 0x00, 0x24, 0x00, 0x0b};

// (module
//   (type $s (struct))
//   (type $f (func))
//   (import "owner" "t" (table 1 (ref null $f)))
//   (import "owner" "g" (global (mut (ref null $f)))))
const std::array<WasmEdge::Byte, 44> CoredumpTypeImporterWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x02,
    0x5f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x1a, 0x02, 0x05, 0x6f, 0x77,
    0x6e, 0x65, 0x72, 0x01, 0x74, 0x01, 0x63, 0x01, 0x00, 0x01, 0x05,
    0x6f, 0x77, 0x6e, 0x65, 0x72, 0x01, 0x67, 0x03, 0x63, 0x01, 0x01};

// The VM of one dump test with the WASI module and coredumps enabled, and the
// scratch directory holding its dumps until the test ends.
class CoredumpState {
public:
  CoredumpState() {
    Dir = std::filesystem::temp_directory_path() /
          u8path("wasmedge-coredump-state-"s +
                 testing::UnitTest::GetInstance()->current_test_info()->name());
    std::error_code Error;
    std::filesystem::remove_all(Dir, Error);
    EXPECT_TRUE(std::filesystem::create_directories(Dir, Error));
    Path = Dir / "state.core";
    Conf.addHostRegistration(HostRegistration::Wasi);
    Conf.getRuntimeConfigure().setEnableCoredump(true);
    VM = std::make_unique<WasmEdge::VM::VM>(Conf);
  }
  ~CoredumpState() {
    std::error_code Error;
    std::filesystem::remove_all(Dir, Error);
  }

  void instantiate() {
    ASSERT_TRUE(VM->validate());
    ASSERT_TRUE(VM->instantiate());
  }

  // Dump the modules of Req to a new file, since the loader keeps the previous
  // dump mapped and Windows refuses to rewrite a mapped file, then parse it.
  void generate(Coredump::Request Req, AST::Module &Output) {
    Path = Dir / ("state-"s + std::to_string(++Generated) + ".core");
    Req.Path = Path;
    const auto Result = Coredump::generate(Req);
    ASSERT_TRUE(Result);
    EXPECT_EQ(*Result, Path);
    EXPECT_TRUE(std::filesystem::exists(Path));
    auto Mod = VM->getLoader().parseModule(Path);
    ASSERT_TRUE(Mod);
    ASSERT_TRUE(VM->getValidator().validate(**Mod));
    Output = std::move(**Mod);
  }

  Configure Conf;
  std::unique_ptr<WasmEdge::VM::VM> VM;
  std::filesystem::path Dir;
  std::filesystem::path Path;
  unsigned Generated = 0;
};

TEST(CoredumpTest, RestorePreflight) {
  CoredumpState State;
  ASSERT_TRUE(State.VM->loadWasm(CoredumpReadBackWasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  ASSERT_TRUE(State.VM->execute("setup"sv));
  auto *Active = const_cast<Runtime::Instance::ModuleInstance *>(
      State.VM->getActiveModule());
  const std::array Modules{Active};
  Coredump::Request Req;
  const std::array<const Runtime::Instance::ModuleInstance *, 1> Sources{
      Active};
  Req.Modules = Sources;
  AST::Module Dump;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
  const auto Plan = Coredump::Detail::preflight({Modules, State.Path});
  ASSERT_TRUE(Plan);
  ASSERT_EQ(Plan->Memories.size(), 1U);
  EXPECT_EQ(Plan->Memories[0].Instance, Active->getMemoryInstances()[0]);
  EXPECT_EQ(Plan->Memories[0].Pages, 1U);
  ASSERT_EQ(Plan->Memories[0].Segments.size(), 1U);
  EXPECT_EQ(Plan->Memories[0].Segments[0].first, 1024U);
  ASSERT_EQ(Plan->Tables.size(), 1U);
  EXPECT_EQ(Plan->Tables[0].Refs.size(), 4U);
  EXPECT_EQ(Plan->Globals.size(), 2U);
  EXPECT_TRUE(Plan->DataDrops.empty());
  EXPECT_TRUE(Plan->ElemDrops.empty());
  const auto WrongCount = Coredump::Detail::preflight({{}, State.Path});
  ASSERT_FALSE(WrongCount);
  EXPECT_EQ(WrongCount.error(), ErrCode::Value::CoredumpMismatch);
  Req.ForWasmgdb = true;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
  const auto Wasmgdb = Coredump::Detail::preflight({Modules, State.Path});
  ASSERT_FALSE(Wasmgdb);
  EXPECT_EQ(Wasmgdb.error(), ErrCode::Value::CoredumpMalformed);
}

TEST(CoredumpTest, RestoreRoundTrip) {
  CoredumpState State;
  ASSERT_TRUE(State.VM->loadWasm(CoredumpReadBackWasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  ASSERT_TRUE(State.VM->execute("setup"sv));
  auto *Active = const_cast<Runtime::Instance::ModuleInstance *>(
      State.VM->getActiveModule());
  const std::array Modules{Active};
  const std::array<const Runtime::Instance::ModuleInstance *, 1> Sources{
      Active};
  Coredump::Request Req;
  Req.Modules = Sources;
  AST::Module Dump;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
  const auto Saved = Loader::Loader::loadFile(State.Path);
  ASSERT_TRUE(Saved);
  const auto SavedPath = State.Path;
  auto &Mem = *Active->getMemoryInstances()[0];
  auto &Table = *Active->getTableInstances()[0];
  for (const bool Grow : {true, false}) {
    if (Grow) {
      ASSERT_TRUE(Mem.growPage(2));
      ASSERT_TRUE(Table.growTable(2));
    } else {
      ASSERT_TRUE(Mem.shrinkPage(1));
      ASSERT_TRUE(Table.shrinkTable(3));
    }
    ASSERT_TRUE(Mem.fillBytes(0xA5, 0, Mem.getSize()));
    ASSERT_TRUE(
        Table.fillRefs(RefVariant(TypeCode::FuncRef), 0, Table.getSize()));
    Active->getGlobalInstances()[0]->setValue(RefVariant(TypeCode::FuncRef));
    Active->getGlobalInstances()[1]->setValue(uint32_t(99));
    ASSERT_TRUE(Coredump::restore({Modules, SavedPath}));
    ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
    const auto Restored = Loader::Loader::loadFile(State.Path);
    ASSERT_TRUE(Restored);
    EXPECT_EQ(*Saved, *Restored);
  }
  ASSERT_TRUE(Mem.growPage(2));
  const auto Bytes = Mem.getBytes(Mem.kPageSize, 2 * Mem.kPageSize);
  ASSERT_TRUE(Bytes);
  EXPECT_TRUE(
      std::all_of(Bytes->begin(), Bytes->end(), [](Byte B) { return B == 0; }));
}

TEST(CoredumpTest, RestoreRejections) {
  CoredumpState State;
  ASSERT_TRUE(State.VM->loadWasm(CoredumpReadBackWasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  ASSERT_TRUE(State.VM->execute("setup"sv));
  auto *Active = const_cast<Runtime::Instance::ModuleInstance *>(
      State.VM->getActiveModule());
  const std::array Modules{Active};
  const std::array<const Runtime::Instance::ModuleInstance *, 1> Sources{
      Active};
  Coredump::Request Req;
  Req.Modules = Sources;
  AST::Module Original;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Original));
  auto &Memory = *Active->getMemoryInstances()[0];
  ASSERT_TRUE(Memory.fillBytes(0xA5, 0, Memory.getSize()));
  Active->getGlobalInstances()[1]->setValue(uint32_t(99));
  AST::Module Before;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Before));
  const auto BeforeBytes = Loader::Loader::loadFile(State.Path);
  ASSERT_TRUE(BeforeBytes);
  using Edit = std::function<void(AST::Module &)>;
  const auto Custom = [](AST::Module &Dump,
                         std::string_view Name) -> std::vector<Byte> & {
    for (auto &Section : Dump.getCustomSections()) {
      if (Section.getName() == Name)
        return Section.getContent();
    }
    std::abort();
  };
  const std::vector<std::pair<ErrCode::Value, Edit>> Cases{
      {ErrCode::Value::CoredumpMalformed,
       [&](auto &D) { Custom(D, "wasmedge.tables")[0] = 2; }},
      {ErrCode::Value::CoredumpMalformed,
       [&](auto &D) { Custom(D, "wasmedge.instances")[0] = 2; }},
      {ErrCode::Value::CoredumpMalformed,
       [&](auto &D) { Custom(D, "wasmedge.globals")[0] = 2; }},
      {ErrCode::Value::CoredumpMalformed,
       [](auto &D) { D.getCustomSections().pop_back(); }},
      {ErrCode::Value::CoredumpMalformed,
       [](auto &D) {
         D.getCustomSections().push_back(D.getCustomSections().back());
       }},
      {ErrCode::Value::CoredumpMalformed,
       [&](auto &D) { Custom(D, "wasmedge.tables").pop_back(); }},
      {ErrCode::Value::CoredumpMalformed,
       [&](auto &D) { Custom(D, "wasmedge.instances").push_back(0); }},
      {ErrCode::Value::CoredumpMalformed,
       [&](auto &D) { Custom(D, "coreinstances")[4] = 9; }},
      {ErrCode::Value::CoredumpMalformed,
       [&](auto &D) { Custom(D, "wasmedge.globals").back() = 99; }},
      {ErrCode::Value::CoredumpMalformed,
       [](auto &D) {
         D.getDataSection().getContent()[0].getExpr().getInstrs()[0].setNum(
             uint32_t(65536));
       }},
      {ErrCode::Value::CoredumpMalformed,
       [](auto &D) {
         auto Seg = D.getDataSection().getContent()[0];
         D.getDataSection().getContent().push_back(Seg);
       }},
      {ErrCode::Value::CoredumpMismatch,
       [](auto &D) {
         D.getGlobalSection().getContent()[1].getGlobalType().setValType(
             TypeCode::I64);
       }},
      {ErrCode::Value::CoredumpMismatch,
       [&](auto &D) { Custom(D, "wasmedge.tables")[2] = 0x6F; }},
      {ErrCode::Value::CoredumpMismatch,
       [](auto &D) {
         D.getMemorySection().getContent()[0].getLimit().setMin(65537);
       }},
      {ErrCode::Value::CoredumpUnsupported,
       [&](auto &D) {
         auto &B = Custom(D, "wasmedge.globals");
         B.resize(6);
         B[5] = Coredump::kRefMissing;
       }},
      {ErrCode::Value::CoredumpUnsupported, [](auto &D) {
         D.getMemorySection().getContent()[0] =
             AST::MemoryType(AST::Limit(1, 3, false, true));
       }}};
  unsigned Case = 0;
  for (const auto &[Expected, Mutate] : Cases) {
    SCOPED_TRACE(Case++);
    auto Dump = Original;
    Mutate(Dump);
    const auto Bytes = Loader::Serializer().serializeModule(Dump);
    ASSERT_TRUE(Bytes);
    const auto InputPath = State.Dir / "invalid.core";
    {
      std::ofstream Output(InputPath, std::ios::binary);
      Output.write(reinterpret_cast<const char *>(Bytes->data()),
                   static_cast<std::streamsize>(Bytes->size()));
      ASSERT_TRUE(Output.good());
    }
    const auto Result = Coredump::restore({Modules, InputPath});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), Expected);
    AST::Module After;
    ASSERT_NO_FATAL_FAILURE(State.generate(Req, After));
    const auto AfterBytes = Loader::Loader::loadFile(State.Path);
    ASSERT_TRUE(AfterBytes);
    EXPECT_EQ(*BeforeBytes, *AfterBytes);
  }
}

TEST(CoredumpTest, RestoreDroppedSegments) {
  CoredumpState State;
  ASSERT_TRUE(State.VM->loadWasm(CoredumpReadBackWasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  auto *Active = const_cast<Runtime::Instance::ModuleInstance *>(
      State.VM->getActiveModule());
  const std::array Modules{Active};
  const std::array<const Runtime::Instance::ModuleInstance *, 1> Sources{
      Active};
  Coredump::Request Req;
  Req.Modules = Sources;
  AST::Module Dump;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
  const auto BeforeDrop = State.Path;
  ASSERT_TRUE(State.VM->execute("setup"sv));
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
  const auto DroppedPath = State.Path;
  const auto Saved = Loader::Loader::loadFile(State.Path);
  ASSERT_TRUE(Saved);
  const auto Result = Coredump::restore({Modules, BeforeDrop});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::CoredumpUnsupported);
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
  const auto After = Loader::Loader::loadFile(State.Path);
  ASSERT_TRUE(After);
  EXPECT_EQ(*Saved, *After);
  WasmEdge::VM::VM Fresh(State.Conf);
  ASSERT_TRUE(Fresh.loadWasm(CoredumpReadBackWasm));
  ASSERT_TRUE(Fresh.validate());
  ASSERT_TRUE(Fresh.instantiate());
  auto *Target =
      const_cast<Runtime::Instance::ModuleInstance *>(Fresh.getActiveModule());
  const std::array Targets{Target};
  ASSERT_FALSE(Target->getOwnedDataInstances()[1]->isDropped());
  ASSERT_FALSE(Target->getElemInstances()[2]->isDropped());
  ASSERT_TRUE(Coredump::restore({Targets, DroppedPath}));
  EXPECT_TRUE(Target->getOwnedDataInstances()[1]->isDropped());
  EXPECT_TRUE(Target->getElemInstances()[2]->isDropped());
  EXPECT_FALSE(Target->getOwnedDataInstances()[0]->isDropped());
  EXPECT_FALSE(Target->getElemInstances()[1]->isDropped());
}

TEST(CoredumpTest, RestoreMemoryGrowAndLimits) {
  CoredumpState State;
  Runtime::Instance::ModuleInstance Source("source");
  Runtime::Instance::ModuleInstance Target("different-name");
  ASSERT_TRUE(Source.addHostMemory(
      "m", std::make_unique<Runtime::Instance::MemoryInstance>(
               AST::MemoryType(3, 3))));
  ASSERT_TRUE(Target.addHostMemory(
      "m", std::make_unique<Runtime::Instance::MemoryInstance>(
               AST::MemoryType(1, 3))));
  auto &Memory = *Source.getMemoryInstances()[0];
  ASSERT_TRUE(Memory.fillBytes(0xA5, Memory.kPageSize * 2, Memory.kPageSize));
  Coredump::Request Req;
  const std::array<const Runtime::Instance::ModuleInstance *, 1> Sources{
      &Source};
  Req.Modules = Sources;
  AST::Module Dump;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
  const std::array Targets{&Target};
  ASSERT_TRUE(Coredump::restore({Targets, State.Path}));
  EXPECT_EQ(Target.getMemoryInstances()[0]->getPageSize(), 3U);
  const auto Bytes = Target.getMemoryInstances()[0]->getBytes(
      Memory.kPageSize * 2, Memory.kPageSize);
  ASSERT_TRUE(Bytes);
  EXPECT_TRUE(std::all_of(Bytes->begin(), Bytes->end(),
                          [](Byte B) { return B == 0xA5; }));
  for (const bool ConfigLimit : {false, true}) {
    Runtime::Instance::ModuleInstance Limited("limited");
    ASSERT_TRUE(Limited.addHostMemory(
        "m",
        std::make_unique<Runtime::Instance::MemoryInstance>(
            AST::MemoryType(1, ConfigLimit ? 3 : 2), ConfigLimit ? 2 : 3)));
    const std::array LimitedModules{&Limited};
    const auto Result = Coredump::restore({LimitedModules, State.Path});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), ErrCode::Value::CoredumpMismatch);
    EXPECT_EQ(Limited.getMemoryInstances()[0]->getPageSize(), 1U);
  }
}

TEST(CoredumpTest, RestoreSharedInstances) {
  CoredumpState State;
  // (module
  //   (import "owner" "memory" (memory (;0;) 1 3))
  //   (import "owner" "table" (table (;0;) 1 4 funcref))
  // )
  std::array<WasmEdge::Byte, 45> Imports{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x02, 0x23, 0x02, 0x05,
      0x6f, 0x77, 0x6e, 0x65, 0x72, 0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79,
      0x02, 0x01, 0x01, 0x03, 0x05, 0x6f, 0x77, 0x6e, 0x65, 0x72, 0x05, 0x74,
      0x61, 0x62, 0x6c, 0x65, 0x01, 0x70, 0x01, 0x01, 0x04};
  Runtime::Instance::ModuleInstance Owner("owner");
  ASSERT_TRUE(Owner.addHostMemory(
      "memory", std::make_unique<Runtime::Instance::MemoryInstance>(
                    AST::MemoryType(1, 3))));
  ASSERT_TRUE(Owner.addHostTable(
      "table", std::make_unique<Runtime::Instance::TableInstance>(
                   AST::TableType(TypeCode::FuncRef, 1, 4))));
  WasmEdge::VM::VM First(State.Conf), Second(State.Conf);
  for (auto *Instance : {&First, &Second}) {
    ASSERT_TRUE(Instance->registerModule(Owner));
    ASSERT_TRUE(Instance->loadWasm(Imports));
    ASSERT_TRUE(Instance->validate());
    ASSERT_TRUE(Instance->instantiate());
  }
  auto *A =
      const_cast<Runtime::Instance::ModuleInstance *>(First.getActiveModule());
  auto *B =
      const_cast<Runtime::Instance::ModuleInstance *>(Second.getActiveModule());
  const std::array Modules{A, B};
  const std::array<const Runtime::Instance::ModuleInstance *, 2> Sources{A, B};
  Coredump::Request Req;
  Req.Modules = Sources;
  AST::Module Dump;
  auto &Mem = *Owner.getMemoryInstances()[0];
  ASSERT_TRUE(Mem.fillBytes(42, 0, 1));
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
  ASSERT_TRUE(Mem.fillBytes(99, 0, 1));
  ASSERT_TRUE(Owner.getTableInstances()[0]->growTable(2));
  ASSERT_TRUE(Coredump::restore({Modules, State.Path}));
  EXPECT_EQ(A->getMemoryInstances()[0], B->getMemoryInstances()[0]);
  EXPECT_EQ(A->getTableInstances()[0], B->getTableInstances()[0]);
  EXPECT_EQ(Mem.getBytes(0, 1)->front(), 42);
  EXPECT_EQ(Owner.getTableInstances()[0]->getSize(), 1U);
  Runtime::Instance::ModuleInstance Independent("independent");
  ASSERT_TRUE(Independent.addHostMemory(
      "memory", std::make_unique<Runtime::Instance::MemoryInstance>(
                    AST::MemoryType(1, 3))));
  ASSERT_TRUE(Independent.addHostTable(
      "table", std::make_unique<Runtime::Instance::TableInstance>(
                   AST::TableType(TypeCode::FuncRef, 1, 4))));
  const std::array WrongModules{A, &Independent};
  const auto Result = Coredump::restore({WrongModules, State.Path});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::CoredumpMismatch);
  EXPECT_EQ(Mem.getBytes(0, 1)->front(), 42);
  const std::array<const Runtime::Instance::ModuleInstance *, 2> Separate{
      A, &Independent};
  Req.Modules = Separate;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
  const auto Reverse = Coredump::restore({Modules, State.Path});
  ASSERT_FALSE(Reverse);
  EXPECT_EQ(Reverse.error(), ErrCode::Value::CoredumpMismatch);
}

TEST(CoredumpTest, RestoreExternReferences) {
  CoredumpState State;
  Runtime::Instance::ModuleInstance Host("host");
  ASSERT_TRUE(Host.addHostTable(
      "table", std::make_unique<Runtime::Instance::TableInstance>(
                   AST::TableType(TypeCode::ExternRef, 1, 2))));
  ASSERT_TRUE(Host.addHostGlobal(
      "global", std::make_unique<Runtime::Instance::GlobalInstance>(
                    AST::GlobalType(TypeCode::ExternRef, ValMut::Var),
                    RefVariant(TypeCode::ExternRef))));
  const std::array Modules{&Host};
  const std::array<const Runtime::Instance::ModuleInstance *, 1> Sources{&Host};
  Coredump::Request Req;
  Req.Modules = Sources;
  AST::Module Dump;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
  int Value = 42;
  ASSERT_TRUE(Host.getTableInstances()[0]->setRefAddr(0, RefVariant(&Value)));
  Host.getGlobalInstances()[0]->setValue(RefVariant(&Value));
  ASSERT_TRUE(Coredump::restore({Modules, State.Path}));
  EXPECT_TRUE(Host.getTableInstances()[0]->getRefAddr(0)->isNull());
  EXPECT_TRUE(
      Host.getGlobalInstances()[0]->getValue().get<RefVariant>().isNull());
  for (const bool Table : {false, true}) {
    if (Table) {
      ASSERT_TRUE(
          Host.getTableInstances()[0]->setRefAddr(0, RefVariant(&Value)));
      Host.getGlobalInstances()[0]->setValue(RefVariant(TypeCode::ExternRef));
    } else {
      Host.getGlobalInstances()[0]->setValue(RefVariant(&Value));
    }
    ASSERT_NO_FATAL_FAILURE(State.generate(Req, Dump));
    const auto Result = Coredump::restore({Modules, State.Path});
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), ErrCode::Value::CoredumpUnsupported);
  }
}

TEST(CoredumpTest, ImportedTypeOwner) {
  CoredumpState State;
  State.Conf.addProposal(Proposal::GC);
  State.Conf.addProposal(Proposal::FunctionReferences);
  State.VM = std::make_unique<WasmEdge::VM::VM>(State.Conf);
  ASSERT_TRUE(State.VM->registerModule("owner", CoredumpTypeOwnerWasm));
  ASSERT_TRUE(State.VM->loadWasm(CoredumpTypeImporterWasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  ASSERT_TRUE(State.VM->snapshot(State.Path));
  const auto Dump = State.VM->getLoader().parseModule(State.Path);
  ASSERT_TRUE(Dump);
  const auto &Tables = (*Dump)->getTableSection().getContent();
  ASSERT_EQ(Tables.size(), 1U);
  EXPECT_EQ(Tables[0].getTableType().getRefType(), ValType(TypeCode::FuncRef));
  const auto &Globals = (*Dump)->getGlobalSection().getContent();
  ASSERT_EQ(Globals.size(), 1U);
  EXPECT_EQ(Globals[0].getGlobalType().getValType(),
            ValType(TypeCode::FuncRef));
}

TEST(CoredumpTest, RestoreImportedTypeContext) {
  CoredumpState State;
  State.Conf.addProposal(Proposal::GC);
  State.Conf.addProposal(Proposal::FunctionReferences);
  State.VM = std::make_unique<WasmEdge::VM::VM>(State.Conf);
  ASSERT_TRUE(State.VM->registerModule("owner", CoredumpTypeOwnerWasm));
  ASSERT_TRUE(State.VM->loadWasm(CoredumpTypeImporterWasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  const auto NullPath = State.Dir / "null.core";
  ASSERT_TRUE(State.VM->snapshot(NullPath));
  ASSERT_TRUE(State.VM->execute("owner", "setup"));
  ASSERT_TRUE(State.VM->snapshot(State.Path));
  ASSERT_TRUE(State.VM->restore(NullPath));
  EXPECT_TRUE(State.VM->getActiveModule()
                  ->getGlobalInstances()[0]
                  ->getValue()
                  .get<RefVariant>()
                  .isNull());
  ASSERT_TRUE(State.VM->restore(State.Path));
  const auto Ref =
      State.VM->getActiveModule()->getTableInstances()[0]->getRefAddr(0);
  ASSERT_TRUE(Ref);
  EXPECT_FALSE(Ref->isNull());
  EXPECT_EQ(Ref->getPtr<void>(), State.VM->getActiveModule()
                                     ->getGlobalInstances()[0]
                                     ->getValue()
                                     .get<RefVariant>()
                                     .getPtr<void>());
}

TEST(CoredumpTest, ModulesOnly) {
  CoredumpState State;
  ASSERT_TRUE(State.VM->registerModule("B"sv, CoredumpModuleB));
  ASSERT_TRUE(State.VM->loadWasm(CoredumpWasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  const auto *Active = State.VM->getActiveModule();
  ASSERT_NE(Active, nullptr);
  const std::array Modules{Active};
  for (const bool ForWasmgdb : {false, true}) {
    SCOPED_TRACE(ForWasmgdb);
    Coredump::Request Req;
    Req.Modules = Modules;
    Req.ForWasmgdb = ForWasmgdb;
    AST::Module Mod;
    ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
    ASSERT_EQ(Mod.getCustomSections().size(), ForWasmgdb ? 4U : 7U);
    for (const auto Name :
         {"core"sv, "coremodules"sv, "coreinstances"sv, "corestack"sv}) {
      ASSERT_NE(findCustomSection(Mod, Name), nullptr);
    }
    CoredumpReader Reader(findCustomSection(Mod, "corestack"sv)->getContent());
    EXPECT_EQ(Reader.readByte(), 0U);
    EXPECT_EQ(Reader.readName(), "main"s);
    EXPECT_EQ(Reader.readU32(), 0U);
    EXPECT_FALSE(Reader.isFailed());
    EXPECT_TRUE(Reader.isEnd());
    ASSERT_EQ(Mod.getMemorySection().getContent().size(), 1U);
    EXPECT_EQ(Mod.getMemorySection().getContent()[0].getLimit().getMin(), 1U);
    const auto Memories = Active->getMemoryInstances();
    ASSERT_EQ(Memories.size(), 1U);
    const auto Memory = Memories[0]->getBytes(0, Memories[0]->getSize());
    ASSERT_TRUE(Memory);
    std::vector<Byte> Restored(Memory->size(), 0);
    const auto &Segments = Mod.getDataSection().getContent();
    EXPECT_EQ(Segments.size(), ForWasmgdb ? 1U : 0U);
    for (const auto &Seg : Segments) {
      EXPECT_EQ(Seg.getIdx(), 0U);
      const auto Offset = Seg.getExpr().getInstrs()[0].getNum().get<uint32_t>();
      ASSERT_LE(Offset + Seg.getData().size(), Restored.size());
      std::copy(Seg.getData().begin(), Seg.getData().end(),
                Restored.begin() + Offset);
    }
    EXPECT_EQ(Restored, (std::vector<Byte>(Memory->begin(), Memory->end())));
    const auto Globals = Active->getGlobalInstances();
    const auto &DumpGlobals = Mod.getGlobalSection().getContent();
    ASSERT_EQ(DumpGlobals.size(), Globals.size());
    ASSERT_EQ(DumpGlobals.size(), 2U);
    EXPECT_EQ(DumpGlobals[0].getExpr().getInstrs()[0].getNum().get<int32_t>(),
              Globals[0]->getValue().get<int32_t>());
    EXPECT_DOUBLE_EQ(
        DumpGlobals[1].getExpr().getInstrs()[0].getNum().get<double>(),
        Globals[1]->getValue().get<double>());
  }
}

TEST(CoredumpTest, RegisteredModule) {
  CoredumpState State;
  ASSERT_TRUE(State.VM->registerModule("B"sv, CoredumpModuleB));
  ASSERT_TRUE(State.VM->loadWasm(CoredumpWasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  const auto *Active = State.VM->getActiveModule();
  const auto *B = State.VM->getStoreManager().findModule("B"sv);
  ASSERT_NE(Active, nullptr);
  ASSERT_NE(B, nullptr);
  const std::array Modules{Active, B};
  Coredump::Request Req;
  Req.Modules = Modules;
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
  const auto *CoreModules = findCustomSection(Mod, "coremodules"sv);
  ASSERT_NE(CoreModules, nullptr);
  CoredumpReader Reader(CoreModules->getContent());
  ASSERT_EQ(Reader.readU32(), 2U);
  for (const auto *Module : Modules) {
    EXPECT_EQ(Reader.readByte(), 0U);
    EXPECT_EQ(Reader.readName(), Module->getModuleName());
  }
  EXPECT_FALSE(Reader.isFailed());
  EXPECT_TRUE(Reader.isEnd());
  const auto *Instances = findCustomSection(Mod, "coreinstances"sv);
  ASSERT_NE(Instances, nullptr);
  CoredumpReader InstanceReader(Instances->getContent());
  ASSERT_EQ(InstanceReader.readU32(), 2U);
  for (uint32_t I = 0; I < 2; ++I) {
    EXPECT_EQ(InstanceReader.readByte(), 0U);
    EXPECT_EQ(InstanceReader.readU32(), I);
    ASSERT_EQ(InstanceReader.readU32(), 1U);
    EXPECT_EQ(InstanceReader.readU32(), I);
    ASSERT_EQ(InstanceReader.readU32(), I == 0 ? 2U : 1U);
    EXPECT_EQ(InstanceReader.readU32(), I == 0 ? 0U : 2U);
    if (I == 0) {
      EXPECT_EQ(InstanceReader.readU32(), 1U);
    }
  }
  EXPECT_FALSE(InstanceReader.isFailed());
  EXPECT_TRUE(InstanceReader.isEnd());
  ASSERT_EQ(Mod.getGlobalSection().getContent().size(), 3U);
  const auto &Global = Mod.getGlobalSection().getContent()[2];
  EXPECT_EQ(Global.getExpr().getInstrs()[0].getNum().get<int32_t>(), 0x1234);
  ASSERT_EQ(Mod.getMemorySection().getContent().size(), 2U);
  ASSERT_EQ(Mod.getDataSection().getContent().size(), 1U);
  const auto &Seg = Mod.getDataSection().getContent()[0];
  EXPECT_EQ(Seg.getIdx(), 1U);
  EXPECT_EQ(Seg.getExpr().getInstrs()[0].getNum().get<int32_t>(), 0);
  ASSERT_EQ(Seg.getData().size(), 1U);
  EXPECT_EQ(Seg.getData()[0], 0x42);

  ScopedWorkingDirectory Guard(State.Dir);
  const auto Result = State.VM->execute("access_out_of_bounds"sv);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::MemoryOutOfBounds);
  size_t Dumps = 0;
  for (const auto &Entry : std::filesystem::directory_iterator(State.Dir)) {
    if (Entry.path().filename().string().rfind("coredump."s, 0) != 0) {
      continue;
    }
    ++Dumps;
    auto Trap = State.VM->getLoader().parseModule(Entry.path());
    ASSERT_TRUE(Trap);
    const auto *Section = findCustomSection(**Trap, "coremodules"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader TrapReader(Section->getContent());
    EXPECT_EQ(TrapReader.readU32(), 1U);
    EXPECT_EQ(TrapReader.readByte(), 0U);
    EXPECT_EQ(TrapReader.readName(), Active->getModuleName());
    EXPECT_FALSE(TrapReader.isFailed());
    EXPECT_TRUE(TrapReader.isEnd());
  }
  EXPECT_EQ(Dumps, 1U);
}

TEST(CoredumpTest, EmptyAndDuplicateModules) {
  CoredumpState State;
  ASSERT_TRUE(State.VM->registerModule("B"sv, CoredumpModuleB));
  ASSERT_TRUE(State.VM->loadWasm(CoredumpWasm));
  Coredump::Request Req;
  Req.Path = State.Path;
  auto Result = Coredump::generate(Req);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::IllegalPath);
  EXPECT_FALSE(std::filesystem::exists(State.Path));
  const std::array<const Runtime::Instance::ModuleInstance *, 1> NullModules{
      nullptr};
  Req.Modules = NullModules;
  Result = Coredump::generate(Req);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::IllegalPath);
  EXPECT_FALSE(std::filesystem::exists(State.Path));

  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  const auto *Active = State.VM->getActiveModule();
  const std::array<const Runtime::Instance::ModuleInstance *, 3> Modules{
      nullptr, Active, Active};
  Req.Modules = Modules;
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
  const auto *Section = findCustomSection(Mod, "coremodules"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Reader(Section->getContent());
  EXPECT_EQ(Reader.readU32(), 1U);
  EXPECT_EQ(Reader.readByte(), 0U);
  EXPECT_EQ(Reader.readName(), Active->getModuleName());
  EXPECT_FALSE(Reader.isFailed());
  EXPECT_TRUE(Reader.isEnd());
  ASSERT_EQ(Mod.getMemorySection().getContent().size(), 1U);
  ASSERT_EQ(Mod.getGlobalSection().getContent().size(), 2U);
}

TEST(CoredumpTest, InvalidPath) {
  CoredumpState State;
  ASSERT_TRUE(State.VM->registerModule("B"sv, CoredumpModuleB));
  ASSERT_TRUE(State.VM->loadWasm(CoredumpWasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  const std::array Modules{State.VM->getActiveModule()};
  Coredump::Request Req;
  Req.Modules = Modules;
  const auto MissingPath = State.Dir / "missing" / "state.core";
  Req.Path = MissingPath;
  const auto Result = Coredump::generate(Req);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::IllegalPath);
  EXPECT_TRUE(std::filesystem::is_empty(State.Dir));
}

TEST(CoredumpTest, StackModulesAppended) {
  CoredumpState State;
  ASSERT_TRUE(State.VM->registerModule("B"sv, CoredumpModuleB));
  ASSERT_TRUE(State.VM->loadWasm(CoredumpWasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  const auto *Active = State.VM->getActiveModule();
  const auto *B = State.VM->getStoreManager().findModule("B"sv);
  const std::array Modules{B};
  Runtime::StackManager Stack;
  Stack.pushFrame(Active, nullptr);
  Coredump::Request Req;
  Req.Modules = Modules;
  Req.Stack = &Stack;
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
  const auto *Section = findCustomSection(Mod, "coremodules"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Reader(Section->getContent());
  EXPECT_EQ(Reader.readU32(), 2U);
  EXPECT_EQ(Reader.readByte(), 0U);
  EXPECT_EQ(Reader.readName(), "B"s);
  EXPECT_EQ(Reader.readByte(), 0U);
  EXPECT_EQ(Reader.readName(), Active->getModuleName());
  EXPECT_FALSE(Reader.isFailed());
  EXPECT_TRUE(Reader.isEnd());
  const auto *Core = findCustomSection(Mod, "core"sv);
  ASSERT_NE(Core, nullptr);
  CoredumpReader CoreReader(Core->getContent());
  EXPECT_EQ(CoreReader.readByte(), 0U);
  EXPECT_EQ(CoreReader.readName(), "B"s);
  EXPECT_FALSE(CoreReader.isFailed());
  EXPECT_TRUE(CoreReader.isEnd());
  const auto *Frames = findCustomSection(Mod, "corestack"sv);
  ASSERT_NE(Frames, nullptr);
  CoredumpReader FrameReader(Frames->getContent());
  EXPECT_EQ(FrameReader.readByte(), 0U);
  EXPECT_EQ(FrameReader.readName(), "main"s);
  EXPECT_EQ(FrameReader.readU32(), 1U);
  EXPECT_EQ(FrameReader.readByte(), 0U);
  EXPECT_EQ(FrameReader.readU32(), 1U);
  for (unsigned I = 0; I < 4; ++I) {
    EXPECT_EQ(FrameReader.readU32(), 0U);
  }
  EXPECT_FALSE(FrameReader.isFailed());
  EXPECT_TRUE(FrameReader.isEnd());
}

TEST(CoredumpTest, RegisteredAndBuiltInModules) {
  CoredumpState State;
  ASSERT_TRUE(State.VM->registerModule("B"sv, CoredumpModuleB));
  ASSERT_TRUE(State.VM->loadWasm(CoredumpWasm));
  const auto *B = State.VM->getStoreManager().findModule("B"sv);
  const Runtime::Instance::ModuleInstance *Wasi =
      State.VM->getImportModule(HostRegistration::Wasi);
  ASSERT_NE(B, nullptr);
  ASSERT_NE(Wasi, nullptr);
  ASSERT_EQ(State.VM->getActiveModule(), nullptr);
  std::vector<const Runtime::Instance::ModuleInstance *> Modules{B, Wasi};
  for (const bool Instantiate : {false, true}) {
    SCOPED_TRACE(Instantiate);
    if (Instantiate) {
      ASSERT_NO_FATAL_FAILURE(State.instantiate());
      Modules.insert(Modules.begin(), State.VM->getActiveModule());
    }
    Coredump::Request Req;
    Req.Modules = Modules;
    AST::Module Mod;
    ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
    const auto *Section = findCustomSection(Mod, "coremodules"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader Reader(Section->getContent());
    EXPECT_EQ(Reader.readU32(), Modules.size());
    for (const auto *Module : Modules) {
      EXPECT_EQ(Reader.readByte(), 0U);
      EXPECT_EQ(Reader.readName(), Module->getModuleName());
    }
    EXPECT_FALSE(Reader.isFailed());
    EXPECT_TRUE(Reader.isEnd());
  }
}

TEST(CoredumpTest, TablesAndSegments) {
  CoredumpState State;
  // (module
  //   (table 2 10 funcref) (memory 1)
  //   (func $f0) (func $f1) (func $f2)
  //   (elem (i32.const 0) $f0)
  //   (elem funcref (ref.func $f1) (ref.func $f2))
  //   (elem funcref (ref.func $f0))
  //   (data "ab") (data "cd")
  //   (func (export "setup")
  //     (table.set (i32.const 1) (ref.func $f2))
  //     (drop (table.grow (ref.null func) (i32.const 2)))
  //     (table.init 1 (i32.const 2) (i32.const 0) (i32.const 2))
  //     (elem.drop 2) (data.drop 1)))
  std::array<WasmEdge::Byte, 127> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x03, 0x05, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x05, 0x01,
      0x70, 0x01, 0x02, 0x0a, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07, 0x09, 0x01,
      0x05, 0x73, 0x65, 0x74, 0x75, 0x70, 0x00, 0x03, 0x09, 0x16, 0x03, 0x00,
      0x41, 0x00, 0x0b, 0x01, 0x00, 0x05, 0x70, 0x02, 0xd2, 0x01, 0x0b, 0xd2,
      0x02, 0x0b, 0x05, 0x70, 0x01, 0xd2, 0x00, 0x0b, 0x0c, 0x01, 0x02, 0x0a,
      0x2b, 0x04, 0x02, 0x00, 0x0b, 0x02, 0x00, 0x0b, 0x02, 0x00, 0x0b, 0x20,
      0x00, 0x41, 0x01, 0xd2, 0x02, 0x26, 0x00, 0xd0, 0x70, 0x41, 0x02, 0xfc,
      0x0f, 0x00, 0x1a, 0x41, 0x02, 0x41, 0x00, 0x41, 0x02, 0xfc, 0x0c, 0x01,
      0x00, 0xfc, 0x0d, 0x02, 0xfc, 0x09, 0x01, 0x0b, 0x0b, 0x09, 0x02, 0x01,
      0x02, 0x61, 0x62, 0x01, 0x02, 0x63, 0x64};
  ASSERT_TRUE(State.VM->loadWasm(Wasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  ASSERT_TRUE(State.VM->execute("setup"sv));
  const std::array Modules{State.VM->getActiveModule()};
  Coredump::Request Req;
  Req.Modules = Modules;
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
  ASSERT_EQ(Mod.getTableSection().getContent().size(), 1U);
  const auto &Type = Mod.getTableSection().getContent()[0].getTableType();
  EXPECT_EQ(Type.getRefType(), ValType(TypeCode::FuncRef));
  EXPECT_EQ(Type.getLimit().getMin(), 4U);
  ASSERT_TRUE(Type.getLimit().hasMax());
  EXPECT_EQ(Type.getLimit().getMax(), 10U);
  const auto *Section = findCustomSection(Mod, "wasmedge.tables"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Tables(Section->getContent());
  EXPECT_EQ(Tables.readU32(), 1U);
  EXPECT_EQ(Tables.readU32(), 1U);
  Tables.expectType(Modules[0]->getTableInstances()[0]->getTableType());
  EXPECT_EQ(Tables.readU64(), 4U);
  EXPECT_EQ(Tables.readU32(), 4U);
  for (const uint32_t Idx : {0U, 2U, 1U, 2U}) {
    EXPECT_EQ(Tables.readRef(), (std::array<uint32_t, 3>{1, 0, Idx}));
  }
  EXPECT_FALSE(Tables.isFailed());
  EXPECT_TRUE(Tables.isEnd());
  Section = findCustomSection(Mod, "wasmedge.instances"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Instances(Section->getContent());
  EXPECT_EQ(Instances.readU32(), 1U);
  EXPECT_EQ(Instances.readU32(), 1U);
  EXPECT_EQ(Instances.readU32(), 1U);
  EXPECT_EQ(Instances.readU32(), 0U);
  EXPECT_EQ(Instances.readU32(), 2U);
  EXPECT_EQ(Instances.readByte(), 0U);
  EXPECT_EQ(Instances.readByte(), 1U);
  EXPECT_EQ(Instances.readU32(), 3U);
  for (const uint32_t Flag : {1U, 0U, 1U}) {
    EXPECT_EQ(Instances.readByte(), Flag);
  }
  EXPECT_FALSE(Instances.isFailed());
  EXPECT_TRUE(Instances.isEnd());
  Section = findCustomSection(Mod, "wasmedge.globals"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Globals(Section->getContent());
  EXPECT_EQ(Globals.readU32(), 1U);
  EXPECT_EQ(Globals.readU32(), 0U);
  EXPECT_FALSE(Globals.isFailed());
  EXPECT_TRUE(Globals.isEnd());
  Req.ForWasmgdb = true;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
  EXPECT_TRUE(Mod.getTableSection().getContent().empty());
  for (const auto &Sec : Mod.getCustomSections()) {
    EXPECT_NE(Sec.getName().find("wasmedge."sv), 0U);
  }
}

TEST(CoredumpTest, SharedTable) {
  CoredumpState State;
  // (module (table (export "table") 1 funcref))
  std::array<WasmEdge::Byte, 25> B{0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00,
                                   0x00, 0x04, 0x04, 0x01, 0x70, 0x00, 0x01,
                                   0x07, 0x09, 0x01, 0x05, 0x74, 0x61, 0x62,
                                   0x6c, 0x65, 0x01, 0x00};
  // (module
  //   (import "tables" "table" (table 1 funcref))
  //   (func $f) (elem declare func $f)
  //   (func (export "setup") (table.set (i32.const 0) (ref.func $f))))
  std::array<WasmEdge::Byte, 72> A{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x02, 0x12, 0x01, 0x06, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x73,
      0x05, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x01, 0x70, 0x00, 0x01, 0x03, 0x03,
      0x02, 0x00, 0x00, 0x07, 0x09, 0x01, 0x05, 0x73, 0x65, 0x74, 0x75, 0x70,
      0x00, 0x01, 0x09, 0x05, 0x01, 0x03, 0x00, 0x01, 0x00, 0x0a, 0x0d, 0x02,
      0x02, 0x00, 0x0b, 0x08, 0x00, 0x41, 0x00, 0xd2, 0x00, 0x26, 0x00, 0x0b};
  ASSERT_TRUE(State.VM->registerModule("tables"sv, B));
  ASSERT_TRUE(State.VM->loadWasm(A));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  ASSERT_TRUE(State.VM->execute("setup"sv));
  const std::array Modules{State.VM->getActiveModule(),
                           State.VM->getStoreManager().findModule("tables"sv)};
  ASSERT_EQ(Modules[0]->getTableInstances()[0],
            Modules[1]->getTableInstances()[0]);
  Coredump::Request Req;
  Req.Modules = Modules;
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
  ASSERT_EQ(Mod.getTableSection().getContent().size(), 1U);
  const auto *Section = findCustomSection(Mod, "wasmedge.tables"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Tables(Section->getContent());
  EXPECT_EQ(Tables.readU32(), 1U);
  EXPECT_EQ(Tables.readU32(), 1U);
  Tables.expectType(Modules[0]->getTableInstances()[0]->getTableType());
  EXPECT_EQ(Tables.readU64(), 1U);
  EXPECT_EQ(Tables.readU32(), 1U);
  EXPECT_EQ(Tables.readRef(), (std::array<uint32_t, 3>{1, 0, 0}));
  EXPECT_FALSE(Tables.isFailed());
  EXPECT_TRUE(Tables.isEnd());
  Section = findCustomSection(Mod, "wasmedge.instances"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Instances(Section->getContent());
  EXPECT_EQ(Instances.readU32(), 1U);
  EXPECT_EQ(Instances.readU32(), 2U);
  for (unsigned I = 0; I < 2; ++I) {
    EXPECT_EQ(Instances.readU32(), 1U);
    EXPECT_EQ(Instances.readU32(), 0U);
    EXPECT_EQ(Instances.readU32(), 0U);
    EXPECT_EQ(Instances.readU32(), I == 0 ? 1U : 0U);
    if (I == 0) {
      EXPECT_EQ(Instances.readByte(), 1U);
    }
  }
  EXPECT_FALSE(Instances.isFailed());
  EXPECT_TRUE(Instances.isEnd());
}

TEST(CoredumpTest, ExternRefTable) {
  CoredumpState State;
  Runtime::Instance::ModuleInstance Host("host"sv);
  auto Table = std::make_unique<Runtime::Instance::TableInstance>(
      AST::TableType(ValType(TypeCode::ExternRef), AST::Limit(2)));
  int Value = 42;
  ASSERT_TRUE(
      Table->setRefAddr(0, RefVariant(ValType(TypeCode::ExternRef), &Value)));
  ASSERT_TRUE(Host.addHostTable("table"sv, std::move(Table)));
  const std::array<const Runtime::Instance::ModuleInstance *, 1> Modules{&Host};
  Coredump::Request Req;
  Req.Modules = Modules;
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
  const auto *Section = findCustomSection(Mod, "wasmedge.tables"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Tables(Section->getContent());
  EXPECT_EQ(Tables.readU32(), 1U);
  EXPECT_EQ(Tables.readU32(), 1U);
  Tables.expectType(Host.getTableInstances()[0]->getTableType());
  EXPECT_EQ(Tables.readU64(), 2U);
  EXPECT_EQ(Tables.readU32(), 2U);
  EXPECT_EQ(Tables.readRef(), (std::array<uint32_t, 3>{2, 0, 0}));
  EXPECT_EQ(Tables.readRef(), (std::array<uint32_t, 3>{0, 0, 0}));
  EXPECT_FALSE(Tables.isFailed());
  EXPECT_TRUE(Tables.isEnd());
}

TEST(CoredumpTest, ForeignFunction) {
  SpecTestModule Spectest;
  CoredumpState State;
  // (module
  //   (import "spectest" "print" (func $print)) (table 1 funcref)
  //   (elem declare func $print)
  //   (func (export "setup") (table.set (i32.const 0) (ref.func $print))))
  std::array<WasmEdge::Byte, 74> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01,
      0x60, 0x00, 0x00, 0x02, 0x12, 0x01, 0x08, 0x73, 0x70, 0x65, 0x63,
      0x74, 0x65, 0x73, 0x74, 0x05, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x00,
      0x00, 0x03, 0x02, 0x01, 0x00, 0x04, 0x04, 0x01, 0x70, 0x00, 0x01,
      0x07, 0x09, 0x01, 0x05, 0x73, 0x65, 0x74, 0x75, 0x70, 0x00, 0x01,
      0x09, 0x05, 0x01, 0x03, 0x00, 0x01, 0x00, 0x0a, 0x0a, 0x01, 0x08,
      0x00, 0x41, 0x00, 0xd2, 0x00, 0x26, 0x00, 0x0b};
  State.Conf.addProposal(Proposal::Threads);
  State.Conf.addProposal(Proposal::Memory64);
  State.VM = std::make_unique<WasmEdge::VM::VM>(State.Conf);
  ASSERT_TRUE(State.VM->registerModule(Spectest));
  ASSERT_TRUE(State.VM->loadWasm(Wasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  ASSERT_TRUE(State.VM->execute("setup"sv));
  const auto *Active = State.VM->getActiveModule();
  const auto Functions = Spectest.getFunctionInstances();
  const auto *Print = Spectest.findFuncExports("print"sv);
  const auto It = std::find(Functions.begin(), Functions.end(), Print);
  ASSERT_NE(It, Functions.end());
  const auto PrintIdx = static_cast<uint32_t>(It - Functions.begin());
  Runtime::Instance::ModuleInstance Holder("holder"sv);
  auto Table = std::make_unique<Runtime::Instance::TableInstance>(
      AST::TableType(ValType(TypeCode::FuncRef), AST::Limit(1)));
  const auto Ref = Active->getTableInstances()[0]->getRefAddr(0);
  ASSERT_TRUE(Ref);
  ASSERT_TRUE(Table->setRefAddr(0, *Ref));
  ASSERT_TRUE(Holder.addHostTable("table"sv, std::move(Table)));
  for (unsigned Case = 0; Case < 3; ++Case) {
    SCOPED_TRACE(Case);
    std::vector<const Runtime::Instance::ModuleInstance *> Modules{Active};
    std::array<uint32_t, 3> Expected{1, 0, 0};
    if (Case == 1) {
      Modules.push_back(&Spectest);
      Expected = {1, 1, PrintIdx};
    } else if (Case == 2) {
      Modules = {&Holder};
      Expected = {2, 0, 0};
    }
    Coredump::Request Req;
    Req.Modules = Modules;
    AST::Module Mod;
    ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
    const auto *Section = findCustomSection(Mod, "wasmedge.tables"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader Reader(Section->getContent());
    EXPECT_EQ(Reader.readU32(), 1U);
    EXPECT_EQ(Reader.readU32(), Case == 1 ? 3U : 1U);
    Reader.expectType(Active->getTableInstances()[0]->getTableType());
    EXPECT_EQ(Reader.readU64(), 1U);
    EXPECT_EQ(Reader.readU32(), 1U);
    EXPECT_EQ(Reader.readRef(), Expected);
    if (Case == 1) {
      for (unsigned I = 0; I < 2; ++I) {
        Reader.expectType(Spectest.getTableInstances()[I]->getTableType());
        EXPECT_EQ(Reader.readU64(), 10U);
        EXPECT_EQ(Reader.readU32(), 10U);
        for (unsigned J = 0; J < 10; ++J) {
          EXPECT_EQ(Reader.readRef(), (std::array<uint32_t, 3>{0, 0, 0}));
        }
      }
    }
    EXPECT_FALSE(Reader.isFailed());
    EXPECT_TRUE(Reader.isEnd());
  }
}

TEST(CoredumpTest, FuncRefGlobal) {
  CoredumpState State;
  // (module
  //   (global (mut funcref) (ref.null func)) (global i32 (i32.const 7))
  //   (func $f0) (elem declare func $f0)
  //   (func (export "setup") (global.set 0 (ref.func $f0))))
  std::array<WasmEdge::Byte, 63> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01,
      0x60, 0x00, 0x00, 0x03, 0x03, 0x02, 0x00, 0x00, 0x06, 0x0b, 0x02,
      0x70, 0x01, 0xd0, 0x70, 0x0b, 0x7f, 0x00, 0x41, 0x07, 0x0b, 0x07,
      0x09, 0x01, 0x05, 0x73, 0x65, 0x74, 0x75, 0x70, 0x00, 0x01, 0x09,
      0x05, 0x01, 0x03, 0x00, 0x01, 0x00, 0x0a, 0x0b, 0x02, 0x02, 0x00,
      0x0b, 0x06, 0x00, 0xd2, 0x00, 0x24, 0x00, 0x0b};
  ASSERT_TRUE(State.VM->loadWasm(Wasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  const std::array Modules{State.VM->getActiveModule()};
  for (const bool Setup : {false, true}) {
    if (Setup) {
      ASSERT_TRUE(State.VM->execute("setup"sv));
    }
    Coredump::Request Req;
    Req.Modules = Modules;
    AST::Module Mod;
    ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
    ASSERT_EQ(Mod.getGlobalSection().getContent().size(), 2U);
    const auto &Init =
        Mod.getGlobalSection().getContent()[0].getExpr().getInstrs()[0];
    EXPECT_EQ(Init.getOpCode(), OpCode::Ref__null);
    EXPECT_EQ(Init.getValType(), ValType(TypeCode::FuncRef));
    const auto *Section = findCustomSection(Mod, "wasmedge.globals"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader Reader(Section->getContent());
    EXPECT_EQ(Reader.readU32(), 1U);
    EXPECT_EQ(Reader.readU32(), 1U);
    EXPECT_EQ(Reader.readU32(), 0U);
    Reader.expectType(Modules[0]->getGlobalInstances()[0]->getGlobalType());
    EXPECT_EQ(Reader.readRef(),
              (std::array<uint32_t, 3>{Setup ? 1U : 0U, 0, 0}));
    EXPECT_FALSE(Reader.isFailed());
    EXPECT_TRUE(Reader.isEnd());
  }
}

TEST(CoredumpTest, ReadBack) {
  CoredumpState State;
  ASSERT_TRUE(State.VM->loadWasm(CoredumpReadBackWasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  const auto *Active = State.VM->getActiveModule();
  const std::array Modules{Active};
  for (const bool Setup : {false, true}) {
    SCOPED_TRACE(Setup);
    if (Setup) {
      ASSERT_TRUE(State.VM->execute("setup"sv));
    }
    Coredump::Request Req;
    Req.Modules = Modules;
    AST::Module Mod;
    ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
    const auto *Section = findCustomSection(Mod, "coreinstances"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader Core(Section->getContent());
    ASSERT_EQ(Core.readU32(), 1U);
    EXPECT_EQ(Core.readByte(), 0U);
    EXPECT_EQ(Core.readU32(), 0U);
    ASSERT_EQ(Core.readU32(), 1U);
    const auto MemoryIdx = Core.readU32();
    ASSERT_LT(MemoryIdx, Mod.getMemorySection().getContent().size());
    ASSERT_EQ(Core.readU32(), 2U);
    const auto RefGlobalIdx = Core.readU32();
    const auto NumGlobalIdx = Core.readU32();
    EXPECT_FALSE(Core.isFailed());
    EXPECT_TRUE(Core.isEnd());
    const auto *Memory = Active->getMemoryInstances()[0];
    EXPECT_EQ(
        Mod.getMemorySection().getContent()[MemoryIdx].getLimit().getMin(),
        Memory->getPageSize());
    std::vector<Byte> Bytes(Memory->getSize(), 0);
    for (const auto &Seg : Mod.getDataSection().getContent()) {
      EXPECT_EQ(Seg.getIdx(), MemoryIdx);
      const auto Offset = Seg.getExpr().getInstrs()[0].getNum().get<uint32_t>();
      ASSERT_LE(Offset + Seg.getData().size(), Bytes.size());
      std::copy(Seg.getData().begin(), Seg.getData().end(),
                Bytes.begin() + Offset);
    }
    const auto LiveBytes = Memory->getBytes(0, Memory->getSize());
    ASSERT_TRUE(LiveBytes);
    EXPECT_EQ(Bytes, (std::vector<Byte>(LiveBytes->begin(), LiveBytes->end())));
    EXPECT_EQ(Bytes[1024], Setup ? 0xFE : 0);
    EXPECT_EQ(Bytes[1025], Setup ? 0xCA : 0);
    const auto &Globals = Mod.getGlobalSection().getContent();
    ASSERT_LT(NumGlobalIdx, Globals.size());
    EXPECT_EQ(
        Globals[NumGlobalIdx].getExpr().getInstrs()[0].getNum().get<int32_t>(),
        Active->getGlobalInstances()[1]->getValue().get<int32_t>());
    Section = findCustomSection(Mod, "wasmedge.globals"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader GlobalReader(Section->getContent());
    EXPECT_EQ(GlobalReader.readU32(), 1U);
    ASSERT_EQ(GlobalReader.readU32(), 1U);
    EXPECT_EQ(GlobalReader.readU32(), RefGlobalIdx);
    GlobalReader.expectType(Active->getGlobalInstances()[0]->getGlobalType());
    const auto *GlobalFunc = GlobalReader.readFunction(Modules);
    EXPECT_EQ(GlobalFunc, Active->getGlobalInstances()[0]
                              ->getValue()
                              .get<RefVariant>()
                              .getPtr<Runtime::Instance::FunctionInstance>());
    EXPECT_EQ(GlobalFunc, Setup ? Active->getFunctionInstances()[0] : nullptr);
    EXPECT_FALSE(GlobalReader.isFailed());
    EXPECT_TRUE(GlobalReader.isEnd());

    Section = findCustomSection(Mod, "wasmedge.tables"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader Tables(Section->getContent());
    EXPECT_EQ(Tables.readU32(), 1U);
    ASSERT_EQ(Tables.readU32(), 1U);
    const auto *Table = Active->getTableInstances()[0];
    Tables.expectType(Table->getTableType());
    const auto Size = Tables.readU64();
    ASSERT_EQ(Size, Table->getSize());
    EXPECT_EQ(Size, Setup ? 4U : 2U);
    ASSERT_EQ(Mod.getTableSection().getContent().size(), 1U);
    EXPECT_EQ(Mod.getTableSection()
                  .getContent()[0]
                  .getTableType()
                  .getLimit()
                  .getMin(),
              Size);
    ASSERT_EQ(Tables.readU32(), Size);
    const auto Refs = Table->getRefs(0, Size);
    ASSERT_TRUE(Refs);
    for (const auto &Ref : *Refs) {
      EXPECT_EQ(Tables.readFunction(Modules),
                Ref.getPtr<Runtime::Instance::FunctionInstance>());
    }
    if (!Setup) {
      EXPECT_TRUE((*Refs)[1].isNull());
    }
    EXPECT_FALSE(Tables.isFailed());
    EXPECT_TRUE(Tables.isEnd());

    Section = findCustomSection(Mod, "wasmedge.instances"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader Instances(Section->getContent());
    EXPECT_EQ(Instances.readU32(), 1U);
    ASSERT_EQ(Instances.readU32(), Modules.size());
    const std::array DumpTables{Table};
    ASSERT_EQ(Instances.readU32(), Active->getTableInstances().size());
    const auto TableIdx = Instances.readU32();
    ASSERT_LT(TableIdx, DumpTables.size());
    EXPECT_EQ(DumpTables[TableIdx], Active->getTableInstances()[0]);
    const auto Data = Active->getOwnedDataInstances();
    ASSERT_EQ(Instances.readU32(), Data.size());
    for (size_t I = 0; I < Data.size(); ++I) {
      const auto Flag = Instances.readByte();
      EXPECT_EQ(Flag, Data[I]->isDropped());
      EXPECT_EQ(Flag, Setup && I == 1);
    }
    const auto Elems = Active->getElemInstances();
    ASSERT_EQ(Instances.readU32(), Elems.size());
    for (size_t I = 0; I < Elems.size(); ++I) {
      const auto Flag = Instances.readByte();
      EXPECT_EQ(Flag, Elems[I]->isDropped());
      EXPECT_EQ(Flag, I == 0 || (Setup && I == 2));
    }
    EXPECT_FALSE(Instances.isFailed());
    EXPECT_TRUE(Instances.isEnd());
  }
}

TEST(CoredumpTest, TypedReferences) {
  CoredumpState State;
  // (module
  //   (type $s (struct (field i32))) (type $f (func))
  //   (func $f0 (type $f))
  //   (table 1 (ref null $s))
  //   (table 1 (ref $f) (ref.func $f0))
  //   (table 1 (ref func) (ref.func $f0))
  //   (global (mut (ref null $s)) (ref.null $s))
  //   (global (ref $f) (ref.func $f0))
  //   (global (mut funcref) (ref.null func)))
  std::array<WasmEdge::Byte, 73> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02,
      0x5f, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x03, 0x02, 0x01, 0x01,
      0x04, 0x17, 0x03, 0x63, 0x00, 0x00, 0x01, 0x40, 0x00, 0x64, 0x01,
      0x00, 0x01, 0xd2, 0x00, 0x0b, 0x40, 0x00, 0x64, 0x70, 0x00, 0x01,
      0xd2, 0x00, 0x0b, 0x06, 0x12, 0x03, 0x63, 0x00, 0x01, 0xd0, 0x00,
      0x0b, 0x64, 0x01, 0x00, 0xd2, 0x00, 0x0b, 0x70, 0x01, 0xd0, 0x70,
      0x0b, 0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b};
  // (module (type $f (func)) (table 1 (ref null $f)))
  std::array<WasmEdge::Byte, 21> B{0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00,
                                   0x00, 0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
                                   0x04, 0x05, 0x01, 0x63, 0x00, 0x00, 0x01};
  ASSERT_TRUE(State.VM->registerModule("typed-b"sv, B));
  ASSERT_TRUE(State.VM->loadWasm(Wasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  const auto *Active = State.VM->getActiveModule();
  const auto *Other = State.VM->getStoreManager().findModule("typed-b"sv);
  ASSERT_NE(Other, nullptr);
  for (const bool Multiple : {false, true}) {
    SCOPED_TRACE(Multiple);
    std::vector<const Runtime::Instance::ModuleInstance *> Modules{Active};
    if (Multiple) {
      Modules.push_back(Other);
    }
    Coredump::Request Req;
    Req.Modules = Modules;
    AST::Module Mod;
    ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
    const auto &Tables = Mod.getTableSection().getContent();
    ASSERT_EQ(Tables.size(), Multiple ? 4U : 3U);
    const auto *Section = findCustomSection(Mod, "wasmedge.tables"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader TableReader(Section->getContent());
    EXPECT_EQ(TableReader.readU32(), 1U);
    EXPECT_EQ(TableReader.readU32(), Tables.size());
    for (size_t I = 0; I < Tables.size(); ++I) {
      EXPECT_EQ(Tables[I].getTableType().getRefType(),
                ValType(I == 0 ? TypeCode::AnyRef : TypeCode::FuncRef));
      const auto *Table = I < 3 ? Active->getTableInstances()[I]
                                : Other->getTableInstances()[0];
      TableReader.expectType(Table->getTableType());
      EXPECT_EQ(TableReader.readU64(), 1U);
      EXPECT_EQ(TableReader.readU32(), 1U);
      EXPECT_EQ(TableReader.readRef(),
                (std::array<uint32_t, 3>{I == 1 || I == 2 ? 1U : 0U, 0, 0}));
    }
    EXPECT_FALSE(TableReader.isFailed());
    EXPECT_TRUE(TableReader.isEnd());
    const auto &Globals = Mod.getGlobalSection().getContent();
    ASSERT_EQ(Globals.size(), 3U);
    Section = findCustomSection(Mod, "wasmedge.globals"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader GlobalReader(Section->getContent());
    EXPECT_EQ(GlobalReader.readU32(), 1U);
    EXPECT_EQ(GlobalReader.readU32(), 3U);
    for (uint32_t I = 0; I < 3; ++I) {
      const ValType Top(I == 0 ? TypeCode::AnyRef : TypeCode::FuncRef);
      EXPECT_EQ(Globals[I].getGlobalType().getValType(), Top);
      EXPECT_EQ(Globals[I].getGlobalType().getValMut(), ValMut::Const);
      const auto Instrs = Globals[I].getExpr().getInstrs();
      const auto &Init = Instrs[0];
      EXPECT_EQ(Init.getOpCode(), OpCode::Ref__null);
      EXPECT_EQ(Init.getValType(), Top);
      EXPECT_EQ(GlobalReader.readU32(), I);
      GlobalReader.expectType(Active->getGlobalInstances()[I]->getGlobalType());
      EXPECT_EQ(GlobalReader.readRef(),
                (std::array<uint32_t, 3>{I == 1 ? 1U : 0U, 0, 0}));
    }
    EXPECT_FALSE(GlobalReader.isFailed());
    EXPECT_TRUE(GlobalReader.isEnd());
    Section = findCustomSection(Mod, "wasmedge.instances"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader Instances(Section->getContent());
    EXPECT_EQ(Instances.readU32(), 1U);
    EXPECT_EQ(Instances.readU32(), Modules.size());
    EXPECT_EQ(Instances.readU32(), 3U);
    for (uint32_t I = 0; I < 3; ++I) {
      EXPECT_EQ(Instances.readU32(), I);
    }
    EXPECT_EQ(Instances.readU32(), 0U);
    EXPECT_EQ(Instances.readU32(), 0U);
    if (Multiple) {
      EXPECT_EQ(Instances.readU32(), 1U);
      EXPECT_EQ(Instances.readU32(), 3U);
      EXPECT_EQ(Instances.readU32(), 0U);
      EXPECT_EQ(Instances.readU32(), 0U);
    }
    EXPECT_FALSE(Instances.isFailed());
    EXPECT_TRUE(Instances.isEnd());
  }
}

TEST(CoredumpTest, EmptySegments) {
  CoredumpState State;
  // (module
  //   (memory 1) (data "") (data (i32.const 0) "")
  //   (elem funcref) (elem declare funcref)
  //   (func (export "setup") (data.drop 0) (elem.drop 0)))
  std::array<WasmEdge::Byte, 68> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07,
      0x09, 0x01, 0x05, 0x73, 0x65, 0x74, 0x75, 0x70, 0x00, 0x00, 0x09, 0x07,
      0x02, 0x05, 0x70, 0x00, 0x07, 0x70, 0x00, 0x0c, 0x01, 0x02, 0x0a, 0x0a,
      0x01, 0x08, 0x00, 0xfc, 0x09, 0x00, 0xfc, 0x0d, 0x00, 0x0b, 0x0b, 0x08,
      0x02, 0x01, 0x00, 0x00, 0x41, 0x00, 0x0b, 0x00};
  ASSERT_TRUE(State.VM->loadWasm(Wasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  const std::array Modules{State.VM->getActiveModule()};
  for (const bool Setup : {false, true}) {
    if (Setup) {
      ASSERT_TRUE(State.VM->execute("setup"sv));
    }
    Coredump::Request Req;
    Req.Modules = Modules;
    AST::Module Mod;
    ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
    const auto *Section = findCustomSection(Mod, "wasmedge.instances"sv);
    ASSERT_NE(Section, nullptr);
    CoredumpReader Reader(Section->getContent());
    EXPECT_EQ(Reader.readU32(), 1U);
    EXPECT_EQ(Reader.readU32(), 1U);
    EXPECT_EQ(Reader.readU32(), 0U);
    for (unsigned I = 0; I < 2; ++I) {
      EXPECT_EQ(Reader.readU32(), 2U);
      EXPECT_EQ(Reader.readByte(), Setup);
      EXPECT_EQ(Reader.readByte(), 1U);
    }
    EXPECT_FALSE(Reader.isFailed());
    EXPECT_TRUE(Reader.isEnd());
  }
}

TEST(CoredumpTest, ReferenceHierarchies) {
  CoredumpState State;
  State.Conf.addProposal(Proposal::ExceptionHandling);
  State.VM = std::make_unique<WasmEdge::VM::VM>(State.Conf);
  const std::array Types{ValType(TypeCode::NullFuncRef),
                         ValType(TypeCode::NullExternRef),
                         ValType(TypeCode::EqRef),
                         ValType(TypeCode::I31Ref),
                         ValType(TypeCode::StructRef),
                         ValType(TypeCode::ArrayRef),
                         ValType(TypeCode::NullRef),
                         ValType(TypeCode::ExnRef),
                         ValType(TypeCode::NullExnRef),
                         ValType(TypeCode::RefNull, uint32_t(999))};
  const std::array Tops{TypeCode::FuncRef, TypeCode::ExternRef,
                        TypeCode::AnyRef,  TypeCode::AnyRef,
                        TypeCode::AnyRef,  TypeCode::AnyRef,
                        TypeCode::AnyRef,  TypeCode::ExnRef,
                        TypeCode::ExnRef,  TypeCode::AnyRef};
  Runtime::Instance::ModuleInstance Host("hierarchies"sv);
  for (size_t I = 0; I < Types.size(); ++I) {
    ASSERT_TRUE(Host.addHostTable(
        std::to_string(I),
        std::make_unique<Runtime::Instance::TableInstance>(
            AST::TableType(Types[I], AST::Limit(130)), RefVariant(Types[I]))));
    ASSERT_TRUE(Host.addHostGlobal(
        std::to_string(I),
        std::make_unique<Runtime::Instance::GlobalInstance>(
            AST::GlobalType(Types[I], ValMut::Var), RefVariant(Types[I]))));
  }
  const std::array<const Runtime::Instance::ModuleInstance *, 1> Modules{&Host};
  Coredump::Request Req;
  Req.Modules = Modules;
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
  ASSERT_EQ(Mod.getTableSection().getContent().size(), Types.size());
  ASSERT_EQ(Mod.getGlobalSection().getContent().size(), Types.size());
  const auto *Section = findCustomSection(Mod, "wasmedge.tables"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Tables(Section->getContent());
  EXPECT_EQ(Tables.readU32(), 1U);
  EXPECT_EQ(Tables.readU32(), Types.size());
  Section = findCustomSection(Mod, "wasmedge.globals"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Globals(Section->getContent());
  EXPECT_EQ(Globals.readU32(), 1U);
  EXPECT_EQ(Globals.readU32(), Types.size());
  for (size_t I = 0; I < Types.size(); ++I) {
    SCOPED_TRACE(I);
    EXPECT_EQ(Mod.getTableSection().getContent()[I].getTableType().getRefType(),
              ValType(Tops[I]));
    EXPECT_EQ(
        Mod.getGlobalSection().getContent()[I].getGlobalType().getValType(),
        ValType(Tops[I]));
    Tables.expectType(Host.getTableInstances()[I]->getTableType());
    EXPECT_EQ(Tables.readU64(), 130U);
    EXPECT_EQ(Tables.readU32(), 130U);
    for (unsigned J = 0; J < 130; ++J) {
      EXPECT_EQ(Tables.readRef(), (std::array<uint32_t, 3>{0, 0, 0}));
    }
    EXPECT_EQ(Globals.readU32(), I);
    Globals.expectType(Host.getGlobalInstances()[I]->getGlobalType());
    EXPECT_EQ(Globals.readRef(), (std::array<uint32_t, 3>{0, 0, 0}));
  }
  EXPECT_FALSE(Tables.isFailed());
  EXPECT_TRUE(Tables.isEnd());
  EXPECT_FALSE(Globals.isFailed());
  EXPECT_TRUE(Globals.isEnd());
}

TEST(CoredumpTest, MissingGCReferences) {
  CoredumpState State;
  // (module
  //   (type $s (struct (field i32)))
  //   (table 1 (ref null $s))
  //   (global (mut (ref null $s)) (ref.null $s))
  //   (func (export "setup")
  //     (global.set 0 (struct.new $s (i32.const 7)))
  //     (table.set (i32.const 0) (global.get 0))))
  std::array<WasmEdge::Byte, 68> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x5f,
      0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x03, 0x02, 0x01, 0x01, 0x04, 0x05,
      0x01, 0x63, 0x00, 0x00, 0x01, 0x06, 0x07, 0x01, 0x63, 0x00, 0x01, 0xd0,
      0x00, 0x0b, 0x07, 0x09, 0x01, 0x05, 0x73, 0x65, 0x74, 0x75, 0x70, 0x00,
      0x00, 0x0a, 0x11, 0x01, 0x0f, 0x00, 0x41, 0x07, 0xfb, 0x00, 0x00, 0x24,
      0x00, 0x41, 0x00, 0x23, 0x00, 0x26, 0x00, 0x0b};
  ASSERT_TRUE(State.VM->loadWasm(Wasm));
  ASSERT_NO_FATAL_FAILURE(State.instantiate());
  ASSERT_TRUE(State.VM->execute("setup"sv));
  const auto *Active = State.VM->getActiveModule();
  const std::array Modules{Active};
  Coredump::Request Req;
  Req.Modules = Modules;
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
  const auto *Section = findCustomSection(Mod, "wasmedge.tables"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Tables(Section->getContent());
  EXPECT_EQ(Tables.readU32(), 1U);
  EXPECT_EQ(Tables.readU32(), 1U);
  Tables.expectType(Active->getTableInstances()[0]->getTableType());
  EXPECT_EQ(Tables.readU64(), 1U);
  EXPECT_EQ(Tables.readU32(), 1U);
  EXPECT_EQ(Tables.readRef(), (std::array<uint32_t, 3>{2, 0, 0}));
  EXPECT_FALSE(Tables.isFailed());
  EXPECT_TRUE(Tables.isEnd());
  Section = findCustomSection(Mod, "wasmedge.globals"sv);
  ASSERT_NE(Section, nullptr);
  CoredumpReader Globals(Section->getContent());
  EXPECT_EQ(Globals.readU32(), 1U);
  EXPECT_EQ(Globals.readU32(), 1U);
  EXPECT_EQ(Globals.readU32(), 0U);
  Globals.expectType(Active->getGlobalInstances()[0]->getGlobalType());
  EXPECT_EQ(Globals.readRef(), (std::array<uint32_t, 3>{2, 0, 0}));
  EXPECT_FALSE(Globals.isFailed());
  EXPECT_TRUE(Globals.isEnd());
}

TEST(CoredumpTest, WasmgdbReferenceGlobal) {
  CoredumpState State;
  Runtime::Instance::ModuleInstance Host("host"sv);
  ASSERT_TRUE(Host.addHostGlobal(
      "ref"sv, std::make_unique<Runtime::Instance::GlobalInstance>(
                   AST::GlobalType(TypeCode::StructRef, ValMut::Var),
                   RefVariant(TypeCode::StructRef))));
  const std::array<const Runtime::Instance::ModuleInstance *, 1> Modules{&Host};
  Coredump::Request Req;
  Req.Modules = Modules;
  for (const bool ForWasmgdb : {false, true}) {
    Req.ForWasmgdb = ForWasmgdb;
    AST::Module Mod;
    ASSERT_NO_FATAL_FAILURE(State.generate(Req, Mod));
    ASSERT_EQ(Mod.getGlobalSection().getContent().size(), 1U);
    const auto &Global = Mod.getGlobalSection().getContent()[0];
    const ValType Expected(ForWasmgdb ? TypeCode::StructRef : TypeCode::AnyRef);
    EXPECT_EQ(Global.getGlobalType().getValType(), Expected);
    const auto Instrs = Global.getExpr().getInstrs();
    EXPECT_EQ(Instrs[0].getOpCode(), OpCode::Ref__null);
    EXPECT_EQ(Instrs[0].getValType(), Expected);
    EXPECT_EQ(Mod.getCustomSections().size(), ForWasmgdb ? 4U : 7U);
  }
}

} // namespace
