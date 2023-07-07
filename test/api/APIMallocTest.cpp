#include "wasmedge/wasmedge.h"

#include <cstdlib>
#include <gtest/gtest.h>

namespace {

char WasmFilePath[] = "apiTestData/cmalloc.wasm";

TEST(MemoryManipulation, ExportedMallocFree) {
  int32_t *Buffer = nullptr;
  int32_t Size = 8;
  int32_t offset = -1;

  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(ConfCxt,
                                        WasmEdge_HostRegistration_Wasi);
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);

  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromFile(VMCxt, WasmFilePath)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VMCxt)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VMCxt)));

  offset =
      WasmEdge_Module_Malloc(VMCxt, Size * sizeof(int32_t), (void **)&Buffer);

  EXPECT_NE(Buffer, nullptr);
  EXPECT_NE(offset, -1);

  EXPECT_EQ(WasmEdge_Module_Free(VMCxt, offset), 0);

  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}