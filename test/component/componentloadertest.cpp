// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/errinfo.h"
#include "validator/validator.h"
#include "vm/vm.h"

#include <gtest/gtest.h>

namespace {

using namespace WasmEdge;

TEST(ComponentLoader, LoadVariantWithRefinesByIndex) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM VM(Conf);

  // Component with variant using numeric index for refines
  // (type (variant
  //   (case "x")
  //   (case "y" string (refines 0))
  //   (case "z" string (refines 1))))
  std::vector<uint8_t> Vec = {0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00,
                              0x07, 0x13, 0x01, 0x71, 0x03, 0x01, 0x78, 0x00,
                              0x00, 0x01, 0x79, 0x01, 0x73, 0x01, 0x00, 0x01,
                              0x7a, 0x01, 0x73, 0x01, 0x01};

  ASSERT_TRUE(VM.loadWasm(Vec));
}

TEST(ComponentLoader, LoadVariantWithoutRefines) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  VM::VM VM(Conf);

  // Component with simple variant without refines (backward compatibility)
  // (type (variant (case "x")))
  std::vector<uint8_t> Vec = {0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00,
                              0x01, 0x00, 0x07, 0x07, 0x01, 0x71,
                              0x01, 0x01, 0x78, 0x00, 0x00};

  ASSERT_TRUE(VM.loadWasm(Vec));
}

} // namespace