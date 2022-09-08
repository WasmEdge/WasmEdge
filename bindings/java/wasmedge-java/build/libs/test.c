#include <wasmedge/wasmedge.h>
#include <stdio.h>
int main() {
  printf("WasmEdge version: %s\n", WasmEdge_VersionGet());
  return 0;
}
