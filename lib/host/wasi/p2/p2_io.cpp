// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/defines.h"
#include "host/wasi/clock.h"
#include "host/wasi/p2/io.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>

#if !WASMEDGE_OS_WINDOWS
#include <poll.h>
#endif

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

namespace {

constexpr uint64_t MaxChunk = 65536;

StreamError failed(__wasi_errno_t Errno) noexcept {
  if (Errno == __WASI_ERRNO_PIPE || Errno == __WASI_ERRNO_CONNRESET) {
    return StreamError{true, {}};
  }
  return StreamError{false, IoError{Errno, "errno " + std::to_string(Errno)}};
}

uint64_t monotonicNow() noexcept {
  __wasi_timestamp_t Now = 0;
  if (auto Res = WASI::Clock::clockTimeGet(__WASI_CLOCKID_MONOTONIC, 1, Now);
      !Res) {
    return 0;
  }
  return Now;
}

// Whether the node is ready for reading or writing right now.
bool readyNow(WASI::VINode &Node, bool Write) noexcept {
#if WASMEDGE_OS_WINDOWS
  (void)Node;
  (void)Write;
  return true;
#else
  auto Handle = Node.getNativeHandler();
  if (!Handle) {
    return true;
  }
  struct pollfd P;
  P.fd = static_cast<int>(*Handle);
  P.events = Write ? POLLOUT : POLLIN;
  P.revents = 0;
  const int Res = ::poll(&P, 1, 0);
  if (Res < 0) {
    return true;
  }
  return Res > 0;
#endif
}

} // namespace

bool DeadlineSource::ready() noexcept { return monotonicNow() >= Deadline; }

bool DeadlineSource::subscribe(WASI::VPoller &Poller,
                               __wasi_userdata_t Tag) noexcept {
  Poller.clock(__WASI_CLOCKID_MONOTONIC, Deadline, 0,
               __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME, Tag);
  return true;
}

bool NodeReadySource::ready() noexcept {
  return Write ? Host.writable(*Node) : Host.readable(*Node);
}

bool NodeReadySource::subscribe(WASI::VPoller &Poller,
                                __wasi_userdata_t Tag) noexcept {
  if (Write) {
    Poller.write(Node, WASI::TriggerType::Level, Tag);
  } else {
    Poller.read(Node, WASI::TriggerType::Level, Tag);
  }
  return true;
}

bool NodeInputStream::ready() noexcept {
  if (Ended || Kind == Mode::File) {
    return true;
  }
  return Host.readable(*Node);
}

bool NodeInputStream::subscribe(WASI::VPoller &Poller,
                                __wasi_userdata_t Tag) noexcept {
  if (Ended || Kind == Mode::File) {
    return false;
  }
  Poller.read(Node, WASI::TriggerType::Level, Tag);
  return true;
}

StreamResult<std::vector<uint8_t>>
NodeInputStream::readNow(uint64_t Len) noexcept {
  std::vector<uint8_t> Buffer(static_cast<size_t>(std::min(Len, MaxChunk)));
  if (Buffer.empty()) {
    return Buffer;
  }
  std::array<Span<uint8_t>, 1> IOVs{Span<uint8_t>(Buffer)};
  __wasi_size_t Count = 0;
  WASI::WasiExpect<void> Res;
  switch (Kind) {
  case Mode::File:
    Res = Node->fdPread(IOVs, Position, Count);
    break;
  case Mode::Fd:
    Res = Node->fdRead(IOVs, Count);
    break;
  case Mode::Socket: {
    __wasi_roflags_t RoFlags = static_cast<__wasi_roflags_t>(0);
    Res =
        Node->sockRecv(IOVs, static_cast<__wasi_riflags_t>(0), Count, RoFlags);
    break;
  }
  }
  if (!Res) {
    Ended = true;
    return Unexpected<StreamError>(failed(Res.error()));
  }
  if (Count == 0) {
    Ended = true;
    return Unexpected<StreamError>(StreamError{true, {}});
  }
  Position += Count;
  Buffer.resize(Count);
  return Buffer;
}

StreamResult<std::vector<uint8_t>>
NodeInputStream::read(uint64_t Len) noexcept {
  if (Ended) {
    return Unexpected<StreamError>(StreamError{true, {}});
  }
  if (!ready()) {
    return std::vector<uint8_t>();
  }
  return readNow(Len);
}

StreamResult<std::vector<uint8_t>>
NodeInputStream::blockingRead(uint64_t Len) noexcept {
  if (Ended) {
    return Unexpected<StreamError>(StreamError{true, {}});
  }
  if (Kind != Mode::File) {
    std::shared_ptr<PollSource> Self(this, [](PollSource *) {});
    Host.poll(Span<const std::shared_ptr<PollSource>>(&Self, 1));
  }
  return readNow(Len);
}

bool NodeOutputStream::ready() noexcept {
  if (Ended || Kind == Mode::File || Kind == Mode::Append) {
    return true;
  }
  return Host.writable(*Node);
}

bool NodeOutputStream::subscribe(WASI::VPoller &Poller,
                                 __wasi_userdata_t Tag) noexcept {
  if (Ended || Kind == Mode::File || Kind == Mode::Append) {
    return false;
  }
  Poller.write(Node, WASI::TriggerType::Level, Tag);
  return true;
}

StreamResult<uint64_t> NodeOutputStream::checkWrite() noexcept {
  if (Ended) {
    return Unexpected<StreamError>(StreamError{true, {}});
  }
  return ready() ? MaxChunk : 0;
}

StreamResult<Runtime::Component::Unit>
NodeOutputStream::write(Span<const uint8_t> Data) noexcept {
  if (Ended) {
    return Unexpected<StreamError>(StreamError{true, {}});
  }
  while (!Data.empty()) {
    std::array<Span<const uint8_t>, 1> IOVs{Data};
    __wasi_size_t Written = 0;
    WASI::WasiExpect<void> Res;
    switch (Kind) {
    case Mode::File:
      Res = Node->fdPwrite(IOVs, Position, Written);
      break;
    case Mode::Append: {
      __wasi_filesize_t End = 0;
      Res = Node->fdSeek(0, __WASI_WHENCE_END, End);
      if (Res) {
        Res = Node->fdPwrite(IOVs, End, Written);
      }
      break;
    }
    case Mode::Fd:
      Res = Node->fdWrite(IOVs, Written);
      break;
    case Mode::Socket:
      Res = Node->sockSend(IOVs, static_cast<__wasi_siflags_t>(0), Written);
      break;
    }
    if (!Res) {
      Ended = true;
      return Unexpected<StreamError>(failed(Res.error()));
    }
    if (Written == 0) {
      Ended = true;
      return Unexpected<StreamError>(StreamError{true, {}});
    }
    Position += Written;
    Data = Data.subspan(Written);
  }
  return Runtime::Component::Unit{};
}

StreamResult<Runtime::Component::Unit> NodeOutputStream::flush() noexcept {
  if (Ended) {
    return Unexpected<StreamError>(StreamError{true, {}});
  }
  return Runtime::Component::Unit{};
}

IoHost::IoHost(WasiComponent::Env &E) noexcept : Env(E), Poller(E.preview1()) {}

std::vector<uint32_t>
IoHost::poll(Span<const std::shared_ptr<PollSource>> Sources) noexcept {
  std::vector<uint32_t> Ready;
  while (true) {
    Ready.clear();
    for (uint32_t I = 0; I < Sources.size(); ++I) {
      if (Sources[I]->ready()) {
        Ready.push_back(I);
      }
    }
    if (!Ready.empty()) {
      return Ready;
    }
    // Nothing is ready: wait on every source that has a wake-up.
    std::vector<__wasi_event_t> Events(Sources.size());
    if (!Poller.prepare(Events)) {
      return Ready;
    }
    uint32_t Waiting = 0;
    for (uint32_t I = 0; I < Sources.size(); ++I) {
      if (Sources[I]->subscribe(Poller, I)) {
        Waiting += 1;
      }
    }
    if (Waiting == 0) {
      Poller.reset();
      return Ready;
    }
    Events.resize(Waiting);
    Poller.wait();
    Poller.result();
    Poller.reset();
  }
}

bool IoHost::readable(WASI::VINode &Node) noexcept {
  return readyNow(Node, false);
}

bool IoHost::writable(WASI::VINode &Node) noexcept {
  return readyNow(Node, true);
}

StreamErrorVal IoHost::errorValue(StreamError &&E) noexcept {
  if (E.Closed) {
    return StreamErrorVal{true, 0};
  }
  return StreamErrorVal{
      false, Errors.add(std::make_shared<IoError>(std::move(E.Error)))};
}

Expect<std::string> ErrorToDebugString::body(Runtime::Component::CallingFrame &,
                                             ErrorBorrow Self) {
  auto Err = Host.errors().get(Self.Rep);
  return Err ? Err->Message : std::string("unknown error");
}

Expect<bool> PollableReady::body(Runtime::Component::CallingFrame &,
                                 PollableBorrow Self) {
  auto Source = Host.pollables().get(Self.Rep);
  return !Source || Source->ready();
}

Expect<void> PollableBlock::body(Runtime::Component::CallingFrame &,
                                 PollableBorrow Self) {
  auto Source = Host.pollables().get(Self.Rep);
  if (Source) {
    Host.poll(Span<const std::shared_ptr<PollSource>>(&Source, 1));
  }
  return {};
}

Expect<std::vector<uint32_t>> Poll::body(Runtime::Component::CallingFrame &,
                                         std::vector<PollableBorrow> In) {
  if (In.empty()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  std::vector<std::shared_ptr<PollSource>> Sources;
  Sources.reserve(In.size());
  for (const auto &P : In) {
    auto Source = Host.pollables().get(P.Rep);
    if (!Source) {
      Source = std::make_shared<AlwaysReady>();
    }
    Sources.push_back(std::move(Source));
  }
  return Host.poll(Sources);
}

Expect<IoResult<std::vector<uint8_t>>>
InputRead::body(Runtime::Component::CallingFrame &, InputBorrow Self,
                uint64_t Len) {
  auto Stream = Host.inputs().get(Self.Rep);
  if (!Stream) {
    return IoResult<std::vector<uint8_t>>(
        Unexpected<StreamErrorVal>(StreamErrorVal{true, 0}));
  }
  auto Res = Blocking ? Stream->blockingRead(Len) : Stream->read(Len);
  if (!Res) {
    return IoResult<std::vector<uint8_t>>(
        Unexpected<StreamErrorVal>(Host.errorValue(std::move(Res.error()))));
  }
  return IoResult<std::vector<uint8_t>>(std::move(*Res));
}

Expect<IoResult<uint64_t>> InputSkip::body(Runtime::Component::CallingFrame &,
                                           InputBorrow Self, uint64_t Len) {
  auto Stream = Host.inputs().get(Self.Rep);
  if (!Stream) {
    return IoResult<uint64_t>(
        Unexpected<StreamErrorVal>(StreamErrorVal{true, 0}));
  }
  auto Res = Blocking ? Stream->blockingRead(Len) : Stream->read(Len);
  if (!Res) {
    return IoResult<uint64_t>(
        Unexpected<StreamErrorVal>(Host.errorValue(std::move(Res.error()))));
  }
  return IoResult<uint64_t>(static_cast<uint64_t>(Res->size()));
}

Expect<Runtime::Component::Own<PollSource>>
InputSubscribe::body(Runtime::Component::CallingFrame &, InputBorrow Self) {
  std::shared_ptr<PollSource> Source = Host.inputs().get(Self.Rep);
  if (!Source) {
    Source = std::make_shared<AlwaysReady>();
  }
  return Runtime::Component::Own<PollSource>{
      Host.pollables().add(std::move(Source))};
}

Expect<IoResult<uint64_t>>
OutputCheckWrite::body(Runtime::Component::CallingFrame &, OutputBorrow Self) {
  auto Stream = Host.outputs().get(Self.Rep);
  if (!Stream) {
    return IoResult<uint64_t>(
        Unexpected<StreamErrorVal>(StreamErrorVal{true, 0}));
  }
  auto Res = Stream->checkWrite();
  if (!Res) {
    return IoResult<uint64_t>(
        Unexpected<StreamErrorVal>(Host.errorValue(std::move(Res.error()))));
  }
  return IoResult<uint64_t>(*Res);
}

Expect<IoResult<Runtime::Component::Unit>>
OutputWrite::body(Runtime::Component::CallingFrame &, OutputBorrow Self,
                  std::vector<uint8_t> Contents) {
  auto Stream = Host.outputs().get(Self.Rep);
  if (!Stream) {
    return IoResult<Runtime::Component::Unit>(
        Unexpected<StreamErrorVal>(StreamErrorVal{true, 0}));
  }
  auto Res = Stream->write(Contents);
  if (!Res) {
    return IoResult<Runtime::Component::Unit>(
        Unexpected<StreamErrorVal>(Host.errorValue(std::move(Res.error()))));
  }
  return IoResult<Runtime::Component::Unit>(Runtime::Component::Unit{});
}

Expect<IoResult<Runtime::Component::Unit>>
OutputFlush::body(Runtime::Component::CallingFrame &, OutputBorrow Self) {
  auto Stream = Host.outputs().get(Self.Rep);
  if (!Stream) {
    return IoResult<Runtime::Component::Unit>(
        Unexpected<StreamErrorVal>(StreamErrorVal{true, 0}));
  }
  auto Res = Stream->flush();
  if (!Res) {
    return IoResult<Runtime::Component::Unit>(
        Unexpected<StreamErrorVal>(Host.errorValue(std::move(Res.error()))));
  }
  return IoResult<Runtime::Component::Unit>(Runtime::Component::Unit{});
}

Expect<Runtime::Component::Own<PollSource>>
OutputSubscribe::body(Runtime::Component::CallingFrame &, OutputBorrow Self) {
  std::shared_ptr<PollSource> Source = Host.outputs().get(Self.Rep);
  if (!Source) {
    Source = std::make_shared<AlwaysReady>();
  }
  return Runtime::Component::Own<PollSource>{
      Host.pollables().add(std::move(Source))};
}

Expect<IoResult<Runtime::Component::Unit>>
OutputWriteZeroes::body(Runtime::Component::CallingFrame &, OutputBorrow Self,
                        uint64_t Len) {
  auto Stream = Host.outputs().get(Self.Rep);
  if (!Stream) {
    return IoResult<Runtime::Component::Unit>(
        Unexpected<StreamErrorVal>(StreamErrorVal{true, 0}));
  }
  const std::vector<uint8_t> Zeroes(4096, 0);
  while (Len > 0) {
    const auto Chunk =
        static_cast<size_t>(std::min<uint64_t>(Len, Zeroes.size()));
    auto Res = Stream->write(Span<const uint8_t>(Zeroes.data(), Chunk));
    if (!Res) {
      return IoResult<Runtime::Component::Unit>(
          Unexpected<StreamErrorVal>(Host.errorValue(std::move(Res.error()))));
    }
    Len -= Chunk;
  }
  return IoResult<Runtime::Component::Unit>(Runtime::Component::Unit{});
}

Expect<IoResult<uint64_t>>
OutputSplice::body(Runtime::Component::CallingFrame &, OutputBorrow Self,
                   InputBorrow Src, uint64_t Len) {
  auto Out = Host.outputs().get(Self.Rep);
  auto In = Host.inputs().get(Src.Rep);
  if (!Out || !In) {
    return IoResult<uint64_t>(
        Unexpected<StreamErrorVal>(StreamErrorVal{true, 0}));
  }
  auto Data = Blocking ? In->blockingRead(Len) : In->read(Len);
  if (!Data) {
    return IoResult<uint64_t>(
        Unexpected<StreamErrorVal>(Host.errorValue(std::move(Data.error()))));
  }
  if (auto Res = Out->write(*Data); !Res) {
    return IoResult<uint64_t>(
        Unexpected<StreamErrorVal>(Host.errorValue(std::move(Res.error()))));
  }
  return IoResult<uint64_t>(static_cast<uint64_t>(Data->size()));
}

IoErrorInstance::IoErrorInstance(IoHost &Host)
    : ComponentInstance("wasi:io/error@0.2.12") {
  const uint32_t Idx = addHostResourceType<IoError>(
      [&Host](uint64_t Rep) { Host.errors().remove(Rep); });
  exportType("error", Idx);
  Host.ErrorType = *getTypeResource(Idx);
  addHostFunc("[method]error.to-debug-string",
              std::make_unique<ErrorToDebugString>(Host));
}

PollInstance::PollInstance(IoHost &Host)
    : ComponentInstance("wasi:io/poll@0.2.12") {
  const uint32_t Idx = addHostResourceType<PollSource>(
      [&Host](uint64_t Rep) { Host.pollables().remove(Rep); });
  exportType("pollable", Idx);
  Host.PollableType = *getTypeResource(Idx);
  addHostFunc("[method]pollable.ready", std::make_unique<PollableReady>(Host));
  addHostFunc("[method]pollable.block", std::make_unique<PollableBlock>(Host));
  addHostFunc("poll", std::make_unique<Poll>(Host));
}

StreamsInstance::StreamsInstance(IoHost &Host)
    : ComponentInstance("wasi:io/streams@0.2.12") {
  exportType("error", addSharedResourceType<IoError>(Host.ErrorType));
  exportType("pollable", addSharedResourceType<PollSource>(Host.PollableType));
  const uint32_t InputIdx = addHostResourceType<InputStream>(
      [&Host](uint64_t Rep) { Host.inputs().remove(Rep); });
  exportType("input-stream", InputIdx);
  Host.InputType = *getTypeResource(InputIdx);
  const uint32_t OutputIdx = addHostResourceType<OutputStream>(
      [&Host](uint64_t Rep) { Host.outputs().remove(Rep); });
  exportType("output-stream", OutputIdx);
  Host.OutputType = *getTypeResource(OutputIdx);
  exportType("stream-error",
             Runtime::Component::Wit<StreamErrorVal>::type(getTypeMinter())
                 .getTypeIndex());
  addHostFunc("[method]input-stream.read",
              std::make_unique<InputRead>(Host, false));
  addHostFunc("[method]input-stream.blocking-read",
              std::make_unique<InputRead>(Host, true));
  addHostFunc("[method]input-stream.skip",
              std::make_unique<InputSkip>(Host, false));
  addHostFunc("[method]input-stream.blocking-skip",
              std::make_unique<InputSkip>(Host, true));
  addHostFunc("[method]input-stream.subscribe",
              std::make_unique<InputSubscribe>(Host));
  addHostFunc("[method]output-stream.check-write",
              std::make_unique<OutputCheckWrite>(Host));
  addHostFunc("[method]output-stream.write",
              std::make_unique<OutputWrite>(Host));
  addHostFunc("[method]output-stream.blocking-write-and-flush",
              std::make_unique<OutputWrite>(Host));
  addHostFunc("[method]output-stream.flush",
              std::make_unique<OutputFlush>(Host));
  addHostFunc("[method]output-stream.blocking-flush",
              std::make_unique<OutputFlush>(Host));
  addHostFunc("[method]output-stream.subscribe",
              std::make_unique<OutputSubscribe>(Host));
  addHostFunc("[method]output-stream.write-zeroes",
              std::make_unique<OutputWriteZeroes>(Host));
  addHostFunc("[method]output-stream.blocking-write-zeroes-and-flush",
              std::make_unique<OutputWriteZeroes>(Host));
  addHostFunc("[method]output-stream.splice",
              std::make_unique<OutputSplice>(Host, false));
  addHostFunc("[method]output-stream.blocking-splice",
              std::make_unique<OutputSplice>(Host, true));
}

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeIoInstances(IoHost &Host) {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Out;
  Out.push_back(std::make_unique<IoErrorInstance>(Host));
  Out.push_back(std::make_unique<PollInstance>(Host));
  Out.push_back(std::make_unique<StreamsInstance>(Host));
  return Out;
}

std::tuple<Runtime::Component::Own<InputStream>,
           Runtime::Component::Own<OutputStream>>
IoHost::socketStreams(const std::shared_ptr<WASI::VINode> &Node) noexcept {
  auto In = std::make_shared<NodeInputStream>(*this, Node,
                                              NodeInputStream::Mode::Socket);
  auto Out = std::make_shared<NodeOutputStream>(*this, Node,
                                                NodeOutputStream::Mode::Socket);
  return {Runtime::Component::Own<InputStream>{inputs().add(std::move(In))},
          Runtime::Component::Own<OutputStream>{outputs().add(std::move(Out))}};
}

Runtime::Component::Own<PollSource>
IoHost::readySource(const std::shared_ptr<WASI::VINode> &Node, bool Active,
                    bool Write) noexcept {
  std::shared_ptr<PollSource> Source;
  if (Active) {
    Source = std::make_shared<NodeReadySource>(*this, Node, Write);
  } else {
    Source = std::make_shared<AlwaysReady>();
  }
  return Runtime::Component::Own<PollSource>{
      pollables().add(std::move(Source))};
}

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
