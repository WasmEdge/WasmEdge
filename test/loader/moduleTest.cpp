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
      : Path(
            std::filesystem::temp_directory_path() /
            std::filesystem::u8path(std::string(Stem) + "-" +
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

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
