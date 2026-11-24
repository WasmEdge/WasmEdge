// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/loader/moduleTest.cpp - Load AST module unit tests --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains unit tests for loading the AST module node and the main
/// function.
///
//===----------------------------------------------------------------------===//

#include "common/filesystem.h"
#include "loader/loader.h"

#include <cstdint>
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace {

using namespace std::literals;

// Owns a uniquely named file under the system temporary directory and removes
// it when the scope exits, including when an assertion returns early.
class ScopedTempFile {
public:
  explicit ScopedTempFile(std::string_view Stem,
                          std::string_view Extension = ""sv)
      : Path(std::filesystem::temp_directory_path() /
             WasmEdge::u8path(std::string(Stem) + "-" +
                              std::to_string(std::hash<std::thread::id>{}(
                                  std::this_thread::get_id())) +
                              std::string(Extension))) {}
  ScopedTempFile(const ScopedTempFile &) = delete;
  ScopedTempFile &operator=(const ScopedTempFile &) = delete;
  ~ScopedTempFile() noexcept {
    std::error_code EC;
    std::filesystem::remove(Path, EC);
  }

  const std::filesystem::path &get() const noexcept { return Path; }

private:
  std::filesystem::path Path;
};

WasmEdge::Configure Conf;
WasmEdge::Loader::Loader Ldr(Conf);

TEST(ModuleTest, LoadModule) {
  std::vector<uint8_t> Vec;

  // 1. Test load empty file
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 2. Test load empty module
  Vec = {0x00U, 0x61U, 0x73U, 0x6DU, 0x01U, 0x00U, 0x00U, 0x00U};
  EXPECT_TRUE(Ldr.parseModule(Vec));

  // 3. Test load module with valid empty sections
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU,                      // Magic
      0x01U, 0x00U, 0x00U, 0x00U,                      // Version
      0x00U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Custom section
      0x01U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Type section
      0x02U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Import section
      0x03U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Function section
      0x04U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Table section
      0x05U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Memory section
      0x06U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Global section
      0x07U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Export section
      0x08U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Start section
      0x09U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Element section
      0x0AU, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Code section
      0x0BU, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U  // Data section
  };
  EXPECT_TRUE(Ldr.parseModule(Vec));

  // 4. Test load module with invalid sections
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU,                      // Magic
      0x01U, 0x00U, 0x00U, 0x00U,                      // Version
      0x00U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Custom section
      0x01U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Type section
      0x02U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Import section
      0x03U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Function section
      0x04U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Table section
      0x05U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Memory section
      0x06U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Global section
      0x07U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Export section
      0x08U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Start section
      0x09U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Element section
      0x0AU, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Code section
      0x0BU, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, // Data section
      0x0DU, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U  // Invalid section
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));
}

TEST(ModuleTest, LoadDataCountSecModule) {
  Conf.setWASMStandard(WasmEdge::Standard::WASM_1);
  WasmEdge::Loader::Loader LdrWASM1(Conf);
  std::vector<uint8_t> Vec;

  // 5. Test load module with invalid datacount section without proposals.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU,       // Magic
      0x01U, 0x00U, 0x00U, 0x00U,       // Version
      0x0CU, 0x01U, 0x01U,              // DataCount section
      0x0BU, 0x03U, 0x01U, 0x01U, 0x00U // Data section
  };
  EXPECT_FALSE(LdrWASM1.parseModule(Vec));

  // 6. Test load module with invalid datacount section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x0CU, 0x00U                // DataCount section, miss data count
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));
}

TEST(ModuleTest, LoadStartSecModule) {
  // 7. Test load module with invalid start section.
  std::vector<unsigned char> Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x08U, 0x00U                // Start section, miss index
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));
}

TEST(ModuleTest, LoadDupSecModule) {
  std::vector<uint8_t> Vec;

  // 8. Test load module with duplicated type section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x01U, 0x01U, 0x00U,        // Type section
      0x01U, 0x01U, 0x00U         // Type section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 9. Test load module with duplicated import section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x02U, 0x01U, 0x00U,        // Import section
      0x02U, 0x01U, 0x00U         // Import section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 10. Test load module with duplicated function section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x03U, 0x01U, 0x00U,        // Function section
      0x03U, 0x01U, 0x00U         // Function section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 11. Test load module with duplicated table section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x04U, 0x01U, 0x00U,        // Table section
      0x04U, 0x01U, 0x00U         // Table section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 12. Test load module with duplicated memory section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x05U, 0x01U, 0x00U,        // Memory section
      0x05U, 0x01U, 0x00U         // Memory section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 13. Test load module with duplicated global section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x06U, 0x01U, 0x00U,        // Global section
      0x06U, 0x01U, 0x00U         // Global section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 14. Test load module with duplicated export section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x07U, 0x01U, 0x00U,        // Export section
      0x07U, 0x01U, 0x00U         // Export section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 15. Test load module with duplicated start section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x08U, 0x01U, 0x00U,        // Start section
      0x08U, 0x01U, 0x00U         // Start section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 16. Test load module with duplicated element section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x09U, 0x01U, 0x00U,        // Element section
      0x09U, 0x01U, 0x00U         // Element section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 17. Test load module with duplicated code section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x0AU, 0x01U, 0x00U,        // Code section
      0x0AU, 0x01U, 0x00U         // Code section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 18. Test load module with duplicated data section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x0BU, 0x01U, 0x00U,        // Data section
      0x0BU, 0x01U, 0x00U         // Data section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  // 19. Test load module with duplicated datacount section.
  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x0CU, 0x01U, 0x00U,        // Datacount section
      0x0CU, 0x01U, 0x00U         // Datacount section duplicated
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));
}

TEST(ModuleTest, CopyModule) {
  // 20. Test copy module with tags.
  std::vector<uint8_t> Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU,                      // Magic
      0x01U, 0x00U, 0x00U, 0x00U,                      // Version
      0x01U, 0x05U, 0x01U, 0x60U, 0x01U, 0x7FU, 0x00U, // Type section
      0x02U, 0x08U, 0x01U, 0x01U, 0x6DU, 0x01U, 0x74U, // Import section
      0x04U, 0x00U, 0x00U,                             // Tag import
      0x0DU, 0x03U, 0x01U, 0x00U, 0x00U                // Tag section
  };
  auto Mod = Ldr.parseModule(Vec);
  ASSERT_TRUE(Mod);

  WasmEdge::AST::Module Copied(**Mod);
  WasmEdge::AST::Module Assigned;
  Assigned = **Mod;
  for (const WasmEdge::AST::Module *Copy : {&Copied, &Assigned}) {
    const auto &Type = Copy->getTypeSection().getContent()[0];
    const auto &ImpDesc = Copy->getImportSection().getContent()[0];
    EXPECT_EQ(&ImpDesc.getExternalTagType().getDefType(), &Type);
    EXPECT_EQ(&Copy->getTagSection().getContent()[0].getDefType(), &Type);
  }
}

TEST(ModuleTest, LoadNativeLibraryMagicNotInAOTMode) {
  const std::vector<std::pair<std::string, std::vector<uint8_t>>> Cases = {
      {"elf", {0x7FU, 0x45U, 0x4CU, 0x46U}},
      {"dll", {0x4DU, 0x5AU}},
      {"macho32", {0xCEU, 0xFAU, 0xEDU, 0xFEU}},
      {"macho64", {0xCFU, 0xFAU, 0xEDU, 0xFEU}}};

  for (const auto &[Name, Magic] : Cases) {
    const ScopedTempFile Artifact("wasmedgeLoaderNativeMagic-" + Name);
    const auto &Path = Artifact.get();
    {
      std::ofstream Fout(Path, std::ios::out | std::ios::binary);
      ASSERT_TRUE(Fout) << Name;
      const std::vector<uint8_t> Junk(64, 0x00U);
      Fout.write(reinterpret_cast<const char *>(Magic.data()),
                 static_cast<std::streamsize>(Magic.size()));
      Fout.write(reinterpret_cast<const char *>(Junk.data()),
                 static_cast<std::streamsize>(Junk.size()));
    }

    auto Res = Ldr.parseModule(Path);
    EXPECT_FALSE(Res) << Name;
    if (!Res) {
      EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::MalformedMagic) << Name;
    }
  }
}

// The text format input is experimental. The loader reads WAT text from a
// buffer and from a file only with the EnableWAT flag. Without it, the text
// goes to the binary loader, which rejects the magic header.
TEST(ModuleTest, LoadTextFormatNeedsFlag) {
  const std::string_view Text = "(module (func (export \"f\")))"sv;
  const WasmEdge::Span<const uint8_t> Bytes(
      reinterpret_cast<const uint8_t *>(Text.data()), Text.size());
  const ScopedTempFile Artifact("wasmedgeLoaderTextFormat", ".wat"sv);
  {
    std::ofstream Fout(Artifact.get(), std::ios::out | std::ios::binary);
    ASSERT_TRUE(Fout);
    Fout.write(Text.data(), static_cast<std::streamsize>(Text.size()));
  }

  auto Res = Ldr.parseModule(Bytes);
  EXPECT_FALSE(Res);
  if (!Res) {
    EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::MalformedMagic);
  }
  Res = Ldr.parseModule(Artifact.get());
  EXPECT_FALSE(Res);
  if (!Res) {
    EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::MalformedMagic);
  }

  WasmEdge::Configure TextConf;
  TextConf.setEnableWAT(true);
  WasmEdge::Loader::Loader TextLdr(TextConf);
  Res = TextLdr.parseModule(Bytes);
  EXPECT_TRUE(Res);
  if (Res) {
    EXPECT_EQ((*Res)->getExportSection().getContent().size(), 1U);
  }
  Res = TextLdr.parseModule(Artifact.get());
  EXPECT_TRUE(Res);
  if (Res) {
    EXPECT_EQ((*Res)->getExportSection().getContent().size(), 1U);
  }
}

// Nonempty text with only whitespace and comments is an empty module. The
// file and the buffer entry points find it as WAT, as they do for a '(;'
// block comment. An empty input is also an empty binary, so it stays with the
// binary loader, which reports the missing header. The file entry point and
// the buffer entry point are tested apart, because a file of zero bytes can
// fail before the format test.
TEST(ModuleTest, LoadTextFormatEmptyModule) {
  WasmEdge::Configure TextConf;
  TextConf.setEnableWAT(true);
  WasmEdge::Loader::Loader TextLdr(TextConf);
  for (const std::string_view Text :
       {";; comment\n"sv, "(; comment ;)"sv, "  \n"sv}) {
    SCOPED_TRACE(Text);
    const WasmEdge::Span<const uint8_t> Bytes(
        reinterpret_cast<const uint8_t *>(Text.data()), Text.size());
    const ScopedTempFile Artifact("wasmedgeLoaderTextEmpty", ".wat"sv);
    {
      std::ofstream Fout(Artifact.get(), std::ios::out | std::ios::binary);
      ASSERT_TRUE(Fout);
      Fout.write(Text.data(), static_cast<std::streamsize>(Text.size()));
    }
    auto Res = TextLdr.parseModule(Bytes);
    ASSERT_TRUE(Res);
    EXPECT_TRUE((*Res)->getExportSection().getContent().empty());
    Res = TextLdr.parseModule(Artifact.get());
    ASSERT_TRUE(Res);
    EXPECT_TRUE((*Res)->getExportSection().getContent().empty());
  }

  auto Res = TextLdr.parseModule(WasmEdge::Span<const uint8_t>());
  EXPECT_FALSE(Res);
  if (!Res) {
    EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::UnexpectedEnd);
  }
  const ScopedTempFile Empty("wasmedgeLoaderEmptyFile", ".wat"sv);
  {
    std::ofstream Fout(Empty.get(), std::ios::out | std::ios::binary);
    ASSERT_TRUE(Fout);
  }
  Res = TextLdr.parseModule(Empty.get());
  EXPECT_FALSE(Res);
  if (!Res) {
    EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::UnexpectedEnd);
  }
}

// One Loader parses several inputs in a row. A WAT parse must not leave the
// state of an earlier parse behind, and a binary parse after a WAT parse must
// still work.
TEST(ModuleTest, LoadTextFormatKeepsLoaderState) {
  const std::string_view Text = "(module (func (export \"f\")))"sv;
  const WasmEdge::Span<const uint8_t> TextBytes(
      reinterpret_cast<const uint8_t *>(Text.data()), Text.size());
  const std::vector<uint8_t> Binary = {0x00U, 0x61U, 0x73U, 0x6DU,
                                       0x01U, 0x00U, 0x00U, 0x00U};
  const ScopedTempFile Artifact("wasmedgeLoaderTextState", ".wat"sv);
  {
    std::ofstream Fout(Artifact.get(), std::ios::out | std::ios::binary);
    ASSERT_TRUE(Fout);
    Fout.write(Text.data(), static_cast<std::streamsize>(Text.size()));
  }

  WasmEdge::Configure TextConf;
  TextConf.setEnableWAT(true);
  WasmEdge::Loader::Loader TextLdr(TextConf);

  // A file parse, then a buffer parse, then a binary parse, on one Loader.
  EXPECT_TRUE(TextLdr.parseModule(Artifact.get()));
  EXPECT_TRUE(TextLdr.parseModule(TextBytes));
  EXPECT_TRUE(TextLdr.parseModule(WasmEdge::Span<const uint8_t>(Binary)));
  EXPECT_TRUE(TextLdr.parseModule(Artifact.get()));
  // A malformed binary after a WAT parse reports its own error, and not an
  // error of the earlier input.
  const std::vector<uint8_t> Bad = {0x00U, 0x61U, 0x73U, 0x6DU};
  auto Res = TextLdr.parseModule(WasmEdge::Span<const uint8_t>(Bad));
  EXPECT_FALSE(Res);
  if (!Res) {
    EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::UnexpectedEnd);
  }
}

// The input of the AOT compiler keeps its format. A binary input gives its
// own bytes, with no copy. A WAT input gives the binary of the serializer.
TEST(ModuleTest, CompileInputKeepsFormat) {
  WasmEdge::Configure TextConf;
  TextConf.setEnableWAT(true);
  WasmEdge::Loader::Loader TextLdr(TextConf);
  const std::vector<uint8_t> Binary = {0x00U, 0x61U, 0x73U, 0x6DU,
                                       0x01U, 0x00U, 0x00U, 0x00U};
  auto BinInput = TextLdr.parseCompileInput(Binary);
  ASSERT_TRUE(BinInput);
  EXPECT_FALSE(BinInput->IsWat);
  auto BinOutput = TextLdr.compileBinary(*BinInput);
  ASSERT_TRUE(BinOutput);
  EXPECT_EQ(BinOutput->data(), Binary.data());

  const std::string_view Text = "(module (func (export \"f\")))"sv;
  auto TextInput = TextLdr.parseCompileInput(WasmEdge::Span<const uint8_t>(
      reinterpret_cast<const uint8_t *>(Text.data()), Text.size()));
  ASSERT_TRUE(TextInput);
  EXPECT_TRUE(TextInput->IsWat);
  auto TextOutput = TextLdr.compileBinary(*TextInput);
  ASSERT_TRUE(TextOutput);
  ASSERT_GE(TextOutput->size(), 4U);
  EXPECT_EQ((*TextOutput)[0], 0x00U);
  EXPECT_EQ((*TextOutput)[1], 0x61U);
  auto Reloaded = TextLdr.parseModule(*TextOutput);
  ASSERT_TRUE(Reloaded);
  EXPECT_EQ((*Reloaded)->getExportSection().getContent().size(), 1U);
}

// The text format converter and the binary loader share the AST rules for the
// block metadata and for the tag types. The text form of a module and the
// binary form that the serializer makes from it must then give the same jump
// offsets, end flags, and tag types.
TEST(ModuleTest, TextAndBinaryMetadataMatch) {
  const std::string_view Text =
      "(module (tag $e) (func (param i32) (result i32)\n"
      "  (block (result i32) (loop (br_if 1 (i32.const 1) (local.get 0)))\n"
      "    (i32.const 2))\n"
      "  (if (result i32) (local.get 0) (then (i32.const 3))\n"
      "    (else (i32.const 4)))\n"
      "  drop\n"
      "  (if (local.get 0) (then nop))\n"
      "  (block $l (try_table (catch $e $l) (throw $e)))))"sv;
  WasmEdge::Configure TextConf;
  TextConf.setEnableWAT(true);
  WasmEdge::Loader::Loader TextLdr(TextConf);
  auto TextMod = TextLdr.parseModule(WasmEdge::Span<const uint8_t>(
      reinterpret_cast<const uint8_t *>(Text.data()), Text.size()));
  ASSERT_TRUE(TextMod);
  auto Binary = TextLdr.serializeModule(**TextMod);
  ASSERT_TRUE(Binary);
  auto BinMod = TextLdr.parseModule(*Binary);
  ASSERT_TRUE(BinMod);

  const auto &TextInstrs =
      (*TextMod)->getCodeSection().getContent()[0].getExpr().getInstrs();
  const auto &BinInstrs =
      (*BinMod)->getCodeSection().getContent()[0].getExpr().getInstrs();
  ASSERT_EQ(TextInstrs.size(), BinInstrs.size());
  for (size_t I = 0; I < TextInstrs.size(); ++I) {
    SCOPED_TRACE(I);
    const auto &T = TextInstrs[I];
    const auto &B = BinInstrs[I];
    ASSERT_EQ(T.getOpCode(), B.getOpCode());
    switch (T.getOpCode()) {
    case WasmEdge::OpCode::If:
      EXPECT_EQ(T.getJumpElse(), B.getJumpElse());
      [[fallthrough]];
    case WasmEdge::OpCode::Block:
    case WasmEdge::OpCode::Loop:
    case WasmEdge::OpCode::Else:
      EXPECT_EQ(T.getJumpEnd(), B.getJumpEnd());
      break;
    case WasmEdge::OpCode::Try_table:
      EXPECT_EQ(T.getTryCatch().JumpEnd, B.getTryCatch().JumpEnd);
      break;
    case WasmEdge::OpCode::End:
      EXPECT_EQ(T.isExprLast(), B.isExprLast());
      EXPECT_EQ(T.isTryBlockLast(), B.isTryBlockLast());
      break;
    default:
      break;
    }
  }

  const auto &TextTag = (*TextMod)->getTagSection().getContent()[0];
  const auto &BinTag = (*BinMod)->getTagSection().getContent()[0];
  EXPECT_EQ(&TextTag.getDefType(),
            &(*TextMod)->getTypeSection().getContent()[TextTag.getTypeIdx()]);
  EXPECT_EQ(&BinTag.getDefType(),
            &(*BinMod)->getTypeSection().getContent()[BinTag.getTypeIdx()]);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
