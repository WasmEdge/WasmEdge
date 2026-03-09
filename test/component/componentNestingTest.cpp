// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "common/configure.h"
#include "loader/loader.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

using namespace WasmEdge;

// Component preamble (magic + version 0x0d 0x00 + layer 0x01 0x00).
static const std::vector<uint8_t> ComponentPreamble = {0x00, 0x61, 0x73, 0x6d,
                                                       0x0d, 0x00, 0x01, 0x00};

/// Encode an unsigned integer as LEB128.
static std::vector<uint8_t> encodeLEB128(uint64_t Value) {
  std::vector<uint8_t> Result;
  do {
    uint8_t Byte = Value & 0x7F;
    Value >>= 7;
    if (Value != 0) {
      Byte |= 0x80;
    }
    Result.push_back(Byte);
  } while (Value != 0);
  return Result;
}

/// Build a component binary with the given nesting depth.
///
/// Depth 0 means a bare component with no nested sections (just the preamble).
/// Depth N means a component containing a nested component section, which
/// itself contains depth N-1.
///
/// The binary is built from the inside out: start with an empty innermost
/// component, then repeatedly wrap it in a component section (ID 0x04).
static std::vector<uint8_t> buildNestedComponent(uint32_t Depth) {
  // Start with the innermost empty component body (no sections, just preamble).
  std::vector<uint8_t> Inner;

  for (uint32_t I = 0; I < Depth; ++I) {
    // The section content is: component preamble + inner body.
    // Section content size = preamble size + inner body size.
    uint64_t ContentSize =
        static_cast<uint64_t>(ComponentPreamble.size() + Inner.size());
    std::vector<uint8_t> SizeEnc = encodeLEB128(ContentSize);

    // Build the full section: section ID (0x04) + LEB128 size + preamble +
    // inner.
    std::vector<uint8_t> Section;
    Section.push_back(0x04); // Component section ID.
    Section.insert(Section.end(), SizeEnc.begin(), SizeEnc.end());
    Section.insert(Section.end(), ComponentPreamble.begin(),
                   ComponentPreamble.end());
    Section.insert(Section.end(), Inner.begin(), Inner.end());

    // The new inner body for the next nesting level is just this section.
    Inner = std::move(Section);
  }

  // The outermost component: preamble + the accumulated inner body.
  std::vector<uint8_t> Result;
  Result.insert(Result.end(), ComponentPreamble.begin(),
                ComponentPreamble.end());
  Result.insert(Result.end(), Inner.begin(), Inner.end());
  return Result;
}

TEST(ComponentNesting, WithinLimit) {
  // A component with nesting depth of 5 should load successfully.
  std::vector<uint8_t> Binary = buildNestedComponent(5);

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Loader::Loader Ldr(Conf);

  auto Result = Ldr.parseWasmUnit(Binary);
  ASSERT_TRUE(Result.has_value())
      << "Expected component with nesting depth 5 to load successfully";

  // Verify it parsed as a Component, not a Module.
  auto *Comp =
      std::get_if<std::unique_ptr<AST::Component::Component>>(&Result.value());
  ASSERT_NE(Comp, nullptr) << "Expected result to be a Component, not a Module";
}

TEST(ComponentNesting, ExceedsLimit) {
  // A component with nesting depth of 101 should fail.
  // MaxComponentNestingDepth is 100, so depth 101 must be rejected.
  std::vector<uint8_t> Binary = buildNestedComponent(101);

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Loader::Loader Ldr(Conf);

  auto Result = Ldr.parseWasmUnit(Binary);
  ASSERT_FALSE(Result.has_value())
      << "Expected component with nesting depth 101 to fail loading";
  EXPECT_EQ(Result.error(), ErrCode::Value::MalformedSection)
      << "Expected MalformedSection error for excessive nesting depth";
}

TEST(ComponentNesting, ExactlyAtLimit) {
  // buildNestedComponent(99) produces 99 nested section layers.
  // loadUnit calls loadComponent once at depth 0, then each nested section
  // triggers one more call, giving 100 total calls (depths 0..99).
  // Since MaxComponentNestingDepth is 100, all calls pass (depth < 100).
  std::vector<uint8_t> Binary = buildNestedComponent(99);

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Loader::Loader Ldr(Conf);

  auto Result = Ldr.parseWasmUnit(Binary);
  ASSERT_TRUE(Result.has_value())
      << "Expected component with nesting depth 99 (100 calls) to load "
         "successfully";
}

TEST(ComponentNesting, OneOverLimit) {
  // buildNestedComponent(100) triggers 101 loadComponent calls.
  // The last call at depth 100 should fail.
  std::vector<uint8_t> Binary = buildNestedComponent(100);

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Loader::Loader Ldr(Conf);

  auto Result = Ldr.parseWasmUnit(Binary);
  ASSERT_FALSE(Result.has_value())
      << "Expected component with nesting depth 100 (101 calls) to fail";
  EXPECT_EQ(Result.error(), ErrCode::Value::MalformedSection);
}

TEST(ComponentNesting, ManySiblingComponents) {
  // A component containing 150 sibling child components (not nested) should
  // load successfully. The depth limit only tracks recursive nesting, not total
  // loadComponent invocations. Each sibling is loaded at depth 2 (outer=0,
  // child=1), not 150.
  //
  // Build: outer preamble + 150 x (section_id + size + child_preamble)
  std::vector<uint8_t> Binary;
  Binary.insert(Binary.end(), ComponentPreamble.begin(),
                ComponentPreamble.end());

  // Each child is an empty component (just preamble, no sections).
  uint64_t ChildSize = ComponentPreamble.size();
  std::vector<uint8_t> SizeEnc = encodeLEB128(ChildSize);

  for (int I = 0; I < 150; ++I) {
    Binary.push_back(0x04); // Component section ID.
    Binary.insert(Binary.end(), SizeEnc.begin(), SizeEnc.end());
    Binary.insert(Binary.end(), ComponentPreamble.begin(),
                  ComponentPreamble.end());
  }

  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Loader::Loader Ldr(Conf);

  auto Result = Ldr.parseWasmUnit(Binary);
  ASSERT_TRUE(Result.has_value())
      << "Expected component with 150 sibling children to load successfully "
         "(depth limit applies to nesting, not total invocations)";
}

} // namespace
