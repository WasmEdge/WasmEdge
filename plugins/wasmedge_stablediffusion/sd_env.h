// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_backend.h"
#include "sd_session.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

class SDEnvironment {
public:
  // A change to a guest file counts from CreatedAt on (see resolveGuestFile).
  // CreatedAt must be earlier than the first guest call.
  explicit SDEnvironment(std::shared_ptr<Backend> B,
                         std::chrono::system_clock::time_point CreatedAt =
                             std::chrono::system_clock::now()) noexcept
      : TheBackend(std::move(B)), CreatedAt(CreatedAt) {}
  std::chrono::system_clock::time_point createdAt() const noexcept {
    return CreatedAt;
  }
  // False when the backend allocation failed. Call backend() only when this is
  // true.
  bool hasBackend() const noexcept { return TheBackend != nullptr; }
  Backend &backend() noexcept { return *TheBackend; }
  SessionTable &sessions() noexcept { return Sessions; }
  // Takes a place for a new session. Returns null when Limits::MaxSessions
  // places are taken.
  SessionSlot takeSlot() noexcept {
    return StableDiffusion::takeSlot(TakenSlots, Limits::MaxSessions);
  }

private:
  // TheBackend and TakenSlots are declared before Sessions, so the sessions are
  // released first.
  std::shared_ptr<Backend> TheBackend;
  std::chrono::system_clock::time_point CreatedAt;
  std::atomic<uint32_t> TakenSlots{0};
  SessionTable Sessions;
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
