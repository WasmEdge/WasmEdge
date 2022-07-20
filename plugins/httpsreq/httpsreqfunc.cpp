// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "httpsreqfunc.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <iostream>


//Based off of: https://www.cs.utah.edu/~swalton/listings/articles/ssl_client.c
//Some of the code was taken from this post: https://stackoverflow.com/questions/52727565/client-in-c-use-gethostbyname-or-getaddrinfo


namespace WasmEdge {
namespace Host {

// Expect<void> DisplayCerts(SSL *ssl)
// {
//     X509 *cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
//     if (cert != nullptr)
//     {
//         std::printf("Server certificates:\n");
//         char *line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
//         std::printf("Subject: %s\n", line);
//         delete line;
//         line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
//         std::printf("Issuer: %s\n", line);
//         delete line;
//         X509_free(cert);
//     }
//     else{
//         std::printf("Info: No client certificates configured.\n");
//     }
// }

Expect<void> SendData::body
([[maybe_unused]] Runtime::Instance::MemoryInstance *MemInst, uint32_t HostPtr, uint32_t HostLen, uint32_t Port, uint32_t BodyPtr, uint32_t BodyLen) {
    // Check memory instance from module.
    if (MemInst == nullptr) 
    {
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

    const SSL_METHOD *method = TLS_client_method(); 
    SSL_CTX *ctx = SSL_CTX_new(method);

    if (ctx == nullptr)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }


    SSL *ssl = SSL_new(ctx);
    if (ssl == nullptr)
    {
        fprintf(stderr, "SSL_new() failed\n");
        exit(EXIT_FAILURE);
    }
    
    // open connection
    int sfd, err;
    struct addrinfo hints = {}, *addrs;
    char port_str[16] = {};

    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::sprintf(port_str, "%d", Port);

    err = getaddrinfo(Env.Host.c_str(), port_str, &hints, &addrs);
    if (err != 0)
    {
        fprintf(stderr, "%s: %s\n", Env.Host.c_str(), gai_strerror(err));
        abort();
    }

    for(struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next)
    {
        sfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sfd == -1)
        {
            err = errno;
            break; 
        }

        if (connect(sfd, addr->ai_addr, addr->ai_addrlen) == 0)
            break;
        err = errno;
        close(sfd);
        sfd = -1;
    }

    freeaddrinfo(addrs);

    if (sfd == -1)
    {
        fprintf(stderr, "%s: %s\n", Env.Host.c_str(), strerror(err));
        abort();
    }

    SSL_set_fd(ssl, sfd);

    const int status = SSL_connect(ssl);
    if (status != 1)
    {
        SSL_get_error(ssl, status);
        ERR_print_errors_fp(stderr); 
        fprintf(stderr, "SSL_connect failed with SSL_get_error code %d\n", status);
        exit(EXIT_FAILURE);
    }

    printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
    // DisplayCerts(ssl);
    const char *chars = Env.BodyStr.c_str();
    SSL_write(ssl, chars, strlen(chars));


    // Receive
    char buffer[2048];
    int nbytess=0;
    while(true)
    {
        nbytess = SSL_read(ssl, buffer, 2048);
        if(nbytess<=0)
        {
            break;
        }
        else{
            printf("ssl_read,byte=%d\n",nbytess);
            printf("rcv:%s\n",buffer);
        }
    }

    SSL_free(ssl);
    close(sfd);
    SSL_CTX_free(ctx);

    return {};
}

} // namespace Host
} // namespace WasmEdge