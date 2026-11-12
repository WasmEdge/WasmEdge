#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "wrapper.h"

int main() {
  const char *ServerSockPath = "/tmp/WasmEdge.socket";
  const char *ClientSockPath = "/tmp/WasmEdge.Client.socket";
  int SockFd, ServerFd;
  socklen_t Size, ClientSize;
  sockaddr_un ClientAddr{};
  sockaddr_un ServerAddr{};

  if (SockFd = WasmedgeSocket(AF_UNIX, SOCK_STREAM, 0); SockFd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }

  ClientAddr.sun_family = AF_UNIX;
  strcpy(ClientAddr.sun_path, ClientSockPath);
  Size = offsetof(sockaddr_un, sun_path) + strlen(ClientAddr.sun_path);

  // unlink(ClientSockPath);
  if (WasmedgeBind(SockFd, reinterpret_cast<sockaddr *>(&ClientAddr), Size) <
      0) {
    perror("ERROR bind");
    exit(1);
  }

  ServerAddr.sun_family = AF_UNIX;
  strcpy(ServerAddr.sun_path, ServerSockPath);
  Size = offsetof(sockaddr_un, sun_path) + strlen(ServerAddr.sun_path);

  if (WasmedgeConnect(SockFd, reinterpret_cast<sockaddr *>(&ServerAddr), Size) <
      0) {
    perror("ERROR connect");
    exit(1);
  }

  constexpr int BufMaxSize = 256;
  char Buf[BufMaxSize + 1];

  while (fgets(Buf, 256, stdin)) {
    auto End = strchr(Buf, '\n');
    if (*End)
      *End = '\0';

    write(SockFd, Buf, strlen(Buf));

    if (int Len = read(SockFd, Buf, BufMaxSize); Len >= 0) {
      Buf[Len] = 0;
      printf("Server: %s\n", Buf);
    } else {
      printf("Server closed...");
    }
  }

  close(SockFd);
}
