// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p2/io.h - wasi:io host -------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:io@0.2.12`:
/// `error`, `poll`, and `streams`, plus the stream and pollable objects the
/// other 0.2 interfaces hand out.
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
#include "runtime/component/resourcetable.h"
#include "runtime/component/wit.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/resource.h"

#include <cstdint>
#include <memory>
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
  /// Register the wake-up with the poller under Tag; false when the source
  /// has none and only ready() can tell.
  virtual bool subscribe(WASI::VPoller &Poller,
                         __wasi_userdata_t Tag) noexcept = 0;
};

/// `input-stream`
class InputStream : public PollSource {
public:
  /// Read up to Len bytes without blocking; empty when nothing is there yet.
  virtual StreamResult<std::vector<uint8_t>> read(uint64_t Len) noexcept = 0;
  /// Read up to Len bytes, waiting for the first.
  virtual StreamResult<std::vector<uint8_t>>
  blockingRead(uint64_t Len) noexcept = 0;
};

/// `output-stream`
class OutputStream : public PollSource {
public:
  /// Bytes that may be written now.
  virtual StreamResult<uint64_t> checkWrite() noexcept = 0;
  /// Write all of Data.
  virtual StreamResult<Runtime::Component::Unit>
  write(Span<const uint8_t> Data) noexcept = 0;
  virtual StreamResult<Runtime::Component::Unit> flush() noexcept = 0;
};

class IoHost;

/// A pollable that is always ready.
class AlwaysReady : public PollSource {
public:
  bool ready() noexcept override { return true; }
  bool subscribe(WASI::VPoller &, __wasi_userdata_t) noexcept override {
    return false;
  }
};

/// A pollable that is ready from a monotonic-clock deadline on.
class DeadlineSource : public PollSource {
public:
  DeadlineSource(uint64_t D) noexcept : Deadline(D) {}
  bool ready() noexcept override;
  bool subscribe(WASI::VPoller &Poller,
                 __wasi_userdata_t Tag) noexcept override;

private:
  uint64_t Deadline;
};

/// A pollable that is ready when a node can be read from or written to.
class NodeReadySource : public PollSource {
public:
  NodeReadySource(IoHost &H, std::shared_ptr<WASI::VINode> N,
                  bool Write) noexcept
      : Host(H), Node(std::move(N)), Write(Write) {}
  bool ready() noexcept override;
  bool subscribe(WASI::VPoller &Poller,
                 __wasi_userdata_t Tag) noexcept override;

private:
  IoHost &Host;
  std::shared_ptr<WASI::VINode> Node;
  bool Write;
};

/// An input stream over a preview-1 node: a file read from an offset, or a
/// standard input or socket read as it arrives.
class NodeInputStream : public InputStream {
public:
  enum class Mode { File, Fd, Socket };
  NodeInputStream(IoHost &H, std::shared_ptr<WASI::VINode> N, Mode M,
                  uint64_t Offset = 0) noexcept
      : Host(H), Node(std::move(N)), Kind(M), Position(Offset) {}
  bool ready() noexcept override;
  bool subscribe(WASI::VPoller &Poller,
                 __wasi_userdata_t Tag) noexcept override;
  StreamResult<std::vector<uint8_t>> read(uint64_t Len) noexcept override;
  StreamResult<std::vector<uint8_t>>
  blockingRead(uint64_t Len) noexcept override;

private:
  StreamResult<std::vector<uint8_t>> readNow(uint64_t Len) noexcept;
  IoHost &Host;
  std::shared_ptr<WASI::VINode> Node;
  Mode Kind;
  uint64_t Position;
  bool Ended = false;
};

/// An output stream over a preview-1 node: a file written at an offset or
/// appended to, or a standard output or socket.
class NodeOutputStream : public OutputStream {
public:
  enum class Mode { File, Append, Fd, Socket };
  NodeOutputStream(IoHost &H, std::shared_ptr<WASI::VINode> N, Mode M,
                   uint64_t Offset = 0) noexcept
      : Host(H), Node(std::move(N)), Kind(M), Position(Offset) {}
  bool ready() noexcept override;
  bool subscribe(WASI::VPoller &Poller,
                 __wasi_userdata_t Tag) noexcept override;
  StreamResult<uint64_t> checkWrite() noexcept override;
  StreamResult<Runtime::Component::Unit>
  write(Span<const uint8_t> Data) noexcept override;
  StreamResult<Runtime::Component::Unit> flush() noexcept override;

private:
  IoHost &Host;
  std::shared_ptr<WASI::VINode> Node;
  Mode Kind;
  uint64_t Position;
  bool Ended = false;
};

/// The state behind `wasi:io@0.2.12`: the resource tables and types the
/// other 0.2 interfaces mint streams and pollables into, and the poller.
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

  /// Wait until at least one source is ready: the indices of the ready ones.
  std::vector<uint32_t>
  poll(Span<const std::shared_ptr<PollSource>> Sources) noexcept;
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
  WASI::VPoller Poller;
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
  Expect<IoResult<Runtime::Component::Unit>>
  body(Runtime::Component::CallingFrame &, OutputBorrow Self,
       std::vector<uint8_t> Contents);
};

/// `[method]output-stream.flush` and `blocking-flush`
class OutputFlush : public IoFunction<OutputFlush> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using IoFunction::IoFunction;
  Expect<IoResult<Runtime::Component::Unit>>
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

/// `[method]output-stream.write-zeroes` and
/// `blocking-write-zeroes-and-flush`
class OutputWriteZeroes : public IoFunction<OutputWriteZeroes> {
public:
  static constexpr const char *ParamNames[] = {"self", "len"};
  using IoFunction::IoFunction;
  Expect<IoResult<Runtime::Component::Unit>>
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

/// The three instances of `wasi:io@0.2.12`, which define the resource types
/// of Host in order.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeIoInstances(IoHost &Host);

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
