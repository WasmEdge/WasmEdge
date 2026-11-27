// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p2/io.h - wasi:io host -------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:io@0.2.12`.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "common/span.h"
#include "host/wasi/component/env.h"
#include "host/wasi/vinode.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/hostfunc.h"
#include "runtime/component/wit.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/resource.h"
#include "system/poll.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

/// `wasi:io/error` `error`: the errno of a failed operation and its text.
struct IoError {
  __wasi_errno_t Errno = __WASI_ERRNO_SUCCESS;
  std::string Message;
};

/// A failed stream operation: closed, or an error worth reporting.
struct StreamError {
  bool Closed = true;
  IoError Error;
};

/// `stream-error` as the guest sees it: the error resource already minted.
struct StreamErrorVal {
  bool Closed = true;
  uint64_t ErrorRep = 0;
};

template <typename T> using StreamResult = Expected<T, StreamError>;

/// Something a pollable waits on.
class PollSource {
public:
  virtual ~PollSource() = default;
  /// True when the operation would not block now.
  virtual bool ready() noexcept = 0;
  /// Add what wakes a wait on it: a deadline into Until or a handle to Entries.
  virtual void
  addInterest(std::optional<std::chrono::steady_clock::time_point> &Until,
              std::vector<PollEntry> &Entries) noexcept = 0;
};

/// `input-stream`
class InputStream : public PollSource {
public:
  /// Read up to Len bytes without blocking; empty when nothing is there yet.
  virtual StreamResult<std::vector<uint8_t>> read(uint64_t Len) noexcept = 0;
};

/// `output-stream`
class OutputStream : public PollSource {
public:
  /// Bytes that may be written now.
  virtual StreamResult<uint64_t> checkWrite() noexcept = 0;
  /// Write all of Data.
  virtual StreamResult<Runtime::Component::WitUnit>
  write(Span<const uint8_t> Data) noexcept = 0;
  virtual StreamResult<Runtime::Component::WitUnit> flush() noexcept = 0;
};

/// The state behind `wasi:io@0.2.12`, shared by the other 0.2 interfaces.
class IoHost {
public:
  IoHost(WasiComponent::Env &E) noexcept;

  WasiComponent::Env &env() noexcept { return Env; }

  Runtime::Component::ResourceTable<IoError> &errors() noexcept {
    return Errors;
  }
  Runtime::Component::ResourceTable<PollSource> &pollables() noexcept {
    return Pollables;
  }
  Runtime::Component::ResourceTable<InputStream> &inputs() noexcept {
    return Inputs;
  }
  Runtime::Component::ResourceTable<OutputStream> &outputs() noexcept {
    return Outputs;
  }
  /// The two `wasi:io` streams over a connected socket node.
  std::tuple<Runtime::Component::Own<InputStream>,
             Runtime::Component::Own<OutputStream>>
  socketStreams(const std::shared_ptr<WASI::VINode> &Node) noexcept;
  /// A pollable that is ready when the socket node is, or always.
  Runtime::Component::Own<PollSource>
  readySource(const std::shared_ptr<WASI::VINode> &Node, bool Active,
              bool Write) noexcept;

  /// The resource types once the instances defined them.
  const Runtime::Instance::Component::ResourceTypeInstance *ErrorType = nullptr;
  const Runtime::Instance::Component::ResourceTypeInstance *PollableType =
      nullptr;
  const Runtime::Instance::Component::ResourceTypeInstance *InputType = nullptr;
  const Runtime::Instance::Component::ResourceTypeInstance *OutputType =
      nullptr;

  /// Park until at least one source is ready: the indices of the ready ones.
  Expect<std::vector<uint32_t>> poll(Runtime::Component::CallingFrame &Frame,
                                     Span<PollSource *const> Sources) noexcept;
  /// True when a read or write on the node would not block now.
  bool readable(WASI::VINode &Node) noexcept;
  bool writable(WASI::VINode &Node) noexcept;

  /// The guest-facing form of a stream error, minting the error resource.
  StreamErrorVal errorValue(StreamError &&E) noexcept;

private:
  WasiComponent::Env &Env;
  Runtime::Component::ResourceTable<IoError> Errors;
  Runtime::Component::ResourceTable<PollSource> Pollables;
  Runtime::Component::ResourceTable<InputStream> Inputs;
  Runtime::Component::ResourceTable<OutputStream> Outputs;
};

/// A pollable that is always ready.
class AlwaysReady : public PollSource {
public:
  bool ready() noexcept override { return true; }
  void addInterest(std::optional<std::chrono::steady_clock::time_point> &,
                   std::vector<PollEntry> &) noexcept override {}
};

/// A pollable that is ready from a monotonic-clock deadline on.
class DeadlineSource : public PollSource {
public:
  DeadlineSource(uint64_t D) noexcept : Deadline(D) {}
  bool ready() noexcept override;
  void addInterest(std::optional<std::chrono::steady_clock::time_point> &Until,
                   std::vector<PollEntry> &Entries) noexcept override;

private:
  uint64_t Deadline;
};

/// A pollable over a stream of the io tables, ready once the stream is gone.
class StreamReadySource : public PollSource {
public:
  StreamReadySource(IoHost &H, uint64_t Rep, bool Output) noexcept
      : Host(H), StreamRep(Rep), Output(Output) {}
  bool ready() noexcept override;
  void addInterest(std::optional<std::chrono::steady_clock::time_point> &Until,
                   std::vector<PollEntry> &Entries) noexcept override;

private:
  /// The stream the pollable names, null when it was dropped.
  PollSource *getStream() const noexcept;
  IoHost &Host;
  uint64_t StreamRep;
  bool Output;
};

/// A pollable that is ready when a node can be read from or written to.
class NodeReadySource : public PollSource {
public:
  NodeReadySource(IoHost &H, std::shared_ptr<WASI::VINode> N,
                  bool Write) noexcept
      : Host(H), Node(std::move(N)), Write(Write) {}
  bool ready() noexcept override;
  void addInterest(std::optional<std::chrono::steady_clock::time_point> &Until,
                   std::vector<PollEntry> &Entries) noexcept override;

private:
  IoHost &Host;
  std::shared_ptr<WASI::VINode> Node;
  bool Write;
};

/// An input stream over a preview-1 node.
class NodeInputStream : public InputStream {
public:
  enum class Mode { File, Fd, Socket };
  NodeInputStream(IoHost &H, std::shared_ptr<WASI::VINode> N, Mode M,
                  uint64_t Offset = 0) noexcept
      : Host(H), Node(std::move(N)), Kind(M), Position(Offset) {}
  bool ready() noexcept override;
  void addInterest(std::optional<std::chrono::steady_clock::time_point> &Until,
                   std::vector<PollEntry> &Entries) noexcept override;
  StreamResult<std::vector<uint8_t>> read(uint64_t Len) noexcept override;

private:
  StreamResult<std::vector<uint8_t>> readNow(uint64_t Len) noexcept;
  IoHost &Host;
  std::shared_ptr<WASI::VINode> Node;
  Mode Kind;
  uint64_t Position;
  bool Ended = false;
};

/// An output stream over a preview-1 node.
class NodeOutputStream : public OutputStream {
public:
  enum class Mode { File, Append, Fd, Socket };
  NodeOutputStream(IoHost &H, std::shared_ptr<WASI::VINode> N, Mode M,
                   uint64_t Offset = 0) noexcept
      : Host(H), Node(std::move(N)), Kind(M), Position(Offset) {}
  bool ready() noexcept override;
  void addInterest(std::optional<std::chrono::steady_clock::time_point> &Until,
                   std::vector<PollEntry> &Entries) noexcept override;
  StreamResult<uint64_t> checkWrite() noexcept override;
  StreamResult<Runtime::Component::WitUnit>
  write(Span<const uint8_t> Data) noexcept override;
  StreamResult<Runtime::Component::WitUnit> flush() noexcept override;

private:
  IoHost &Host;
  std::shared_ptr<WASI::VINode> Node;
  Mode Kind;
  uint64_t Position;
  bool Ended = false;
};

using ErrorBorrow = Runtime::Component::Borrow<IoError>;
using PollableBorrow = Runtime::Component::Borrow<PollSource>;
using InputBorrow = Runtime::Component::Borrow<InputStream>;
using OutputBorrow = Runtime::Component::Borrow<OutputStream>;
template <typename T> using IoResult = Expected<T, StreamErrorVal>;

} // namespace WasiP2
} // namespace Host

namespace Runtime {
namespace Component {

template <> struct Wit<Host::WasiP2::StreamErrorVal> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitVariant::type(
        Mint,
        {{"last-operation-failed", Wit<Own<Host::WasiP2::IoError>>::type(Mint)},
         {"closed", std::nullopt}});
  }
  static Host::WasiP2::StreamErrorVal from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP2::StreamErrorVal E;
    E.Closed = Val.Case == 1;
    if (!E.Closed && Val.Payload) {
      E.ErrorRep = Wit<Own<Host::WasiP2::IoError>>::from(*Val.Payload).Rep;
    }
    return E;
  }
  static ComponentValVariant into(Host::WasiP2::StreamErrorVal &&E) noexcept {
    if (E.Closed) {
      return WitVariant::into(1);
    }
    return WitVariant::into(0, Wit<Own<Host::WasiP2::IoError>>::into(
                                   Own<Host::WasiP2::IoError>{E.ErrorRep}));
  }
};

} // namespace Component
} // namespace Runtime

namespace Host {
namespace WasiP2 {

/// The base of the host functions over the io state.
template <typename T>
class IoFunction : public Runtime::Component::HostFunction<T> {
public:
  IoFunction(IoHost &H) noexcept : Host(H) {}

protected:
  IoHost &Host;
};

/// `[method]error.to-debug-string: func() -> string`
class ErrorToDebugString : public IoFunction<ErrorToDebugString> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using IoFunction::IoFunction;
  Expect<std::string> body(Runtime::Component::CallingFrame &,
                           ErrorBorrow Self);
};

/// `[method]pollable.ready: func() -> bool`
class PollableReady : public IoFunction<PollableReady> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using IoFunction::IoFunction;
  Expect<bool> body(Runtime::Component::CallingFrame &, PollableBorrow Self);
};

/// `[method]pollable.block: func()`
class PollableBlock : public IoFunction<PollableBlock> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using IoFunction::IoFunction;
  Expect<void> body(Runtime::Component::CallingFrame &, PollableBorrow Self);
};

/// `poll: func(in: list<borrow<pollable>>) -> list<u32>`
class Poll : public IoFunction<Poll> {
public:
  static constexpr const char *ParamNames[] = {"in"};
  using IoFunction::IoFunction;
  Expect<std::vector<uint32_t>> body(Runtime::Component::CallingFrame &,
                                     std::vector<PollableBorrow> In);
};

/// `[method]input-stream.read` and `blocking-read`
class InputRead : public IoFunction<InputRead> {
public:
  static constexpr const char *ParamNames[] = {"self", "len"};
  InputRead(IoHost &H, bool Blocking) noexcept
      : IoFunction(H), Blocking(Blocking) {}
  Expect<IoResult<std::vector<uint8_t>>>
  body(Runtime::Component::CallingFrame &, InputBorrow Self, uint64_t Len);

private:
  bool Blocking;
};

/// `[method]input-stream.skip` and `blocking-skip`
class InputSkip : public IoFunction<InputSkip> {
public:
  static constexpr const char *ParamNames[] = {"self", "len"};
  InputSkip(IoHost &H, bool Blocking) noexcept
      : IoFunction(H), Blocking(Blocking) {}
  Expect<IoResult<uint64_t>> body(Runtime::Component::CallingFrame &,
                                  InputBorrow Self, uint64_t Len);

private:
  bool Blocking;
};

/// `[method]input-stream.subscribe: func() -> pollable`
class InputSubscribe : public IoFunction<InputSubscribe> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using IoFunction::IoFunction;
  Expect<Runtime::Component::Own<PollSource>>
  body(Runtime::Component::CallingFrame &, InputBorrow Self);
};

/// `[method]output-stream.check-write: func() -> result<u64, stream-error>`
class OutputCheckWrite : public IoFunction<OutputCheckWrite> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using IoFunction::IoFunction;
  Expect<IoResult<uint64_t>> body(Runtime::Component::CallingFrame &,
                                  OutputBorrow Self);
};

/// `[method]output-stream.write` and `blocking-write-and-flush`
class OutputWrite : public IoFunction<OutputWrite> {
public:
  static constexpr const char *ParamNames[] = {"self", "contents"};
  using IoFunction::IoFunction;
  Expect<IoResult<Runtime::Component::WitUnit>>
  body(Runtime::Component::CallingFrame &, OutputBorrow Self,
       std::vector<uint8_t> Contents);
};

/// `[method]output-stream.flush` and `blocking-flush`
class OutputFlush : public IoFunction<OutputFlush> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using IoFunction::IoFunction;
  Expect<IoResult<Runtime::Component::WitUnit>>
  body(Runtime::Component::CallingFrame &, OutputBorrow Self);
};

/// `[method]output-stream.subscribe: func() -> pollable`
class OutputSubscribe : public IoFunction<OutputSubscribe> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using IoFunction::IoFunction;
  Expect<Runtime::Component::Own<PollSource>>
  body(Runtime::Component::CallingFrame &, OutputBorrow Self);
};

/// `[method]output-stream.write-zeroes` and its blocking form
class OutputWriteZeroes : public IoFunction<OutputWriteZeroes> {
public:
  static constexpr const char *ParamNames[] = {"self", "len"};
  using IoFunction::IoFunction;
  Expect<IoResult<Runtime::Component::WitUnit>>
  body(Runtime::Component::CallingFrame &, OutputBorrow Self, uint64_t Len);
};

/// `[method]output-stream.splice` and `blocking-splice`
class OutputSplice : public IoFunction<OutputSplice> {
public:
  static constexpr const char *ParamNames[] = {"self", "src", "len"};
  OutputSplice(IoHost &H, bool Blocking) noexcept
      : IoFunction(H), Blocking(Blocking) {}
  Expect<IoResult<uint64_t>> body(Runtime::Component::CallingFrame &,
                                  OutputBorrow Self, InputBorrow Src,
                                  uint64_t Len);

private:
  bool Blocking;
};

/// `wasi:io/error@0.2.12`
class IoErrorInstance : public Runtime::Instance::ComponentInstance {
public:
  IoErrorInstance(IoHost &Host);
};

/// `wasi:io/poll@0.2.12`
class PollInstance : public Runtime::Instance::ComponentInstance {
public:
  PollInstance(IoHost &Host);
};

/// `wasi:io/streams@0.2.12`
class StreamsInstance : public Runtime::Instance::ComponentInstance {
public:
  StreamsInstance(IoHost &Host);
};

/// The three instances of `wasi:io@0.2.12`, in order.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
newIoInstances(IoHost &Host);

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
