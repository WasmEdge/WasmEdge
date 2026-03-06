// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

//===-- wasmedge/test/executor/callStackDepthTest.cpp ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for the call stack depth limit enforcement.
/// It verifies that deeply recursive Wasm functions are terminated with
/// a CallStackOverflow error rather than exhausting the native stack.
/// It also directly tests StackManager::pushFrame depth checking.
///
//===----------------------------------------------------------------------===//

#include "ast/instruction.h"
#include "common/spdlog.h"
#include "runtime/stackmgr.h"
#include "vm/vm.h"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

using namespace WasmEdge;

// ---------------------------------------------------------------------------
// Wasm-level tests
// ---------------------------------------------------------------------------

// A minimal Wasm module with a single function that calls itself infinitely.
//
// WAT equivalent:
//   (module
//     (type (func))
//     (func (export "recurse") (type 0) (call 0))
//   )
//
std::array<Byte, 39> InfiniteRecursionWasm{
    // Magic + Version
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    // Type section (id=1, size=4): 1 type -> () -> ()
    0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
    // Function section (id=3, size=2): 1 func, type 0
    0x03, 0x02, 0x01, 0x00,
    // Export section (id=7, size=11): 1 export "recurse" -> func 0
    0x07, 0x0b, 0x01,
    0x07,                                     // string length 7
    0x72, 0x65, 0x63, 0x75, 0x72, 0x73, 0x65, // "recurse"
    0x00,                                     // export kind: func
    0x00,                                     // func index 0
    // Code section (id=10, size=6): 1 body
    0x0a, 0x06, 0x01,
    0x04,       // body size = 4
    0x00,       // 0 local declaration groups
    0x10, 0x00, // call func 0
    0x0b        // end
};

// A Wasm module with two functions that call each other (mutual recursion).
//
// WAT equivalent:
//   (module
//     (type (func))
//     (func $a (export "ping") (type 0) (call 1))
//     (func $b (type 0) (call 0))
//   )
//
std::array<Byte, 42> MutualRecursionWasm{
    // Magic + Version
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    // Type section (id=1, size=4): 1 type -> () -> ()
    0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
    // Function section (id=3, size=3): 2 funcs, both type 0
    0x03, 0x03, 0x02, 0x00, 0x00,
    // Export section (id=7, size=8): 1 export "ping" -> func 0
    0x07, 0x08, 0x01,
    0x04,                   // string length 4
    0x70, 0x69, 0x6e, 0x67, // "ping"
    0x00,                   // export kind: func
    0x00,                   // func index 0
    // Code section (id=10, size=11): 2 bodies
    0x0a, 0x0b, 0x02,
    // body 0: call func 1
    0x04, 0x00, 0x10, 0x01, 0x0b,
    // body 1: call func 0
    0x04, 0x00, 0x10, 0x00, 0x0b};

// A Wasm module with a deep but finite call chain that stays under the limit.
// Builds a chain: f0 -> f1 -> f2 -> ... -> f_{N-1}, where f_{N-1} returns.
// The call depth is N + 1 (including the dummy frame from runFunction).
//
static std::vector<Byte> buildCallChainWasm(uint32_t Depth) {
  // We build a module with `Depth` functions:
  //   func 0..Depth-2: call func i+1
  //   func Depth-1: nop; end (base case)
  // Export func 0 as "chain".
  std::vector<Byte> Wasm;

  // Magic + Version
  Wasm.insert(Wasm.end(), {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00});

  // Type section: 1 type -> () -> ()
  Wasm.insert(Wasm.end(), {0x01, 0x04, 0x01, 0x60, 0x00, 0x00});

  // Function section: Depth functions, all type 0
  {
    // Section id=3
    // Content: count(LEB128) + count * 0x00
    std::vector<Byte> Content;
    // Encode Depth as LEB128
    uint32_t Val = Depth;
    do {
      Byte B = Val & 0x7F;
      Val >>= 7;
      if (Val != 0) {
        B |= 0x80;
      }
      Content.push_back(B);
    } while (Val != 0);
    for (uint32_t I = 0; I < Depth; ++I) {
      Content.push_back(0x00); // type index 0
    }
    // Section header
    Wasm.push_back(0x03); // section id
    // Encode content size as LEB128
    uint32_t Size = static_cast<uint32_t>(Content.size());
    do {
      Byte B = Size & 0x7F;
      Size >>= 7;
      if (Size != 0) {
        B |= 0x80;
      }
      Wasm.push_back(B);
    } while (Size != 0);
    Wasm.insert(Wasm.end(), Content.begin(), Content.end());
  }

  // Export section: export func 0 as "chain"
  Wasm.insert(Wasm.end(), {0x07, 0x09, 0x01, 0x05, 0x63, 0x68, 0x61, 0x69, 0x6e,
                           0x00, 0x00});

  // Code section: Depth function bodies
  {
    std::vector<Byte> Bodies;
    for (uint32_t I = 0; I < Depth; ++I) {
      if (I < Depth - 1) {
        // call func (I+1): body_size=depends, 0 locals, call I+1, end
        std::vector<Byte> CallTarget;
        uint32_t Target = I + 1;
        do {
          Byte B = Target & 0x7F;
          Target >>= 7;
          if (Target != 0) {
            B |= 0x80;
          }
          CallTarget.push_back(B);
        } while (Target != 0);
        uint32_t BodySize =
            1 + 1 + static_cast<uint32_t>(CallTarget.size()) + 1;
        // Encode body size as LEB128
        uint32_t BS = BodySize;
        do {
          Byte B = BS & 0x7F;
          BS >>= 7;
          if (BS != 0) {
            B |= 0x80;
          }
          Bodies.push_back(B);
        } while (BS != 0);
        Bodies.push_back(0x00); // 0 local decl groups
        Bodies.push_back(0x10); // call opcode
        Bodies.insert(Bodies.end(), CallTarget.begin(), CallTarget.end());
        Bodies.push_back(0x0b); // end
      } else {
        // Base case: just return. body = [0x00, 0x0b]
        Bodies.push_back(0x02); // body size = 2
        Bodies.push_back(0x00); // 0 local decl groups
        Bodies.push_back(0x0b); // end
      }
    }

    // Prepend count of bodies (LEB128)
    std::vector<Byte> CountBytes;
    uint32_t Count = Depth;
    do {
      Byte B = Count & 0x7F;
      Count >>= 7;
      if (Count != 0) {
        B |= 0x80;
      }
      CountBytes.push_back(B);
    } while (Count != 0);

    std::vector<Byte> Content;
    Content.insert(Content.end(), CountBytes.begin(), CountBytes.end());
    Content.insert(Content.end(), Bodies.begin(), Bodies.end());

    // Section header
    Wasm.push_back(0x0a); // section id
    uint32_t Size = static_cast<uint32_t>(Content.size());
    do {
      Byte B = Size & 0x7F;
      Size >>= 7;
      if (Size != 0) {
        B |= 0x80;
      }
      Wasm.push_back(B);
    } while (Size != 0);
    Wasm.insert(Wasm.end(), Content.begin(), Content.end());
  }

  return Wasm;
}

TEST(CallStackDepthTest, InfiniteRecursionTraps) {
  Configure Conf;
  VM::VM VM(Conf);

  ASSERT_TRUE(VM.loadWasm(InfiniteRecursionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  auto Result = VM.execute("recurse");
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::CallStackOverflow);
}

TEST(CallStackDepthTest, MutualRecursionTraps) {
  Configure Conf;
  VM::VM VM(Conf);

  ASSERT_TRUE(VM.loadWasm(MutualRecursionWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  auto Result = VM.execute("ping");
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::CallStackOverflow);
}

TEST(CallStackDepthTest, DeepCallChainJustUnderLimitSucceeds) {
  // The total call depth includes: 1 dummy frame (from runFunction) +
  // 1 enterFunction frame + (Depth-1) more call frames = Depth+1 total.
  // MaxCallDepth = 1000. With Depth functions in a chain, the total frames
  // pushed = 1 (dummy) + Depth (one per function in the chain) = Depth+1.
  // We need Depth+1 <= 1000, so Depth <= 999.
  // Use 998 to have some margin.
  const uint32_t Depth = 998;
  auto Wasm = buildCallChainWasm(Depth);

  Configure Conf;
  VM::VM VM(Conf);

  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  auto Result = VM.execute("chain");
  EXPECT_TRUE(Result);
}

TEST(CallStackDepthTest, DeepCallChainAtLimitTraps) {
  // Build a chain that exceeds the limit.
  // 1 dummy frame + Depth function frames = Depth+1 total pushFrame calls.
  // We need Depth+1 > 1000, so Depth >= 1000.
  const uint32_t Depth = 1000;
  auto Wasm = buildCallChainWasm(Depth);

  Configure Conf;
  VM::VM VM(Conf);

  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  auto Result = VM.execute("chain");
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::CallStackOverflow);
}

// ---------------------------------------------------------------------------
// Direct StackManager unit tests
// ---------------------------------------------------------------------------

TEST(CallStackDepthTest, StackManagerPushFrameOverflow) {
  Runtime::StackManager StackMgr;
  // A dummy instruction for the iterator.
  AST::Instruction DummyInstr(OpCode::End);
  AST::InstrView View(&DummyInstr, 1);
  auto It = View.begin();

  // Fill the frame stack up to MaxCallDepth.
  for (uint32_t I = 0; I < Runtime::StackManager::MaxCallDepth; ++I) {
    auto Res = StackMgr.pushFrame(nullptr, It, 0, 0);
    ASSERT_TRUE(Res) << "pushFrame should succeed at depth " << I;
  }

  // The next pushFrame must fail with CallStackOverflow.
  auto Res = StackMgr.pushFrame(nullptr, It, 0, 0);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::CallStackOverflow);
}

TEST(CallStackDepthTest, StackManagerTailCallBypassesLimit) {
  Runtime::StackManager StackMgr;
  AST::Instruction DummyInstr(OpCode::End);
  AST::InstrView View(&DummyInstr, 1);
  auto It = View.begin();

  // Fill the frame stack up to MaxCallDepth.
  for (uint32_t I = 0; I < Runtime::StackManager::MaxCallDepth; ++I) {
    auto Res = StackMgr.pushFrame(nullptr, It, 0, 0);
    ASSERT_TRUE(Res) << "pushFrame should succeed at depth " << I;
  }

  // A regular pushFrame should fail at this point.
  auto ResFail = StackMgr.pushFrame(nullptr, It, 0, 0);
  ASSERT_FALSE(ResFail);
  EXPECT_EQ(ResFail.error(), ErrCode::Value::CallStackOverflow);

  // A tail call should succeed because it reuses the current frame.
  auto ResTail = StackMgr.pushFrame(nullptr, It, 0, 0, true);
  EXPECT_TRUE(ResTail);
}

TEST(CallStackDepthTest, StackManagerPopAndPushAgain) {
  Runtime::StackManager StackMgr;
  AST::Instruction DummyInstr(OpCode::End);
  AST::InstrView View(&DummyInstr, 1);
  auto It = View.begin();

  // Fill up to MaxCallDepth.
  for (uint32_t I = 0; I < Runtime::StackManager::MaxCallDepth; ++I) {
    auto Res = StackMgr.pushFrame(nullptr, It, 0, 0);
    ASSERT_TRUE(Res);
  }

  // Overflow should fail.
  auto ResFail = StackMgr.pushFrame(nullptr, It, 0, 0);
  ASSERT_FALSE(ResFail);

  // Pop one frame.
  StackMgr.popFrame();

  // Now pushing should succeed again (we are at MaxCallDepth - 1).
  auto ResOk = StackMgr.pushFrame(nullptr, It, 0, 0);
  EXPECT_TRUE(ResOk);

  // And overflow again.
  auto ResFail2 = StackMgr.pushFrame(nullptr, It, 0, 0);
  ASSERT_FALSE(ResFail2);
  EXPECT_EQ(ResFail2.error(), ErrCode::Value::CallStackOverflow);
}

TEST(CallStackDepthTest, StackManagerMaxCallDepthConstant) {
  // Verify the constant is what we expect.
  EXPECT_EQ(Runtime::StackManager::MaxCallDepth, 1000U);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
