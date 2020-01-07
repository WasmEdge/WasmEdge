// SPDX-License-Identifier: Apache-2.0
#include "easyloggingpp/easylogging++.h"
#include "proxy/cmdparser.h"
#include "proxy/proxy.h"
#include "vm/configure.h"
#include "vm/result.h"
#include "vm/vm.h"

#include <cstdio>
#include <cstring>

INITIALIZE_EASYLOGGINGPP

int main(int Argc, char *Argv[]) {
  START_EASYLOGGINGPP(Argc, Argv);
  SSVM::Proxy::CmdParser Cmd;
  Cmd.parseCommandLine(Argc, Argv);

  SSVM::Proxy::Proxy VMProxy;
  VMProxy.setInputJSONPath(Cmd.getInputJSONPath());
  VMProxy.setOutputJSONPath(Cmd.getOutputJSONPath());
  VMProxy.setWasmPath(Cmd.getWasmPath());
  VMProxy.runRequest();

  return 0;
}
