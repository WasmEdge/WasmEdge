// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/spdlog.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/module.h"
#include "gtest/gtest.h"

#include <atomic>
#include <cstdlib>
#include <thread>
#include <vector>

namespace {

class HostFunc : public WasmEdge::Runtime::HostFunctionBase {
public:
  HostFunc() : WasmEdge::Runtime::HostFunctionBase(0) {}
  WasmEdge::Expect<void> run(const WasmEdge::Runtime::CallingFrame &,
                             WasmEdge::Span<const WasmEdge::ValVariant>,
                             WasmEdge::Span<WasmEdge::ValVariant>) override {
    return {};
  }
};

class TestModuleInstance : public WasmEdge::Runtime::Instance::ModuleInstance {
public:
  TestModuleInstance(std::string_view Name)
      : WasmEdge::Runtime::Instance::ModuleInstance(Name) {}

  const WasmEdge::AST::SubType *
  publicUnsafeGetType(uint32_t Idx) const noexcept {
    return unsafeGetType(Idx);
  }

  WasmEdge::Span<const WasmEdge::AST::SubType *const>
  publicGetTypeList() const noexcept {
    return getTypeList();
  }
};

TEST(TypeRaceTest, ConcurrentReadWrite) {
  WasmEdge::Log::setErrorLoggingLevel();
  auto ModStub = std::make_unique<TestModuleInstance>("test_mod");

  std::atomic<bool> Running = true;
  std::vector<std::thread> Readers;
  std::vector<std::thread> Writers;

  // reader threads: Constantly read types
  for (int I = 0; I < 4; ++I) {
    Readers.emplace_back([&]() {
      while (Running) {
        uint32_t Size = ModStub->publicGetTypeList().size();
        if (Size > 0) {
          const auto *Type = ModStub->publicUnsafeGetType(rand() % Size);
          assuming(Type != nullptr);
        }
      }
    });
  }

  // writer Thread: Constantly add new types (via addHostFunc)
  Writers.emplace_back([&]() {
    int Count = 0;
    while (Running) {
      auto Func = std::make_unique<HostFunc>();
      ModStub->addHostFunc("func" + std::to_string(Count++), std::move(Func));

      // limiting growth to avoid OOM in test
      if (Count > 10000) {
        Running = false;
      }

      if (Count % 100 == 0) {
        std::this_thread::yield();
      }
    }
  });

  for (auto &T : Writers)
    T.join();
  Running = false;
  for (auto &T : Readers)
    T.join();

  // If we reached here without crashing, the RCU pattern works for avoiding
  // use after free crash.
  ASSERT_GT(ModStub->getFuncExportNum(), 1000);
}

} // namespace
