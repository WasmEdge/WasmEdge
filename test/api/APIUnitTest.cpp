// SPDX-License-Identifier: Apache-2.0
#include "api/wasmedge.h"

#include "gtest/gtest.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <string>
#include <vector>

namespace {

std::vector<char> ArgsVec = {
    'a', 'r', 'g', '1', '\0',
    /// arg1
    'a', 'r', 'g', '2', '\0'
    /// arg2
};
std::vector<char> EnvsVec = {
    'E', 'N', 'V', '1', '=', 'V', 'A', 'L', '1', '\0',
    /// ENV1=VAL1
    'E', 'N', 'V', '2', '=', 'V', 'A', 'L', '2', '\0',
    /// ENV2=VAL2
    'E', 'N', 'V', '3', '=', 'V', 'A', 'L', '3', '\0'
    /// ENV3=VAL3
};
std::vector<char> DirsVec = {
    '.', ':', '.', '\0'
    /// .:.
};
std::vector<char> PreopensVec = {
    'a', 'p', 'i', 'T', 'e', 's', 't', 'D', 'a', 't', 'a', '\0',
    /// apiTestData
    'M', 'a', 'k', 'e', 'f', 'i', 'l', 'e', '\0',
    /// Makefile
    'C', 'M', 'a', 'k', 'e', 'F', 'i', 'l', 'e', 's', '\0',
    /// CMakeFiles
    's', 's', 'v', 'm', 'A', 'P', 'I', 'C', 'o', 'r', 'e', 'T', 'e', 's', 't',
    's', '\0'
    /// wasmedgeAPICoreTests
};
char *Args[] = {&ArgsVec[0], &ArgsVec[5]};
char *Envs[] = {&EnvsVec[0], &EnvsVec[10], &EnvsVec[20]};
char *Dirs[] = {&DirsVec[0]};
char *Preopens[] = {&PreopensVec[0], &PreopensVec[12], &PreopensVec[21],
                    &PreopensVec[32]};
char TPath[] = "apiTestData/test.wasm";

WasmEdge_Result ExternAdd(void *, WasmEdge_MemoryInstanceContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /// {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(WasmEdge_ValueGetExternRef(In[0]));
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(*Val1 + Val2);
  return WasmEdge_Result_Success;
}

WasmEdge_Result ExternSub(void *, WasmEdge_MemoryInstanceContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /// {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(WasmEdge_ValueGetExternRef(In[0]));
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(*Val1 - Val2);
  return WasmEdge_Result_Success;
}

WasmEdge_Result ExternMul(void *, WasmEdge_MemoryInstanceContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /// {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(WasmEdge_ValueGetExternRef(In[0]));
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(*Val1 * Val2);
  return WasmEdge_Result_Success;
}

WasmEdge_Result ExternDiv(void *, WasmEdge_MemoryInstanceContext *,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {
  /// {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(WasmEdge_ValueGetExternRef(In[0]));
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(*Val1 / Val2);
  return WasmEdge_Result_Success;
}

WasmEdge_Result ExternTerm(void *, WasmEdge_MemoryInstanceContext *,
                           const WasmEdge_Value *, WasmEdge_Value *Out) {
  /// {} -> {i32}
  Out[0] = WasmEdge_ValueGenI32(1234);
  return WasmEdge_Result_Terminate;
}

WasmEdge_Result ExternFail(void *, WasmEdge_MemoryInstanceContext *,
                           const WasmEdge_Value *, WasmEdge_Value *Out) {
  /// {} -> {i32}
  Out[0] = WasmEdge_ValueGenI32(5678);
  return WasmEdge_Result_Fail;
}

WasmEdge_Result ExternWrap(void *This, void *Data,
                           WasmEdge_MemoryInstanceContext *MemCxt,
                           const WasmEdge_Value *In, const uint32_t,
                           WasmEdge_Value *Out, const uint32_t) {
  using HostFuncType =
      WasmEdge_Result(void *, WasmEdge_MemoryInstanceContext *,
                      const WasmEdge_Value *, WasmEdge_Value *);
  HostFuncType *Func = reinterpret_cast<HostFuncType *>(This);
  return Func(Data, MemCxt, In, Out);
}

/// Helper function to create import module with host functions
WasmEdge_ImportObjectContext *createExternModule(std::string_view Name,
                                                 bool IsWrap = false) {
  /// Create import object
  WasmEdge_String HostName = WasmEdge_StringCreateByCString(Name.data());
  WasmEdge_ImportObjectContext *ImpObj =
      WasmEdge_ImportObjectCreate(HostName, nullptr);
  WasmEdge_StringDelete(HostName);
  enum WasmEdge_ValType Param[2] = {WasmEdge_ValType_ExternRef,
                                    WasmEdge_ValType_I32},
                        Result[1] = {WasmEdge_ValType_I32};
  WasmEdge_FunctionTypeContext *HostFType =
      WasmEdge_FunctionTypeCreate(Param, 2, Result, 1);
  WasmEdge_HostFunctionContext *HostFunc = nullptr;

  /// Add host function "func-add"
  HostName = WasmEdge_StringCreateByCString("func-add");
  if (IsWrap) {
    HostFunc = WasmEdge_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternAdd), 0);
  } else {
    HostFunc = WasmEdge_HostFunctionCreate(HostFType, ExternAdd, 0);
  }
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);
  /// Add host function "func-sub"
  HostName = WasmEdge_StringCreateByCString("func-sub");
  if (IsWrap) {
    HostFunc = WasmEdge_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternSub), 0);
  } else {
    HostFunc = WasmEdge_HostFunctionCreate(HostFType, ExternSub, 0);
  }
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);
  /// Add host function "func-mul"
  HostName = WasmEdge_StringCreateByCString("func-mul");
  if (IsWrap) {
    HostFunc = WasmEdge_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternMul), 0);
  } else {
    HostFunc = WasmEdge_HostFunctionCreate(HostFType, ExternMul, 0);
  }
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);
  /// Add host function "func-div"
  HostName = WasmEdge_StringCreateByCString("func-div");
  if (IsWrap) {
    HostFunc = WasmEdge_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternDiv), 0);
  } else {
    HostFunc = WasmEdge_HostFunctionCreate(HostFType, ExternDiv, 0);
  }
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  WasmEdge_FunctionTypeDelete(HostFType);
  HostFType = WasmEdge_FunctionTypeCreate(nullptr, 0, Result, 1);

  /// Add host function "func-term"
  HostName = WasmEdge_StringCreateByCString("func-term");
  if (IsWrap) {
    HostFunc = WasmEdge_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternTerm), 0);
  } else {
    HostFunc = WasmEdge_HostFunctionCreate(HostFType, ExternTerm, 0);
  }
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);
  /// Add host function "func-fail"
  HostName = WasmEdge_StringCreateByCString("func-fail");
  if (IsWrap) {
    HostFunc = WasmEdge_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternFail), 0);
  } else {
    HostFunc = WasmEdge_HostFunctionCreate(HostFType, ExternFail, 0);
  }
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  WasmEdge_FunctionTypeDelete(HostFType);
  return ImpObj;
}

/// Helper function to load wasm file into AST module.
WasmEdge_ASTModuleContext *loadModule(const WasmEdge_ConfigureContext *Conf) {
  WasmEdge_ASTModuleContext *Mod = nullptr;
  WasmEdge_LoaderContext *Loader = WasmEdge_LoaderCreate(Conf);
  WasmEdge_LoaderParseFromFile(Loader, &Mod, TPath);
  WasmEdge_LoaderDelete(Loader);
  return Mod;
}

/// Helper function to validate wasm module.
bool validateModule(const WasmEdge_ConfigureContext *Conf,
                    const WasmEdge_ASTModuleContext *Mod) {
  WasmEdge_ValidatorContext *Validator = WasmEdge_ValidatorCreate(Conf);
  WasmEdge_Result Res = WasmEdge_ValidatorValidate(Validator, Mod);
  WasmEdge_ValidatorDelete(Validator);
  return WasmEdge_ResultOK(Res);
}

/// Helper function to register and instantiate module.
bool instantiateModule(const WasmEdge_ConfigureContext *Conf,
                       WasmEdge_StoreContext *Store,
                       const WasmEdge_ASTModuleContext *Mod,
                       WasmEdge_ImportObjectContext *ImpObj) {
  WasmEdge_InterpreterContext *Interp =
      WasmEdge_InterpreterCreate(Conf, nullptr);
  WasmEdge_String Name = WasmEdge_StringCreateByCString("module");
  if (!WasmEdge_ResultOK(
          WasmEdge_InterpreterRegisterImport(Interp, Store, ImpObj))) {
    return false;
  }
  if (!WasmEdge_ResultOK(
          WasmEdge_InterpreterRegisterModule(Interp, Store, Mod, Name))) {
    return false;
  }
  if (!WasmEdge_ResultOK(WasmEdge_InterpreterInstantiate(Interp, Store, Mod))) {
    return false;
  }
  WasmEdge_InterpreterDelete(Interp);
  WasmEdge_StringDelete(Name);
  return true;
}

TEST(APICoreTest, Version) {
  EXPECT_EQ(std::string(WASMEDGE_VERSION), std::string(WasmEdge_VersionGet()));
  EXPECT_EQ(static_cast<uint32_t>(WASMEDGE_VERSION_MAJOR),
            WasmEdge_VersionGetMajor());
  EXPECT_EQ(static_cast<uint32_t>(WASMEDGE_VERSION_MINOR),
            WasmEdge_VersionGetMinor());
  EXPECT_EQ(static_cast<uint32_t>(WASMEDGE_VERSION_PATCH),
            WasmEdge_VersionGetPatch());
}

TEST(APICoreTest, Log) {
  WasmEdge_LogSetDebugLevel();
  EXPECT_TRUE(true);
  WasmEdge_LogSetErrorLevel();
  EXPECT_TRUE(true);
}

TEST(APICoreTest, Value) {
  std::vector<uint32_t> Vec = {1U, 2U, 3U};
  WasmEdge_Value Val = WasmEdge_ValueGenI32(INT32_MAX);
  EXPECT_EQ(WasmEdge_ValueGetI32(Val), INT32_MAX);
  Val = WasmEdge_ValueGenI64(INT64_MAX);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), INT64_MAX);
  Val = WasmEdge_ValueGenF32(1 / 0.0f);
  EXPECT_EQ(WasmEdge_ValueGetF32(Val), 1 / 0.0f);
  Val = WasmEdge_ValueGenF64(-1 / 0.0);
  EXPECT_EQ(WasmEdge_ValueGetF64(Val), -1 / 0.0);
  Val = WasmEdge_ValueGenV128(static_cast<__int128>(INT64_MAX) * 2 + 1);
  EXPECT_EQ(WasmEdge_ValueGetV128(Val),
            static_cast<__int128>(INT64_MAX) * 2 + 1);
  Val = WasmEdge_ValueGenNullRef(WasmEdge_RefType_FuncRef);
  EXPECT_TRUE(WasmEdge_ValueIsNullRef(Val));
  Val = WasmEdge_ValueGenFuncRef(123U);
  EXPECT_EQ(WasmEdge_ValueGetFuncIdx(Val), 123U);
  Val = WasmEdge_ValueGenExternRef(&Vec);
  EXPECT_EQ(
      static_cast<std::vector<uint32_t> *>(WasmEdge_ValueGetExternRef(Val))
          ->data()[1],
      2U);
}

TEST(APICoreTest, String) {
  /// Test to delete nullptr.
  WasmEdge_String Str = {.Length = 0, .Buf = nullptr};
  WasmEdge_StringDelete(Str);
  EXPECT_TRUE(true);
  /// Test strings.
  WasmEdge_String Str1 = WasmEdge_StringCreateByCString("test_string");
  WasmEdge_String Str2 = WasmEdge_StringCreateByCString("test_string");
  EXPECT_TRUE(WasmEdge_StringIsEqual(Str1, Str2));
  const char CStr[] = "test_string_.....";
  WasmEdge_String Str3 = WasmEdge_StringCreateByBuffer(CStr, 11);
  EXPECT_TRUE(WasmEdge_StringIsEqual(Str1, Str3));
  WasmEdge_String Str4 = WasmEdge_StringWrap(CStr, 11);
  EXPECT_TRUE(WasmEdge_StringIsEqual(Str3, Str4));
  WasmEdge_String Str5 = WasmEdge_StringWrap(CStr, 13);
  EXPECT_FALSE(WasmEdge_StringIsEqual(Str3, Str5));
  char Buf[256];
  EXPECT_EQ(WasmEdge_StringCopy(Str3, nullptr, 0), 0U);
  EXPECT_EQ(WasmEdge_StringCopy(Str3, Buf, 5), 5U);
  EXPECT_EQ(std::strncmp(Str3.Buf, Buf, 5), 0);
  EXPECT_EQ(WasmEdge_StringCopy(Str3, Buf, 256), 11U);
  WasmEdge_StringDelete(Str1);
  WasmEdge_StringDelete(Str2);
  WasmEdge_StringDelete(Str3);
}

TEST(APICoreTest, Result) {
  WasmEdge_Result Res1 = {.Code = 0x00}; /// Success
  WasmEdge_Result Res2 = {.Code = 0x01}; /// Terminated -> Success
  WasmEdge_Result Res3 = {.Code = 0x02}; /// Failed
  EXPECT_TRUE(WasmEdge_ResultOK(Res1));
  EXPECT_TRUE(WasmEdge_ResultOK(Res2));
  EXPECT_FALSE(WasmEdge_ResultOK(Res3));
  EXPECT_EQ(WasmEdge_ResultGetCode(Res1), 0x00U);
  EXPECT_EQ(WasmEdge_ResultGetCode(Res2), 0x01U);
  EXPECT_EQ(WasmEdge_ResultGetCode(Res3), 0x02U);
  EXPECT_NE(WasmEdge_ResultGetMessage(Res1), nullptr);
  EXPECT_NE(WasmEdge_ResultGetMessage(Res2), nullptr);
  EXPECT_NE(WasmEdge_ResultGetMessage(Res3), nullptr);
}

TEST(APICoreTest, Configure) {
  WasmEdge_ConfigureContext *ConfNull = nullptr;
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  /// Tests for proposals.
  WasmEdge_ConfigureAddProposal(ConfNull, WasmEdge_Proposal_SIMD);
  WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_SIMD);
  WasmEdge_ConfigureAddProposal(ConfNull, WasmEdge_Proposal_Memory64);
  WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_Memory64);
  EXPECT_FALSE(WasmEdge_ConfigureHasProposal(ConfNull, WasmEdge_Proposal_SIMD));
  EXPECT_TRUE(WasmEdge_ConfigureHasProposal(Conf, WasmEdge_Proposal_SIMD));
  EXPECT_FALSE(
      WasmEdge_ConfigureHasProposal(ConfNull, WasmEdge_Proposal_Memory64));
  EXPECT_TRUE(WasmEdge_ConfigureHasProposal(Conf, WasmEdge_Proposal_Memory64));
  WasmEdge_ConfigureRemoveProposal(Conf, WasmEdge_Proposal_SIMD);
  WasmEdge_ConfigureRemoveProposal(ConfNull, WasmEdge_Proposal_SIMD);
  EXPECT_FALSE(WasmEdge_ConfigureHasProposal(ConfNull, WasmEdge_Proposal_SIMD));
  EXPECT_FALSE(WasmEdge_ConfigureHasProposal(Conf, WasmEdge_Proposal_SIMD));
  EXPECT_FALSE(
      WasmEdge_ConfigureHasProposal(ConfNull, WasmEdge_Proposal_Memory64));
  EXPECT_TRUE(WasmEdge_ConfigureHasProposal(Conf, WasmEdge_Proposal_Memory64));
  /// Tests for host registrations.
  WasmEdge_ConfigureAddHostRegistration(ConfNull,
                                        WasmEdge_HostRegistration_Wasi);
  WasmEdge_ConfigureAddHostRegistration(Conf, WasmEdge_HostRegistration_Wasi);
  EXPECT_FALSE(WasmEdge_ConfigureHasHostRegistration(
      ConfNull, WasmEdge_HostRegistration_Wasi));
  EXPECT_TRUE(WasmEdge_ConfigureHasHostRegistration(
      Conf, WasmEdge_HostRegistration_Wasi));
  WasmEdge_ConfigureRemoveHostRegistration(ConfNull,
                                           WasmEdge_HostRegistration_Wasi);
  WasmEdge_ConfigureRemoveHostRegistration(Conf,
                                           WasmEdge_HostRegistration_Wasi);
  EXPECT_FALSE(WasmEdge_ConfigureHasHostRegistration(
      ConfNull, WasmEdge_HostRegistration_Wasi));
  EXPECT_FALSE(WasmEdge_ConfigureHasHostRegistration(
      Conf, WasmEdge_HostRegistration_Wasi));
  /// Tests for memory limits.
  WasmEdge_ConfigureSetMaxMemoryPage(ConfNull, 1234U);
  WasmEdge_ConfigureSetMaxMemoryPage(Conf, 1234U);
  EXPECT_NE(WasmEdge_ConfigureGetMaxMemoryPage(ConfNull), 1234U);
  EXPECT_EQ(WasmEdge_ConfigureGetMaxMemoryPage(Conf), 1234U);
  /// Tests for AOT conpiler configurations.
  WasmEdge_ConfigureCompilerSetOptimizationLevel(
      ConfNull, WasmEdge_CompilerOptimizationLevel_Os);
  WasmEdge_ConfigureCompilerSetOptimizationLevel(
      Conf, WasmEdge_CompilerOptimizationLevel_Os);
  EXPECT_NE(WasmEdge_ConfigureCompilerGetOptimizationLevel(ConfNull),
            WasmEdge_CompilerOptimizationLevel_Os);
  EXPECT_EQ(WasmEdge_ConfigureCompilerGetOptimizationLevel(Conf),
            WasmEdge_CompilerOptimizationLevel_Os);
  WasmEdge_ConfigureCompilerSetDumpIR(ConfNull, true);
  WasmEdge_ConfigureCompilerSetDumpIR(Conf, true);
  EXPECT_NE(WasmEdge_ConfigureCompilerIsDumpIR(ConfNull), true);
  EXPECT_EQ(WasmEdge_ConfigureCompilerIsDumpIR(Conf), true);
  WasmEdge_ConfigureCompilerSetInstructionCounting(ConfNull, true);
  WasmEdge_ConfigureCompilerSetInstructionCounting(Conf, true);
  EXPECT_NE(WasmEdge_ConfigureCompilerIsInstructionCounting(ConfNull), true);
  EXPECT_EQ(WasmEdge_ConfigureCompilerIsInstructionCounting(Conf), true);
  WasmEdge_ConfigureCompilerSetCostMeasuring(ConfNull, true);
  WasmEdge_ConfigureCompilerSetCostMeasuring(Conf, true);
  EXPECT_NE(WasmEdge_ConfigureCompilerIsCostMeasuring(ConfNull), true);
  EXPECT_EQ(WasmEdge_ConfigureCompilerIsCostMeasuring(Conf), true);
  /// Test to delete nullptr.
  WasmEdge_ConfigureDelete(ConfNull);
  EXPECT_TRUE(true);
  WasmEdge_ConfigureDelete(Conf);
  EXPECT_TRUE(true);
}

#ifdef WASMEDGE_BUILD_AOT_RUNTIME
TEST(APICoreTest, Compiler) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();

  /// Compiler creation and deletion
  WasmEdge_CompilerContext *Compiler = WasmEdge_CompilerCreate(nullptr);
  EXPECT_NE(Compiler, nullptr);
  WasmEdge_CompilerDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_CompilerDelete(Compiler);
  EXPECT_TRUE(true);
  Compiler = WasmEdge_CompilerCreate(Conf);

  /// Compile file
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_CompilerCompile(Compiler, TPath, "test.so")));
  /// File not found
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_CompilerCompile(
      Compiler, "not_exist.wasm", "not_exist.wasm.so")));
  /// Parse failed
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_CompilerCompile(
      Compiler, "../spec/testSuites/core/binary/binary.4.wasm", "binary.so")));

  WasmEdge_CompilerDelete(Compiler);
  WasmEdge_ConfigureDelete(Conf);
}
#endif

TEST(APICoreTest, Loader) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_ReferenceTypes);
  WasmEdge_ASTModuleContext *Mod = nullptr;
  WasmEdge_ASTModuleContext **ModPtr = &Mod;

  /// Loader creation and deletion
  WasmEdge_LoaderContext *Loader = WasmEdge_LoaderCreate(nullptr);
  EXPECT_NE(Loader, nullptr);
  WasmEdge_LoaderDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_LoaderDelete(Loader);
  EXPECT_TRUE(true);
  Loader = WasmEdge_LoaderCreate(Conf);

  /// Parse from file
  Mod = nullptr;
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_LoaderParseFromFile(Loader, ModPtr, TPath)));
  EXPECT_NE(Mod, nullptr);
  WasmEdge_ASTModuleDelete(Mod);
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_LoaderParseFromFile(nullptr, ModPtr, TPath)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_LoaderParseFromFile(Loader, nullptr, TPath)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_LoaderParseFromFile(Loader, ModPtr, "file")));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_LoaderParseFromFile(nullptr, nullptr, TPath)));

  /// Parse from buffer
  std::ifstream Wasm(TPath, std::ios::binary | std::ios::ate);
  Wasm.seekg(0, std::ios::end);
  std::vector<uint8_t> Buf(static_cast<uint32_t>(Wasm.tellg()));
  Wasm.seekg(0, std::ios::beg);
  EXPECT_TRUE(Wasm.read(reinterpret_cast<char *>(Buf.data()),
                        static_cast<uint32_t>(Buf.size())));
  Wasm.close();
  Mod = nullptr;
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_LoaderParseFromBuffer(
      Loader, ModPtr, Buf.data(), static_cast<uint32_t>(Buf.size()))));
  EXPECT_NE(Mod, nullptr);
  WasmEdge_ASTModuleDelete(Mod);
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_LoaderParseFromBuffer(
      nullptr, ModPtr, Buf.data(), static_cast<uint32_t>(Buf.size()))));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_LoaderParseFromBuffer(
      Loader, nullptr, Buf.data(), static_cast<uint32_t>(Buf.size()))));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_LoaderParseFromBuffer(Loader, ModPtr, nullptr, 0)));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_LoaderParseFromBuffer(
      nullptr, nullptr, Buf.data(), static_cast<uint32_t>(Buf.size()))));

  /// AST module deletion
  WasmEdge_ASTModuleDelete(nullptr);
  EXPECT_TRUE(true);

  WasmEdge_LoaderDelete(Loader);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(APICoreTest, Validator) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_ReferenceTypes);

  /// Validator creation and deletion
  WasmEdge_ValidatorContext *Validator = WasmEdge_ValidatorCreate(nullptr);
  EXPECT_NE(Validator, nullptr);
  WasmEdge_ValidatorDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_ValidatorDelete(Validator);
  EXPECT_TRUE(true);
  Validator = WasmEdge_ValidatorCreate(Conf);

  /// Load and parse file
  WasmEdge_ASTModuleContext *Mod = loadModule(Conf);
  EXPECT_NE(Mod, nullptr);

  /// Validation
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_ValidatorValidate(Validator, Mod)));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_ValidatorValidate(nullptr, Mod)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_ValidatorValidate(Validator, nullptr)));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_ValidatorValidate(nullptr, nullptr)));

  WasmEdge_ASTModuleDelete(Mod);
  WasmEdge_ValidatorDelete(Validator);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(APICoreTest, InterpreterWithStatistics) {
  /// Create contexts
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_ReferenceTypes);
  WasmEdge_StoreContext *Store = WasmEdge_StoreCreate();

  /// Load and validate file
  WasmEdge_ASTModuleContext *Mod = loadModule(Conf);
  EXPECT_NE(Mod, nullptr);
  EXPECT_TRUE(validateModule(Conf, Mod));

  /// Statistics creation and deletion
  WasmEdge_StatisticsContext *Stat = WasmEdge_StatisticsCreate();
  EXPECT_NE(Stat, nullptr);
  WasmEdge_StatisticsDelete(Stat);
  EXPECT_TRUE(true);
  WasmEdge_StatisticsDelete(nullptr);
  EXPECT_TRUE(true);
  Stat = WasmEdge_StatisticsCreate();
  EXPECT_NE(Stat, nullptr);

  /// Statistics set cost table
  std::vector<uint64_t> CostTable(512, 20ULL);
  WasmEdge_StatisticsSetCostTable(nullptr, &CostTable[0], 512);
  EXPECT_TRUE(true);
  WasmEdge_StatisticsSetCostTable(Stat, nullptr, 0);
  EXPECT_TRUE(true);
  WasmEdge_StatisticsSetCostTable(Stat, &CostTable[0], 512);
  EXPECT_TRUE(true);

  /// Statistics set cost limit
  WasmEdge_StatisticsSetCostLimit(Stat, 100000000000000ULL);
  EXPECT_TRUE(true);
  WasmEdge_StatisticsSetCostLimit(nullptr, 1ULL);
  EXPECT_TRUE(true);

  /// Interpreter creation and deletion
  WasmEdge_InterpreterContext *Interp =
      WasmEdge_InterpreterCreate(nullptr, nullptr);
  EXPECT_NE(Interp, nullptr);
  WasmEdge_InterpreterDelete(Interp);
  EXPECT_TRUE(true);
  Interp = WasmEdge_InterpreterCreate(Conf, nullptr);
  EXPECT_NE(Interp, nullptr);
  WasmEdge_InterpreterDelete(Interp);
  EXPECT_TRUE(true);
  Interp = WasmEdge_InterpreterCreate(nullptr, Stat);
  EXPECT_NE(Interp, nullptr);
  WasmEdge_InterpreterDelete(Interp);
  EXPECT_TRUE(true);
  Interp = WasmEdge_InterpreterCreate(Conf, Stat);
  EXPECT_NE(Interp, nullptr);
  WasmEdge_InterpreterDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_ConfigureDelete(Conf);

  /// Register import object
  WasmEdge_ImportObjectContext *ImpObj = createExternModule("extern");
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ImportObjectContext *ImpObjWrap =
      createExternModule("extern-wrap", true);
  EXPECT_NE(ImpObjWrap, nullptr);
  WasmEdge_ImportObjectContext *ImpObj2 = createExternModule("extern");
  EXPECT_NE(ImpObj2, nullptr);
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterRegisterImport(nullptr, Store, ImpObj)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterRegisterImport(Interp, nullptr, ImpObj)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterRegisterImport(Interp, Store, nullptr)));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_InterpreterRegisterImport(Interp, Store, ImpObj)));
  /// Name conflict
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterRegisterImport(Interp, Store, ImpObj2)));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_InterpreterRegisterImport(Interp, Store, ImpObjWrap)));
  WasmEdge_ImportObjectDelete(ImpObj2);

  /// Register wasm module
  WasmEdge_String ModName = WasmEdge_StringCreateByCString("module");
  WasmEdge_String ModName2 = WasmEdge_StringCreateByCString("extern");
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterRegisterModule(nullptr, Store, Mod, ModName)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterRegisterModule(Interp, nullptr, Mod, ModName)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterRegisterModule(Interp, Store, nullptr, ModName)));
  /// Name conflict
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterRegisterModule(Interp, Store, Mod, ModName2)));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_InterpreterRegisterModule(Interp, Store, Mod, ModName)));
  WasmEdge_StringDelete(ModName);
  WasmEdge_StringDelete(ModName2);

  /// Instantiate wasm module
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_InterpreterInstantiate(nullptr, Store, Mod)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_InterpreterInstantiate(Interp, nullptr, Mod)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInstantiate(Interp, Store, nullptr)));
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_InterpreterInstantiate(Interp, Store, Mod)));
  /// Override instantiated wasm
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_InterpreterInstantiate(Interp, Store, Mod)));
  WasmEdge_ASTModuleDelete(Mod);

  /// Invoke functions
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("func-mul-2");
  WasmEdge_String FuncName2 = WasmEdge_StringCreateByCString("func-mul-3");
  WasmEdge_Value P[2], R[2];
  P[0] = WasmEdge_ValueGenI32(123);
  P[1] = WasmEdge_ValueGenI32(456);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(nullptr, Store, FuncName, P, 2, R, 2)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, nullptr, FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, P, 1, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, nullptr, 0, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, nullptr, 2, R, 2)));
  /// Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, P, 2, R, 2)));
  P[0] = WasmEdge_ValueGenI32(123);
  /// Function not found
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName2, P, 2, R, 2)));
  /// Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, P, 2, nullptr, 1)));
  WasmEdge_StringDelete(FuncName);
  WasmEdge_StringDelete(FuncName2);

  /// Invoke functions call to host functions
  /// Get table and set external reference
  uint32_t TestValue;
  WasmEdge_String TabName = WasmEdge_StringCreateByCString("tab-ext");
  WasmEdge_TableInstanceContext *TabCxt =
      WasmEdge_StoreFindTable(Store, TabName);
  EXPECT_NE(TabCxt, nullptr);
  WasmEdge_StringDelete(TabName);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(
      TabCxt, WasmEdge_ValueGenExternRef(&TestValue), 0)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(
      TabCxt, WasmEdge_ValueGenExternRef(&TestValue), 1)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(
      TabCxt, WasmEdge_ValueGenExternRef(&TestValue), 2)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(
      TabCxt, WasmEdge_ValueGenExternRef(&TestValue), 3)));
  /// Call add: (777) + (223)
  FuncName = WasmEdge_StringCreateByCString("func-host-add");
  P[0] = WasmEdge_ValueGenI32(223);
  TestValue = 777;
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, P, 1, R, 1)));
  EXPECT_EQ(1000, WasmEdge_ValueGetI32(R[0]));
  WasmEdge_StringDelete(FuncName);
  /// Call sub: (123) - (456)
  FuncName = WasmEdge_StringCreateByCString("func-host-sub");
  P[0] = WasmEdge_ValueGenI32(456);
  TestValue = 123;
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, P, 1, R, 1)));
  EXPECT_EQ(-333, WasmEdge_ValueGetI32(R[0]));
  WasmEdge_StringDelete(FuncName);
  /// Call mul: (-30) * (-66)
  FuncName = WasmEdge_StringCreateByCString("func-host-mul");
  P[0] = WasmEdge_ValueGenI32(-66);
  TestValue = static_cast<uint32_t>(-30);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, P, 1, R, 1)));
  EXPECT_EQ(1980, WasmEdge_ValueGetI32(R[0]));
  WasmEdge_StringDelete(FuncName);
  /// Call div: (-9999) / (1234)
  FuncName = WasmEdge_StringCreateByCString("func-host-div");
  P[0] = WasmEdge_ValueGenI32(1234);
  TestValue = static_cast<uint32_t>(-9999);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_InterpreterInvoke(Interp, Store, FuncName, P, 1, R, 1)));
  EXPECT_EQ(-8, WasmEdge_ValueGetI32(R[0]));
  WasmEdge_StringDelete(FuncName);

  /// Invoke functions of registered module
  ModName = WasmEdge_StringCreateByCString("extern");
  ModName2 = WasmEdge_StringCreateByCString("error-name");
  FuncName = WasmEdge_StringCreateByCString("func-add");
  FuncName2 = WasmEdge_StringCreateByCString("func-add2");
  TestValue = 5000;
  P[0] = WasmEdge_ValueGenExternRef(&TestValue);
  P[1] = WasmEdge_ValueGenI32(1500);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, R, 1)));
  EXPECT_EQ(6500, WasmEdge_ValueGetI32(R[0]));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      nullptr, Store, ModName, FuncName, P, 2, R, 1)));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, nullptr, ModName, FuncName, P, 2, R, 1)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 1, R, 1)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, nullptr, 0, R, 1)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, nullptr, 2, R, 1)));
  /// Function type mismatch
  P[1] = WasmEdge_ValueGenI64(1500);
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, R, 1)));
  P[1] = WasmEdge_ValueGenI32(1500);
  /// Module not found
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName2, FuncName, P, 2, R, 1)));
  /// Function not found
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName2, P, 2, R, 1)));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, R, 0)));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, nullptr, 1)));
  WasmEdge_StringDelete(ModName);
  WasmEdge_StringDelete(ModName2);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_StringDelete(FuncName2);

  /// Invoke host function to terminate or fail execution
  ModName = WasmEdge_StringCreateByCString("extern");
  FuncName = WasmEdge_StringCreateByCString("func-term");
  WasmEdge_Result Res = WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, nullptr, 0, R, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  WasmEdge_StringDelete(FuncName);
  FuncName = WasmEdge_StringCreateByCString("func-fail");
  Res = WasmEdge_InterpreterInvokeRegistered(Interp, Store, ModName, FuncName,
                                             nullptr, 0, R, 1);
  EXPECT_FALSE(WasmEdge_ResultOK(Res));
  EXPECT_GT(WasmEdge_ResultGetCode(Res), 0x01U);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_StringDelete(ModName);

  /// Invoke host function with binding to functions
  ModName = WasmEdge_StringCreateByCString("extern-wrap");
  FuncName = WasmEdge_StringCreateByCString("func-sub");
  TestValue = 1234;
  P[0] = WasmEdge_ValueGenExternRef(&TestValue);
  P[1] = WasmEdge_ValueGenI32(1500);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, R, 1)));
  EXPECT_EQ(-266, WasmEdge_ValueGetI32(R[0]));
  WasmEdge_StringDelete(FuncName);
  FuncName = WasmEdge_StringCreateByCString("func-term");
  Res = WasmEdge_InterpreterInvokeRegistered(Interp, Store, ModName, FuncName,
                                             nullptr, 0, R, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  WasmEdge_StringDelete(FuncName);
  FuncName = WasmEdge_StringCreateByCString("func-fail");
  Res = WasmEdge_InterpreterInvokeRegistered(Interp, Store, ModName, FuncName,
                                             nullptr, 0, R, 1);
  EXPECT_FALSE(WasmEdge_ResultOK(Res));
  EXPECT_GT(WasmEdge_ResultGetCode(Res), 0x01U);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_StringDelete(ModName);

  /// Statistics get instruction count
  EXPECT_GT(WasmEdge_StatisticsGetInstrCount(Stat), 0ULL);
  EXPECT_EQ(WasmEdge_StatisticsGetInstrCount(nullptr), 0ULL);

  /// Statistics get instruction per second
  EXPECT_GT(WasmEdge_StatisticsGetInstrPerSecond(Stat), 0.0);
  EXPECT_EQ(WasmEdge_StatisticsGetInstrPerSecond(nullptr), 0.0);

  /// Statistics get total cost
  EXPECT_GT(WasmEdge_StatisticsGetTotalCost(Stat), 0ULL);
  EXPECT_EQ(WasmEdge_StatisticsGetTotalCost(nullptr), 0ULL);

  WasmEdge_InterpreterDelete(Interp);
  WasmEdge_StoreDelete(Store);
  WasmEdge_StatisticsDelete(Stat);
  WasmEdge_ImportObjectDelete(ImpObj);
  WasmEdge_ImportObjectDelete(ImpObjWrap);
}

TEST(APICoreTest, Store) {
  /// Create contexts
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_ReferenceTypes);
  WasmEdge_StoreContext *Store = WasmEdge_StoreCreate();
  WasmEdge_String Names[15], ErrName, ModName[3];
  ModName[0] = WasmEdge_StringCreateByCString("module");
  ModName[1] = WasmEdge_StringCreateByCString("extern");
  ModName[2] = WasmEdge_StringCreateByCString("no-such-module");
  ErrName = WasmEdge_StringCreateByCString("invalid-instance-name");

  /// Store list exports before instantiation
  EXPECT_EQ(WasmEdge_StoreListFunctionLength(Store), 0U);
  EXPECT_EQ(WasmEdge_StoreListTableLength(Store), 0U);
  EXPECT_EQ(WasmEdge_StoreListMemoryLength(Store), 0U);
  EXPECT_EQ(WasmEdge_StoreListGlobalLength(Store), 0U);
  EXPECT_EQ(WasmEdge_StoreListFunctionRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(WasmEdge_StoreListTableRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(WasmEdge_StoreListMemoryRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(WasmEdge_StoreListGlobalRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(WasmEdge_StoreListModuleLength(Store), 0U);

  /// Register host module and instantiate wasm module
  WasmEdge_ImportObjectContext *ImpObj = createExternModule("extern");
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ASTModuleContext *Mod = loadModule(Conf);
  EXPECT_NE(Mod, nullptr);
  EXPECT_TRUE(validateModule(Conf, Mod));
  EXPECT_TRUE(instantiateModule(Conf, Store, Mod, ImpObj));
  WasmEdge_ASTModuleDelete(Mod);
  WasmEdge_ConfigureDelete(Conf);

  /// Store deletion
  WasmEdge_StoreDelete(nullptr);
  EXPECT_TRUE(true);

  /// Store list function exports
  EXPECT_EQ(WasmEdge_StoreListFunctionLength(Store), 11U);
  EXPECT_EQ(WasmEdge_StoreListFunctionLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_StoreListFunction(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_StoreListFunction(Store, nullptr, 15), 11U);
  EXPECT_EQ(WasmEdge_StoreListFunction(Store, Names, 4), 11U);
  for (uint32_t I = 0; I < 4; I++) {
    WasmEdge_StringDelete(Names[I]);
  }
  EXPECT_EQ(WasmEdge_StoreListFunction(Store, Names, 15), 11U);
  /// Delete names later

  /// Store find function
  EXPECT_NE(WasmEdge_StoreFindFunction(Store, Names[7]), nullptr);
  EXPECT_EQ(WasmEdge_StoreFindFunction(nullptr, Names[7]), nullptr);
  EXPECT_EQ(WasmEdge_StoreFindFunction(Store, ErrName), nullptr);

  /// Function instance get function type
  WasmEdge_FunctionInstanceContext *FuncCxt =
      WasmEdge_StoreFindFunction(Store, Names[7]);
  EXPECT_NE(WasmEdge_FunctionInstanceGetFunctionType(FuncCxt), nullptr);
  EXPECT_EQ(WasmEdge_FunctionInstanceGetFunctionType(nullptr), nullptr);
  for (uint32_t I = 0; I < 11; I++) {
    WasmEdge_StringDelete(Names[I]);
  }

  /// Store list function exports registered
  EXPECT_EQ(WasmEdge_StoreListFunctionRegisteredLength(Store, ModName[0]), 11U);
  EXPECT_EQ(WasmEdge_StoreListFunctionRegisteredLength(Store, ModName[1]), 6U);
  EXPECT_EQ(WasmEdge_StoreListFunctionRegisteredLength(Store, ModName[2]), 0U);
  EXPECT_EQ(WasmEdge_StoreListFunctionRegisteredLength(nullptr, ModName[0]),
            0U);
  EXPECT_EQ(
      WasmEdge_StoreListFunctionRegistered(nullptr, ModName[0], Names, 15), 0U);
  EXPECT_EQ(
      WasmEdge_StoreListFunctionRegistered(Store, ModName[0], nullptr, 15),
      11U);
  EXPECT_EQ(WasmEdge_StoreListFunctionRegistered(Store, ModName[0], Names, 4),
            11U);
  for (uint32_t I = 0; I < 4; I++) {
    WasmEdge_StringDelete(Names[I]);
  }
  EXPECT_EQ(WasmEdge_StoreListFunctionRegistered(Store, ModName[0], Names, 15),
            11U);
  /// Delete names later

  /// Store find function registered
  EXPECT_NE(WasmEdge_StoreFindFunctionRegistered(Store, ModName[0], Names[7]),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindFunctionRegistered(nullptr, ModName[0], Names[7]),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindFunctionRegistered(Store, ModName[0], ErrName),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindFunctionRegistered(Store, ModName[2], Names[7]),
            nullptr);
  for (uint32_t I = 0; I < 11; I++) {
    WasmEdge_StringDelete(Names[I]);
  }

  /// Store list table exports
  EXPECT_EQ(WasmEdge_StoreListTableLength(Store), 2U);
  EXPECT_EQ(WasmEdge_StoreListTableLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_StoreListTable(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_StoreListTable(Store, nullptr, 15), 2U);
  EXPECT_EQ(WasmEdge_StoreListTable(Store, Names, 1), 2U);
  WasmEdge_StringDelete(Names[0]);
  EXPECT_EQ(WasmEdge_StoreListTable(Store, Names, 15), 2U);
  /// Delete names later

  /// Store find table
  EXPECT_NE(WasmEdge_StoreFindTable(Store, Names[1]), nullptr);
  EXPECT_EQ(WasmEdge_StoreFindTable(nullptr, Names[1]), nullptr);
  EXPECT_EQ(WasmEdge_StoreFindTable(Store, ErrName), nullptr);
  WasmEdge_StringDelete(Names[0]);
  WasmEdge_StringDelete(Names[1]);

  /// Store list table exports registered
  EXPECT_EQ(WasmEdge_StoreListTableRegisteredLength(Store, ModName[0]), 2U);
  EXPECT_EQ(WasmEdge_StoreListTableRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(WasmEdge_StoreListTableRegisteredLength(Store, ModName[2]), 0U);
  EXPECT_EQ(WasmEdge_StoreListTableRegisteredLength(nullptr, ModName[0]), 0U);
  EXPECT_EQ(WasmEdge_StoreListTableRegistered(nullptr, ModName[0], Names, 15),
            0U);
  EXPECT_EQ(WasmEdge_StoreListTableRegistered(Store, ModName[0], nullptr, 15),
            2U);
  EXPECT_EQ(WasmEdge_StoreListTableRegistered(Store, ModName[0], Names, 1), 2U);
  WasmEdge_StringDelete(Names[0]);
  EXPECT_EQ(WasmEdge_StoreListTableRegistered(Store, ModName[0], Names, 15),
            2U);
  /// Delete names later

  /// Store find table registered
  EXPECT_NE(WasmEdge_StoreFindTableRegistered(Store, ModName[0], Names[1]),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindTableRegistered(nullptr, ModName[0], Names[1]),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindTableRegistered(Store, ModName[0], ErrName),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindTableRegistered(Store, ModName[2], Names[1]),
            nullptr);
  WasmEdge_StringDelete(Names[0]);
  WasmEdge_StringDelete(Names[1]);

  /// Store list memory exports
  EXPECT_EQ(WasmEdge_StoreListMemoryLength(Store), 1U);
  EXPECT_EQ(WasmEdge_StoreListMemoryLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_StoreListMemory(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_StoreListMemory(Store, nullptr, 15), 1U);
  EXPECT_EQ(WasmEdge_StoreListMemory(Store, Names, 0), 1U);
  EXPECT_EQ(WasmEdge_StoreListMemory(Store, Names, 15), 1U);
  /// Delete names later

  /// Store find memory
  EXPECT_NE(WasmEdge_StoreFindMemory(Store, Names[0]), nullptr);
  EXPECT_EQ(WasmEdge_StoreFindMemory(nullptr, Names[0]), nullptr);
  EXPECT_EQ(WasmEdge_StoreFindMemory(Store, ErrName), nullptr);
  WasmEdge_StringDelete(Names[0]);

  /// Store list memory exports registered
  EXPECT_EQ(WasmEdge_StoreListMemoryRegisteredLength(Store, ModName[0]), 1U);
  EXPECT_EQ(WasmEdge_StoreListMemoryRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(WasmEdge_StoreListMemoryRegisteredLength(Store, ModName[2]), 0U);
  EXPECT_EQ(WasmEdge_StoreListMemoryRegisteredLength(nullptr, ModName[0]), 0U);
  EXPECT_EQ(WasmEdge_StoreListMemoryRegistered(nullptr, ModName[0], Names, 15),
            0U);
  EXPECT_EQ(WasmEdge_StoreListMemoryRegistered(Store, ModName[0], nullptr, 15),
            1U);
  EXPECT_EQ(WasmEdge_StoreListMemoryRegistered(Store, ModName[0], Names, 0),
            1U);
  EXPECT_EQ(WasmEdge_StoreListMemoryRegistered(Store, ModName[0], Names, 15),
            1U);
  /// Delete names later

  /// Store find memory registered
  EXPECT_NE(WasmEdge_StoreFindMemoryRegistered(Store, ModName[0], Names[0]),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindMemoryRegistered(nullptr, ModName[0], Names[0]),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindMemoryRegistered(Store, ModName[0], ErrName),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindMemoryRegistered(Store, ModName[2], Names[0]),
            nullptr);
  WasmEdge_StringDelete(Names[0]);

  /// Store list global exports
  EXPECT_EQ(WasmEdge_StoreListGlobalLength(Store), 2U);
  EXPECT_EQ(WasmEdge_StoreListGlobalLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_StoreListGlobal(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_StoreListGlobal(Store, nullptr, 15), 2U);
  EXPECT_EQ(WasmEdge_StoreListGlobal(Store, Names, 1), 2U);
  WasmEdge_StringDelete(Names[0]);
  EXPECT_EQ(WasmEdge_StoreListGlobal(Store, Names, 15), 2U);
  /// Delete names later

  /// Store find global
  EXPECT_NE(WasmEdge_StoreFindGlobal(Store, Names[1]), nullptr);
  EXPECT_EQ(WasmEdge_StoreFindGlobal(nullptr, Names[1]), nullptr);
  EXPECT_EQ(WasmEdge_StoreFindGlobal(Store, ErrName), nullptr);
  WasmEdge_StringDelete(Names[0]);
  WasmEdge_StringDelete(Names[1]);

  /// Store list global exports registered
  EXPECT_EQ(WasmEdge_StoreListGlobalRegisteredLength(Store, ModName[0]), 2U);
  EXPECT_EQ(WasmEdge_StoreListGlobalRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(WasmEdge_StoreListGlobalRegisteredLength(Store, ModName[2]), 0U);
  EXPECT_EQ(WasmEdge_StoreListGlobalRegisteredLength(nullptr, ModName[0]), 0U);
  EXPECT_EQ(WasmEdge_StoreListGlobalRegistered(nullptr, ModName[0], Names, 15),
            0U);
  EXPECT_EQ(WasmEdge_StoreListGlobalRegistered(Store, ModName[0], nullptr, 15),
            2U);
  EXPECT_EQ(WasmEdge_StoreListGlobalRegistered(Store, ModName[0], Names, 1),
            2U);
  WasmEdge_StringDelete(Names[0]);
  EXPECT_EQ(WasmEdge_StoreListGlobalRegistered(Store, ModName[0], Names, 15),
            2U);
  /// Delete names later

  /// Store find global registered
  EXPECT_NE(WasmEdge_StoreFindGlobalRegistered(Store, ModName[0], Names[1]),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindGlobalRegistered(nullptr, ModName[0], Names[1]),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindGlobalRegistered(Store, ModName[0], ErrName),
            nullptr);
  EXPECT_EQ(WasmEdge_StoreFindGlobalRegistered(Store, ModName[2], Names[1]),
            nullptr);
  WasmEdge_StringDelete(Names[0]);
  WasmEdge_StringDelete(Names[1]);

  /// Store list module
  EXPECT_EQ(WasmEdge_StoreListModuleLength(Store), 2U);
  EXPECT_EQ(WasmEdge_StoreListModuleLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_StoreListModule(nullptr, Names, 15), 0U);
  EXPECT_EQ(WasmEdge_StoreListModule(Store, nullptr, 15), 2U);
  EXPECT_EQ(WasmEdge_StoreListModule(Store, Names, 1), 2U);
  WasmEdge_StringDelete(Names[0]);
  EXPECT_EQ(WasmEdge_StoreListModule(Store, Names, 15), 2U);
  WasmEdge_StringDelete(Names[0]);
  WasmEdge_StringDelete(Names[1]);

  WasmEdge_StringDelete(ModName[0]);
  WasmEdge_StringDelete(ModName[1]);
  WasmEdge_StringDelete(ModName[2]);
  WasmEdge_StoreDelete(Store);
  WasmEdge_ImportObjectDelete(ImpObj);
}

TEST(APICoreTest, FunctionType) {
  std::vector<WasmEdge_ValType> Param = {
      WasmEdge_ValType_I32,  WasmEdge_ValType_I64, WasmEdge_ValType_ExternRef,
      WasmEdge_ValType_V128, WasmEdge_ValType_F64, WasmEdge_ValType_F32};
  std::vector<WasmEdge_ValType> Result = {WasmEdge_ValType_FuncRef,
                                          WasmEdge_ValType_ExternRef,
                                          WasmEdge_ValType_V128};
  enum WasmEdge_ValType Buf1[6], Buf2[2];
  WasmEdge_FunctionTypeContext *FType =
      WasmEdge_FunctionTypeCreate(&Param[0], 6, &Result[0], 3);
  EXPECT_EQ(WasmEdge_FunctionTypeGetParametersLength(FType), 6U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetParametersLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturnsLength(FType), 3U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturnsLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetParameters(FType, Buf1, 6), 6U);
  EXPECT_EQ(Param, std::vector<WasmEdge_ValType>(Buf1, Buf1 + 6));
  EXPECT_EQ(WasmEdge_FunctionTypeGetParameters(FType, Buf2, 2), 6U);
  EXPECT_EQ(std::vector<WasmEdge_ValType>(Param.cbegin(), Param.cbegin() + 2),
            std::vector<WasmEdge_ValType>(Buf2, Buf2 + 2));
  EXPECT_EQ(WasmEdge_FunctionTypeGetParameters(nullptr, Buf1, 6), 0U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturns(FType, Buf1, 6), 3U);
  EXPECT_EQ(Result, std::vector<WasmEdge_ValType>(Buf1, Buf1 + 3));
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturns(FType, Buf2, 2), 3U);
  EXPECT_EQ(std::vector<WasmEdge_ValType>(Result.cbegin(), Result.cbegin() + 2),
            std::vector<WasmEdge_ValType>(Buf2, Buf2 + 2));
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturns(nullptr, Buf1, 6), 0U);
  WasmEdge_FunctionTypeDelete(FType);

  FType = WasmEdge_FunctionTypeCreate(nullptr, 0, nullptr, 0);
  EXPECT_EQ(WasmEdge_FunctionTypeGetParameters(FType, Buf1, 6), 0U);
  EXPECT_EQ(WasmEdge_FunctionTypeGetReturns(FType, Buf1, 6), 0U);
  WasmEdge_FunctionTypeDelete(FType);
}

TEST(APICoreTest, Instance) {
  WasmEdge_Value Val, TmpVal;
  /// WasmEdge_FunctionInstanceGetFunctionType() tested in `Store` test case.

  /// Table instance creation
  WasmEdge_TableInstanceContext *TabCxt = WasmEdge_TableInstanceCreate(
      WasmEdge_RefType_ExternRef,
      WasmEdge_Limit{.HasMax = false, .Min = 10, .Max = 0});
  EXPECT_NE(TabCxt, nullptr);
  WasmEdge_TableInstanceDelete(TabCxt);
  EXPECT_TRUE(true);
  TabCxt = WasmEdge_TableInstanceCreate(
      WasmEdge_RefType_ExternRef,
      WasmEdge_Limit{.HasMax = true, .Min = 10, .Max = 20});
  EXPECT_NE(TabCxt, nullptr);

  /// Table instance get reference type
  EXPECT_EQ(WasmEdge_TableInstanceGetRefType(TabCxt),
            WasmEdge_RefType_ExternRef);
  EXPECT_EQ(WasmEdge_TableInstanceGetRefType(nullptr),
            WasmEdge_RefType_FuncRef);

  /// Table instance set data
  Val = WasmEdge_ValueGenExternRef(&TabCxt);
  TmpVal = WasmEdge_ValueGenFuncRef(2);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(TabCxt, Val, 5)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(TabCxt, TmpVal, 6)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(nullptr, Val, 5)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(TabCxt, Val, 15)));

  /// Table instance get data
  Val = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_TableInstanceGetData(TabCxt, &Val, 5)));
  EXPECT_EQ(reinterpret_cast<WasmEdge_TableInstanceContext **>(
                WasmEdge_ValueGetExternRef(Val)),
            &TabCxt);
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_TableInstanceGetData(nullptr, &Val, 5)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_TableInstanceGetData(TabCxt, &Val, 15)));

  /// Table instance get size and grow
  EXPECT_EQ(WasmEdge_TableInstanceGetSize(TabCxt), 10U);
  EXPECT_EQ(WasmEdge_TableInstanceGetSize(nullptr), 0U);
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_TableInstanceGrow(nullptr, 8)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_TableInstanceGrow(TabCxt, 8)));
  EXPECT_EQ(WasmEdge_TableInstanceGetSize(TabCxt), 18U);
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_TableInstanceGrow(TabCxt, 8)));
  EXPECT_EQ(WasmEdge_TableInstanceGetSize(TabCxt), 18U);
  Val = WasmEdge_ValueGenExternRef(&TabCxt);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_TableInstanceSetData(TabCxt, Val, 15)));
  Val = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_TableInstanceGetData(TabCxt, &Val, 15)));
  EXPECT_EQ(reinterpret_cast<WasmEdge_TableInstanceContext **>(
                WasmEdge_ValueGetExternRef(Val)),
            &TabCxt);

  /// Table instance deletion
  WasmEdge_TableInstanceDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_TableInstanceDelete(TabCxt);
  EXPECT_TRUE(true);

  /// Memory instance creation
  WasmEdge_MemoryInstanceContext *MemCxt = WasmEdge_MemoryInstanceCreate(
      WasmEdge_Limit{.HasMax = false, .Min = 1, .Max = 0});
  EXPECT_NE(MemCxt, nullptr);
  WasmEdge_MemoryInstanceDelete(MemCxt);
  EXPECT_TRUE(true);
  MemCxt = WasmEdge_MemoryInstanceCreate(
      WasmEdge_Limit{.HasMax = true, .Min = 1, .Max = 3});
  EXPECT_NE(MemCxt, nullptr);

  /// Memory instance set data
  std::vector<uint8_t> DataSet = {'t', 'e', 's', 't', ' ',
                                  'd', 'a', 't', 'a', '\n'};
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceSetData(MemCxt, DataSet.data(), 100, 10)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceSetData(nullptr, DataSet.data(), 100, 10)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceSetData(MemCxt, nullptr, 100, 0)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceSetData(nullptr, nullptr, 100, 0)));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceSetData(MemCxt, DataSet.data(), 100, 0)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceSetData(MemCxt, DataSet.data(), 65536, 10)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceSetData(MemCxt, DataSet.data(), 65530, 10)));

  /// Memory instance get data
  std::vector<uint8_t> DataGet;
  DataGet.resize(10);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceGetData(MemCxt, DataGet.data(), 100, 10)));
  EXPECT_EQ(DataGet, DataSet);
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceGetData(nullptr, DataGet.data(), 100, 10)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceGetData(MemCxt, nullptr, 100, 0)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceGetData(nullptr, nullptr, 100, 0)));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceGetData(MemCxt, DataGet.data(), 100, 0)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceGetData(MemCxt, DataGet.data(), 65536, 10)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceGetData(MemCxt, DataGet.data(), 65530, 10)));

  /// Memory instance get pointer
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointer(nullptr, 100, 10));
  EXPECT_NE(nullptr, WasmEdge_MemoryInstanceGetPointer(MemCxt, 100, 10));
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointer(MemCxt, 65536, 10));
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointer(MemCxt, 65530, 10));
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointerConst(nullptr, 100, 10));
  EXPECT_NE(nullptr, WasmEdge_MemoryInstanceGetPointerConst(MemCxt, 100, 10));
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointerConst(MemCxt, 65536, 10));
  EXPECT_EQ(nullptr, WasmEdge_MemoryInstanceGetPointerConst(MemCxt, 65530, 10));
  EXPECT_TRUE(std::equal(DataSet.cbegin(), DataSet.cend(),
                         WasmEdge_MemoryInstanceGetPointer(MemCxt, 100, 10)));
  EXPECT_TRUE(
      std::equal(DataSet.cbegin(), DataSet.cend(),
                 WasmEdge_MemoryInstanceGetPointerConst(MemCxt, 100, 10)));

  /// Memory instance get size and grow
  EXPECT_EQ(WasmEdge_MemoryInstanceGetPageSize(MemCxt), 1U);
  EXPECT_EQ(WasmEdge_MemoryInstanceGetPageSize(nullptr), 0U);
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_MemoryInstanceGrowPage(nullptr, 1)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_MemoryInstanceGrowPage(MemCxt, 1)));
  EXPECT_EQ(WasmEdge_MemoryInstanceGetPageSize(MemCxt), 2U);
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_MemoryInstanceGrowPage(MemCxt, 2)));
  EXPECT_EQ(WasmEdge_MemoryInstanceGetPageSize(MemCxt), 2U);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceSetData(MemCxt, DataSet.data(), 70000, 10)));
  DataGet.clear();
  DataGet.resize(10);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_MemoryInstanceGetData(MemCxt, DataGet.data(), 70000, 10)));
  EXPECT_EQ(DataGet, DataSet);

  /// Memory instance deletion
  WasmEdge_MemoryInstanceDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_MemoryInstanceDelete(MemCxt);
  EXPECT_TRUE(true);

  /// Global instance creation
  WasmEdge_GlobalInstanceContext *GlobCCxt = WasmEdge_GlobalInstanceCreate(
      WasmEdge_ValueGenI64(55555555555LL), WasmEdge_Mutability_Const);
  WasmEdge_GlobalInstanceContext *GlobVCxt = WasmEdge_GlobalInstanceCreate(
      WasmEdge_ValueGenI64(66666666666LL), WasmEdge_Mutability_Var);
  EXPECT_NE(GlobCCxt, nullptr);
  EXPECT_NE(GlobVCxt, nullptr);

  /// Global instance get value type
  EXPECT_EQ(WasmEdge_GlobalInstanceGetValType(GlobCCxt), WasmEdge_ValType_I64);
  EXPECT_EQ(WasmEdge_GlobalInstanceGetValType(GlobVCxt), WasmEdge_ValType_I64);
  EXPECT_EQ(WasmEdge_GlobalInstanceGetValType(nullptr), WasmEdge_ValType_I32);

  /// Global instance get mutability
  EXPECT_EQ(WasmEdge_GlobalInstanceGetMutability(GlobCCxt),
            WasmEdge_Mutability_Const);
  EXPECT_EQ(WasmEdge_GlobalInstanceGetMutability(GlobVCxt),
            WasmEdge_Mutability_Var);
  EXPECT_EQ(WasmEdge_GlobalInstanceGetMutability(nullptr),
            WasmEdge_Mutability_Const);

  /// Global instance get value
  Val = WasmEdge_GlobalInstanceGetValue(GlobCCxt);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 55555555555LL);
  Val = WasmEdge_GlobalInstanceGetValue(GlobVCxt);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 66666666666LL);
  Val = WasmEdge_GlobalInstanceGetValue(nullptr);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 0LL);

  /// Global instance set value
  Val = WasmEdge_ValueGenI64(77777777777LL);
  WasmEdge_GlobalInstanceSetValue(GlobCCxt, Val);
  Val = WasmEdge_GlobalInstanceGetValue(GlobCCxt);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 55555555555LL);
  Val = WasmEdge_ValueGenI64(88888888888LL);
  WasmEdge_GlobalInstanceSetValue(GlobVCxt, Val);
  Val = WasmEdge_GlobalInstanceGetValue(GlobVCxt);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 88888888888LL);
  Val = WasmEdge_ValueGenF32(12.345f);
  WasmEdge_GlobalInstanceSetValue(GlobVCxt, Val);
  Val = WasmEdge_GlobalInstanceGetValue(GlobVCxt);
  EXPECT_EQ(WasmEdge_ValueGetI64(Val), 88888888888LL);
  WasmEdge_GlobalInstanceSetValue(nullptr, Val);
  EXPECT_TRUE(true);

  /// Global instance deletion
  WasmEdge_GlobalInstanceDelete(nullptr);
  EXPECT_TRUE(true);
  WasmEdge_GlobalInstanceDelete(GlobCCxt);
  EXPECT_TRUE(true);
  WasmEdge_GlobalInstanceDelete(GlobVCxt);
  EXPECT_TRUE(true);
}

TEST(APICoreTest, ImportObject) {
  WasmEdge_String HostName = WasmEdge_StringCreateByCString("extern");
  WasmEdge_ConfigureContext *Conf = nullptr;
  WasmEdge_VMContext *VM = nullptr;
  WasmEdge_ImportObjectContext *ImpObj = nullptr;
  WasmEdge_FunctionTypeContext *HostFType = nullptr;
  WasmEdge_HostFunctionContext *HostFunc = nullptr;
  WasmEdge_TableInstanceContext *HostTable = nullptr;
  WasmEdge_MemoryInstanceContext *HostMemory = nullptr;
  WasmEdge_GlobalInstanceContext *HostGlobal = nullptr;
  enum WasmEdge_ValType Param[2], Result[1];

  /// Create import object with name ""
  ImpObj = WasmEdge_ImportObjectCreate({.Length = 0, .Buf = nullptr}, nullptr);
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ImportObjectDelete(ImpObj);

  /// Create import object with name "extern"
  ImpObj = WasmEdge_ImportObjectCreate(HostName, nullptr);
  WasmEdge_StringDelete(HostName);
  EXPECT_NE(ImpObj, nullptr);

  /// Add host function "func-add": {externref, i32} -> {i32}
  Param[0] = WasmEdge_ValType_ExternRef;
  Param[1] = WasmEdge_ValType_I32;
  Result[0] = WasmEdge_ValType_I32;
  HostFType = WasmEdge_FunctionTypeCreate(Param, 2, Result, 1);
  HostFunc = WasmEdge_HostFunctionCreate(nullptr, ExternAdd, 0);
  EXPECT_EQ(HostFunc, nullptr);
  HostFunc = WasmEdge_HostFunctionCreate(HostFType, nullptr, 0);
  EXPECT_EQ(HostFunc, nullptr);
  HostFunc = WasmEdge_HostFunctionCreate(HostFType, ExternAdd, 0);
  EXPECT_NE(HostFunc, nullptr);
  WasmEdge_HostFunctionDelete(HostFunc);
  EXPECT_TRUE(true);
  HostFunc = WasmEdge_HostFunctionCreate(HostFType, ExternAdd, 0);
  EXPECT_NE(HostFunc, nullptr);
  HostName = WasmEdge_StringCreateByCString("func-add");
  WasmEdge_ImportObjectAddHostFunction(nullptr, HostName, HostFunc);
  EXPECT_TRUE(true);
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, nullptr);
  EXPECT_TRUE(true);
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  EXPECT_TRUE(true);
  WasmEdge_StringDelete(HostName);

  /// Add host function for binding "func-add-binding"
  HostFunc = WasmEdge_HostFunctionCreateBinding(
      nullptr, ExternWrap, reinterpret_cast<void *>(ExternAdd), 0);
  EXPECT_EQ(HostFunc, nullptr);
  HostFunc = WasmEdge_HostFunctionCreateBinding(
      HostFType, nullptr, reinterpret_cast<void *>(ExternAdd), 0);
  EXPECT_EQ(HostFunc, nullptr);
  HostFunc = WasmEdge_HostFunctionCreateBinding(
      HostFType, ExternWrap, reinterpret_cast<void *>(ExternAdd), 0);
  EXPECT_NE(HostFunc, nullptr);
  WasmEdge_HostFunctionDelete(HostFunc);
  WasmEdge_FunctionTypeDelete(HostFType);

  /// Add host table "table"
  WasmEdge_Limit TabLimit = {.HasMax = true, .Min = 10, .Max = 20};
  HostTable = WasmEdge_TableInstanceCreate(WasmEdge_RefType_FuncRef, TabLimit);
  HostName = WasmEdge_StringCreateByCString("table");
  WasmEdge_ImportObjectAddTable(nullptr, HostName, HostTable);
  EXPECT_TRUE(true);
  WasmEdge_ImportObjectAddTable(ImpObj, HostName, nullptr);
  EXPECT_TRUE(true);
  WasmEdge_ImportObjectAddTable(ImpObj, HostName, HostTable);
  EXPECT_TRUE(true);
  WasmEdge_StringDelete(HostName);

  /// Add host memory "memory"
  WasmEdge_Limit MemLimit = {.HasMax = true, .Min = 1, .Max = 2};
  HostMemory = WasmEdge_MemoryInstanceCreate(MemLimit);
  HostName = WasmEdge_StringCreateByCString("memory");
  WasmEdge_ImportObjectAddMemory(nullptr, HostName, HostMemory);
  EXPECT_TRUE(true);
  WasmEdge_ImportObjectAddMemory(ImpObj, HostName, nullptr);
  EXPECT_TRUE(true);
  WasmEdge_ImportObjectAddMemory(ImpObj, HostName, HostMemory);
  EXPECT_TRUE(true);
  WasmEdge_StringDelete(HostName);

  /// Add host global "global_i32": const 666
  HostGlobal = WasmEdge_GlobalInstanceCreate(WasmEdge_ValueGenI32(666),
                                             WasmEdge_Mutability_Const);
  HostName = WasmEdge_StringCreateByCString("global_i32");
  WasmEdge_ImportObjectAddGlobal(nullptr, HostName, HostGlobal);
  EXPECT_TRUE(true);
  WasmEdge_ImportObjectAddGlobal(ImpObj, HostName, nullptr);
  EXPECT_TRUE(true);
  WasmEdge_ImportObjectAddGlobal(ImpObj, HostName, HostGlobal);
  EXPECT_TRUE(true);
  WasmEdge_StringDelete(HostName);

  WasmEdge_ImportObjectDelete(ImpObj);

  /// Create WASI.
  ImpObj =
      WasmEdge_ImportObjectCreateWASI(Args, 2, Envs, 3, Dirs, 1, Preopens, 4);
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ImportObjectDelete(ImpObj);
  ImpObj = WasmEdge_ImportObjectCreateWASI(nullptr, 0, nullptr, 0, nullptr, 0,
                                           nullptr, 0);
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ImportObjectDelete(ImpObj);
  ImpObj =
      WasmEdge_ImportObjectCreateWASI(Args, 0, Envs, 3, Dirs, 1, Preopens, 4);
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ImportObjectDelete(ImpObj);

  /// Initialize WASI in VM.
  Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(Conf, WasmEdge_HostRegistration_Wasi);
  VM = WasmEdge_VMCreate(Conf, nullptr);
  WasmEdge_ConfigureDelete(Conf);
  ImpObj =
      WasmEdge_VMGetImportModuleContext(VM, WasmEdge_HostRegistration_Wasi);
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ImportObjectInitWASI(nullptr, Args, 2, Envs, 3, Dirs, 1, Preopens,
                                4);
  EXPECT_TRUE(true);
  WasmEdge_ImportObjectInitWASI(ImpObj, Args, 2, Envs, 3, Dirs, 1, Preopens, 4);
  EXPECT_TRUE(true);
  WasmEdge_VMDelete(VM);

  /// Create wasmedge_process.
  ImpObj = WasmEdge_ImportObjectCreateWasmEdgeProcess(Args, 2, false);
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ImportObjectDelete(ImpObj);
  ImpObj = WasmEdge_ImportObjectCreateWasmEdgeProcess(nullptr, 0, false);
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ImportObjectDelete(ImpObj);
  ImpObj = WasmEdge_ImportObjectCreateWasmEdgeProcess(Args, 2, true);
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ImportObjectDelete(ImpObj);
  ImpObj = WasmEdge_ImportObjectCreateWasmEdgeProcess(nullptr, 0, true);
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ImportObjectDelete(ImpObj);

  /// Initialize wasmedge_process in VM.
  Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(
      Conf, WasmEdge_HostRegistration_WasmEdge_Process);
  VM = WasmEdge_VMCreate(Conf, nullptr);
  WasmEdge_ConfigureDelete(Conf);
  ImpObj = WasmEdge_VMGetImportModuleContext(
      VM, WasmEdge_HostRegistration_WasmEdge_Process);
  EXPECT_NE(ImpObj, nullptr);
  WasmEdge_ImportObjectInitWasmEdgeProcess(nullptr, Args, 2, false);
  EXPECT_TRUE(true);
  WasmEdge_ImportObjectInitWasmEdgeProcess(ImpObj, Args, 2, false);
  EXPECT_TRUE(true);
  WasmEdge_VMDelete(VM);
}

TEST(APICoreTest, VM) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddProposal(Conf, WasmEdge_Proposal_ReferenceTypes);
  WasmEdge_ConfigureAddHostRegistration(Conf, WasmEdge_HostRegistration_Wasi);
  WasmEdge_StoreContext *Store = WasmEdge_StoreCreate();
  WasmEdge_ImportObjectContext *ImpObj = createExternModule("extern");
  WasmEdge_String ModName, ModName2, FuncName, FuncName2, Names[15];
  WasmEdge_FunctionTypeContext *FuncType = nullptr;
  WasmEdge_Value P[10], R[10];
  WasmEdge_FunctionTypeContext *FuncTypes[15];

  /// WASM from file
  std::ifstream Wasm(TPath, std::ios::binary | std::ios::ate);
  std::vector<uint8_t> Buf(static_cast<uint32_t>(Wasm.tellg()));
  Wasm.seekg(0, std::ios::beg);
  EXPECT_TRUE(Wasm.read(reinterpret_cast<char *>(Buf.data()),
                        static_cast<uint32_t>(Buf.size())));
  Wasm.close();

  /// Load and validate to wasm AST
  WasmEdge_ASTModuleContext *Mod = loadModule(Conf);
  EXPECT_NE(Mod, nullptr);
  EXPECT_TRUE(validateModule(Conf, Mod));

  /// VM creation and deletion
  WasmEdge_VMContext *VM = WasmEdge_VMCreate(nullptr, nullptr);
  EXPECT_NE(VM, nullptr);
  WasmEdge_VMDelete(VM);
  EXPECT_TRUE(true);
  WasmEdge_VMDelete(nullptr);
  EXPECT_TRUE(true);
  VM = WasmEdge_VMCreate(Conf, nullptr);
  EXPECT_NE(VM, nullptr);
  WasmEdge_VMDelete(VM);
  VM = WasmEdge_VMCreate(nullptr, Store);
  EXPECT_NE(VM, nullptr);
  WasmEdge_VMDelete(VM);
  VM = WasmEdge_VMCreate(Conf, Store);
  WasmEdge_ConfigureDelete(Conf);

  /// VM register module from import
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromImport(nullptr, ImpObj)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromImport(VM, nullptr)));
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromImport(VM, ImpObj)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromImport(VM, ImpObj)));

  /// VM register module from file
  ModName = WasmEdge_StringCreateByCString("reg-wasm-file");
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRegisterModuleFromFile(nullptr, ModName, TPath)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRegisterModuleFromFile(VM, ModName, "no_file")));
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromFile(VM, ModName, TPath)));
  WasmEdge_StringDelete(ModName);

  /// VM register module from buffer
  ModName = WasmEdge_StringCreateByCString("reg-wasm-buffer");
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromBuffer(
      nullptr, ModName, Buf.data(), static_cast<uint32_t>(Buf.size()))));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRegisterModuleFromBuffer(VM, ModName, nullptr, 0)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromBuffer(
      VM, ModName, Buf.data(), static_cast<uint32_t>(Buf.size()))));
  WasmEdge_StringDelete(ModName);

  /// VM register module from AST module
  ModName = WasmEdge_StringCreateByCString("reg-wasm-ast");
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRegisterModuleFromASTModule(nullptr, ModName, Mod)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRegisterModuleFromASTModule(VM, ModName, nullptr)));
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRegisterModuleFromASTModule(VM, ModName, Mod)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRegisterModuleFromASTModule(VM, ModName, Mod)));
  WasmEdge_StringDelete(ModName);

  ModName = WasmEdge_StringCreateByCString("reg-wasm-buffer");
  ModName2 = WasmEdge_StringCreateByCString("reg-wasm-error");
  FuncName = WasmEdge_StringCreateByCString("func-mul-2");
  FuncName2 = WasmEdge_StringCreateByCString("func-mul-3");
  P[0] = WasmEdge_ValueGenI32(123);
  P[1] = WasmEdge_ValueGenI32(456);

  /// VM run wasm from file
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(nullptr, TPath, FuncName, P, 2, R, 2)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, "no_file", FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 1, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, nullptr, 0, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, nullptr, 2, R, 2)));
  /// Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, R, 2)));
  P[0] = WasmEdge_ValueGenI32(123);
  /// Function not found
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName2, P, 2, R, 1)));
  /// Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, nullptr, 1)));

  /// VM run wasm from buffer
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2, R,
      2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      nullptr, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2, R,
      2)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromBuffer(VM, nullptr, 0, FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 1, R,
      2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, nullptr, 0,
      R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, nullptr, 2,
      R, 2)));
  /// Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2, R,
      2)));
  P[0] = WasmEdge_ValueGenI32(123);
  /// Function not found
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName2, P, 2, R,
      2)));
  /// Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2, R,
      1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2,
      nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMRunWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()), FuncName, P, 2,
      nullptr, 1)));

  /// VM run wasm from AST module
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(nullptr, Mod, FuncName, P, 2, R, 2)));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, nullptr, FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 1, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, nullptr, 0, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, nullptr, 2, R, 2)));
  /// Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, R, 2)));
  P[0] = WasmEdge_ValueGenI32(123);
  /// Function not found
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName2, P, 2, R, 2)));
  /// Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, nullptr, 1)));

  /// VM load wasm from file
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromFile(VM, TPath)));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromFile(nullptr, TPath)));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromFile(VM, "file")));

  /// VM load wasm from buffer
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromBuffer(
      VM, Buf.data(), static_cast<uint32_t>(Buf.size()))));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromBuffer(
      nullptr, Buf.data(), static_cast<uint32_t>(Buf.size()))));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromBuffer(VM, nullptr, 0)));

  /// VM load wasm from AST module
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(nullptr, Mod)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, nullptr)));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(nullptr, nullptr)));

  /// VM validate
  WasmEdge_VMCleanup(VM);
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMValidate(VM)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMValidate(nullptr)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VM)));

  /// VM instantiate
  WasmEdge_VMCleanup(VM);
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VM)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VM)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VM)));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(nullptr)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VM)));

  /// VM execute
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  WasmEdge_VMCleanup(VM);
  /// Inited phase
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, R, 2)));
  /// Loaded phase
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, R, 2)));
  /// Validated phase
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VM)));
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, R, 2)));
  /// Instantiated phase
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VM)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(nullptr, FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 1, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, nullptr, 0, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, nullptr, 2, R, 2)));
  /// Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_FALSE(WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, R, 2)));
  P[0] = WasmEdge_ValueGenI32(123);
  /// Function not found
  EXPECT_FALSE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName2, P, 2, R, 2)));
  /// Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VM, FuncName, P, 2, nullptr, 1)));

  /// WasmEdge_VMExecuteRegistered
  R[0] = WasmEdge_ValueGenI32(0);
  R[1] = WasmEdge_ValueGenI32(0);
  WasmEdge_VMCleanup(VM);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  EXPECT_EQ(912, WasmEdge_ValueGetI32(R[1]));
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(nullptr, ModName, FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 1, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, nullptr, 0, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, nullptr, 2, R, 2)));
  /// Function type mismatch
  P[0] = WasmEdge_ValueGenI64(123);
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 2, R, 2)));
  P[0] = WasmEdge_ValueGenI32(123);
  /// Module not found
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName2, FuncName, P, 2, R, 2)));
  /// Function not found
  EXPECT_FALSE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName2, P, 2, R, 2)));
  /// Discard result
  R[0] = WasmEdge_ValueGenI32(0);
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, WasmEdge_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(WasmEdge_ResultOK(
      WasmEdge_VMExecuteRegistered(VM, ModName, FuncName, P, 2, nullptr, 1)));

  /// VM get function type
  WasmEdge_VMCleanup(VM);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VM)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VM)));
  FuncType = WasmEdge_VMGetFunctionType(VM, FuncName);
  EXPECT_NE(FuncType, nullptr);
  WasmEdge_FunctionTypeDelete(FuncType);
  FuncType = WasmEdge_VMGetFunctionType(nullptr, FuncName);
  EXPECT_EQ(FuncType, nullptr);
  FuncType = WasmEdge_VMGetFunctionType(VM, FuncName2);
  EXPECT_EQ(FuncType, nullptr);

  /// VM get function type registered
  FuncType = WasmEdge_VMGetFunctionTypeRegistered(VM, ModName, FuncName);
  EXPECT_NE(FuncType, nullptr);
  WasmEdge_FunctionTypeDelete(FuncType);
  FuncType = WasmEdge_VMGetFunctionTypeRegistered(nullptr, ModName, FuncName);
  EXPECT_EQ(FuncType, nullptr);
  FuncType = WasmEdge_VMGetFunctionTypeRegistered(VM, ModName2, FuncName);
  EXPECT_EQ(FuncType, nullptr);
  FuncType = WasmEdge_VMGetFunctionTypeRegistered(VM, ModName, FuncName2);
  EXPECT_EQ(FuncType, nullptr);

  WasmEdge_StringDelete(FuncName);
  WasmEdge_StringDelete(FuncName2);
  WasmEdge_StringDelete(ModName);
  WasmEdge_StringDelete(ModName2);

  /// VM get function list
  EXPECT_EQ(WasmEdge_VMGetFunctionListLength(VM), 11U);
  EXPECT_EQ(WasmEdge_VMGetFunctionListLength(nullptr), 0U);
  EXPECT_EQ(WasmEdge_VMGetFunctionList(nullptr, Names, FuncTypes, 15), 0U);
  EXPECT_EQ(WasmEdge_VMGetFunctionList(VM, nullptr, FuncTypes, 15), 11U);
  EXPECT_EQ(WasmEdge_VMGetFunctionList(VM, Names, nullptr, 15), 11U);
  for (uint32_t I = 0; I < 11; I++) {
    WasmEdge_StringDelete(Names[I]);
    WasmEdge_FunctionTypeDelete(FuncTypes[I]);
  }
  EXPECT_EQ(WasmEdge_VMGetFunctionList(VM, nullptr, nullptr, 15), 11U);
  EXPECT_EQ(WasmEdge_VMGetFunctionList(VM, Names, FuncTypes, 4), 11U);
  for (uint32_t I = 0; I < 4; I++) {
    WasmEdge_StringDelete(Names[I]);
    WasmEdge_FunctionTypeDelete(FuncTypes[I]);
  }
  EXPECT_EQ(WasmEdge_VMGetFunctionList(VM, Names, FuncTypes, 15), 11U);
  for (uint32_t I = 0; I < 11; I++) {
    WasmEdge_StringDelete(Names[I]);
    WasmEdge_FunctionTypeDelete(FuncTypes[I]);
  }

  /// VM cleanup
  WasmEdge_VMCleanup(VM);
  EXPECT_TRUE(true);
  WasmEdge_VMCleanup(nullptr);
  EXPECT_TRUE(true);

  /// VM get import module
  EXPECT_NE(
      WasmEdge_VMGetImportModuleContext(VM, WasmEdge_HostRegistration_Wasi),
      nullptr);
  EXPECT_EQ(WasmEdge_VMGetImportModuleContext(
                VM, WasmEdge_HostRegistration_WasmEdge_Process),
            nullptr);
  EXPECT_EQ(WasmEdge_VMGetImportModuleContext(nullptr,
                                              WasmEdge_HostRegistration_Wasi),
            nullptr);

  /// VM get store
  EXPECT_EQ(WasmEdge_VMGetStoreContext(VM), Store);
  EXPECT_EQ(WasmEdge_VMGetStoreContext(nullptr), nullptr);

  /// VM get statistics
  EXPECT_NE(WasmEdge_VMGetStatisticsContext(VM), nullptr);
  EXPECT_EQ(WasmEdge_VMGetStatisticsContext(nullptr), nullptr);

  WasmEdge_ASTModuleDelete(Mod);
  WasmEdge_ImportObjectDelete(ImpObj);
  WasmEdge_StoreDelete(Store);
  WasmEdge_VMDelete(VM);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
