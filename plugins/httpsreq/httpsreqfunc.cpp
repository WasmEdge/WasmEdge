// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "httpsreqfunc.h"
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

Expect<void> SendData::body(Runtime::Instance::MemoryInstance *MemInst,
                            uint32_t HostPtr, uint32_t HostLen, uint32_t Port,
                            uint32_t BodyPtr, uint32_t BodyLen) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  char *Host = MemInst->getPointer<char *>(HostPtr);
  std::string NewHost;
  std::copy_n(Host, HostLen, std::back_inserter(NewHost));
  Env.Host = std::move(NewHost);

  Env.Port = Port;

  char *BodyStr = MemInst->getPointer<char *>(BodyPtr);
  std::string NewBodyStr;
  std::copy_n(BodyStr, BodyLen, std::back_inserter(NewBodyStr));
  Env.BodyStr = std::move(NewBodyStr);

  const SSL_METHOD *Method = TLS_client_method();
  SSL_CTX *Ctx = SSL_CTX_new(Method);

  if (Ctx == nullptr) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  SSL *Ssl = SSL_new(Ctx);
  if (Ssl == nullptr) {
    fprintf(stderr, "SSL_new() failed\n");
    exit(EXIT_FAILURE);
  }

  // open connection
  int Sfd, Err;
  struct addrinfo Hints = {}, *Addrs;
  char PortStr[16] = {};

  Hints.ai_family = AF_INET;
  Hints.ai_socktype = SOCK_STREAM;
  Hints.ai_protocol = IPPROTO_TCP;

  std::sprintf(PortStr, "%d", Port);

  Err = getaddrinfo(Env.Host.c_str(), PortStr, &Hints, &Addrs);
  if (Err != 0) {
    fprintf(stderr, "%s: %s\n", Env.Host.c_str(), gai_strerror(Err));
    abort();
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
    fprintf(stderr, "%s: %s\n", Env.Host.c_str(), strerror(Err));
    abort();
  }

  SSL_set_fd(Ssl, Sfd);

  const int Status = SSL_connect(Ssl);
  if (Status != 1) {
    SSL_get_error(Ssl, Status);
    ERR_print_errors_fp(stderr);
    fprintf(stderr, "SSL_connect failed with SSL_get_error code %d\n", Status);
    exit(EXIT_FAILURE);
  }

  printf("Connected with %s encryption\n", SSL_get_cipher(Ssl));

  SSL_write(Ssl, BodyStr, strlen(Env.BodyStr.c_str()));

  // Receive
  char Buffer[2048];
  int Nbytes = 0;
  while (true) {
    Nbytes = SSL_read(Ssl, Buffer, 2048);
    if (Nbytes <= 0) {
      break;
    }
    printf("SSL_Read,byte=%d\n", Nbytes);
    printf("rcv:%s\n", Buffer);
  }

  SSL_free(Ssl);
  close(Sfd);
  SSL_CTX_free(Ctx);

  return {};
}

} // namespace Host
} // namespace WasmEdge