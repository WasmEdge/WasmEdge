#include <wasmedge/wasmedge.h>

#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>
#include <vector>
using namespace std;

template <class result_t = std::chrono::nanoseconds,
          class clock_t = std::chrono::steady_clock,
          class duration_t = std::chrono::nanoseconds>
auto since(std::chrono::time_point<clock_t, duration_t> const &Start) {
  return std::chrono::duration_cast<result_t>(clock_t::now() - Start);
}

const int WIDTH = 1200;
const int HEIGHT = 800;

int main(int argc, char **argv) {
  /* Create the VM context. */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddProposal(ConfCxt, WasmEdge_Proposal_MultiMemories);
  WasmEdge_ConfigureRemoveProposal(ConfCxt, WasmEdge_Proposal_ReferenceTypes);
  WasmEdge_ConfigureAddProposal(ConfCxt, WasmEdge_Proposal_Threads);
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);

  WasmEdge_Result Res;

  WasmEdge_String ExportName = WasmEdge_StringCreateByCString("env");
  WasmEdge_ModuleInstanceContext *HostModCxt =
      WasmEdge_ModuleInstanceCreate(ExportName);
  WasmEdge_Limit MemLimit = {
      .HasMax = true, .Shared = true, .Min = 60, .Max = 60};
  WasmEdge_MemoryTypeContext *MemTypeCxt = WasmEdge_MemoryTypeCreate(MemLimit);
  WasmEdge_MemoryInstanceContext *HostMemory =
      WasmEdge_MemoryInstanceCreate(MemTypeCxt);
  WasmEdge_String MemoryName = WasmEdge_StringCreateByCString("memory");
  WasmEdge_ModuleInstanceAddMemory(HostModCxt, MemoryName, HostMemory);
  WasmEdge_StringDelete(MemoryName);
  WasmEdge_VMRegisterModuleFromImport(VMCxt, HostModCxt);

  const double X = -0.743644786;
  const double Y = 0.1318252536;
  const double D = 0.00029336;
  int MaxIterations = 10000;
  WasmEdge_String ModName = WasmEdge_StringCreateByCString("mandelbrot");
  Res = WasmEdge_VMRegisterModuleFromFile(VMCxt, ModName, "./mandelbrot.so");
  if (!WasmEdge_ResultOK(Res)) {
    cout << "Registration failed:" << WasmEdge_ResultGetMessage(Res) << "\n";
    return -1;
  }

  /* The parameters and returns arrays. */

  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("mandelbrotThread");

  auto Start = std::chrono::steady_clock::now();
  int NumThreads;
  if (argc > 1) {
    NumThreads = atoi(argv[1]);
  } else {
    NumThreads = 4;
  }
  cout << "Number of Threads: " << NumThreads << "\n";

  std::vector<std::thread> Threads;
  for (int Tid = 0; Tid < NumThreads; ++Tid) {
    Threads.push_back(std::thread(
        [&](int Rank) {
          WasmEdge_Value Params[6] = {
              WasmEdge_ValueGenI32(MaxIterations),
              WasmEdge_ValueGenI32(NumThreads),
              WasmEdge_ValueGenI32(Rank),
              WasmEdge_ValueGenF64(X),
              WasmEdge_ValueGenF64(Y),
              WasmEdge_ValueGenF64(D),
          };
          Res = WasmEdge_VMExecuteRegistered(VMCxt, ModName, FuncName, Params,
                                             6, NULL, 0);
          if (WasmEdge_ResultOK(Res)) {
            cout << "Got the result: ok\n";
          } else {
            cout << "Error message: " << WasmEdge_ResultGetMessage(Res) << "\n";
          }
        },
        Tid));
  }
  for (auto &Thread : Threads) {
    Thread.join();
  }
  cout << "Elapsed Time: " << since(Start).count() / 1e6 << "\n";

  FuncName = WasmEdge_StringCreateByCString("getImage");
  WasmEdge_Value Returns[1];
  Res = WasmEdge_VMExecuteRegistered(VMCxt, ModName, FuncName, NULL, 0, Returns,
                                     1);
  int64_t Offset = (int64_t)WasmEdge_ValueGetExternRef(Returns[0]);
  cout << "Offset: " << Offset << "\n";

  unsigned char Image[WIDTH * HEIGHT * 4];
  Res = WasmEdge_MemoryInstanceGetData(HostMemory, Image, Offset,
                                       WIDTH * HEIGHT * 4);
  if (WasmEdge_ResultOK(Res)) {
    cout << "Get memory: ok\n";
  } else {
    cout << "Error message: " << WasmEdge_ResultGetMessage(Res) << "\n";
    return -1;
  }

  FILE *File = fopen("output-wasmedge.bin", "wb");
  fwrite(Image, sizeof(char), sizeof(Image), File);
  fclose(File);

  /* Resources deallocations. */
  WasmEdge_ModuleInstanceDelete(HostModCxt);
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_StringDelete(ModName);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_MemoryTypeDelete(MemTypeCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  return 0;
}
