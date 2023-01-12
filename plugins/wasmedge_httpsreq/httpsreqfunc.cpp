// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "httpsreqfunc.h"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <algorithm>
#include <cstdio>
#include <errno.h>
#include <netdb.h>
#include <resolv.h>
#include <string.h>
#include <string>
#include <unistd.h>

namespace WasmEdge {
namespace Host {

Expect<void> WasmEdgeHttpsReqSendData::body(const Runtime::CallingFrame &Frame,
                                            uint32_t HostPtr, uint32_t HostLen,
                                            uint32_t Port, uint32_t BodyPtr,
                                            uint32_t BodyLen) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const char *Host = MemInst->getPointer<const char *>(HostPtr);
  const char *Body = MemInst->getPointer<const char *>(BodyPtr);
  if (Host == nullptr) {
    spdlog::error("[WasmEdge Httpsreq] Fail to get Host");
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  if (Body == nullptr) {
    spdlog::error("[WasmEdge Httpsreq] Fail to get Body");
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  std::string HostStr, BodyStr, PortStr = std::to_string(Port);
  std::copy_n(Host, HostLen, std::back_inserter(HostStr));
  std::copy_n(Body, BodyLen, std::back_inserter(BodyStr));

  const SSL_METHOD *Method = TLS_client_method();
  SSL_CTX *Ctx = SSL_CTX_new(Method);
  if (Ctx == nullptr) {
    ERR_print_errors_fp(stderr);
    spdlog::error("[WasmEdge Httpsreq] SSL_CTX_new() failed");
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  SSL *Ssl = SSL_new(Ctx);
  if (Ssl == nullptr) {
    spdlog::error("[WasmEdge Httpsreq] SSL_new() failed");
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // open connection
  int Sfd, Err;
  struct addrinfo Hints = {}, *Addrs;

  Hints.ai_family = AF_INET;
  Hints.ai_socktype = SOCK_STREAM;
  Hints.ai_protocol = IPPROTO_TCP;

  Err = getaddrinfo(HostStr.c_str(), PortStr.c_str(), &Hints, &Addrs);
  if (Err != 0) {
    spdlog::error("[WasmEdge Httpsreq] {}", gai_strerror(Err));
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  for (struct addrinfo *Addr = Addrs; Addr != NULL; Addr = Addr->ai_next) {
    Sfd = socket(Addr->ai_family, Addr->ai_socktype, Addr->ai_protocol);
    if (Sfd == -1) {
      Err = errno;
      break;
    }
    if (connect(Sfd, Addr->ai_addr, Addr->ai_addrlen) == 0)
      break;
    Err = errno;
    close(Sfd);
    Sfd = -1;
  }

  freeaddrinfo(Addrs);

  if (Sfd == -1) {
    spdlog::error("[WasmEdge Httpsreq] {}", strerror(Err));
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  SSL_set_fd(Ssl, Sfd);

  const int Status = SSL_connect(Ssl);
  if (Status != 1) {
    SSL_get_error(Ssl, Status);
    ERR_print_errors_fp(stderr);
    spdlog::error("[WasmEdge Httpsreq] SSL_get_error code {}", Status);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  SSL_write(Ssl, BodyStr.c_str(), BodyLen);

  // Receive
  char Buffer[1024];
  int Nbytes = 0;
  Env.Rcv = "";
  while (true) {
    Nbytes = SSL_read(Ssl, Buffer, 1024);
    if (Nbytes <= 0) {
      break;
    }
    std::string Buf(Buffer, Nbytes);
    Env.Rcv = Env.Rcv + Buf;
  }

  SSL_free(Ssl);
  close(Sfd);
  SSL_CTX_free(Ctx);

  return {};
}

Expect<void> WasmEdgeHttpsReqGetRcv::body(const Runtime::CallingFrame &Frame,
                                          uint32_t BufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  char *Buf = MemInst->getPointer<char *>(BufPtr);
  std::copy_n(Env.Rcv.begin(), Env.Rcv.size(), Buf);
  return {};
}

Expect<uint32_t>
WasmEdgeHttpsReqGetRcvLen::body(const Runtime::CallingFrame &) {
  return static_cast<uint32_t>(Env.Rcv.size());
}

} // namespace Host
} // namespace WasmEdge
