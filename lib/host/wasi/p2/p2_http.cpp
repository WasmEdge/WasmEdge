// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "host/wasi/component/node.h"
#include "host/wasi/component/result.h"
#include "host/wasi/p2/http.h"
#include "host/wasi/p3/sockets.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

namespace {

constexpr std::array<uint8_t, 2> Crlf{'\r', '\n'};

std::string toLowerCase(std::string_view S) {
  std::string Out(S);
  for (auto &C : Out) {
    C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
  }
  return Out;
}

template <typename T> HttpResult<T> fail(HttpError E) noexcept {
  return Unexpected<HttpError>(std::move(E));
}

HeaderStatus headerFail(HeaderError E) noexcept {
  return Unexpected<HeaderError>(E);
}

// The host and port of an authority; a missing port is the scheme's.
bool splitAuthority(std::string_view Authority, std::string &Host,
                    uint16_t &Port) {
  if (Authority.empty()) {
    return false;
  }
  std::string_view Rest = Authority;
  if (Rest.front() == '[') {
    const auto End = Rest.find(']');
    if (End == std::string_view::npos) {
      return false;
    }
    Host = std::string(Rest.substr(1, End - 1));
    Rest = Rest.substr(End + 1);
    if (Rest.empty()) {
      return true;
    }
    if (Rest.front() != ':') {
      return false;
    }
    Rest = Rest.substr(1);
  } else {
    const auto Colon = Rest.rfind(':');
    if (Colon == std::string_view::npos) {
      Host = std::string(Rest);
      return true;
    }
    Host = std::string(Rest.substr(0, Colon));
    Rest = Rest.substr(Colon + 1);
  }
  if (Rest.empty() || Rest.size() > 5) {
    return false;
  }
  uint32_t P = 0;
  for (char C : Rest) {
    if (!std::isdigit(static_cast<unsigned char>(C))) {
      return false;
    }
    P = P * 10 + static_cast<uint32_t>(C - '0');
  }
  if (P == 0 || P > 65535) {
    return false;
  }
  Port = static_cast<uint16_t>(P);
  return !Host.empty();
}

// A non-blocking connection to Host:Port, trying each resolved address.
Expect<HttpResult<std::shared_ptr<WASI::VINode>>>
connectTo(Runtime::Component::CallingFrame &Frame, const std::string &Host,
          uint16_t Port) {
  auto Addresses = resolveHttpAddresses(Host, Port);
  if (!Addresses) {
    return fail<std::shared_ptr<WASI::VINode>>(std::move(Addresses.error()));
  }
  std::optional<HttpError> Last;
  for (const auto &Raw : *Addresses) {
    auto Node =
        WASI::VINode::sockOpen(Raw.Family, __WASI_SOCK_TYPE_SOCK_STREAM);
    if (Node) {
      if (auto Res = (*Node)->fdFdstatSetFlags(__WASI_FDFLAGS_NONBLOCK); !Res) {
        Node = WASI::WasiUnexpect(Res.error());
      }
    }
    if (!Node) {
      Last = getConnectError(Node.error());
      continue;
    }
    EXPECTED_TRY(auto Res, WasiComponent::sockConnect(Frame, **Node, Raw.Family,
                                                      Raw.bytes(), Raw.Port));
    if (!Res) {
      Last = getConnectError(Res.error());
      continue;
    }
    return HttpResult<std::shared_ptr<WASI::VINode>>(std::move(*Node));
  }
  return fail<std::shared_ptr<WASI::VINode>>(
      Last ? *Last : HttpError::of(HttpError::DestinationUnavailable));
}

// A header section from Data[Pos..] up to and past the blank line.
bool parseHeaders(const std::vector<uint8_t> &Data, size_t &Pos, Fields &Out) {
  while (true) {
    const auto Begin = Data.begin() + static_cast<std::ptrdiff_t>(Pos);
    const auto Eol = std::search(Begin, Data.end(), Crlf.begin(), Crlf.end());
    if (Eol == Data.end()) {
      return false;
    }
    std::string_view Line(reinterpret_cast<const char *>(&*Begin),
                          static_cast<size_t>(Eol - Begin));
    Pos += Line.size() + 2;
    if (Line.empty()) {
      return true;
    }
    const auto Colon = Line.find(':');
    if (Colon == std::string_view::npos || Colon == 0) {
      return false;
    }
    std::string_view Name = Line.substr(0, Colon);
    std::string_view Value = Line.substr(Colon + 1);
    while (!Value.empty() && (Value.front() == ' ' || Value.front() == '\t')) {
      Value.remove_prefix(1);
    }
    while (!Value.empty() && (Value.back() == ' ' || Value.back() == '\t')) {
      Value.remove_suffix(1);
    }
    if (!Fields::isValidName(Name)) {
      return false;
    }
    Out.Entries.emplace_back(toLowerCase(Name),
                             std::vector<uint8_t>(Value.begin(), Value.end()));
  }
}

// Chunked transfer coding: the chunks into Body, the trailers into Trailers.
bool decodeChunked(const std::vector<uint8_t> &Data, size_t Pos,
                   std::vector<uint8_t> &Body, Fields &Trailers) {
  while (true) {
    const auto Begin = Data.begin() + static_cast<std::ptrdiff_t>(Pos);
    const auto Eol = std::search(Begin, Data.end(), Crlf.begin(), Crlf.end());
    if (Eol == Data.end()) {
      return false;
    }
    std::string_view Line(reinterpret_cast<const char *>(&*Begin),
                          static_cast<size_t>(Eol - Begin));
    Pos += Line.size() + 2;
    const auto Semi = Line.find(';');
    std::string_view Hex =
        Semi == std::string_view::npos ? Line : Line.substr(0, Semi);
    if (Hex.empty()) {
      return false;
    }
    size_t Size = 0;
    for (char C : Hex) {
      const auto U = static_cast<unsigned char>(C);
      if (!std::isxdigit(U)) {
        return false;
      }
      const size_t Digit =
          std::isdigit(U) ? static_cast<size_t>(C - '0')
                          : static_cast<size_t>(std::tolower(U) - 'a' + 10);
      if (Size > (std::numeric_limits<size_t>::max() >> 4)) {
        return false;
      }
      Size = (Size << 4) | Digit;
    }
    if (Size == 0) {
      return parseHeaders(Data, Pos, Trailers);
    }
    if (Pos + Size + 2 > Data.size()) {
      return false;
    }
    Body.insert(Body.end(), Data.begin() + static_cast<std::ptrdiff_t>(Pos),
                Data.begin() + static_cast<std::ptrdiff_t>(Pos + Size));
    Pos += Size;
    if (Data[Pos] != '\r' || Data[Pos + 1] != '\n') {
      return false;
    }
    Pos += 2;
  }
}

// The response out of the raw reply.
HttpResult<std::unique_ptr<IncomingResponse>>
parseResponse(const std::vector<uint8_t> &Data, bool HeadRequest) {
  const auto Eol =
      std::search(Data.begin(), Data.end(), Crlf.begin(), Crlf.end());
  if (Eol == Data.end()) {
    return HttpResult<std::unique_ptr<IncomingResponse>>(
        fail<std::unique_ptr<IncomingResponse>>(
            HttpError::of(HttpError::HttpResponseIncomplete)));
  }
  std::string_view Line(reinterpret_cast<const char *>(Data.data()),
                        static_cast<size_t>(Eol - Data.begin()));
  // "HTTP/1.x SP status SP reason"
  if (Line.size() < 12 || Line.substr(0, 7) != "HTTP/1." || Line[8] != ' ') {
    return HttpResult<std::unique_ptr<IncomingResponse>>(
        fail<std::unique_ptr<IncomingResponse>>(
            HttpError::of(HttpError::HttpProtocolError)));
  }
  uint32_t Status = 0;
  for (size_t I = 9; I < 12; ++I) {
    if (!std::isdigit(static_cast<unsigned char>(Line[I]))) {
      return HttpResult<std::unique_ptr<IncomingResponse>>(
          fail<std::unique_ptr<IncomingResponse>>(
              HttpError::of(HttpError::HttpProtocolError)));
    }
    Status = Status * 10 + static_cast<uint32_t>(Line[I] - '0');
  }
  auto Response = std::make_unique<IncomingResponse>();
  Response->Status = static_cast<uint16_t>(Status);
  Response->Headers.Immutable = true;
  size_t Pos = Line.size() + 2;
  if (!parseHeaders(Data, Pos, Response->Headers)) {
    return HttpResult<std::unique_ptr<IncomingResponse>>(
        fail<std::unique_ptr<IncomingResponse>>(
            HttpError::of(HttpError::HttpResponseIncomplete)));
  }
  Response->Body = std::make_unique<IncomingBody>();
  const bool NoBody = HeadRequest || (Status >= 100 && Status < 200) ||
                      Status == 204 || Status == 304;
  if (NoBody) {
    return HttpResult<std::unique_ptr<IncomingResponse>>(std::move(Response));
  }
  auto Coding = Response->Headers.first("transfer-encoding");
  if (Coding && Fields::isSameName(*Coding, "chunked")) {
    Fields Trailers;
    Trailers.Immutable = true;
    if (!decodeChunked(Data, Pos, Response->Body->Data, Trailers)) {
      return HttpResult<std::unique_ptr<IncomingResponse>>(
          fail<std::unique_ptr<IncomingResponse>>(
              HttpError::of(HttpError::HttpResponseIncomplete)));
    }
    if (!Trailers.Entries.empty()) {
      Response->Body->Trailers = std::move(Trailers);
    }
    return HttpResult<std::unique_ptr<IncomingResponse>>(std::move(Response));
  }
  if (Coding) {
    HttpError E = HttpError::of(HttpError::HttpResponseTransferCoding);
    E.Text = *Coding;
    return HttpResult<std::unique_ptr<IncomingResponse>>(
        fail<std::unique_ptr<IncomingResponse>>(std::move(E)));
  }
  if (auto Length = Response->Headers.first("content-length")) {
    uint64_t N = 0;
    for (char C : *Length) {
      if (!std::isdigit(static_cast<unsigned char>(C))) {
        return HttpResult<std::unique_ptr<IncomingResponse>>(
            fail<std::unique_ptr<IncomingResponse>>(
                HttpError::of(HttpError::HttpProtocolError)));
      }
      N = N * 10 + static_cast<uint64_t>(C - '0');
    }
    if (Pos + N > Data.size()) {
      HttpError E = HttpError::of(HttpError::HttpResponseBodySize);
      E.Size = Data.size() - Pos;
      return HttpResult<std::unique_ptr<IncomingResponse>>(
          fail<std::unique_ptr<IncomingResponse>>(std::move(E)));
    }
    Response->Body->Data.assign(Data.begin() + static_cast<std::ptrdiff_t>(Pos),
                                Data.begin() +
                                    static_cast<std::ptrdiff_t>(Pos + N));
    return HttpResult<std::unique_ptr<IncomingResponse>>(std::move(Response));
  }
  Response->Body->Data.assign(Data.begin() + static_cast<std::ptrdiff_t>(Pos),
                              Data.end());
  return HttpResult<std::unique_ptr<IncomingResponse>>(std::move(Response));
}

// A pollable over the future FutureRep names, ready once it has a result.
class FutureResponseSource : public PollSource {
public:
  FutureResponseSource(HttpHost &H, uint64_t Rep) noexcept
      : Http(H), FutureRep(Rep) {}
  bool ready() noexcept override {
    const FutureIncomingResponse *Future = Http.FutureResponses.get(FutureRep);
    return Future == nullptr || Future->Result.has_value();
  }
  void addInterest(std::optional<std::chrono::steady_clock::time_point> &,
                   std::vector<PollEntry> &) noexcept override {}

private:
  HttpHost &Http;
  uint64_t FutureRep;
};

} // namespace

HttpResult<std::vector<WasiP3::RawAddress>>
resolveHttpAddresses(const std::string &Host, uint16_t Port) {
  auto Addresses = WasiP3::resolveHostname(Host);
  if (!Addresses) {
    return HttpResult<std::vector<WasiP3::RawAddress>>(
        Unexpected<HttpError>(getResolveError(Addresses.error())));
  }
  if (Addresses->empty()) {
    return HttpResult<std::vector<WasiP3::RawAddress>>(
        Unexpected<HttpError>(HttpError::of(HttpError::DestinationNotFound)));
  }
  std::vector<WasiP3::RawAddress> Raws;
  for (const auto &A : *Addresses) {
    WasiP3::IpSocketAddress Addr;
    Addr.Family = A.Family;
    if (A.Family == WasiP3::IpAddressFamily::Ipv4) {
      Addr.V4 = WasiP3::Ipv4SocketAddress{Port, A.V4};
    } else {
      Addr.V6 = WasiP3::Ipv6SocketAddress{Port, 0, A.V6, 0};
    }
    Raws.push_back(Addr.raw());
  }
  return Raws;
}

HttpError getConnectError(__wasi_errno_t Errno) noexcept {
  switch (Errno) {
  case __WASI_ERRNO_CONNREFUSED:
    return HttpError::of(HttpError::ConnectionRefused);
  case __WASI_ERRNO_TIMEDOUT:
    return HttpError::of(HttpError::ConnectionTimeout);
  case __WASI_ERRNO_NETUNREACH:
  case __WASI_ERRNO_HOSTUNREACH:
  case __WASI_ERRNO_NETDOWN:
    return HttpError::of(HttpError::DestinationUnavailable);
  case __WASI_ERRNO_ACCES:
  case __WASI_ERRNO_PERM:
    return HttpError::of(HttpError::DestinationIpProhibited);
  case __WASI_ERRNO_MFILE:
  case __WASI_ERRNO_NFILE:
    return HttpError::of(HttpError::ConnectionLimitReached);
  default:
    return HttpError::internal("connect failed");
  }
}

HttpError getResolveError(__wasi_errno_t Errno) noexcept {
  switch (Errno) {
  case __WASI_ERRNO_AIAGAIN:
    return HttpError::of(HttpError::DnsTimeout);
  case __WASI_ERRNO_AINONAME:
  case __WASI_ERRNO_AINODATA:
    return HttpError::of(HttpError::DestinationNotFound);
  default: {
    HttpError E = HttpError::of(HttpError::DnsError);
    E.Dns.Rcode = "resolver failure";
    return E;
  }
  }
}

bool Fields::isSameName(std::string_view A, std::string_view B) noexcept {
  if (A.size() != B.size()) {
    return false;
  }
  for (size_t I = 0; I < A.size(); ++I) {
    if (std::tolower(static_cast<unsigned char>(A[I])) !=
        std::tolower(static_cast<unsigned char>(B[I]))) {
      return false;
    }
  }
  return true;
}

bool Fields::isValidName(std::string_view Name) noexcept {
  if (Name.empty()) {
    return false;
  }
  for (char C : Name) {
    const auto U = static_cast<unsigned char>(C);
    if (U <= 0x20 || U >= 0x7F ||
        std::strchr("()<>@,;:\\\"/[]?={}", C) != nullptr) {
      return false;
    }
  }
  return true;
}

bool Fields::isValidValue(Span<const uint8_t> Value) noexcept {
  for (uint8_t B : Value) {
    if (B == '\r' || B == '\n' || B == 0) {
      return false;
    }
  }
  return true;
}

bool Fields::isForbiddenName(std::string_view Name) noexcept {
  for (const char *F :
       {"connection", "keep-alive", "proxy-authenticate", "proxy-authorization",
        "proxy-connection", "te", "transfer-encoding", "upgrade",
        "http2-settings", "host"}) {
    if (Fields::isSameName(Name, F)) {
      return true;
    }
  }
  return false;
}

std::string Method::text() const {
  switch (Case) {
  case Method::Get:
    return "GET";
  case Method::Head:
    return "HEAD";
  case Method::Post:
    return "POST";
  case Method::Put:
    return "PUT";
  case Method::Delete:
    return "DELETE";
  case Method::Connect:
    return "CONNECT";
  case Method::Options:
    return "OPTIONS";
  case Method::Trace:
    return "TRACE";
  case Method::Patch:
    return "PATCH";
  case Method::Other:
    return Name;
  }
  return "GET";
}

std::unique_ptr<Fields> Fields::immutableCopy(const Fields *From) {
  auto Out = std::make_unique<Fields>();
  if (From) {
    Out->Entries = From->Entries;
  }
  Out->Immutable = true;
  return Out;
}

std::vector<std::vector<uint8_t>> Fields::get(std::string_view Name) const {
  std::vector<std::vector<uint8_t>> Out;
  for (const auto &[N, V] : Entries) {
    if (Fields::isSameName(N, Name)) {
      Out.push_back(V);
    }
  }
  return Out;
}

bool Fields::has(std::string_view Name) const noexcept {
  for (const auto &[N, V] : Entries) {
    if (Fields::isSameName(N, Name)) {
      return true;
    }
  }
  return false;
}

std::optional<std::string> Fields::first(std::string_view Name) const {
  for (const auto &[N, V] : Entries) {
    if (Fields::isSameName(N, Name)) {
      return std::string(V.begin(), V.end());
    }
  }
  return std::nullopt;
}

StreamResult<uint64_t> BodyOutputStream::checkWrite() noexcept {
  const OutgoingBody *Body = Http.OutgoingBodies.get(BodyRep);
  if (!Body || Body->Finished) {
    return Unexpected<StreamError>(StreamError{true, {}});
  }
  return uint64_t(65536);
}

StreamResult<Runtime::Component::WitUnit>
BodyOutputStream::write(Span<const uint8_t> Data) noexcept {
  OutgoingBody *Body = Http.OutgoingBodies.get(BodyRep);
  if (!Body || Body->Finished) {
    return Unexpected<StreamError>(StreamError{true, {}});
  }
  Body->Data.insert(Body->Data.end(), Data.begin(), Data.end());
  return Runtime::Component::WitUnit{};
}

StreamResult<std::vector<uint8_t>>
BodyInputStream::read(uint64_t Len) noexcept {
  IncomingBody *Body = Http.IncomingBodies.get(BodyRep);
  if (!Body || Body->Offset >= Body->Data.size()) {
    return Unexpected<StreamError>(StreamError{true, {}});
  }
  const size_t N = static_cast<size_t>(
      std::min<uint64_t>(Len, Body->Data.size() - Body->Offset));
  std::vector<uint8_t> Out(
      Body->Data.begin() + static_cast<std::ptrdiff_t>(Body->Offset),
      Body->Data.begin() + static_cast<std::ptrdiff_t>(Body->Offset + N));
  Body->Offset += N;
  return Out;
}

HttpResult<std::vector<uint8_t>> encodeHttp11(const OutgoingRequest &Request,
                                              const OutgoingBody *Content,
                                              std::string &Host,
                                              uint16_t &Port) {
  if (Request.S && Request.S->Case == Scheme::Https) {
    return HttpResult<std::vector<uint8_t>>(fail<std::vector<uint8_t>>(
        HttpError::internal("TLS is not supported")));
  }
  if (Request.S && Request.S->Case == Scheme::Other) {
    return HttpResult<std::vector<uint8_t>>(fail<std::vector<uint8_t>>(
        HttpError::of(HttpError::HttpRequestUriInvalid)));
  }
  if (!Request.Authority) {
    return HttpResult<std::vector<uint8_t>>(fail<std::vector<uint8_t>>(
        HttpError::of(HttpError::HttpRequestUriInvalid)));
  }
  Port = 80;
  if (!splitAuthority(*Request.Authority, Host, Port)) {
    return HttpResult<std::vector<uint8_t>>(fail<std::vector<uint8_t>>(
        HttpError::of(HttpError::HttpRequestUriInvalid)));
  }
  const std::string MethodName = Request.M.text();
  if (!Fields::isValidName(MethodName)) {
    return HttpResult<std::vector<uint8_t>>(fail<std::vector<uint8_t>>(
        HttpError::of(HttpError::HttpRequestMethodInvalid)));
  }
  std::string Target = Request.PathWithQuery.value_or("/");
  if (Target.empty()) {
    Target = "/";
  }
  for (char C : Target) {
    const auto U = static_cast<unsigned char>(C);
    if (U <= 0x20 || U >= 0x7F) {
      return HttpResult<std::vector<uint8_t>>(fail<std::vector<uint8_t>>(
          HttpError::of(HttpError::HttpRequestUriInvalid)));
    }
  }
  const auto &Body = Content ? Content->Data : std::vector<uint8_t>{};
  if (auto Length = Request.Headers.first("content-length")) {
    uint64_t N = 0;
    for (char C : *Length) {
      if (!std::isdigit(static_cast<unsigned char>(C))) {
        return HttpResult<std::vector<uint8_t>>(fail<std::vector<uint8_t>>(
            HttpError::of(HttpError::HttpRequestHeaderSize)));
      }
      N = N * 10 + static_cast<uint64_t>(C - '0');
    }
    if (N != Body.size()) {
      HttpError E = HttpError::of(HttpError::HttpRequestBodySize);
      E.Size = Body.size();
      return HttpResult<std::vector<uint8_t>>(
          fail<std::vector<uint8_t>>(std::move(E)));
    }
  }

  std::string Head = MethodName + " " + Target + " HTTP/1.1\r\n";
  Head += "Host: " + *Request.Authority + "\r\n";
  Head += "Connection: close\r\n";
  bool HasLength = false;
  for (const auto &[Name, Value] : Request.Headers.Entries) {
    if (Fields::isSameName(Name, "content-length")) {
      HasLength = true;
    }
    Head += Name + ": " + std::string(Value.begin(), Value.end()) + "\r\n";
  }
  const bool BodyMethod = Request.M.Case == Method::Post ||
                          Request.M.Case == Method::Put ||
                          Request.M.Case == Method::Patch;
  const bool Chunked =
      Content && Content->Trailers && !Content->Trailers->Entries.empty();
  if (Chunked) {
    if (HasLength) {
      return HttpResult<std::vector<uint8_t>>(fail<std::vector<uint8_t>>(
          HttpError::of(HttpError::HttpRequestTrailerSize)));
    }
    Head += "Transfer-Encoding: chunked\r\n";
  } else if (!HasLength && (!Body.empty() || BodyMethod)) {
    Head += "Content-Length: " + std::to_string(Body.size()) + "\r\n";
  }
  Head += "\r\n";
  std::vector<uint8_t> Payload;
  if (Chunked) {
    std::string Chunk;
    if (!Body.empty()) {
      char Hex[32];
      std::snprintf(Hex, sizeof(Hex), "%zx\r\n", Body.size());
      Chunk = Hex;
    }
    Payload.assign(Chunk.begin(), Chunk.end());
    if (!Body.empty()) {
      Payload.insert(Payload.end(), Body.begin(), Body.end());
      Payload.push_back('\r');
      Payload.push_back('\n');
    }
    std::string Tail = "0\r\n";
    for (const auto &[Name, Value] : Content->Trailers->Entries) {
      Tail += Name + ": " + std::string(Value.begin(), Value.end()) + "\r\n";
    }
    Tail += "\r\n";
    Payload.insert(Payload.end(), Tail.begin(), Tail.end());
  } else {
    Payload = Body;
  }

  std::vector<uint8_t> Wire(Head.begin(), Head.end());
  Wire.insert(Wire.end(), Payload.begin(), Payload.end());
  return Wire;
}

HttpResult<std::unique_ptr<IncomingResponse>>
decodeHttp11(const std::vector<uint8_t> &Reply, bool HeadRequest) {
  if (Reply.empty()) {
    return HttpResult<std::unique_ptr<IncomingResponse>>(
        fail<std::unique_ptr<IncomingResponse>>(
            HttpError::of(HttpError::HttpResponseIncomplete)));
  }
  return parseResponse(Reply, HeadRequest);
}

Expect<HttpResult<std::unique_ptr<IncomingResponse>>>
exchangeHttp11(Runtime::Component::CallingFrame &Frame,
               const OutgoingRequest &Request, const OutgoingBody *Body) {
  std::string Host;
  uint16_t Port = 80;
  auto Wire = encodeHttp11(Request, Body, Host, Port);
  if (!Wire) {
    return fail<std::unique_ptr<IncomingResponse>>(std::move(Wire.error()));
  }
  EXPECTED_TRY(auto Node, connectTo(Frame, Host, Port));
  if (!Node) {
    return fail<std::unique_ptr<IncomingResponse>>(std::move(Node.error()));
  }
  EXPECTED_TRY(auto Sent, WasiComponent::sockSend(Frame, **Node, *Wire));
  if (!Sent) {
    return fail<std::unique_ptr<IncomingResponse>>(
        HttpError::of(HttpError::ConnectionWriteTimeout));
  }
  std::vector<uint8_t> Reply;
  std::vector<uint8_t> Buffer(65536);
  while (true) {
    EXPECTED_TRY(auto Read, WasiComponent::sockRecv(Frame, **Node, Buffer));
    if (!Read) {
      return fail<std::unique_ptr<IncomingResponse>>(
          HttpError::of(HttpError::ConnectionTerminated));
    }
    if (*Read == 0) {
      break;
    }
    Reply.insert(Reply.end(), Buffer.begin(), Buffer.begin() + *Read);
  }
  return decodeHttp11(Reply, Request.M.Case == Method::Head);
}

Expect<void> HttpHost::runExchange(Runtime::Component::CallingFrame &Frame,
                                   uint64_t Rep) {
  EXPECTED_TRY(Frame.waitUntil([this, Rep]() {
    const FutureIncomingResponse *Future = FutureResponses.get(Rep);
    if (Future == nullptr) {
      return true;
    }
    const OutgoingBody *Body = OutgoingBodies.get(Future->Request.BodyRep);
    return Body == nullptr || Body->Finished || Body->Dropped;
  }));
  FutureIncomingResponse *Future = FutureResponses.get(Rep);
  if (Frame.isCancelled() || Future == nullptr) {
    return {};
  }
  // The future may be dropped while the exchange parks.
  const OutgoingRequest Request = Future->Request;
  auto Body = OutgoingBodies.remove(Request.BodyRep);
  Future->Request.BodyRep = 0;
  EXPECTED_TRY(auto Reply, exchangeHttp11(Frame, Request, Body.get()));
  Future = FutureResponses.get(Rep);
  if (Future != nullptr && !Frame.isCancelled()) {
    Future->Result = std::move(Reply);
  }
  return {};
}

void HttpHost::removeOutgoingRequest(uint64_t Rep) noexcept {
  if (auto Request = OutgoingRequests.remove(Rep)) {
    OutgoingBodies.remove(Request->BodyRep);
  }
}

void HttpHost::removeOutgoingResponse(uint64_t Rep) noexcept {
  if (auto Response = OutgoingResponses.remove(Rep)) {
    OutgoingBodies.remove(Response->BodyRep);
  }
}

void HttpHost::removeFutureResponse(uint64_t Rep) noexcept {
  if (auto Future = FutureResponses.remove(Rep)) {
    OutgoingBodies.remove(Future->Request.BodyRep);
  }
}

Expect<Runtime::Component::Own<Fields>>
FieldsNew::body(Runtime::Component::CallingFrame &) {
  return Runtime::Component::Own<Fields>{
      Http.FieldsTable.add(std::make_unique<Fields>())};
}

Expect<Expected<Runtime::Component::Own<Fields>, HeaderError>>
FieldsFromList::body(Runtime::Component::CallingFrame &,
                     std::vector<FieldEntry> Entries) {
  auto F = std::make_unique<Fields>();
  for (auto &[Name, Value] : Entries) {
    if (!Fields::isValidName(Name) || !Fields::isValidValue(Value)) {
      return Expected<Runtime::Component::Own<Fields>, HeaderError>(
          Unexpected<HeaderError>(HeaderError::InvalidSyntax));
    }
    if (Fields::isForbiddenName(Name)) {
      return Expected<Runtime::Component::Own<Fields>, HeaderError>(
          Unexpected<HeaderError>(HeaderError::Forbidden));
    }
    F->Entries.emplace_back(std::move(Name), std::move(Value));
  }
  return Expected<Runtime::Component::Own<Fields>, HeaderError>(
      Runtime::Component::Own<Fields>{Http.FieldsTable.add(std::move(F))});
}

Expect<std::vector<std::vector<uint8_t>>>
FieldsGet::body(Runtime::Component::CallingFrame &, FieldsBorrow Self,
                std::string Name) {
  auto F = Http.FieldsTable.get(Self.Rep);
  if (!F) {
    return std::vector<std::vector<uint8_t>>{};
  }
  return F->get(Name);
}

Expect<bool> FieldsHas::body(Runtime::Component::CallingFrame &,
                             FieldsBorrow Self, std::string Name) {
  auto F = Http.FieldsTable.get(Self.Rep);
  return F && F->has(Name);
}

Expect<HeaderStatus> FieldsSet::body(Runtime::Component::CallingFrame &,
                                     FieldsBorrow Self, std::string Name,
                                     std::vector<std::vector<uint8_t>> Values) {
  auto F = Http.FieldsTable.get(Self.Rep);
  if (!F || F->Immutable) {
    return headerFail(HeaderError::Immutable);
  }
  if (!Fields::isValidName(Name)) {
    return headerFail(HeaderError::InvalidSyntax);
  }
  for (const auto &V : Values) {
    if (!Fields::isValidValue(V)) {
      return headerFail(HeaderError::InvalidSyntax);
    }
  }
  if (Fields::isForbiddenName(Name)) {
    return headerFail(HeaderError::Forbidden);
  }
  F->Entries.erase(std::remove_if(F->Entries.begin(), F->Entries.end(),
                                  [&Name](const auto &E) {
                                    return Fields::isSameName(E.first, Name);
                                  }),
                   F->Entries.end());
  for (auto &V : Values) {
    F->Entries.emplace_back(Name, std::move(V));
  }
  return HeaderStatus(Runtime::Component::WitUnit{});
}

Expect<HeaderStatus> FieldsDelete::body(Runtime::Component::CallingFrame &,
                                        FieldsBorrow Self, std::string Name) {
  auto F = Http.FieldsTable.get(Self.Rep);
  if (!F || F->Immutable) {
    return headerFail(HeaderError::Immutable);
  }
  if (!Fields::isValidName(Name)) {
    return headerFail(HeaderError::InvalidSyntax);
  }
  if (Fields::isForbiddenName(Name)) {
    return headerFail(HeaderError::Forbidden);
  }
  F->Entries.erase(std::remove_if(F->Entries.begin(), F->Entries.end(),
                                  [&Name](const auto &E) {
                                    return Fields::isSameName(E.first, Name);
                                  }),
                   F->Entries.end());
  return HeaderStatus(Runtime::Component::WitUnit{});
}

Expect<HeaderStatus> FieldsAppend::body(Runtime::Component::CallingFrame &,
                                        FieldsBorrow Self, std::string Name,
                                        std::vector<uint8_t> Value) {
  auto F = Http.FieldsTable.get(Self.Rep);
  if (!F || F->Immutable) {
    return headerFail(HeaderError::Immutable);
  }
  if (!Fields::isValidName(Name) || !Fields::isValidValue(Value)) {
    return headerFail(HeaderError::InvalidSyntax);
  }
  if (Fields::isForbiddenName(Name)) {
    return headerFail(HeaderError::Forbidden);
  }
  F->Entries.emplace_back(std::move(Name), std::move(Value));
  return HeaderStatus(Runtime::Component::WitUnit{});
}

Expect<std::vector<FieldEntry>>
FieldsEntries::body(Runtime::Component::CallingFrame &, FieldsBorrow Self) {
  std::vector<FieldEntry> Out;
  if (auto F = Http.FieldsTable.get(Self.Rep)) {
    for (const auto &[Name, Value] : F->Entries) {
      Out.emplace_back(Name, Value);
    }
  }
  return Out;
}

Expect<Runtime::Component::Own<Fields>>
FieldsClone::body(Runtime::Component::CallingFrame &, FieldsBorrow Self) {
  auto Copy = std::make_unique<Fields>();
  if (auto F = Http.FieldsTable.get(Self.Rep)) {
    Copy->Entries = F->Entries;
  }
  return Runtime::Component::Own<Fields>{Http.FieldsTable.add(std::move(Copy))};
}

Expect<Method> IncomingRequestMethod::body(Runtime::Component::CallingFrame &,
                                           IncomingRequestBorrow Self) {
  auto R = Http.IncomingRequests.get(Self.Rep);
  return R ? R->M : Method{};
}

Expect<std::optional<std::string>>
IncomingRequestText::body(Runtime::Component::CallingFrame &,
                          IncomingRequestBorrow Self) {
  auto R = Http.IncomingRequests.get(Self.Rep);
  if (!R) {
    return std::optional<std::string>{};
  }
  return Authority ? R->Authority : R->PathWithQuery;
}

Expect<std::optional<Scheme>>
IncomingRequestScheme::body(Runtime::Component::CallingFrame &,
                            IncomingRequestBorrow Self) {
  auto R = Http.IncomingRequests.get(Self.Rep);
  return R ? R->S : std::optional<Scheme>{};
}

Expect<Runtime::Component::Own<Fields>>
IncomingRequestHeaders::body(Runtime::Component::CallingFrame &,
                             IncomingRequestBorrow Self) {
  auto R = Http.IncomingRequests.get(Self.Rep);
  return Runtime::Component::Own<Fields>{
      Http.FieldsTable.add(Fields::immutableCopy(R ? &R->Headers : nullptr))};
}

Expect<Expected<Runtime::Component::Own<IncomingBody>,
                Runtime::Component::WitUnit>>
IncomingRequestConsume::body(Runtime::Component::CallingFrame &,
                             IncomingRequestBorrow Self) {
  auto R = Http.IncomingRequests.get(Self.Rep);
  if (!R || !R->Body) {
    return WasiComponent::failUnit<Runtime::Component::Own<IncomingBody>>();
  }
  return Expected<Runtime::Component::Own<IncomingBody>,
                  Runtime::Component::WitUnit>(
      Runtime::Component::Own<IncomingBody>{
          Http.IncomingBodies.add(std::move(R->Body))});
}

Expect<Runtime::Component::Own<OutgoingRequest>>
OutgoingRequestNew::body(Runtime::Component::CallingFrame &,
                         Runtime::Component::Own<Fields> Headers) {
  auto R = std::make_unique<OutgoingRequest>();
  if (auto Taken = Http.FieldsTable.remove(Headers.Rep)) {
    R->Headers = std::move(*Taken);
  }
  R->Headers.Immutable = true;
  return Runtime::Component::Own<OutgoingRequest>{
      Http.OutgoingRequests.add(std::move(R))};
}

Expect<Expected<Runtime::Component::Own<OutgoingBody>,
                Runtime::Component::WitUnit>>
OutgoingRequestBody::body(Runtime::Component::CallingFrame &,
                          OutgoingRequestBorrow Self) {
  auto R = Http.OutgoingRequests.get(Self.Rep);
  if (!R || R->BodyRep != 0) {
    return WasiComponent::failUnit<Runtime::Component::Own<OutgoingBody>>();
  }
  R->BodyRep = Http.OutgoingBodies.add(std::make_unique<OutgoingBody>());
  return Expected<Runtime::Component::Own<OutgoingBody>,
                  Runtime::Component::WitUnit>(
      Runtime::Component::Own<OutgoingBody>{R->BodyRep});
}

Expect<Method> OutgoingRequestMethod::body(Runtime::Component::CallingFrame &,
                                           OutgoingRequestBorrow Self) {
  auto R = Http.OutgoingRequests.get(Self.Rep);
  return R ? R->M : Method{};
}

Expect<Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>>
OutgoingRequestSetMethod::body(Runtime::Component::CallingFrame &,
                               OutgoingRequestBorrow Self, Method M) {
  auto R = Http.OutgoingRequests.get(Self.Rep);
  if (!R || (M.Case == Method::Other && !Fields::isValidName(M.Name))) {
    return WasiComponent::failUnit<Runtime::Component::WitUnit>();
  }
  R->M = std::move(M);
  return Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>(
      Runtime::Component::WitUnit{});
}

Expect<std::optional<std::string>>
OutgoingRequestText::body(Runtime::Component::CallingFrame &,
                          OutgoingRequestBorrow Self) {
  auto R = Http.OutgoingRequests.get(Self.Rep);
  if (!R) {
    return std::optional<std::string>{};
  }
  return Authority ? R->Authority : R->PathWithQuery;
}

Expect<Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>>
OutgoingRequestSetText::body(Runtime::Component::CallingFrame &,
                             OutgoingRequestBorrow Self,
                             std::optional<std::string> Value) {
  auto R = Http.OutgoingRequests.get(Self.Rep);
  if (!R) {
    return WasiComponent::failUnit<Runtime::Component::WitUnit>();
  }
  if (Value) {
    for (char C : *Value) {
      const auto U = static_cast<unsigned char>(C);
      if (U <= 0x20 || U >= 0x7F) {
        return WasiComponent::failUnit<Runtime::Component::WitUnit>();
      }
    }
    if (Authority) {
      std::string Host;
      uint16_t Port = 0;
      if (!splitAuthority(*Value, Host, Port)) {
        return WasiComponent::failUnit<Runtime::Component::WitUnit>();
      }
    }
  }
  (Authority ? R->Authority : R->PathWithQuery) = std::move(Value);
  return Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>(
      Runtime::Component::WitUnit{});
}

Expect<std::optional<Scheme>>
OutgoingRequestScheme::body(Runtime::Component::CallingFrame &,
                            OutgoingRequestBorrow Self) {
  auto R = Http.OutgoingRequests.get(Self.Rep);
  return R ? R->S : std::optional<Scheme>{};
}

Expect<Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>>
OutgoingRequestSetScheme::body(Runtime::Component::CallingFrame &,
                               OutgoingRequestBorrow Self,
                               std::optional<Scheme> S) {
  auto R = Http.OutgoingRequests.get(Self.Rep);
  if (!R) {
    return WasiComponent::failUnit<Runtime::Component::WitUnit>();
  }
  if (S && S->Case == Scheme::Other) {
    if (S->Name.empty()) {
      return WasiComponent::failUnit<Runtime::Component::WitUnit>();
    }
    for (char C : S->Name) {
      if (!std::isalnum(static_cast<unsigned char>(C)) && C != '+' &&
          C != '-' && C != '.') {
        return WasiComponent::failUnit<Runtime::Component::WitUnit>();
      }
    }
  }
  R->S = std::move(S);
  return Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>(
      Runtime::Component::WitUnit{});
}

Expect<Runtime::Component::Own<Fields>>
OutgoingRequestHeaders::body(Runtime::Component::CallingFrame &,
                             OutgoingRequestBorrow Self) {
  auto R = Http.OutgoingRequests.get(Self.Rep);
  return Runtime::Component::Own<Fields>{
      Http.FieldsTable.add(Fields::immutableCopy(R ? &R->Headers : nullptr))};
}

Expect<Runtime::Component::Own<RequestOptions>>
RequestOptionsNew::body(Runtime::Component::CallingFrame &) {
  return Runtime::Component::Own<RequestOptions>{
      Http.Options.add(std::make_unique<RequestOptions>())};
}

Expect<std::optional<uint64_t>>
RequestOptionsGet::body(Runtime::Component::CallingFrame &,
                        OptionsBorrow Self) {
  auto O = Http.Options.get(Self.Rep);
  if (!O) {
    return std::optional<uint64_t>{};
  }
  switch (W) {
  case Which::Connect:
    return O->ConnectTimeout;
  case Which::FirstByte:
    return O->FirstByteTimeout;
  case Which::BetweenBytes:
    return O->BetweenBytesTimeout;
  }
  return std::optional<uint64_t>{};
}

Expect<Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>>
RequestOptionsSet::body(Runtime::Component::CallingFrame &, OptionsBorrow Self,
                        std::optional<uint64_t> Duration) {
  auto O = Http.Options.get(Self.Rep);
  if (!O) {
    return WasiComponent::failUnit<Runtime::Component::WitUnit>();
  }
  switch (W) {
  case Which::Connect:
    O->ConnectTimeout = Duration;
    break;
  case Which::FirstByte:
    O->FirstByteTimeout = Duration;
    break;
  case Which::BetweenBytes:
    O->BetweenBytesTimeout = Duration;
    break;
  }
  return Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>(
      Runtime::Component::WitUnit{});
}

Expect<void> ResponseOutparamSet::body(
    Runtime::Component::CallingFrame &,
    Runtime::Component::Own<ResponseOutparam> Param,
    HttpResult<Runtime::Component::Own<OutgoingResponse>> Response) {
  auto P = Http.Outparams.get(Param.Rep);
  if (P) {
    if (Response) {
      if (auto R = Http.OutgoingResponses.remove(Response->Rep)) {
        P->Response = HttpResult<OutgoingResponse>(std::move(*R));
      } else {
        P->Response =
            fail<OutgoingResponse>(HttpError::internal("no response"));
      }
    } else {
      P->Response = fail<OutgoingResponse>(std::move(Response.error()));
    }
  }
  return {};
}

Expect<uint16_t>
IncomingResponseStatus::body(Runtime::Component::CallingFrame &,
                             IncomingResponseBorrow Self) {
  auto R = Http.IncomingResponses.get(Self.Rep);
  return R ? R->Status : uint16_t(0);
}

Expect<Runtime::Component::Own<Fields>>
IncomingResponseHeaders::body(Runtime::Component::CallingFrame &,
                              IncomingResponseBorrow Self) {
  auto R = Http.IncomingResponses.get(Self.Rep);
  return Runtime::Component::Own<Fields>{
      Http.FieldsTable.add(Fields::immutableCopy(R ? &R->Headers : nullptr))};
}

Expect<Expected<Runtime::Component::Own<IncomingBody>,
                Runtime::Component::WitUnit>>
IncomingResponseConsume::body(Runtime::Component::CallingFrame &,
                              IncomingResponseBorrow Self) {
  auto R = Http.IncomingResponses.get(Self.Rep);
  if (!R || !R->Body) {
    return WasiComponent::failUnit<Runtime::Component::Own<IncomingBody>>();
  }
  return Expected<Runtime::Component::Own<IncomingBody>,
                  Runtime::Component::WitUnit>(
      Runtime::Component::Own<IncomingBody>{
          Http.IncomingBodies.add(std::move(R->Body))});
}

Expect<
    Expected<Runtime::Component::Own<InputStream>, Runtime::Component::WitUnit>>
IncomingBodyStream::body(Runtime::Component::CallingFrame &,
                         IncomingBodyBorrow Self) {
  auto B = Http.IncomingBodies.get(Self.Rep);
  if (!B || B->StreamTaken) {
    return WasiComponent::failUnit<Runtime::Component::Own<InputStream>>();
  }
  B->StreamTaken = true;
  return Expected<Runtime::Component::Own<InputStream>,
                  Runtime::Component::WitUnit>(
      Runtime::Component::Own<InputStream>{Http.Io.inputs().add(
          std::make_unique<BodyInputStream>(Http, Self.Rep))});
}

Expect<Runtime::Component::Own<FutureTrailers>>
IncomingBodyFinish::body(Runtime::Component::CallingFrame &,
                         Runtime::Component::Own<IncomingBody> This) {
  auto F = std::make_unique<FutureTrailers>();
  if (auto Body = Http.IncomingBodies.remove(This.Rep)) {
    F->Trailers = std::move(Body->Trailers);
  }
  return Runtime::Component::Own<FutureTrailers>{
      Http.Trailers.add(std::move(F))};
}

Expect<Runtime::Component::Own<PollSource>>
FutureTrailersSubscribe::body(Runtime::Component::CallingFrame &,
                              TrailersBorrow) {
  return Runtime::Component::Own<PollSource>{
      Http.Io.pollables().add(std::make_unique<AlwaysReady>())};
}

Expect<FutureTrailersGet::Result>
FutureTrailersGet::body(Runtime::Component::CallingFrame &,
                        TrailersBorrow Self) {
  auto F = Http.Trailers.get(Self.Rep);
  if (!F) {
    return Result{};
  }
  if (F->Taken) {
    return Result{
        Expected<HttpResult<std::optional<Runtime::Component::Own<Fields>>>,
                 Runtime::Component::WitUnit>(
            Unexpected<Runtime::Component::WitUnit>(
                Runtime::Component::WitUnit{}))};
  }
  F->Taken = true;
  std::optional<Runtime::Component::Own<Fields>> Trailers;
  if (F->Trailers) {
    Trailers = Runtime::Component::Own<Fields>{Http.FieldsTable.add(
        std::make_unique<Fields>(std::move(*F->Trailers)))};
    F->Trailers.reset();
  }
  return Result{
      Expected<HttpResult<std::optional<Runtime::Component::Own<Fields>>>,
               Runtime::Component::WitUnit>(
          HttpResult<std::optional<Runtime::Component::Own<Fields>>>(
              std::move(Trailers)))};
}

Expect<Runtime::Component::Own<OutgoingResponse>>
OutgoingResponseNew::body(Runtime::Component::CallingFrame &,
                          Runtime::Component::Own<Fields> Headers) {
  auto R = std::make_unique<OutgoingResponse>();
  if (auto Taken = Http.FieldsTable.remove(Headers.Rep)) {
    R->Headers = std::move(*Taken);
  }
  R->Headers.Immutable = true;
  return Runtime::Component::Own<OutgoingResponse>{
      Http.OutgoingResponses.add(std::move(R))};
}

Expect<uint16_t>
OutgoingResponseStatus::body(Runtime::Component::CallingFrame &,
                             OutgoingResponseBorrow Self) {
  auto R = Http.OutgoingResponses.get(Self.Rep);
  return R ? R->Status : uint16_t(0);
}

Expect<Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>>
OutgoingResponseSetStatus::body(Runtime::Component::CallingFrame &,
                                OutgoingResponseBorrow Self, uint16_t Status) {
  auto R = Http.OutgoingResponses.get(Self.Rep);
  if (!R || Status < 100 || Status > 999) {
    return WasiComponent::failUnit<Runtime::Component::WitUnit>();
  }
  R->Status = Status;
  return Expected<Runtime::Component::WitUnit, Runtime::Component::WitUnit>(
      Runtime::Component::WitUnit{});
}

Expect<Runtime::Component::Own<Fields>>
OutgoingResponseHeaders::body(Runtime::Component::CallingFrame &,
                              OutgoingResponseBorrow Self) {
  auto R = Http.OutgoingResponses.get(Self.Rep);
  return Runtime::Component::Own<Fields>{
      Http.FieldsTable.add(Fields::immutableCopy(R ? &R->Headers : nullptr))};
}

Expect<Expected<Runtime::Component::Own<OutgoingBody>,
                Runtime::Component::WitUnit>>
OutgoingResponseBody::body(Runtime::Component::CallingFrame &,
                           OutgoingResponseBorrow Self) {
  auto R = Http.OutgoingResponses.get(Self.Rep);
  if (!R || R->BodyRep != 0) {
    return WasiComponent::failUnit<Runtime::Component::Own<OutgoingBody>>();
  }
  R->BodyRep = Http.OutgoingBodies.add(std::make_unique<OutgoingBody>());
  return Expected<Runtime::Component::Own<OutgoingBody>,
                  Runtime::Component::WitUnit>(
      Runtime::Component::Own<OutgoingBody>{R->BodyRep});
}

Expect<Expected<Runtime::Component::Own<OutputStream>,
                Runtime::Component::WitUnit>>
OutgoingBodyWrite::body(Runtime::Component::CallingFrame &,
                        OutgoingBodyBorrow Self) {
  auto B = Http.OutgoingBodies.get(Self.Rep);
  if (!B || B->StreamTaken || B->Finished) {
    return WasiComponent::failUnit<Runtime::Component::Own<OutputStream>>();
  }
  B->StreamTaken = true;
  return Expected<Runtime::Component::Own<OutputStream>,
                  Runtime::Component::WitUnit>(
      Runtime::Component::Own<OutputStream>{Http.Io.outputs().add(
          std::make_unique<BodyOutputStream>(Http, Self.Rep))});
}

Expect<HttpStatus> OutgoingBodyFinish::body(
    Runtime::Component::CallingFrame &,
    Runtime::Component::Own<OutgoingBody> This,
    std::optional<Runtime::Component::Own<Fields>> Trailers) {
  auto B = Http.OutgoingBodies.get(This.Rep);
  std::optional<Fields> T;
  if (Trailers) {
    if (auto Taken = Http.FieldsTable.remove(Trailers->Rep)) {
      T = std::move(*Taken);
      T->Immutable = true;
    }
  }
  if (!B) {
    return HttpStatus(
        Unexpected<HttpError>(HttpError::internal("no such body")));
  }
  if (B->Finished) {
    return HttpStatus(
        Unexpected<HttpError>(HttpError::internal("body already finished")));
  }
  B->Finished = true;
  B->Trailers = std::move(T);
  return HttpStatus(Runtime::Component::WitUnit{});
}

Expect<Runtime::Component::Own<PollSource>>
FutureResponseSubscribe::body(Runtime::Component::CallingFrame &,
                              FutureResponseBorrow Self) {
  return Runtime::Component::Own<PollSource>{Http.Io.pollables().add(
      std::make_unique<FutureResponseSource>(Http, Self.Rep))};
}

Expect<FutureResponseGet::Result>
FutureResponseGet::body(Runtime::Component::CallingFrame &,
                        FutureResponseBorrow Self) {
  auto F = Http.FutureResponses.get(Self.Rep);
  if (!F) {
    return Result{};
  }
  if (F->Taken) {
    return Result{
        Expected<HttpResult<Runtime::Component::Own<IncomingResponse>>,
                 Runtime::Component::WitUnit>(
            Unexpected<Runtime::Component::WitUnit>(
                Runtime::Component::WitUnit{}))};
  }
  if (!F->Result.has_value()) {
    return Result{};
  }
  auto &Res = *F->Result;
  F->Taken = true;
  if (!Res) {
    return Result{
        Expected<HttpResult<Runtime::Component::Own<IncomingResponse>>,
                 Runtime::Component::WitUnit>(
            HttpResult<Runtime::Component::Own<IncomingResponse>>(
                Unexpected<HttpError>(Res.error())))};
  }
  return Result{Expected<HttpResult<Runtime::Component::Own<IncomingResponse>>,
                         Runtime::Component::WitUnit>(
      HttpResult<Runtime::Component::Own<IncomingResponse>>(
          Runtime::Component::Own<IncomingResponse>{
              Http.IncomingResponses.add(std::move(*Res))}))};
}

Expect<std::optional<HttpError>>
HttpErrorCode::body(Runtime::Component::CallingFrame &, ErrorBorrow Err) {
  auto E = Http.Io.errors().get(Err.Rep);
  if (!E) {
    return std::optional<HttpError>{};
  }
  switch (E->Errno) {
  case __WASI_ERRNO_CONNRESET:
  case __WASI_ERRNO_PIPE:
    return std::optional<HttpError>{
        HttpError::of(HttpError::ConnectionTerminated)};
  case __WASI_ERRNO_TIMEDOUT:
    return std::optional<HttpError>{
        HttpError::of(HttpError::ConnectionReadTimeout)};
  default:
    return std::optional<HttpError>{};
  }
}

Expect<HttpResult<Runtime::Component::Own<FutureIncomingResponse>>>
OutgoingHandle::body(
    Runtime::Component::CallingFrame &Frame,
    Runtime::Component::Own<OutgoingRequest> Request,
    std::optional<Runtime::Component::Own<RequestOptions>> Options) {
  auto R = Http.OutgoingRequests.remove(Request.Rep);
  RequestOptions O;
  if (Options) {
    if (auto Opt = Http.Options.remove(Options->Rep)) {
      O = *Opt;
    }
  }
  if (!R) {
    return HttpResult<Runtime::Component::Own<FutureIncomingResponse>>(
        fail<Runtime::Component::Own<FutureIncomingResponse>>(
            HttpError::internal("no such request")));
  }
  if (!R->Authority) {
    Http.OutgoingBodies.remove(R->BodyRep);
    return HttpResult<Runtime::Component::Own<FutureIncomingResponse>>(
        fail<Runtime::Component::Own<FutureIncomingResponse>>(
            HttpError::of(HttpError::HttpRequestUriInvalid)));
  }
  if (R->S && R->S->Case == Scheme::Https) {
    Http.OutgoingBodies.remove(R->BodyRep);
    return HttpResult<Runtime::Component::Own<FutureIncomingResponse>>(
        fail<Runtime::Component::Own<FutureIncomingResponse>>(
            HttpError::internal("TLS is not supported")));
  }
  auto F = std::make_unique<FutureIncomingResponse>();
  F->Request = std::move(*R);
  F->Options = O;
  const uint64_t Rep = Http.FutureResponses.add(std::move(F));
  Frame.spawn([&Host = Http, Rep](Runtime::Component::CallingFrame &Spawned) {
    return Host.runExchange(Spawned, Rep);
  });
  return HttpResult<Runtime::Component::Own<FutureIncomingResponse>>(
      Runtime::Component::Own<FutureIncomingResponse>{Rep});
}

HttpTypesInstance::HttpTypesInstance(HttpHost &Http)
    : ComponentInstance("wasi:http/types@0.2.12") {
  const auto Mint = getTypeMinter();
  exportType("duration", Mint.primDefType(PrimValType::U64).getTypeIndex());
  exportType("input-stream",
             addSharedResourceType<InputStream>(Http.Io.InputType));
  exportType("output-stream",
             addSharedResourceType<OutputStream>(Http.Io.OutputType));
  exportType("io-error", addSharedResourceType<IoError>(Http.Io.ErrorType));
  exportType("pollable",
             addSharedResourceType<PollSource>(Http.Io.PollableType));
  exportType("method",
             Runtime::Component::Wit<Method>::type(Mint).getTypeIndex());
  exportType("scheme",
             Runtime::Component::Wit<Scheme>::type(Mint).getTypeIndex());
  exportType(
      "DNS-error-payload",
      Runtime::Component::Wit<DnsErrorPayload>::type(Mint).getTypeIndex());
  exportType("TLS-alert-received-payload",
             Runtime::Component::Wit<TlsAlertReceivedPayload>::type(Mint)
                 .getTypeIndex());
  exportType(
      "field-size-payload",
      Runtime::Component::Wit<FieldSizePayload>::type(Mint).getTypeIndex());
  exportType("error-code",
             Runtime::Component::Wit<HttpError>::type(Mint).getTypeIndex());
  exportType("header-error",
             Runtime::Component::Wit<HeaderError>::type(Mint).getTypeIndex());
  const uint32_t Str = Mint.primDefType(PrimValType::String).getTypeIndex();
  exportType("field-key", Str);
  exportType("field-name", Str);
  exportType(
      "field-value",
      Runtime::Component::Wit<std::vector<uint8_t>>::type(Mint).getTypeIndex());
  const uint32_t FieldsIdx = addHostResourceType<Fields>(
      [&Http](uint64_t Rep) { Http.FieldsTable.remove(Rep); });
  exportType("fields", FieldsIdx);
  exportType("headers", FieldsIdx);
  exportType("trailers", FieldsIdx);
  exportType("incoming-request",
             addHostResourceType<IncomingRequest>(
                 [&Http](uint64_t Rep) { Http.IncomingRequests.remove(Rep); }));
  const uint32_t OutReqIdx = addHostResourceType<OutgoingRequest>(
      [&Http](uint64_t Rep) { Http.removeOutgoingRequest(Rep); });
  exportType("outgoing-request", OutReqIdx);
  Http.OutgoingRequestType = *getTypeResource(OutReqIdx);
  const uint32_t OptIdx = addHostResourceType<RequestOptions>(
      [&Http](uint64_t Rep) { Http.Options.remove(Rep); });
  exportType("request-options", OptIdx);
  Http.OptionsType = *getTypeResource(OptIdx);
  exportType("response-outparam",
             addHostResourceType<ResponseOutparam>(
                 [&Http](uint64_t Rep) { Http.Outparams.remove(Rep); }));
  exportType("status-code", Mint.primDefType(PrimValType::U16).getTypeIndex());
  exportType("incoming-response",
             addHostResourceType<IncomingResponse>([&Http](uint64_t Rep) {
               Http.IncomingResponses.remove(Rep);
             }));
  exportType("incoming-body",
             addHostResourceType<IncomingBody>(
                 [&Http](uint64_t Rep) { Http.IncomingBodies.remove(Rep); }));
  exportType("future-trailers",
             addHostResourceType<FutureTrailers>(
                 [&Http](uint64_t Rep) { Http.Trailers.remove(Rep); }));
  exportType("outgoing-response",
             addHostResourceType<OutgoingResponse>(
                 [&Http](uint64_t Rep) { Http.removeOutgoingResponse(Rep); }));
  exportType("outgoing-body",
             addHostResourceType<OutgoingBody>([&Http](uint64_t Rep) {
               if (OutgoingBody *Body = Http.OutgoingBodies.get(Rep)) {
                 Body->Dropped = true;
               }
             }));
  const uint32_t FutureIdx = addHostResourceType<FutureIncomingResponse>(
      [&Http](uint64_t Rep) { Http.removeFutureResponse(Rep); });
  exportType("future-incoming-response", FutureIdx);
  Http.FutureResponseType = *getTypeResource(FutureIdx);

  addHostFunc("[constructor]fields", std::make_unique<FieldsNew>(Http));
  addHostFunc("[static]fields.from-list",
              std::make_unique<FieldsFromList>(Http));
  addHostFunc("[method]fields.get", std::make_unique<FieldsGet>(Http));
  addHostFunc("[method]fields.has", std::make_unique<FieldsHas>(Http));
  addHostFunc("[method]fields.set", std::make_unique<FieldsSet>(Http));
  addHostFunc("[method]fields.delete", std::make_unique<FieldsDelete>(Http));
  addHostFunc("[method]fields.append", std::make_unique<FieldsAppend>(Http));
  addHostFunc("[method]fields.entries", std::make_unique<FieldsEntries>(Http));
  addHostFunc("[method]fields.clone", std::make_unique<FieldsClone>(Http));
  addHostFunc("[method]incoming-request.method",
              std::make_unique<IncomingRequestMethod>(Http));
  addHostFunc("[method]incoming-request.path-with-query",
              std::make_unique<IncomingRequestText>(Http, false));
  addHostFunc("[method]incoming-request.scheme",
              std::make_unique<IncomingRequestScheme>(Http));
  addHostFunc("[method]incoming-request.authority",
              std::make_unique<IncomingRequestText>(Http, true));
  addHostFunc("[method]incoming-request.headers",
              std::make_unique<IncomingRequestHeaders>(Http));
  addHostFunc("[method]incoming-request.consume",
              std::make_unique<IncomingRequestConsume>(Http));
  addHostFunc("[constructor]outgoing-request",
              std::make_unique<OutgoingRequestNew>(Http));
  addHostFunc("[method]outgoing-request.body",
              std::make_unique<OutgoingRequestBody>(Http));
  addHostFunc("[method]outgoing-request.method",
              std::make_unique<OutgoingRequestMethod>(Http));
  addHostFunc("[method]outgoing-request.set-method",
              std::make_unique<OutgoingRequestSetMethod>(Http));
  addHostFunc("[method]outgoing-request.path-with-query",
              std::make_unique<OutgoingRequestText>(Http, false));
  addHostFunc("[method]outgoing-request.set-path-with-query",
              std::make_unique<OutgoingRequestSetText>(Http, false));
  addHostFunc("[method]outgoing-request.scheme",
              std::make_unique<OutgoingRequestScheme>(Http));
  addHostFunc("[method]outgoing-request.set-scheme",
              std::make_unique<OutgoingRequestSetScheme>(Http));
  addHostFunc("[method]outgoing-request.authority",
              std::make_unique<OutgoingRequestText>(Http, true));
  addHostFunc("[method]outgoing-request.set-authority",
              std::make_unique<OutgoingRequestSetText>(Http, true));
  addHostFunc("[method]outgoing-request.headers",
              std::make_unique<OutgoingRequestHeaders>(Http));
  addHostFunc("[constructor]request-options",
              std::make_unique<RequestOptionsNew>(Http));
  addHostFunc("[method]request-options.connect-timeout",
              std::make_unique<RequestOptionsGet>(
                  Http, RequestOptionsGet::Which::Connect));
  addHostFunc("[method]request-options.set-connect-timeout",
              std::make_unique<RequestOptionsSet>(
                  Http, RequestOptionsGet::Which::Connect));
  addHostFunc("[method]request-options.first-byte-timeout",
              std::make_unique<RequestOptionsGet>(
                  Http, RequestOptionsGet::Which::FirstByte));
  addHostFunc("[method]request-options.set-first-byte-timeout",
              std::make_unique<RequestOptionsSet>(
                  Http, RequestOptionsGet::Which::FirstByte));
  addHostFunc("[method]request-options.between-bytes-timeout",
              std::make_unique<RequestOptionsGet>(
                  Http, RequestOptionsGet::Which::BetweenBytes));
  addHostFunc("[method]request-options.set-between-bytes-timeout",
              std::make_unique<RequestOptionsSet>(
                  Http, RequestOptionsGet::Which::BetweenBytes));
  addHostFunc("[static]response-outparam.set",
              std::make_unique<ResponseOutparamSet>(Http));
  addHostFunc("[method]incoming-response.status",
              std::make_unique<IncomingResponseStatus>(Http));
  addHostFunc("[method]incoming-response.headers",
              std::make_unique<IncomingResponseHeaders>(Http));
  addHostFunc("[method]incoming-response.consume",
              std::make_unique<IncomingResponseConsume>(Http));
  addHostFunc("[method]incoming-body.stream",
              std::make_unique<IncomingBodyStream>(Http));
  addHostFunc("[static]incoming-body.finish",
              std::make_unique<IncomingBodyFinish>(Http));
  addHostFunc("[method]future-trailers.subscribe",
              std::make_unique<FutureTrailersSubscribe>(Http));
  addHostFunc("[method]future-trailers.get",
              std::make_unique<FutureTrailersGet>(Http));
  addHostFunc("[constructor]outgoing-response",
              std::make_unique<OutgoingResponseNew>(Http));
  addHostFunc("[method]outgoing-response.status-code",
              std::make_unique<OutgoingResponseStatus>(Http));
  addHostFunc("[method]outgoing-response.set-status-code",
              std::make_unique<OutgoingResponseSetStatus>(Http));
  addHostFunc("[method]outgoing-response.headers",
              std::make_unique<OutgoingResponseHeaders>(Http));
  addHostFunc("[method]outgoing-response.body",
              std::make_unique<OutgoingResponseBody>(Http));
  addHostFunc("[method]outgoing-body.write",
              std::make_unique<OutgoingBodyWrite>(Http));
  addHostFunc("[static]outgoing-body.finish",
              std::make_unique<OutgoingBodyFinish>(Http));
  addHostFunc("[method]future-incoming-response.subscribe",
              std::make_unique<FutureResponseSubscribe>(Http));
  addHostFunc("[method]future-incoming-response.get",
              std::make_unique<FutureResponseGet>(Http));
  addHostFunc("http-error-code", std::make_unique<HttpErrorCode>(Http));
}

OutgoingHandlerInstance::OutgoingHandlerInstance(HttpHost &Http)
    : ComponentInstance("wasi:http/outgoing-handler@0.2.12") {
  exportType("outgoing-request",
             addSharedResourceType<OutgoingRequest>(Http.OutgoingRequestType));
  exportType("request-options",
             addSharedResourceType<RequestOptions>(Http.OptionsType));
  exportType(
      "future-incoming-response",
      addSharedResourceType<FutureIncomingResponse>(Http.FutureResponseType));
  exportType(
      "error-code",
      Runtime::Component::Wit<HttpError>::type(getTypeMinter()).getTypeIndex());
  addHostFunc("handle", std::make_unique<OutgoingHandle>(Http));
}

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
newHttpInstances(HttpHost &Http) {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  Insts.push_back(std::make_unique<HttpTypesInstance>(Http));
  Insts.push_back(std::make_unique<OutgoingHandlerInstance>(Http));
  return Insts;
}

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
