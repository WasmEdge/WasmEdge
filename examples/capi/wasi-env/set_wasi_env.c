#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <wasmedge/wasmedge.h>

int main(int argc, const char *const argv[]) {
  /* Turn on the WASI config. */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(ConfCxt,
                                        WasmEdge_HostRegistration_Wasi);

  /* Create the VM context. */
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);
  WasmEdge_ConfigureDelete(ConfCxt);

  /* The envs. */
  const char EnvStrs[] = {
      'E', 'N', 'V', '1', '=', 'V', 'A', 'L', '1', '\0',
      // ENV1=VAL1
      'E', 'N', 'V', '2', '=', 'V', 'A', 'L', '2', '\0',
      // ENV2=VAL2
      'E', 'N', 'V', '3', '=', 'V', 'A', 'L', '3', '\0'
      // ENV3=VAL3
  };
  const char *const Envs[] = {&EnvStrs[0], &EnvStrs[10], &EnvStrs[20]};

  /* Redirect stdout to a file */
  int OutFd = open("stdout.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (OutFd < 0) {
    perror("Failed to open stdout.txt");
    return 1;
  }

  /* Set the envs and args. */
  WasmEdge_ModuleInstanceContext *WasiCxt =
      WasmEdge_VMGetImportModuleContext(VMCxt, WasmEdge_HostRegistration_Wasi);

  /* Init WASI with env, args, preopens, and fd redirection */
  WasmEdge_ModuleInstanceInitWASIWithFds(WasiCxt, argv, argc, Envs, 3, NULL,
                                         0,            // No preopens
                                         STDIN_FILENO, // fd_in = 0 (stdin)
                                         OutFd, // fd_out = redirect to file
                                         STDERR_FILENO // fd_err = stderr
  );

  /* Instantiate the WASM file. */
  WasmEdge_Result Res;
  Res = WasmEdge_VMLoadWasmFromFile(VMCxt, "wasi_get_env.wasm");
  if (!WasmEdge_ResultOK(Res)) {
    printf("Load WASM failed. Error message: %s\n",
           WasmEdge_ResultGetMessage(Res));
  }
  Res = WasmEdge_VMValidate(VMCxt);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Validate WASM failed. Error message: %s\n",
           WasmEdge_ResultGetMessage(Res));
  }
  Res = WasmEdge_VMInstantiate(VMCxt);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Instantiate WASM failed. Error message: %s\n",
           WasmEdge_ResultGetMessage(Res));
  }

  /* Run the WASM function: print_env. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("print_env");
  Res = WasmEdge_VMExecute(VMCxt, FuncName, NULL, 0, NULL, 0);
  WasmEdge_StringDelete(FuncName);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Execution Failed. Error message: %s\n",
           WasmEdge_ResultGetMessage(Res));
  }

  /* Resources deallocations. */
  close(OutFd);
  WasmEdge_VMDelete(VMCxt);
  return 0;
}
