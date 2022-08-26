// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "httpsreqfunc.h"
#include "common/errcode.h"
#include "common/log.h"
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <resolv.h>
#include <string.h>
#include <unistd.h>

// Some of the code was taken from this post:
// https://stackoverflow.com/questions/52727565/client-in-c-use-gethostbyname-or-getaddrinfo

namespace WasmEdge {
namespace Host {

Expect<void> WasmEdgeHttpsReqSendData::body(
    Runtime::Instance::MemoryInstance *MemInst, uint32_t HostPtr,
    uint32_t HostLen, uint32_t Port, uint32_t BodyPtr, uint32_t BodyLen) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  char *Host = MemInst->getPointer<char *>(HostPtr);
  char *Body = MemInst->getPointer<char *>(BodyPtr);
  if (Host == nullptr) {
    spdlog::error("[Wasmedge Httpsreq] Fail to get Host");
    return Unexpect(ErrCode::ExecutionFailed);
  }
  if (Body == nullptr) {
    spdlog::error("[Wasmedge Httpsreq] Fail to get Body");
    return Unexpect(ErrCode::ExecutionFailed);
  }
  std::string HostStr, BodyStr, PortStr = std::to_string(Port);
  std::copy_n(Host, HostLen, std::back_inserter(HostStr));
  std::copy_n(Body, BodyLen, std::back_inserter(BodyStr));

  const SSL_METHOD *Method = TLS_client_method();
  SSL_CTX *Ctx = SSL_CTX_new(Method);
  if (Ctx == nullptr) {
    ERR_print_errors_fp(stderr);
    spdlog::error("[Wasmedge Httpsreq] SSL_CTX_new() failed");
    return Unexpect(ErrCode::ExecutionFailed);
  }
  SSL *Ssl = SSL_new(Ctx);
  if (Ssl == nullptr) {
    spdlog::error("[Wasmedge Httpsreq] SSL_new() failed");
    return Unexpect(ErrCode::ExecutionFailed);
  }

  // open connection
  int Sfd, Err;
  struct addrinfo Hints = {}, *Addrs;

  Hints.ai_family = AF_INET;
  Hints.ai_socktype = SOCK_STREAM;
  Hints.ai_protocol = IPPROTO_TCP;

  Err = getaddrinfo(HostStr.c_str(), PortStr.c_str(), &Hints, &Addrs);
  if (Err != 0) {
    spdlog::error("[Wasmedge Httpsreq] gai_strerror(Err)");
    return Unexpect(ErrCode::ExecutionFailed);
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
    spdlog::error("[Wasmedge Httpsreq] Fail to get fd");
    return Unexpect(ErrCode::ExecutionFailed);
  }

  SSL_set_fd(Ssl, Sfd);

  const int Status = SSL_connect(Ssl);
  if (Status != 1) {
    SSL_get_error(Ssl, Status);
    ERR_print_errors_fp(stderr);
    spdlog::error("[Wasmedge Httpsreq] SSL_get_error code" +
                  std::to_string(Status));
    return Unexpect(ErrCode::ExecutionFailed);
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

Expect<void>
WasmEdgeHttpsReqGetRcv::body(Runtime::Instance::MemoryInstance *MemInst,
                             uint32_t BufPtr) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }
  char *Buf = MemInst->getPointer<char *>(BufPtr);
  std::copy_n(Env.Rcv.begin(), Env.Rcv.size(), Buf);
  return {};
}

Expect<uint32_t>
WasmEdgeHttpsReqGetRcvLen::body(Runtime::Instance::MemoryInstance *) {
  return static_cast<uint32_t>(Env.Rcv.size());
}

} // namespace Host
} // namespace WasmEdge