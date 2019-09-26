#include "vm/result.h"
#include "vm/vm.h"

int main(int Argc, char *Argv[]) {
  std::string InputPath(Argv[1]);
  SSVM::VM::VM VM;
  SSVM::Result Result;
  VM.setPath(InputPath);
  VM.appendArgument(0U);
  VM.execute();
  Result = VM.getResult();
  return 0;
}
