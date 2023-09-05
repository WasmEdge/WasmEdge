#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "wrapper.h"

int main() {
  const char *SockPath = "/tmp/WasmEdge.socket";
  int SockFd, ClientFd;
  socklen_t Size, ClientSize;
  sockaddr_un Addr{};
  sockaddr_un ClientAddr{};

  if (SockFd = WasmedgeSocket(AF_UNIX, SOCK_STREAM, 0); SockFd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }

  Addr.sun_family = AF_UNIX;
  strcpy(Addr.sun_path, SockPath);
  Size = offsetof(sockaddr_un, sun_path) + strlen(Addr.sun_path);

  // unlink(SockPath);
  if (WasmedgeBind(SockFd, reinterpret_cast<sockaddr *>(&Addr), Size) < 0) {
    perror("ERROR bind");
    exit(1);
  }

  if (WasmedgeListen(SockFd, 10) < 0) {
    perror("ERROR listen");
    exit(1);
  }

  ClientSize = sizeof(ClientAddr);
  if (ClientFd = WasmedgeAccept(
          SockFd, reinterpret_cast<sockaddr *>(&ClientAddr), &ClientSize);
      ClientFd < 0) {
    perror("ERROR accept");
    exit(1);
  }

  constexpr int BufMaxSize = 256;
  char Buf[BufMaxSize + 1];

  while (true) {
    if (int Len = read(ClientFd, Buf, BufMaxSize); Len >= 0) {
      if (Len == 0)
        break;

      Buf[Len] = 0;
      printf("Client: %s\n", Buf);

      // reverse the string
      std::reverse(Buf, Buf + Len);
      write(ClientFd, Buf, strlen(Buf));
    } else {
      perror("ERROR read");
      exit(1);
    }
  }

  printf("Server Closed\n");
  close(ClientFd);
  close(SockFd);
  return 0;
}
