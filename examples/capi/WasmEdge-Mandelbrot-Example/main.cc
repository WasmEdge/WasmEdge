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
auto since(std::chrono::time_point<clock_t, duration_t> const &start) {
  return std::chrono::duration_cast<result_t>(clock_t::now() - start);
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

  double x = -0.743644786;
  double y = 0.1318252536;
  double d = 0.00029336;
  int maxIterations = 10000;
  WasmEdge_String ModName = WasmEdge_StringCreateByCString("mandelbrot");
  Res = WasmEdge_VMRegisterModuleFromFile(VMCxt, ModName, "./mandelbrot.so");
  if (!WasmEdge_ResultOK(Res)) {
    cout << "Registration failed:" << WasmEdge_ResultGetMessage(Res) << "\n";
    return -1;
  }

  /* The parameters and returns arrays. */

  WasmEdge_String FuncName =
      WasmEdge_StringCreateByCString("mandelbrot_thread");

  auto start = std::chrono::steady_clock::now();
  int num_threads;
  if (argc > 1) {
    num_threads = atoi(argv[1]);
  } else {
    num_threads = 4;
  }
  cout << "Number of threads: " << num_threads << "\n";

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.push_back(std::thread(
        [&](int rank) {
          WasmEdge_Value Params[6] = {
              WasmEdge_ValueGenI32(maxIterations),
              WasmEdge_ValueGenI32(num_threads),
              WasmEdge_ValueGenI32(rank),
              WasmEdge_ValueGenF64(x),
              WasmEdge_ValueGenF64(y),
              WasmEdge_ValueGenF64(d),
          };
          Res = WasmEdge_VMExecuteRegistered(VMCxt, ModName, FuncName, Params,
                                             6, NULL, 0);
          if (WasmEdge_ResultOK(Res)) {
            cout << "Got the result: ok\n";
          } else {
            cout << "Error message: " << WasmEdge_ResultGetMessage(Res) << "\n";
          }
        },
        i));
  }
  for (auto &thread : threads) {
    thread.join();
  }
  cout << "Elapsed Time: " << since(start).count() / 1e6 << std::endl;

  FuncName = WasmEdge_StringCreateByCString("getImage");
  WasmEdge_Value Returns[1];
  Res = WasmEdge_VMExecuteRegistered(VMCxt, ModName, FuncName, NULL, 0, Returns,
                                     1);
  int64_t offset = (int64_t)WasmEdge_ValueGetExternRef(Returns[0]);
  cout << "Offset: " << offset << "\n";

  unsigned char image[WIDTH * HEIGHT * 4];
  Res = WasmEdge_MemoryInstanceGetData(HostMemory, image, offset,
                                       WIDTH * HEIGHT * 4);
  if (WasmEdge_ResultOK(Res)) {
    cout << "Get memory: ok\n";
  } else {
    cout << "Error message: " << WasmEdge_ResultGetMessage(Res) << "\n";
    return -1;
  }

  FILE *pFile = fopen("output-wasmedge.bin", "wb");
  fwrite(image, sizeof(char), sizeof(image), pFile);
  fclose(pFile);

  /* Resources deallocations. */
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_MemoryTypeDelete(MemTypeCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  return 0;
}