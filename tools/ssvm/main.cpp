#include "vm/vm.h"
#include "vm/result.h"

int main(int Argc, char* Argv[]) {
  std::string InputPath(Argv[1]);
  SSVM::VM::VM VM;
  SSVM::Result Result;
  VM.setPath(InputPath);
  VM.setInput(std::vector<uint8_t>());
  VM.execute(Result);
  return 0;
}
