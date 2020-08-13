// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/ast/sectionTest.cpp - AST section unit tests ------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST section nodes.
///
//===----------------------------------------------------------------------===//

#include "common/ast/section.h"
#include "loader/filemgr.h"
#include "gtest/gtest.h"

namespace {

SSVM::FileMgrVector Mgr;

TEST(SectionTest, LoadCustomSection) {
  /// 1. Test load custom section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load custom section without contents.
  ///   3.  Load custom section with contents.
  Mgr.clearBuffer();
  SSVM::AST::CustomSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::CustomSection Sec2;
  EXPECT_TRUE(Sec2.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x87U, 0x80U, 0x80U, 0x80U, 0x00U,              /// Content size = 7
      0x00U, 0xFFU, 0xEEU, 0xDDU, 0xCCU, 0xBBU, 0xAAU /// Content
  };
  Mgr.setCode(Vec3);
  SSVM::AST::CustomSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SectionTest, LoadTypeSection) {
  /// 2. Test load type section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load type section without contents.
  ///   3.  Load type section with zero vector length.
  ///   4.  Load type section with contents.
  Mgr.clearBuffer();
  SSVM::AST::TypeSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::TypeSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x81U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 1
      0x00U                              /// Vector length = 0
  };
  Mgr.setCode(Vec3);
  SSVM::AST::TypeSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x93U, 0x80U, 0x80U, 0x80U, 0x00U,        /// Content size = 19
      0x03U,                                    /// Vector length = 3
      0x60U, 0x02U, 0x7CU, 0x7DU, 0x01U, 0x7CU, /// vec[0]
      0x60U, 0x02U, 0x7DU, 0x7EU, 0x01U, 0x7DU, /// vec[1]
      0x60U, 0x02U, 0x7EU, 0x7FU, 0x01U, 0x7EU  /// vec[2]
  };
  Mgr.setCode(Vec4);
  SSVM::AST::TypeSection Sec4;
  EXPECT_TRUE(Sec4.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SectionTest, LoadImportSection) {
  /// 3. Test load import section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load import section without contents.
  ///   3.  Load import section with zero vector length.
  ///   4.  Load import section with contents.
  Mgr.clearBuffer();
  SSVM::AST::ImportSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::ImportSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x81U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 1
      0x00U                              /// Vector length = 0
  };
  Mgr.setCode(Vec3);
  SSVM::AST::ImportSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0xAEU, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 46
      0x03U,                             /// Vector length = 3
      /// vec[0]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               /// ModName: "test"
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// ExtName: "Loader"
      0x00U, 0x00U,                                    /// function type
      /// vec[1]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               /// ModName: "test"
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// ExtName: "Loader"
      0x02U, 0x01U, 0x00U, 0x0FU,                      /// Memory type
      /// vec[2]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               /// ModName: "test"
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// ExtName: "Loader"
      0x03U, 0x7CU, 0x00U                              /// Global type
  };
  Mgr.setCode(Vec4);
  SSVM::AST::ImportSection Sec4;
  EXPECT_TRUE(Sec4.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SectionTest, LoadFunctionSection) {
  /// 4. Test load function section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load function section without contents.
  ///   3.  Load function section with zero vector length.
  ///   4.  Load function section with contents.
  Mgr.clearBuffer();
  SSVM::AST::FunctionSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::FunctionSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x81U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 1
      0x00U,                             /// Vector length = 0
  };
  Mgr.setCode(Vec3);
  SSVM::AST::FunctionSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x89U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 9
      0x03U,                             /// Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// vec[0]
      0x00U,                             /// vec[1]
      0xB9U, 0x60U                       /// vec[2]
  };
  Mgr.setCode(Vec4);
  SSVM::AST::FunctionSection Sec4;
  EXPECT_TRUE(Sec4.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SectionTest, LoadTableSection) {
  /// 5. Test load table section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load table section without contents.
  ///   3.  Load table section with zero vector length.
  ///   4.  Load table section with contents.
  Mgr.clearBuffer();
  SSVM::AST::TableSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::TableSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x81U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 1
      0x00U                              /// Vector length = 0
  };
  Mgr.setCode(Vec3);
  SSVM::AST::TableSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x8DU, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 13
      0x03U,                             /// Vector length = 3
      0x70U, 0x01U, 0x00U, 0x0FU,        /// vec[0]
      0x70U, 0x01U, 0x00U, 0x0EU,        /// vec[1]
      0x70U, 0x01U, 0x00U, 0x0DU         /// vec[2]
  };
  Mgr.setCode(Vec4);
  SSVM::AST::TableSection Sec4;
  EXPECT_TRUE(Sec4.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SectionTest, LoadMemorySection) {
  /// 6. Test load memory section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load memory section without contents.
  ///   3.  Load memory section with zero vector length.
  ///   4.  Load memory section with contents.
  Mgr.clearBuffer();
  SSVM::AST::MemorySection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::MemorySection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x81U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 1
      0x00U                              /// Vector length = 0
  };
  Mgr.setCode(Vec3);
  SSVM::AST::MemorySection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x8AU, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 10
      0x03U,                             /// Vector length = 3
      0x01U, 0x00U, 0x0FU,               /// vec[0]
      0x01U, 0x00U, 0x0EU,               /// vec[1]
      0x01U, 0x00U, 0x0DU                /// vec[2]
  };
  Mgr.setCode(Vec4);
  SSVM::AST::MemorySection Sec4;
  EXPECT_TRUE(Sec4.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SectionTest, LoadGlobalSection) {
  /// 7. Test load global section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load global section without contents.
  ///   3.  Load global section with zero vector length.
  ///   4.  Load global section with contents.
  Mgr.clearBuffer();
  SSVM::AST::GlobalSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::GlobalSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x81U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 1
      0x00U                              /// Vector length = 0
  };
  Mgr.setCode(Vec3);
  SSVM::AST::GlobalSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x8DU, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 13
      0x03U,                             /// Vector length = 3
      0x7CU, 0x00U, 0x0BU,               /// vec[0]
      0x7DU, 0x00U, 0x45U, 0x0BU,        /// vec[1]
      0x7EU, 0x01U, 0x46U, 0x47U, 0x0BU  /// vec[2]
  };
  Mgr.setCode(Vec4);
  SSVM::AST::GlobalSection Sec4;
  EXPECT_TRUE(Sec4.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SectionTest, LoadExportSection) {
  /// 8. Test load export section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load export section without contents.
  ///   3.  Load export section with zero vector length.
  ///   4.  Load export section with contents.
  Mgr.clearBuffer();
  SSVM::AST::ExportSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::ExportSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x81U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 1
      0x00U                              /// Vector length = 0
  };
  Mgr.setCode(Vec3);
  SSVM::AST::ExportSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0xA8U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 40
      0x03U,                             /// Vector length = 3
      /// vec[0]
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// ExtName: Loader
      0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU,        /// function type and idx
      /// vec[1]
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// ExtName: Loader
      0x01U, 0xFDU, 0xFFU, 0xFFU, 0xFFU, 0x0FU,        /// Table type and idx
      /// vec[2]
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, /// ExtName: Loader
      0x02U, 0xFBU, 0xFFU, 0xFFU, 0xFFU, 0x0FU         /// Memory type and idx
  };
  Mgr.setCode(Vec4);
  SSVM::AST::ExportSection Sec4;
  EXPECT_TRUE(Sec4.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SectionTest, LoadStartSection) {
  /// 9. Test load start section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load start section without contents.
  ///   3.  Load start section with contents.
  Mgr.clearBuffer();
  SSVM::AST::StartSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::StartSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x85U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 5
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  /// Content
  };
  Mgr.setCode(Vec3);
  SSVM::AST::StartSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SectionTest, LoadElementSection) {
  /// 10. Test load element section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load element section without contents.
  ///   3.  Load element section with zero vector length.
  ///   4.  Load element section with contents.
  Mgr.clearBuffer();
  SSVM::AST::ElementSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::ElementSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x81U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 1
      0x00U                              /// Vector length = 0
  };
  Mgr.setCode(Vec3);
  SSVM::AST::ElementSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x9CU, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 28
      0x03U,                             /// Vector length = 3
      /// vec[0]
      0x00U,                      /// Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU, /// Expression
      0x03U, 0x00U, 0x0AU, 0x0FU, /// Vec(3)
      /// vec[1]
      0x00U,                      /// Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU, /// Expression
      0x03U, 0x0AU, 0x0BU, 0x0CU, /// Vec(3)
      /// vec[2]
      0x00U,                      /// Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU, /// Expression
      0x03U, 0x03U, 0x06U, 0x09U  /// Vec(3)
  };
  Mgr.setCode(Vec4);
  SSVM::AST::ElementSection Sec4;
  EXPECT_TRUE(Sec4.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SectionTest, LoadCodeSection) {
  /// 11. Test load code section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load code section without contents.
  ///   3.  Load code section with zero vector length.
  ///   4.  Load code section with contents.
  Mgr.clearBuffer();
  SSVM::AST::CodeSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::CodeSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x81U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 1
      0x00U                              /// Vector length = 0
  };
  Mgr.setCode(Vec3);
  SSVM::AST::CodeSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0xABU, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 43
      0x03U,                             /// Vector length = 3
      /// vec[0]
      0x89U, 0x80U, 0x80U, 0x80U, 0x00U, /// Code segment size = 9
      0x02U, 0x01U, 0x7CU, 0x02U, 0x7DU, /// Local vec(2)
      0x45U, 0x46U, 0x47U, 0x0BU,        /// Expression
      /// vec[1]
      0x89U, 0x80U, 0x80U, 0x80U, 0x00U, /// Code segment size = 9
      0x02U, 0x03U, 0x7CU, 0x04U, 0x7DU, /// Local vec(2)
      0x45U, 0x46U, 0x47U, 0x0BU,        /// Expression
      /// vec[2]
      0x89U, 0x80U, 0x80U, 0x80U, 0x00U, /// Code segment size = 9
      0x02U, 0x05U, 0x7CU, 0x06U, 0x7DU, /// Local vec(2)
      0x45U, 0x46U, 0x47U, 0x0BU         /// Expression
  };
  Mgr.setCode(Vec4);
  SSVM::AST::CodeSection Sec4;
  EXPECT_TRUE(Sec4.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

TEST(SectionTest, LoadDataSection) {
  /// 12. Test load data section.
  ///
  ///   1.  Load invalid empty section.
  ///   2.  Load data section without contents.
  ///   3.  Load data section with zero vector length.
  ///   4.  Load data section with contents.
  Mgr.clearBuffer();
  SSVM::AST::DataSection Sec1;
  EXPECT_FALSE(Sec1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x80U, 0x80U, 0x80U, 0x80U, 0x00U /// Content size = 0
  };
  Mgr.setCode(Vec2);
  SSVM::AST::DataSection Sec2;
  EXPECT_FALSE(Sec2.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x81U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 1
      0x00U                              /// Vector length = 0
  };
  Mgr.setCode(Vec3);
  SSVM::AST::DataSection Sec3;
  EXPECT_TRUE(Sec3.loadBinary(Mgr) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0xA0U, 0x80U, 0x80U, 0x80U, 0x00U, /// Content size = 43
      0x03U,                             /// Vector length = 3
      /// vec[0]
      0x00U,                             /// Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU,        /// Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U, /// Vector length = 4, "test"
      /// vec[1]
      0x01U,                             /// Prefix 0x01
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U, /// Vector length = 4, "test"
      /// vec[2]
      0x02U,                             /// Prefix 0x02
      0xF0U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// Memory index
      0x45U, 0x46U, 0x47U, 0x0BU,        /// Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U  /// Vector length = 4, "test"
  };
  Mgr.setCode(Vec4);
  SSVM::AST::DataSection Sec4;
  EXPECT_TRUE(Sec4.loadBinary(Mgr) && Mgr.getRemainSize() == 0);
}

} // namespace
