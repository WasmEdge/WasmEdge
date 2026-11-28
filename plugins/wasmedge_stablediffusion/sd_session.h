// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_abi.h"
#include "sd_backend.h"
#include "sd_image.h"
#include "sd_request.h"

#include "common/expected.h"
#include "common/span.h"
#include "plugin/resource_table.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

struct ResultInfo {
  uint32_t Width, Height, Channels, Size;
};
// Returns InvalidArgument when the request needs something that the session
// does not have.
ErrNo checkAgainst(const Capabilities &Caps,
                   const GenerationRequest &Req) noexcept;
struct ReleaseSessionSlot { // gives a place back
  void operator()(std::atomic<uint32_t> *Taken) const noexcept;
};
// A place among the sessions of one module instance. A session takes its place
// before its backend context is created and gives it back after the context is
// freed. Thus a module instance never has more contexts than places. A null
// slot holds no place.
using SessionSlot = std::unique_ptr<std::atomic<uint32_t>, ReleaseSessionSlot>;
// Takes a place if fewer than Limit are taken. Returns null otherwise. Taken
// must outlive the slot.
SessionSlot takeSlot(std::atomic<uint32_t> &Taken, uint32_t Limit) noexcept;
class Session {
public:
  explicit Session(std::unique_ptr<BackendContext> Ctx,
                   SessionSlot Slot = nullptr) noexcept;
  const Capabilities &capabilities() const noexcept; // immutable, no lock
  // Calls Decode under the operation lock and generates with its inputs. Thus
  // a call that waits for the lock holds no decoded image. If Decode fails,
  // returns its code and keeps the results. A null Decode gives no inputs.
  ErrNo
  generate(const GenerationRequest &Req, const RequestFiles &Files,
           const std::function<Expected<GenerationInputs, ErrNo>()> &Decode,
           uint32_t &Count) noexcept;
  ErrNo resultInfo(uint32_t Index, ResultFormat Format,
                   ResultInfo &Info) noexcept;
  // Calls Copy with the bytes, under the operation lock, only if BufLen is
  // large enough. Sets Size whenever the result exists.
  ErrNo
  resultRead(uint32_t Index, ResultFormat Format, uint32_t BufLen,
             uint32_t &Size,
             const std::function<ErrNo(Span<const uint8_t>)> &Copy) noexcept;

private:
  // Declared before Ctx, so the place is free only after the context is.
  SessionSlot Slot;
  std::mutex Op;
  std::unique_ptr<BackendContext> Ctx;
  ImageBatch Results;
  std::vector<EncodedPng> Encoded;
  bool Poisoned = false;
  const Capabilities Caps;
};
using SessionTable = Plugin::ResourceTable<Session>;

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
