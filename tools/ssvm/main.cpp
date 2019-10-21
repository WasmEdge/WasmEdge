#include "vm/configure.h"
#include "vm/result.h"
#include "vm/vm.h"

int main(int Argc, char *Argv[]) {
  std::string InputPath("ethereum/erc20.wasm");
  SSVM::VM::Configure Conf(SSVM::VM::Configure::VMType::Ewasm);
  SSVM::VM::VM VM(Conf);
  SSVM::Result Result;
  VM.setPath(InputPath);
  VM.execute();
  Result = VM.getResult();
  return 0;
}
