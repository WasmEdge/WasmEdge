// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

//===-- wasmedge/test/moduleinstance/ModuleInstanceTest.cpp ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Tests for ModuleInstance teardown: the subclass destructor must run before
/// the HostDataFinalizer on both the direct and cascade paths, and a deep
/// dependency chain must tear down iteratively without overflowing the stack.
///
//===----------------------------------------------------------------------===//

#include "runtime/instance/module.h"

#include <cstddef>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace WasmEdge;

/// Events recorded by the subclass destructor and the finalizer, in order.
enum class Event {
  DerivedDtor,
  Finalizer,
  MiddleFinalizerEnter,
  MiddleFinalizerAfterTerminate
};

/// Test-only subclass that records its destruction and exposes addDependency
/// so tests can wire a consumer -> provider edge.
class TestModule final : public Runtime::Instance::ModuleInstance {
public:
  TestModule(std::string_view Name, std::vector<Event> *Events,
             std::function<void(void *)> Finalizer)
      : TestModule(Name, Events, Events, std::move(Finalizer)) {}

  TestModule(std::string_view Name, void *Data, std::vector<Event> *Events,
             std::function<void(void *)> Finalizer)
      : ModuleInstance(Name, Data, std::move(Finalizer)),
        RecordedEvents(Events) {}

  ~TestModule() noexcept override {
    if (RecordedEvents != nullptr) {
      RecordedEvents->push_back(Event::DerivedDtor);
    }
  }

  using ModuleInstance::addDependency;

private:
  std::vector<Event> *RecordedEvents;
};

/// Finalizer that recovers the event vector from its HostData argument.
std::function<void(void *)> makeFinalizer() {
  return [](void *Data) {
    auto *Events = static_cast<std::vector<Event> *>(Data);
    if (Events != nullptr) {
      Events->push_back(Event::Finalizer);
    }
  };
}

struct FinalizerContext {
  TestModule *Provider;
  std::vector<Event> *Events;
};

} // namespace

TEST(ModuleInstanceTest, DirectDeleteRunsDerivedDtorBeforeFinalizer) {
  std::vector<Event> Events;
  {
    auto Mod = std::make_unique<TestModule>("direct", &Events, makeFinalizer());
    (void)Mod;
  }
  ASSERT_EQ(Events.size(), 2u) << "expected exactly [DerivedDtor, Finalizer]";
  EXPECT_EQ(Events[0], Event::DerivedDtor);
  EXPECT_EQ(Events[1], Event::Finalizer);
}

TEST(ModuleInstanceTest, CascadeDeleteRunsDerivedDtorBeforeFinalizer) {
  std::vector<Event> ProviderEvents;
  auto *Provider = new TestModule("provider", &ProviderEvents, makeFinalizer());
  auto Consumer = std::make_unique<TestModule>("consumer", nullptr, nullptr);
  Consumer->addDependency(*Provider);
  Provider->terminate();
  Consumer.reset();
  ASSERT_EQ(ProviderEvents.size(), 2u)
      << "cascade did not run ~TestModule and the finalizer exactly once";
  EXPECT_EQ(ProviderEvents[0], Event::DerivedDtor);
  EXPECT_EQ(ProviderEvents[1], Event::Finalizer);
}

TEST(ModuleInstanceTest, CascadeKeepsProviderPinnedUntilFinalizerCompletes) {
  std::vector<Event> Events;
  auto *Provider = new TestModule("provider", &Events, makeFinalizer());
  FinalizerContext FinalizerCtx{Provider, &Events};
  auto *Middle =
      new TestModule("middle", &FinalizerCtx, nullptr, [](void *Data) {
        auto *Ctx = static_cast<FinalizerContext *>(Data);
        Ctx->Events->push_back(Event::MiddleFinalizerEnter);
        Ctx->Provider->terminate();
        Ctx->Events->push_back(Event::MiddleFinalizerAfterTerminate);
      });
  auto *Consumer = new TestModule("consumer", nullptr, nullptr);
  Middle->addDependency(*Provider);
  Consumer->addDependency(*Middle);
  Middle->terminate();
  Consumer->terminate();
  ASSERT_EQ(Events.size(), 4u)
      << "provider must remain pinned until the importer finalizer completes";
  EXPECT_EQ(Events[0], Event::MiddleFinalizerEnter);
  EXPECT_EQ(Events[1], Event::MiddleFinalizerAfterTerminate);
  EXPECT_EQ(Events[2], Event::DerivedDtor);
  EXPECT_EQ(Events[3], Event::Finalizer);
}

TEST(ModuleInstanceTest, DeepCascadeDoesNotOverflowStack) {
  constexpr std::size_t N = 50000;
  std::vector<Event> Events;
  Events.reserve(2 * N);
  std::vector<TestModule *> Mods;
  Mods.reserve(N);
  for (std::size_t I = 0; I < N; ++I) {
    Mods.push_back(new TestModule("m", &Events, makeFinalizer()));
  }
  for (std::size_t I = 0; I + 1 < N; ++I) {
    Mods[I]->addDependency(*Mods[I + 1]);
  }
  for (std::size_t I = 1; I < N; ++I) {
    Mods[I]->terminate();
  }
  Mods[0]->terminate();
  EXPECT_EQ(Events.size(), 2 * N)
      << "every module in the chain must run its dtor and finalizer once";
}
