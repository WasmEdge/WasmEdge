// SPDX-License-Identifier: Apache-2.0
#include "api/ssvm.h"

#include "gtest/gtest.h"

#include <cstdint>
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
    /// ssvmAPICoreTests
};
char *Args[] = {&ArgsVec[0], &ArgsVec[5]};
char *Envs[] = {&EnvsVec[0], &EnvsVec[10], &EnvsVec[20]};
char *Dirs[] = {&DirsVec[0]};
char *Preopens[] = {&PreopensVec[0], &PreopensVec[12], &PreopensVec[21],
                    &PreopensVec[32]};
char TPath[] = "apiTestData/test.wasm";

SSVM_Result ExternAdd(void *Data, SSVM_MemoryInstanceContext *MemCxt,
                      const SSVM_Value *In, SSVM_Value *Out) {
  /// {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(SSVM_ValueGetExternRef(In[0]));
  int32_t Val2 = SSVM_ValueGetI32(In[1]);
  Out[0] = SSVM_ValueGenI32(*Val1 + Val2);
  return SSVM_Result_Success;
}

SSVM_Result ExternSub(void *Data, SSVM_MemoryInstanceContext *MemCxt,
                      const SSVM_Value *In, SSVM_Value *Out) {
  /// {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(SSVM_ValueGetExternRef(In[0]));
  int32_t Val2 = SSVM_ValueGetI32(In[1]);
  Out[0] = SSVM_ValueGenI32(*Val1 - Val2);
  return SSVM_Result_Success;
}

SSVM_Result ExternMul(void *Data, SSVM_MemoryInstanceContext *MemCxt,
                      const SSVM_Value *In, SSVM_Value *Out) {
  /// {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(SSVM_ValueGetExternRef(In[0]));
  int32_t Val2 = SSVM_ValueGetI32(In[1]);
  Out[0] = SSVM_ValueGenI32(*Val1 * Val2);
  return SSVM_Result_Success;
}

SSVM_Result ExternDiv(void *Data, SSVM_MemoryInstanceContext *MemCxt,
                      const SSVM_Value *In, SSVM_Value *Out) {
  /// {externref, i32} -> {i32}
  int32_t *Val1 = static_cast<int32_t *>(SSVM_ValueGetExternRef(In[0]));
  int32_t Val2 = SSVM_ValueGetI32(In[1]);
  Out[0] = SSVM_ValueGenI32(*Val1 / Val2);
  return SSVM_Result_Success;
}

SSVM_Result ExternTerm(void *Data, SSVM_MemoryInstanceContext *MemCxt,
                       const SSVM_Value *In, SSVM_Value *Out) {
  /// {} -> {i32}
  Out[0] = SSVM_ValueGenI32(1234);
  return SSVM_Result_Terminate;
}

SSVM_Result ExternFail(void *Data, SSVM_MemoryInstanceContext *MemCxt,
                       const SSVM_Value *In, SSVM_Value *Out) {
  /// {} -> {i32}
  Out[0] = SSVM_ValueGenI32(5678);
  return SSVM_Result_Fail;
}

SSVM_Result ExternWrap(void *This, void *Data,
                       SSVM_MemoryInstanceContext *MemCxt, const SSVM_Value *In,
                       const uint32_t InLen, SSVM_Value *Out,
                       const uint32_t OutLen) {
  using HostFuncType = SSVM_Result(void *, SSVM_MemoryInstanceContext *,
                                   const SSVM_Value *, SSVM_Value *);
  HostFuncType *Func = reinterpret_cast<HostFuncType *>(This);
  return Func(Data, MemCxt, In, Out);
}

/// Helper function to create import module with host functions
SSVM_ImportObjectContext *createExternModule(std::string_view Name,
                                             bool IsWrap = false) {
  /// Create import object
  SSVM_String HostName = SSVM_StringCreateByCString(Name.data());
  SSVM_ImportObjectContext *ImpObj = SSVM_ImportObjectCreate(HostName, nullptr);
  SSVM_StringDelete(HostName);
  enum SSVM_ValType Param[2] = {SSVM_ValType_ExternRef, SSVM_ValType_I32},
                    Result[1] = {SSVM_ValType_I32};
  SSVM_FunctionTypeContext *HostFType =
      SSVM_FunctionTypeCreate(Param, 2, Result, 1);
  SSVM_HostFunctionContext *HostFunc = nullptr;

  /// Add host function "func-add"
  HostName = SSVM_StringCreateByCString("func-add");
  if (IsWrap) {
    HostFunc = SSVM_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternAdd), 0);
  } else {
    HostFunc = SSVM_HostFunctionCreate(HostFType, ExternAdd, 0);
  }
  SSVM_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  SSVM_StringDelete(HostName);
  /// Add host function "func-sub"
  HostName = SSVM_StringCreateByCString("func-sub");
  if (IsWrap) {
    HostFunc = SSVM_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternSub), 0);
  } else {
    HostFunc = SSVM_HostFunctionCreate(HostFType, ExternSub, 0);
  }
  SSVM_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  SSVM_StringDelete(HostName);
  /// Add host function "func-mul"
  HostName = SSVM_StringCreateByCString("func-mul");
  if (IsWrap) {
    HostFunc = SSVM_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternMul), 0);
  } else {
    HostFunc = SSVM_HostFunctionCreate(HostFType, ExternMul, 0);
  }
  SSVM_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  SSVM_StringDelete(HostName);
  /// Add host function "func-div"
  HostName = SSVM_StringCreateByCString("func-div");
  if (IsWrap) {
    HostFunc = SSVM_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternDiv), 0);
  } else {
    HostFunc = SSVM_HostFunctionCreate(HostFType, ExternDiv, 0);
  }
  SSVM_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  SSVM_StringDelete(HostName);

  SSVM_FunctionTypeDelete(HostFType);
  HostFType = SSVM_FunctionTypeCreate(nullptr, 0, Result, 1);

  /// Add host function "func-term"
  HostName = SSVM_StringCreateByCString("func-term");
  if (IsWrap) {
    HostFunc = SSVM_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternTerm), 0);
  } else {
    HostFunc = SSVM_HostFunctionCreate(HostFType, ExternTerm, 0);
  }
  SSVM_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  SSVM_StringDelete(HostName);
  /// Add host function "func-fail"
  HostName = SSVM_StringCreateByCString("func-fail");
  if (IsWrap) {
    HostFunc = SSVM_HostFunctionCreateBinding(
        HostFType, ExternWrap, reinterpret_cast<void *>(ExternFail), 0);
  } else {
    HostFunc = SSVM_HostFunctionCreate(HostFType, ExternFail, 0);
  }
  SSVM_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  SSVM_StringDelete(HostName);

  SSVM_FunctionTypeDelete(HostFType);
  return ImpObj;
}

/// Helper function to load wasm file into AST module.
SSVM_ASTModuleContext *loadModule(const SSVM_ConfigureContext *Conf) {
  SSVM_ASTModuleContext *Mod = nullptr;
  SSVM_LoaderContext *Loader = SSVM_LoaderCreate(Conf);
  SSVM_LoaderParseFromFile(Loader, &Mod, TPath);
  SSVM_LoaderDelete(Loader);
  return Mod;
}

/// Helper function to validate wasm module.
bool validateModule(const SSVM_ConfigureContext *Conf,
                    const SSVM_ASTModuleContext *Mod) {
  SSVM_ValidatorContext *Validator = SSVM_ValidatorCreate(Conf);
  SSVM_Result Res = SSVM_ValidatorValidate(Validator, Mod);
  SSVM_ValidatorDelete(Validator);
  return SSVM_ResultOK(Res);
}

/// Helper function to register and instantiate module.
bool instantiateModule(const SSVM_ConfigureContext *Conf,
                       SSVM_StoreContext *Store,
                       const SSVM_ASTModuleContext *Mod,
                       SSVM_ImportObjectContext *ImpObj) {
  SSVM_InterpreterContext *Interp = SSVM_InterpreterCreate(Conf, nullptr);
  SSVM_String Name = SSVM_StringCreateByCString("module");
  if (!SSVM_ResultOK(SSVM_InterpreterRegisterImport(Interp, Store, ImpObj))) {
    return false;
  }
  if (!SSVM_ResultOK(
          SSVM_InterpreterRegisterModule(Interp, Store, Mod, Name))) {
    return false;
  }
  if (!SSVM_ResultOK(SSVM_InterpreterInstantiate(Interp, Store, Mod))) {
    return false;
  }
  SSVM_InterpreterDelete(Interp);
  SSVM_StringDelete(Name);
  return true;
}

TEST(APICoreTest, Version) {
  EXPECT_EQ(std::string(SSVM_VERSION), std::string(SSVM_VersionGet()));
  EXPECT_EQ(static_cast<uint32_t>(SSVM_VERSION_MAJOR), SSVM_VersionGetMajor());
  EXPECT_EQ(static_cast<uint32_t>(SSVM_VERSION_MINOR), SSVM_VersionGetMinor());
  EXPECT_EQ(static_cast<uint32_t>(SSVM_VERSION_PATCH), SSVM_VersionGetPatch());
}

TEST(APICoreTest, Log) {
  SSVM_LogSetDebugLevel();
  EXPECT_TRUE(true);
  SSVM_LogSetErrorLevel();
  EXPECT_TRUE(true);
}

TEST(APICoreTest, Value) {
  std::vector<uint32_t> Vec = {1U, 2U, 3U};
  SSVM_Value Val = SSVM_ValueGenI32(INT32_MAX);
  EXPECT_EQ(SSVM_ValueGetI32(Val), INT32_MAX);
  Val = SSVM_ValueGenI64(INT64_MAX);
  EXPECT_EQ(SSVM_ValueGetI64(Val), INT64_MAX);
  Val = SSVM_ValueGenF32(1 / 0.0f);
  EXPECT_EQ(SSVM_ValueGetF32(Val), 1 / 0.0f);
  Val = SSVM_ValueGenF64(-1 / 0.0f);
  EXPECT_EQ(SSVM_ValueGetF64(Val), -1 / 0.0f);
  Val = SSVM_ValueGenV128(static_cast<__int128>(INT64_MAX) * 2 + 1);
  EXPECT_EQ(SSVM_ValueGetV128(Val), static_cast<__int128>(INT64_MAX) * 2 + 1);
  Val = SSVM_ValueGenNullRef(SSVM_RefType_FuncRef);
  EXPECT_TRUE(SSVM_ValueIsNullRef(Val));
  Val = SSVM_ValueGenFuncRef(123U);
  EXPECT_EQ(SSVM_ValueGetFuncIdx(Val), 123U);
  Val = SSVM_ValueGenExternRef(&Vec);
  EXPECT_EQ(static_cast<std::vector<uint32_t> *>(SSVM_ValueGetExternRef(Val))
                ->data()[1],
            2U);
}

TEST(APICoreTest, String) {
  /// Test to delete nullptr.
  SSVM_String Str = {.Length = 0, .Buf = nullptr};
  SSVM_StringDelete(Str);
  EXPECT_TRUE(true);
  /// Test strings.
  SSVM_String Str1 = SSVM_StringCreateByCString("test_string");
  SSVM_String Str2 = SSVM_StringCreateByCString("test_string");
  EXPECT_TRUE(SSVM_StringIsEqual(Str1, Str2));
  const char CStr[] = "test_string_.....";
  SSVM_String Str3 = SSVM_StringCreateByBuffer(CStr, 11);
  EXPECT_TRUE(SSVM_StringIsEqual(Str1, Str3));
  SSVM_String Str4 = SSVM_StringWrap(CStr, 11);
  EXPECT_TRUE(SSVM_StringIsEqual(Str3, Str4));
  SSVM_String Str5 = SSVM_StringWrap(CStr, 13);
  EXPECT_FALSE(SSVM_StringIsEqual(Str3, Str5));
  SSVM_StringDelete(Str1);
  SSVM_StringDelete(Str2);
  SSVM_StringDelete(Str3);
}

TEST(APICoreTest, Result) {
  SSVM_Result Res1 = {.Code = 0x00}; /// Success
  SSVM_Result Res2 = {.Code = 0x01}; /// Terminated -> Success
  SSVM_Result Res3 = {.Code = 0x02}; /// Failed
  EXPECT_TRUE(SSVM_ResultOK(Res1));
  EXPECT_TRUE(SSVM_ResultOK(Res2));
  EXPECT_FALSE(SSVM_ResultOK(Res3));
  EXPECT_EQ(SSVM_ResultGetCode(Res1), 0x00U);
  EXPECT_EQ(SSVM_ResultGetCode(Res2), 0x01U);
  EXPECT_EQ(SSVM_ResultGetCode(Res3), 0x02U);
  EXPECT_NE(SSVM_ResultGetMessage(Res1), nullptr);
  EXPECT_NE(SSVM_ResultGetMessage(Res2), nullptr);
  EXPECT_NE(SSVM_ResultGetMessage(Res3), nullptr);
}

TEST(APICoreTest, Configure) {
  SSVM_ConfigureContext *ConfNull = nullptr;
  SSVM_ConfigureContext *Conf = SSVM_ConfigureCreate();
  /// Tests for proposals.
  SSVM_ConfigureAddProposal(ConfNull, SSVM_Proposal_SIMD);
  SSVM_ConfigureAddProposal(Conf, SSVM_Proposal_SIMD);
  SSVM_ConfigureAddProposal(ConfNull, SSVM_Proposal_Memory64);
  SSVM_ConfigureAddProposal(Conf, SSVM_Proposal_Memory64);
  EXPECT_FALSE(SSVM_ConfigureHasProposal(ConfNull, SSVM_Proposal_SIMD));
  EXPECT_TRUE(SSVM_ConfigureHasProposal(Conf, SSVM_Proposal_SIMD));
  EXPECT_FALSE(SSVM_ConfigureHasProposal(ConfNull, SSVM_Proposal_Memory64));
  EXPECT_TRUE(SSVM_ConfigureHasProposal(Conf, SSVM_Proposal_Memory64));
  SSVM_ConfigureRemoveProposal(Conf, SSVM_Proposal_SIMD);
  SSVM_ConfigureRemoveProposal(ConfNull, SSVM_Proposal_SIMD);
  EXPECT_FALSE(SSVM_ConfigureHasProposal(ConfNull, SSVM_Proposal_SIMD));
  EXPECT_FALSE(SSVM_ConfigureHasProposal(Conf, SSVM_Proposal_SIMD));
  EXPECT_FALSE(SSVM_ConfigureHasProposal(ConfNull, SSVM_Proposal_Memory64));
  EXPECT_TRUE(SSVM_ConfigureHasProposal(Conf, SSVM_Proposal_Memory64));
  /// Tests for host registrations.
  SSVM_ConfigureAddHostRegistration(ConfNull, SSVM_HostRegistration_Wasi);
  SSVM_ConfigureAddHostRegistration(Conf, SSVM_HostRegistration_Wasi);
  EXPECT_FALSE(
      SSVM_ConfigureHasHostRegistration(ConfNull, SSVM_HostRegistration_Wasi));
  EXPECT_TRUE(
      SSVM_ConfigureHasHostRegistration(Conf, SSVM_HostRegistration_Wasi));
  SSVM_ConfigureRemoveHostRegistration(ConfNull, SSVM_HostRegistration_Wasi);
  SSVM_ConfigureRemoveHostRegistration(Conf, SSVM_HostRegistration_Wasi);
  EXPECT_FALSE(
      SSVM_ConfigureHasHostRegistration(ConfNull, SSVM_HostRegistration_Wasi));
  EXPECT_FALSE(
      SSVM_ConfigureHasHostRegistration(Conf, SSVM_HostRegistration_Wasi));
  /// Tests for memory limits.
  SSVM_ConfigureSetMaxMemoryPage(ConfNull, 1234U);
  SSVM_ConfigureSetMaxMemoryPage(Conf, 1234U);
  EXPECT_NE(SSVM_ConfigureGetMaxMemoryPage(ConfNull), 1234U);
  EXPECT_EQ(SSVM_ConfigureGetMaxMemoryPage(Conf), 1234U);
  /// Test to delete nullptr.
  SSVM_ConfigureDelete(ConfNull);
  EXPECT_TRUE(true);
  SSVM_ConfigureDelete(Conf);
  EXPECT_TRUE(true);
}

TEST(APICoreTest, Loader) {
  SSVM_ConfigureContext *Conf = SSVM_ConfigureCreate();
  SSVM_ConfigureAddProposal(Conf, SSVM_Proposal_ReferenceTypes);
  SSVM_ASTModuleContext *Mod = nullptr;
  SSVM_ASTModuleContext **ModPtr = &Mod;

  /// Loader creation and deletion
  SSVM_LoaderContext *Loader = SSVM_LoaderCreate(nullptr);
  EXPECT_NE(Loader, nullptr);
  SSVM_LoaderDelete(nullptr);
  EXPECT_TRUE(true);
  SSVM_LoaderDelete(Loader);
  EXPECT_TRUE(true);
  Loader = SSVM_LoaderCreate(Conf);

  /// Parse from file
  Mod = nullptr;
  EXPECT_TRUE(SSVM_ResultOK(SSVM_LoaderParseFromFile(Loader, ModPtr, TPath)));
  EXPECT_NE(Mod, nullptr);
  SSVM_ASTModuleDelete(Mod);
  EXPECT_FALSE(SSVM_ResultOK(SSVM_LoaderParseFromFile(nullptr, ModPtr, TPath)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_LoaderParseFromFile(Loader, nullptr, TPath)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_LoaderParseFromFile(Loader, ModPtr, "file")));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_LoaderParseFromFile(nullptr, nullptr, TPath)));

  /// Parse from buffer
  std::ifstream Wasm(TPath, std::ios::binary | std::ios::ate);
  std::vector<uint8_t> Buf(Wasm.tellg());
  Wasm.seekg(0, std::ios::beg);
  EXPECT_TRUE(Wasm.read(reinterpret_cast<char *>(Buf.data()), Buf.size()));
  Wasm.close();
  Mod = nullptr;
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_LoaderParseFromBuffer(Loader, ModPtr, Buf.data(), Buf.size())));
  EXPECT_NE(Mod, nullptr);
  SSVM_ASTModuleDelete(Mod);
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_LoaderParseFromBuffer(nullptr, ModPtr, Buf.data(), Buf.size())));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_LoaderParseFromBuffer(Loader, nullptr, Buf.data(), Buf.size())));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_LoaderParseFromBuffer(Loader, ModPtr, nullptr, 0)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_LoaderParseFromBuffer(nullptr, nullptr, Buf.data(), Buf.size())));

  /// AST module deletion
  SSVM_ASTModuleDelete(nullptr);
  EXPECT_TRUE(true);

  SSVM_LoaderDelete(Loader);
  SSVM_ConfigureDelete(Conf);
}

TEST(APICoreTest, Validator) {
  SSVM_ConfigureContext *Conf = SSVM_ConfigureCreate();
  SSVM_ConfigureAddProposal(Conf, SSVM_Proposal_ReferenceTypes);

  /// Validator creation and deletion
  SSVM_ValidatorContext *Validator = SSVM_ValidatorCreate(nullptr);
  EXPECT_NE(Validator, nullptr);
  SSVM_ValidatorDelete(nullptr);
  EXPECT_TRUE(true);
  SSVM_ValidatorDelete(Validator);
  EXPECT_TRUE(true);
  Validator = SSVM_ValidatorCreate(Conf);

  /// Load and parse file
  SSVM_ASTModuleContext *Mod = loadModule(Conf);
  EXPECT_NE(Mod, nullptr);

  /// Validation
  EXPECT_TRUE(SSVM_ResultOK(SSVM_ValidatorValidate(Validator, Mod)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_ValidatorValidate(nullptr, Mod)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_ValidatorValidate(Validator, nullptr)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_ValidatorValidate(nullptr, nullptr)));

  SSVM_ASTModuleDelete(Mod);
  SSVM_ValidatorDelete(Validator);
  SSVM_ConfigureDelete(Conf);
}

TEST(APICoreTest, InterpreterWithStatistics) {
  /// Create contexts
  SSVM_ConfigureContext *Conf = SSVM_ConfigureCreate();
  SSVM_ConfigureAddProposal(Conf, SSVM_Proposal_ReferenceTypes);
  SSVM_StoreContext *Store = SSVM_StoreCreate();

  /// Load and validate file
  SSVM_ASTModuleContext *Mod = loadModule(Conf);
  EXPECT_NE(Mod, nullptr);
  EXPECT_TRUE(validateModule(Conf, Mod));

  /// Statistics creation and deletion
  SSVM_StatisticsContext *Stat = SSVM_StatisticsCreate();
  EXPECT_NE(Stat, nullptr);
  SSVM_StatisticsDelete(Stat);
  EXPECT_TRUE(true);
  SSVM_StatisticsDelete(nullptr);
  EXPECT_TRUE(true);
  Stat = SSVM_StatisticsCreate();
  EXPECT_NE(Stat, nullptr);

  /// Statistics set cost table
  std::vector<uint64_t> CostTable(512, 20ULL);
  SSVM_StatisticsSetCostTable(nullptr, &CostTable[0], 512);
  EXPECT_TRUE(true);
  SSVM_StatisticsSetCostTable(Stat, nullptr, 0);
  EXPECT_TRUE(true);
  SSVM_StatisticsSetCostTable(Stat, &CostTable[0], 512);
  EXPECT_TRUE(true);

  /// Statistics set cost limit
  SSVM_StatisticsSetCostLimit(Stat, 100000000000000ULL);
  EXPECT_TRUE(true);
  SSVM_StatisticsSetCostLimit(nullptr, 1ULL);
  EXPECT_TRUE(true);

  /// Interpreter creation and deletion
  SSVM_InterpreterContext *Interp = SSVM_InterpreterCreate(nullptr, nullptr);
  EXPECT_NE(Interp, nullptr);
  SSVM_InterpreterDelete(Interp);
  EXPECT_TRUE(true);
  Interp = SSVM_InterpreterCreate(Conf, nullptr);
  EXPECT_NE(Interp, nullptr);
  SSVM_InterpreterDelete(Interp);
  EXPECT_TRUE(true);
  Interp = SSVM_InterpreterCreate(nullptr, Stat);
  EXPECT_NE(Interp, nullptr);
  SSVM_InterpreterDelete(Interp);
  EXPECT_TRUE(true);
  Interp = SSVM_InterpreterCreate(Conf, Stat);
  EXPECT_NE(Interp, nullptr);
  SSVM_InterpreterDelete(nullptr);
  EXPECT_TRUE(true);
  SSVM_ConfigureDelete(Conf);

  /// Register import object
  SSVM_ImportObjectContext *ImpObj = createExternModule("extern");
  EXPECT_NE(ImpObj, nullptr);
  SSVM_ImportObjectContext *ImpObjWrap =
      createExternModule("extern-wrap", true);
  EXPECT_NE(ImpObjWrap, nullptr);
  SSVM_ImportObjectContext *ImpObj2 = createExternModule("extern");
  EXPECT_NE(ImpObj2, nullptr);
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_InterpreterRegisterImport(nullptr, Store, ImpObj)));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_InterpreterRegisterImport(Interp, nullptr, ImpObj)));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_InterpreterRegisterImport(Interp, Store, nullptr)));
  EXPECT_TRUE(
      SSVM_ResultOK(SSVM_InterpreterRegisterImport(Interp, Store, ImpObj)));
  /// Name conflict
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_InterpreterRegisterImport(Interp, Store, ImpObj2)));
  EXPECT_TRUE(
      SSVM_ResultOK(SSVM_InterpreterRegisterImport(Interp, Store, ImpObjWrap)));
  SSVM_ImportObjectDelete(ImpObj2);

  /// Register wasm module
  SSVM_String ModName = SSVM_StringCreateByCString("module");
  SSVM_String ModName2 = SSVM_StringCreateByCString("extern");
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_InterpreterRegisterModule(nullptr, Store, Mod, ModName)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_InterpreterRegisterModule(Interp, nullptr, Mod, ModName)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_InterpreterRegisterModule(Interp, Store, nullptr, ModName)));
  /// Name conflict
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_InterpreterRegisterModule(Interp, Store, Mod, ModName2)));
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_InterpreterRegisterModule(Interp, Store, Mod, ModName)));
  SSVM_StringDelete(ModName);
  SSVM_StringDelete(ModName2);

  /// Instantiate wasm module
  EXPECT_FALSE(SSVM_ResultOK(SSVM_InterpreterInstantiate(nullptr, Store, Mod)));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_InterpreterInstantiate(Interp, nullptr, Mod)));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_InterpreterInstantiate(Interp, Store, nullptr)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_InterpreterInstantiate(Interp, Store, Mod)));
  /// Override instantiated wasm
  EXPECT_TRUE(SSVM_ResultOK(SSVM_InterpreterInstantiate(Interp, Store, Mod)));
  SSVM_ASTModuleDelete(Mod);

  /// Invoke functions
  SSVM_String FuncName = SSVM_StringCreateByCString("func-mul-2");
  SSVM_String FuncName2 = SSVM_StringCreateByCString("func-mul-3");
  SSVM_Value P[2], R[2];
  P[0] = SSVM_ValueGenI32(123);
  P[1] = SSVM_ValueGenI32(456);
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  EXPECT_EQ(912, SSVM_ValueGetI32(R[1]));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(nullptr, Store, FuncName, P, 2, R, 2)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, nullptr, FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, P, 1, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, nullptr, 0, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, nullptr, 2, R, 2)));
  /// Function type mismatch
  P[0] = SSVM_ValueGenI64(123);
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, P, 2, R, 2)));
  P[0] = SSVM_ValueGenI32(123);
  /// Function not found
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName2, P, 2, R, 2)));
  /// Discard result
  R[0] = SSVM_ValueGenI32(0);
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, P, 2, nullptr, 1)));
  SSVM_StringDelete(FuncName);
  SSVM_StringDelete(FuncName2);

  /// Invoke functions call to host functions
  /// Get table and set external reference
  uint32_t TestValue;
  SSVM_String TabName = SSVM_StringCreateByCString("tab-ext");
  SSVM_TableInstanceContext *TabCxt = SSVM_StoreFindTable(Store, TabName);
  EXPECT_NE(TabCxt, nullptr);
  SSVM_StringDelete(TabName);
  EXPECT_TRUE(SSVM_ResultOK(SSVM_TableInstanceSetData(
      TabCxt, SSVM_ValueGenExternRef(&TestValue), 0)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_TableInstanceSetData(
      TabCxt, SSVM_ValueGenExternRef(&TestValue), 1)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_TableInstanceSetData(
      TabCxt, SSVM_ValueGenExternRef(&TestValue), 2)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_TableInstanceSetData(
      TabCxt, SSVM_ValueGenExternRef(&TestValue), 3)));
  /// Call add: (777) + (223)
  FuncName = SSVM_StringCreateByCString("func-host-add");
  P[0] = SSVM_ValueGenI32(223);
  TestValue = 777;
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, P, 1, R, 1)));
  EXPECT_EQ(1000, SSVM_ValueGetI32(R[0]));
  SSVM_StringDelete(FuncName);
  /// Call sub: (123) - (456)
  FuncName = SSVM_StringCreateByCString("func-host-sub");
  P[0] = SSVM_ValueGenI32(456);
  TestValue = 123;
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, P, 1, R, 1)));
  EXPECT_EQ(-333, SSVM_ValueGetI32(R[0]));
  SSVM_StringDelete(FuncName);
  /// Call mul: (-30) * (-66)
  FuncName = SSVM_StringCreateByCString("func-host-mul");
  P[0] = SSVM_ValueGenI32(-66);
  TestValue = -30;
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, P, 1, R, 1)));
  EXPECT_EQ(1980, SSVM_ValueGetI32(R[0]));
  SSVM_StringDelete(FuncName);
  /// Call div: (-9999) / (1234)
  FuncName = SSVM_StringCreateByCString("func-host-div");
  P[0] = SSVM_ValueGenI32(1234);
  TestValue = -9999;
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_InterpreterInvoke(Interp, Store, FuncName, P, 1, R, 1)));
  EXPECT_EQ(-8, SSVM_ValueGetI32(R[0]));
  SSVM_StringDelete(FuncName);

  /// Invoke functions of registered module
  ModName = SSVM_StringCreateByCString("extern");
  ModName2 = SSVM_StringCreateByCString("error-name");
  FuncName = SSVM_StringCreateByCString("func-add");
  FuncName2 = SSVM_StringCreateByCString("func-add2");
  TestValue = 5000;
  P[0] = SSVM_ValueGenExternRef(&TestValue);
  P[1] = SSVM_ValueGenI32(1500);
  EXPECT_TRUE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, R, 1)));
  EXPECT_EQ(6500, SSVM_ValueGetI32(R[0]));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      nullptr, Store, ModName, FuncName, P, 2, R, 1)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, nullptr, ModName, FuncName, P, 2, R, 1)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 1, R, 1)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, nullptr, 0, R, 1)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, nullptr, 2, R, 1)));
  /// Function type mismatch
  P[1] = SSVM_ValueGenI64(1500);
  EXPECT_FALSE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, R, 1)));
  P[1] = SSVM_ValueGenI32(1500);
  /// Module not found
  EXPECT_FALSE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName2, FuncName, P, 2, R, 1)));
  /// Function not found
  EXPECT_FALSE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName2, P, 2, R, 1)));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, R, 0)));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, nullptr, 1)));
  SSVM_StringDelete(ModName);
  SSVM_StringDelete(ModName2);
  SSVM_StringDelete(FuncName);
  SSVM_StringDelete(FuncName2);

  /// Invoke host function to terminate or fail execution
  ModName = SSVM_StringCreateByCString("extern");
  FuncName = SSVM_StringCreateByCString("func-term");
  SSVM_Result Res = SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, nullptr, 0, R, 1);
  EXPECT_TRUE(SSVM_ResultOK(Res));
  SSVM_StringDelete(FuncName);
  FuncName = SSVM_StringCreateByCString("func-fail");
  Res = SSVM_InterpreterInvokeRegistered(Interp, Store, ModName, FuncName,
                                         nullptr, 0, R, 1);
  EXPECT_FALSE(SSVM_ResultOK(Res));
  EXPECT_GT(SSVM_ResultGetCode(Res), 0x01U);
  SSVM_StringDelete(FuncName);
  SSVM_StringDelete(ModName);

  /// Invoke host function with binding to functions
  ModName = SSVM_StringCreateByCString("extern-wrap");
  FuncName = SSVM_StringCreateByCString("func-sub");
  TestValue = 1234;
  P[0] = SSVM_ValueGenExternRef(&TestValue);
  P[1] = SSVM_ValueGenI32(1500);
  EXPECT_TRUE(SSVM_ResultOK(SSVM_InterpreterInvokeRegistered(
      Interp, Store, ModName, FuncName, P, 2, R, 1)));
  EXPECT_EQ(-266, SSVM_ValueGetI32(R[0]));
  SSVM_StringDelete(FuncName);
  FuncName = SSVM_StringCreateByCString("func-term");
  Res = SSVM_InterpreterInvokeRegistered(Interp, Store, ModName, FuncName,
                                         nullptr, 0, R, 1);
  EXPECT_TRUE(SSVM_ResultOK(Res));
  SSVM_StringDelete(FuncName);
  FuncName = SSVM_StringCreateByCString("func-fail");
  Res = SSVM_InterpreterInvokeRegistered(Interp, Store, ModName, FuncName,
                                         nullptr, 0, R, 1);
  EXPECT_FALSE(SSVM_ResultOK(Res));
  EXPECT_GT(SSVM_ResultGetCode(Res), 0x01U);
  SSVM_StringDelete(FuncName);
  SSVM_StringDelete(ModName);

  /// Statistics get instruction count
  EXPECT_GT(SSVM_StatisticsGetInstrCount(Stat), 0ULL);
  EXPECT_EQ(SSVM_StatisticsGetInstrCount(nullptr), 0ULL);

  /// Statistics get instruction per second
  EXPECT_GT(SSVM_StatisticsGetInstrPerSecond(Stat), 0.0);
  EXPECT_EQ(SSVM_StatisticsGetInstrPerSecond(nullptr), 0.0);

  /// Statistics get total cost
  EXPECT_GT(SSVM_StatisticsGetTotalCost(Stat), 0ULL);
  EXPECT_EQ(SSVM_StatisticsGetTotalCost(nullptr), 0ULL);

  SSVM_InterpreterDelete(Interp);
  SSVM_StoreDelete(Store);
  SSVM_StatisticsDelete(Stat);
  SSVM_ImportObjectDelete(ImpObj);
  SSVM_ImportObjectDelete(ImpObjWrap);
}

TEST(APICoreTest, Store) {
  /// Create contexts
  SSVM_ConfigureContext *Conf = SSVM_ConfigureCreate();
  SSVM_ConfigureAddProposal(Conf, SSVM_Proposal_ReferenceTypes);
  SSVM_StoreContext *Store = SSVM_StoreCreate();
  SSVM_String Names[15], ErrName, ModName[3];
  ModName[0] = SSVM_StringCreateByCString("module");
  ModName[1] = SSVM_StringCreateByCString("extern");
  ModName[2] = SSVM_StringCreateByCString("no-such-module");
  ErrName = SSVM_StringCreateByCString("invalid-instance-name");

  /// Store list exports before instantiation
  EXPECT_EQ(SSVM_StoreListFunctionLength(Store), 0U);
  EXPECT_EQ(SSVM_StoreListTableLength(Store), 0U);
  EXPECT_EQ(SSVM_StoreListMemoryLength(Store), 0U);
  EXPECT_EQ(SSVM_StoreListGlobalLength(Store), 0U);
  EXPECT_EQ(SSVM_StoreListFunctionRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(SSVM_StoreListTableRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(SSVM_StoreListMemoryRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(SSVM_StoreListGlobalRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(SSVM_StoreListModuleLength(Store), 0U);

  /// Register host module and instantiate wasm module
  SSVM_ImportObjectContext *ImpObj = createExternModule("extern");
  EXPECT_NE(ImpObj, nullptr);
  SSVM_ASTModuleContext *Mod = loadModule(Conf);
  EXPECT_NE(Mod, nullptr);
  EXPECT_TRUE(validateModule(Conf, Mod));
  EXPECT_TRUE(instantiateModule(Conf, Store, Mod, ImpObj));
  SSVM_ASTModuleDelete(Mod);
  SSVM_ConfigureDelete(Conf);

  /// Store deletion
  SSVM_StoreDelete(nullptr);
  EXPECT_TRUE(true);

  /// Store list function exports
  EXPECT_EQ(SSVM_StoreListFunctionLength(Store), 11U);
  EXPECT_EQ(SSVM_StoreListFunctionLength(nullptr), 0U);
  EXPECT_EQ(SSVM_StoreListFunction(nullptr, Names, 15), 0U);
  EXPECT_EQ(SSVM_StoreListFunction(Store, nullptr, 15), 11U);
  EXPECT_EQ(SSVM_StoreListFunction(Store, Names, 4), 11U);
  for (uint32_t I = 0; I < 4; I++) {
    SSVM_StringDelete(Names[I]);
  }
  EXPECT_EQ(SSVM_StoreListFunction(Store, Names, 15), 11U);
  /// Delete names later

  /// Store find function
  EXPECT_NE(SSVM_StoreFindFunction(Store, Names[7]), nullptr);
  EXPECT_EQ(SSVM_StoreFindFunction(nullptr, Names[7]), nullptr);
  EXPECT_EQ(SSVM_StoreFindFunction(Store, ErrName), nullptr);

  /// Function instance get function type
  SSVM_FunctionInstanceContext *FuncCxt =
      SSVM_StoreFindFunction(Store, Names[7]);
  EXPECT_NE(SSVM_FunctionInstanceGetFunctionType(FuncCxt), nullptr);
  EXPECT_EQ(SSVM_FunctionInstanceGetFunctionType(nullptr), nullptr);
  for (uint32_t I = 0; I < 11; I++) {
    SSVM_StringDelete(Names[I]);
  }

  /// Store list function exports registered
  EXPECT_EQ(SSVM_StoreListFunctionRegisteredLength(Store, ModName[0]), 11U);
  EXPECT_EQ(SSVM_StoreListFunctionRegisteredLength(Store, ModName[1]), 6U);
  EXPECT_EQ(SSVM_StoreListFunctionRegisteredLength(Store, ModName[2]), 0U);
  EXPECT_EQ(SSVM_StoreListFunctionRegisteredLength(nullptr, ModName[0]), 0U);
  EXPECT_EQ(SSVM_StoreListFunctionRegistered(nullptr, ModName[0], Names, 15),
            0U);
  EXPECT_EQ(SSVM_StoreListFunctionRegistered(Store, ModName[0], nullptr, 15),
            11U);
  EXPECT_EQ(SSVM_StoreListFunctionRegistered(Store, ModName[0], Names, 4), 11U);
  for (uint32_t I = 0; I < 4; I++) {
    SSVM_StringDelete(Names[I]);
  }
  EXPECT_EQ(SSVM_StoreListFunctionRegistered(Store, ModName[0], Names, 15),
            11U);
  /// Delete names later

  /// Store find function registered
  EXPECT_NE(SSVM_StoreFindFunctionRegistered(Store, ModName[0], Names[7]),
            nullptr);
  EXPECT_EQ(SSVM_StoreFindFunctionRegistered(nullptr, ModName[0], Names[7]),
            nullptr);
  EXPECT_EQ(SSVM_StoreFindFunctionRegistered(Store, ModName[0], ErrName),
            nullptr);
  EXPECT_EQ(SSVM_StoreFindFunctionRegistered(Store, ModName[2], Names[7]),
            nullptr);
  for (uint32_t I = 0; I < 11; I++) {
    SSVM_StringDelete(Names[I]);
  }

  /// Store list table exports
  EXPECT_EQ(SSVM_StoreListTableLength(Store), 2U);
  EXPECT_EQ(SSVM_StoreListTableLength(nullptr), 0U);
  EXPECT_EQ(SSVM_StoreListTable(nullptr, Names, 15), 0U);
  EXPECT_EQ(SSVM_StoreListTable(Store, nullptr, 15), 2U);
  EXPECT_EQ(SSVM_StoreListTable(Store, Names, 1), 2U);
  SSVM_StringDelete(Names[0]);
  EXPECT_EQ(SSVM_StoreListTable(Store, Names, 15), 2U);
  /// Delete names later

  /// Store find table
  EXPECT_NE(SSVM_StoreFindTable(Store, Names[1]), nullptr);
  EXPECT_EQ(SSVM_StoreFindTable(nullptr, Names[1]), nullptr);
  EXPECT_EQ(SSVM_StoreFindTable(Store, ErrName), nullptr);
  SSVM_StringDelete(Names[0]);
  SSVM_StringDelete(Names[1]);

  /// Store list table exports registered
  EXPECT_EQ(SSVM_StoreListTableRegisteredLength(Store, ModName[0]), 2U);
  EXPECT_EQ(SSVM_StoreListTableRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(SSVM_StoreListTableRegisteredLength(Store, ModName[2]), 0U);
  EXPECT_EQ(SSVM_StoreListTableRegisteredLength(nullptr, ModName[0]), 0U);
  EXPECT_EQ(SSVM_StoreListTableRegistered(nullptr, ModName[0], Names, 15), 0U);
  EXPECT_EQ(SSVM_StoreListTableRegistered(Store, ModName[0], nullptr, 15), 2U);
  EXPECT_EQ(SSVM_StoreListTableRegistered(Store, ModName[0], Names, 1), 2U);
  SSVM_StringDelete(Names[0]);
  EXPECT_EQ(SSVM_StoreListTableRegistered(Store, ModName[0], Names, 15), 2U);
  /// Delete names later

  /// Store find table registered
  EXPECT_NE(SSVM_StoreFindTableRegistered(Store, ModName[0], Names[1]),
            nullptr);
  EXPECT_EQ(SSVM_StoreFindTableRegistered(nullptr, ModName[0], Names[1]),
            nullptr);
  EXPECT_EQ(SSVM_StoreFindTableRegistered(Store, ModName[0], ErrName), nullptr);
  EXPECT_EQ(SSVM_StoreFindTableRegistered(Store, ModName[2], Names[1]),
            nullptr);
  SSVM_StringDelete(Names[0]);
  SSVM_StringDelete(Names[1]);

  /// Store list memory exports
  EXPECT_EQ(SSVM_StoreListMemoryLength(Store), 1U);
  EXPECT_EQ(SSVM_StoreListMemoryLength(nullptr), 0U);
  EXPECT_EQ(SSVM_StoreListMemory(nullptr, Names, 15), 0U);
  EXPECT_EQ(SSVM_StoreListMemory(Store, nullptr, 15), 1U);
  EXPECT_EQ(SSVM_StoreListMemory(Store, Names, 0), 1U);
  EXPECT_EQ(SSVM_StoreListMemory(Store, Names, 15), 1U);
  /// Delete names later

  /// Store find memory
  EXPECT_NE(SSVM_StoreFindMemory(Store, Names[0]), nullptr);
  EXPECT_EQ(SSVM_StoreFindMemory(nullptr, Names[0]), nullptr);
  EXPECT_EQ(SSVM_StoreFindMemory(Store, ErrName), nullptr);
  SSVM_StringDelete(Names[0]);

  /// Store list memory exports registered
  EXPECT_EQ(SSVM_StoreListMemoryRegisteredLength(Store, ModName[0]), 1U);
  EXPECT_EQ(SSVM_StoreListMemoryRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(SSVM_StoreListMemoryRegisteredLength(Store, ModName[2]), 0U);
  EXPECT_EQ(SSVM_StoreListMemoryRegisteredLength(nullptr, ModName[0]), 0U);
  EXPECT_EQ(SSVM_StoreListMemoryRegistered(nullptr, ModName[0], Names, 15), 0U);
  EXPECT_EQ(SSVM_StoreListMemoryRegistered(Store, ModName[0], nullptr, 15), 1U);
  EXPECT_EQ(SSVM_StoreListMemoryRegistered(Store, ModName[0], Names, 0), 1U);
  EXPECT_EQ(SSVM_StoreListMemoryRegistered(Store, ModName[0], Names, 15), 1U);
  /// Delete names later

  /// Store find memory registered
  EXPECT_NE(SSVM_StoreFindMemoryRegistered(Store, ModName[0], Names[0]),
            nullptr);
  EXPECT_EQ(SSVM_StoreFindMemoryRegistered(nullptr, ModName[0], Names[0]),
            nullptr);
  EXPECT_EQ(SSVM_StoreFindMemoryRegistered(Store, ModName[0], ErrName),
            nullptr);
  EXPECT_EQ(SSVM_StoreFindMemoryRegistered(Store, ModName[2], Names[0]),
            nullptr);
  SSVM_StringDelete(Names[0]);

  /// Store list global exports
  EXPECT_EQ(SSVM_StoreListGlobalLength(Store), 2U);
  EXPECT_EQ(SSVM_StoreListGlobalLength(nullptr), 0U);
  EXPECT_EQ(SSVM_StoreListGlobal(nullptr, Names, 15), 0U);
  EXPECT_EQ(SSVM_StoreListGlobal(Store, nullptr, 15), 2U);
  EXPECT_EQ(SSVM_StoreListGlobal(Store, Names, 1), 2U);
  SSVM_StringDelete(Names[0]);
  EXPECT_EQ(SSVM_StoreListGlobal(Store, Names, 15), 2U);
  /// Delete names later

  /// Store find global
  EXPECT_NE(SSVM_StoreFindGlobal(Store, Names[1]), nullptr);
  EXPECT_EQ(SSVM_StoreFindGlobal(nullptr, Names[1]), nullptr);
  EXPECT_EQ(SSVM_StoreFindGlobal(Store, ErrName), nullptr);
  SSVM_StringDelete(Names[0]);
  SSVM_StringDelete(Names[1]);

  /// Store list global exports registered
  EXPECT_EQ(SSVM_StoreListGlobalRegisteredLength(Store, ModName[0]), 2U);
  EXPECT_EQ(SSVM_StoreListGlobalRegisteredLength(Store, ModName[1]), 0U);
  EXPECT_EQ(SSVM_StoreListGlobalRegisteredLength(Store, ModName[2]), 0U);
  EXPECT_EQ(SSVM_StoreListGlobalRegisteredLength(nullptr, ModName[0]), 0U);
  EXPECT_EQ(SSVM_StoreListGlobalRegistered(nullptr, ModName[0], Names, 15), 0U);
  EXPECT_EQ(SSVM_StoreListGlobalRegistered(Store, ModName[0], nullptr, 15), 2U);
  EXPECT_EQ(SSVM_StoreListGlobalRegistered(Store, ModName[0], Names, 1), 2U);
  SSVM_StringDelete(Names[0]);
  EXPECT_EQ(SSVM_StoreListGlobalRegistered(Store, ModName[0], Names, 15), 2U);
  /// Delete names later

  /// Store find global registered
  EXPECT_NE(SSVM_StoreFindGlobalRegistered(Store, ModName[0], Names[1]),
            nullptr);
  EXPECT_EQ(SSVM_StoreFindGlobalRegistered(nullptr, ModName[0], Names[1]),
            nullptr);
  EXPECT_EQ(SSVM_StoreFindGlobalRegistered(Store, ModName[0], ErrName),
            nullptr);
  EXPECT_EQ(SSVM_StoreFindGlobalRegistered(Store, ModName[2], Names[1]),
            nullptr);
  SSVM_StringDelete(Names[0]);
  SSVM_StringDelete(Names[1]);

  /// Store list module
  EXPECT_EQ(SSVM_StoreListModuleLength(Store), 2U);
  EXPECT_EQ(SSVM_StoreListModuleLength(nullptr), 0U);
  EXPECT_EQ(SSVM_StoreListModule(nullptr, Names, 15), 0U);
  EXPECT_EQ(SSVM_StoreListModule(Store, nullptr, 15), 2U);
  EXPECT_EQ(SSVM_StoreListModule(Store, Names, 1), 2U);
  SSVM_StringDelete(Names[0]);
  EXPECT_EQ(SSVM_StoreListModule(Store, Names, 15), 2U);
  SSVM_StringDelete(Names[0]);
  SSVM_StringDelete(Names[1]);

  SSVM_StringDelete(ModName[0]);
  SSVM_StringDelete(ModName[1]);
  SSVM_StringDelete(ModName[2]);
  SSVM_StoreDelete(Store);
  SSVM_ImportObjectDelete(ImpObj);
}

TEST(APICoreTest, FunctionType) {
  std::vector<SSVM_ValType> Param = {SSVM_ValType_I32,       SSVM_ValType_I64,
                                     SSVM_ValType_ExternRef, SSVM_ValType_V128,
                                     SSVM_ValType_F64,       SSVM_ValType_F32};
  std::vector<SSVM_ValType> Result = {
      SSVM_ValType_FuncRef, SSVM_ValType_ExternRef, SSVM_ValType_V128};
  enum SSVM_ValType Buf1[6], Buf2[2];
  SSVM_FunctionTypeContext *FType =
      SSVM_FunctionTypeCreate(&Param[0], 6, &Result[0], 3);
  EXPECT_EQ(SSVM_FunctionTypeGetParametersLength(FType), 6U);
  EXPECT_EQ(SSVM_FunctionTypeGetParametersLength(nullptr), 0U);
  EXPECT_EQ(SSVM_FunctionTypeGetReturnsLength(FType), 3U);
  EXPECT_EQ(SSVM_FunctionTypeGetReturnsLength(nullptr), 0U);
  EXPECT_EQ(SSVM_FunctionTypeGetParameters(FType, Buf1, 6), 6U);
  EXPECT_EQ(Param, std::vector<SSVM_ValType>(Buf1, Buf1 + 6));
  EXPECT_EQ(SSVM_FunctionTypeGetParameters(FType, Buf2, 2), 6U);
  EXPECT_EQ(std::vector<SSVM_ValType>(Param.cbegin(), Param.cbegin() + 2),
            std::vector<SSVM_ValType>(Buf2, Buf2 + 2));
  EXPECT_EQ(SSVM_FunctionTypeGetParameters(nullptr, Buf1, 6), 0U);
  EXPECT_EQ(SSVM_FunctionTypeGetReturns(FType, Buf1, 6), 3U);
  EXPECT_EQ(Result, std::vector<SSVM_ValType>(Buf1, Buf1 + 3));
  EXPECT_EQ(SSVM_FunctionTypeGetReturns(FType, Buf2, 2), 3U);
  EXPECT_EQ(std::vector<SSVM_ValType>(Result.cbegin(), Result.cbegin() + 2),
            std::vector<SSVM_ValType>(Buf2, Buf2 + 2));
  EXPECT_EQ(SSVM_FunctionTypeGetReturns(nullptr, Buf1, 6), 0U);
  SSVM_FunctionTypeDelete(FType);

  FType = SSVM_FunctionTypeCreate(nullptr, 0, nullptr, 0);
  EXPECT_EQ(SSVM_FunctionTypeGetParameters(FType, Buf1, 6), 0U);
  EXPECT_EQ(SSVM_FunctionTypeGetReturns(FType, Buf1, 6), 0U);
  SSVM_FunctionTypeDelete(FType);
}

TEST(APICoreTest, Instance) {
  SSVM_Value Val, TmpVal;
  /// SSVM_FunctionInstanceGetFunctionType() tested in `Store` test case.

  /// Table instance creation
  SSVM_TableInstanceContext *TabCxt = SSVM_TableInstanceCreate(
      SSVM_RefType_ExternRef, SSVM_Limit{.HasMax = false, .Min = 10});
  EXPECT_NE(TabCxt, nullptr);
  SSVM_TableInstanceDelete(TabCxt);
  EXPECT_TRUE(true);
  TabCxt = SSVM_TableInstanceCreate(
      SSVM_RefType_ExternRef, SSVM_Limit{.HasMax = true, .Min = 10, .Max = 20});
  EXPECT_NE(TabCxt, nullptr);

  /// Table instance get reference type
  EXPECT_EQ(SSVM_TableInstanceGetRefType(TabCxt), SSVM_RefType_ExternRef);
  EXPECT_EQ(SSVM_TableInstanceGetRefType(nullptr), SSVM_RefType_FuncRef);

  /// Table instance set data
  Val = SSVM_ValueGenExternRef(&TabCxt);
  TmpVal = SSVM_ValueGenFuncRef(2);
  EXPECT_TRUE(SSVM_ResultOK(SSVM_TableInstanceSetData(TabCxt, Val, 5)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_TableInstanceSetData(TabCxt, TmpVal, 6)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_TableInstanceSetData(nullptr, Val, 5)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_TableInstanceSetData(TabCxt, Val, 15)));

  /// Table instance get data
  Val = SSVM_ValueGenI32(0);
  EXPECT_TRUE(SSVM_ResultOK(SSVM_TableInstanceGetData(TabCxt, &Val, 5)));
  EXPECT_EQ(reinterpret_cast<SSVM_TableInstanceContext **>(
                SSVM_ValueGetExternRef(Val)),
            &TabCxt);
  EXPECT_FALSE(SSVM_ResultOK(SSVM_TableInstanceGetData(nullptr, &Val, 5)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_TableInstanceGetData(TabCxt, &Val, 15)));

  /// Table instance get size and grow
  EXPECT_EQ(SSVM_TableInstanceGetSize(TabCxt), 10U);
  EXPECT_EQ(SSVM_TableInstanceGetSize(nullptr), 0U);
  EXPECT_FALSE(SSVM_ResultOK(SSVM_TableInstanceGrow(nullptr, 8)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_TableInstanceGrow(TabCxt, 8)));
  EXPECT_EQ(SSVM_TableInstanceGetSize(TabCxt), 18U);
  EXPECT_FALSE(SSVM_ResultOK(SSVM_TableInstanceGrow(TabCxt, 8)));
  EXPECT_EQ(SSVM_TableInstanceGetSize(TabCxt), 18U);
  Val = SSVM_ValueGenExternRef(&TabCxt);
  EXPECT_TRUE(SSVM_ResultOK(SSVM_TableInstanceSetData(TabCxt, Val, 15)));
  Val = SSVM_ValueGenI32(0);
  EXPECT_TRUE(SSVM_ResultOK(SSVM_TableInstanceGetData(TabCxt, &Val, 15)));
  EXPECT_EQ(reinterpret_cast<SSVM_TableInstanceContext **>(
                SSVM_ValueGetExternRef(Val)),
            &TabCxt);

  /// Table instance deletion
  SSVM_TableInstanceDelete(nullptr);
  EXPECT_TRUE(true);
  SSVM_TableInstanceDelete(TabCxt);
  EXPECT_TRUE(true);

  /// Memory instance creation
  SSVM_MemoryInstanceContext *MemCxt =
      SSVM_MemoryInstanceCreate(SSVM_Limit{.HasMax = false, .Min = 1});
  EXPECT_NE(MemCxt, nullptr);
  SSVM_MemoryInstanceDelete(MemCxt);
  EXPECT_TRUE(true);
  MemCxt =
      SSVM_MemoryInstanceCreate(SSVM_Limit{.HasMax = true, .Min = 1, .Max = 3});
  EXPECT_NE(MemCxt, nullptr);

  /// Memory instance set data
  std::vector<uint8_t> DataSet = {'t', 'e', 's', 't', ' ',
                                  'd', 'a', 't', 'a', '\n'};
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_MemoryInstanceSetData(MemCxt, DataSet.data(), 100, 10)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_MemoryInstanceSetData(nullptr, DataSet.data(), 100, 10)));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_MemoryInstanceSetData(MemCxt, nullptr, 100, 0)));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_MemoryInstanceSetData(nullptr, nullptr, 100, 0)));
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_MemoryInstanceSetData(MemCxt, DataSet.data(), 100, 0)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_MemoryInstanceSetData(MemCxt, DataSet.data(), 65536, 10)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_MemoryInstanceSetData(MemCxt, DataSet.data(), 65530, 10)));

  /// Memory instance get data
  std::vector<uint8_t> DataGet;
  DataGet.resize(10);
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_MemoryInstanceGetData(MemCxt, DataGet.data(), 100, 10)));
  EXPECT_EQ(DataGet, DataSet);
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_MemoryInstanceGetData(nullptr, DataGet.data(), 100, 10)));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_MemoryInstanceGetData(MemCxt, nullptr, 100, 0)));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_MemoryInstanceGetData(nullptr, nullptr, 100, 0)));
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_MemoryInstanceGetData(MemCxt, DataGet.data(), 100, 0)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_MemoryInstanceGetData(MemCxt, DataGet.data(), 65536, 10)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_MemoryInstanceGetData(MemCxt, DataGet.data(), 65530, 10)));

  /// Memory instance get size and grow
  EXPECT_EQ(SSVM_MemoryInstanceGetPageSize(MemCxt), 1U);
  EXPECT_EQ(SSVM_MemoryInstanceGetPageSize(nullptr), 0U);
  EXPECT_FALSE(SSVM_ResultOK(SSVM_MemoryInstanceGrowPage(nullptr, 1)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_MemoryInstanceGrowPage(MemCxt, 1)));
  EXPECT_EQ(SSVM_MemoryInstanceGetPageSize(MemCxt), 2U);
  EXPECT_FALSE(SSVM_ResultOK(SSVM_MemoryInstanceGrowPage(MemCxt, 2)));
  EXPECT_EQ(SSVM_MemoryInstanceGetPageSize(MemCxt), 2U);
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_MemoryInstanceSetData(MemCxt, DataSet.data(), 70000, 10)));
  DataGet.clear();
  DataGet.resize(10);
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_MemoryInstanceGetData(MemCxt, DataGet.data(), 70000, 10)));
  EXPECT_EQ(DataGet, DataSet);

  /// Memory instance deletion
  SSVM_MemoryInstanceDelete(nullptr);
  EXPECT_TRUE(true);
  SSVM_MemoryInstanceDelete(MemCxt);
  EXPECT_TRUE(true);

  /// Global instance creation
  SSVM_GlobalInstanceContext *GlobCCxt = SSVM_GlobalInstanceCreate(
      SSVM_ValueGenI64(55555555555LL), SSVM_Mutability_Const);
  SSVM_GlobalInstanceContext *GlobVCxt = SSVM_GlobalInstanceCreate(
      SSVM_ValueGenI64(66666666666LL), SSVM_Mutability_Var);
  EXPECT_NE(GlobCCxt, nullptr);
  EXPECT_NE(GlobVCxt, nullptr);

  /// Global instance get value type
  EXPECT_EQ(SSVM_GlobalInstanceGetValType(GlobCCxt), SSVM_ValType_I64);
  EXPECT_EQ(SSVM_GlobalInstanceGetValType(GlobVCxt), SSVM_ValType_I64);
  EXPECT_EQ(SSVM_GlobalInstanceGetValType(nullptr), SSVM_ValType_I32);

  /// Global instance get mutability
  EXPECT_EQ(SSVM_GlobalInstanceGetMutability(GlobCCxt), SSVM_Mutability_Const);
  EXPECT_EQ(SSVM_GlobalInstanceGetMutability(GlobVCxt), SSVM_Mutability_Var);
  EXPECT_EQ(SSVM_GlobalInstanceGetMutability(nullptr), SSVM_Mutability_Const);

  /// Global instance get value
  Val = SSVM_GlobalInstanceGetValue(GlobCCxt);
  EXPECT_EQ(SSVM_ValueGetI64(Val), 55555555555LL);
  Val = SSVM_GlobalInstanceGetValue(GlobVCxt);
  EXPECT_EQ(SSVM_ValueGetI64(Val), 66666666666LL);
  Val = SSVM_GlobalInstanceGetValue(nullptr);
  EXPECT_EQ(SSVM_ValueGetI64(Val), 0LL);

  /// Global instance set value
  Val = SSVM_ValueGenI64(77777777777LL);
  SSVM_GlobalInstanceSetValue(GlobCCxt, Val);
  Val = SSVM_GlobalInstanceGetValue(GlobCCxt);
  EXPECT_EQ(SSVM_ValueGetI64(Val), 55555555555LL);
  Val = SSVM_ValueGenI64(88888888888LL);
  SSVM_GlobalInstanceSetValue(GlobVCxt, Val);
  Val = SSVM_GlobalInstanceGetValue(GlobVCxt);
  EXPECT_EQ(SSVM_ValueGetI64(Val), 88888888888LL);
  Val = SSVM_ValueGenF32(12.345);
  SSVM_GlobalInstanceSetValue(GlobVCxt, Val);
  Val = SSVM_GlobalInstanceGetValue(GlobVCxt);
  EXPECT_EQ(SSVM_ValueGetI64(Val), 88888888888LL);
  SSVM_GlobalInstanceSetValue(nullptr, Val);
  EXPECT_TRUE(true);

  /// Global instance deletion
  SSVM_GlobalInstanceDelete(nullptr);
  EXPECT_TRUE(true);
  SSVM_GlobalInstanceDelete(GlobCCxt);
  EXPECT_TRUE(true);
  SSVM_GlobalInstanceDelete(GlobVCxt);
  EXPECT_TRUE(true);
}

TEST(APICoreTest, ImportObject) {
  SSVM_String HostName = SSVM_StringCreateByCString("extern");
  SSVM_ImportObjectContext *ImpObj = nullptr;
  SSVM_FunctionTypeContext *HostFType = nullptr;
  SSVM_HostFunctionContext *HostFunc = nullptr;
  SSVM_TableInstanceContext *HostTable = nullptr;
  SSVM_MemoryInstanceContext *HostMemory = nullptr;
  SSVM_GlobalInstanceContext *HostGlobal = nullptr;
  enum SSVM_ValType Param[2], Result[1];

  /// Create import object with name ""
  ImpObj = SSVM_ImportObjectCreate({.Length = 0, .Buf = nullptr}, nullptr);
  EXPECT_NE(ImpObj, nullptr);
  SSVM_ImportObjectDelete(ImpObj);

  /// Create import object with name "extern"
  ImpObj = SSVM_ImportObjectCreate(HostName, nullptr);
  SSVM_StringDelete(HostName);
  EXPECT_NE(ImpObj, nullptr);

  /// Add host function "func-add": {externref, i32} -> {i32}
  Param[0] = SSVM_ValType_ExternRef;
  Param[1] = SSVM_ValType_I32;
  Result[0] = SSVM_ValType_I32;
  HostFType = SSVM_FunctionTypeCreate(Param, 2, Result, 1);
  HostFunc = SSVM_HostFunctionCreate(nullptr, ExternAdd, 0);
  EXPECT_EQ(HostFunc, nullptr);
  HostFunc = SSVM_HostFunctionCreate(HostFType, nullptr, 0);
  EXPECT_EQ(HostFunc, nullptr);
  HostFunc = SSVM_HostFunctionCreate(HostFType, ExternAdd, 0);
  EXPECT_NE(HostFunc, nullptr);
  SSVM_HostFunctionDelete(HostFunc);
  EXPECT_TRUE(true);
  HostFunc = SSVM_HostFunctionCreate(HostFType, ExternAdd, 0);
  EXPECT_NE(HostFunc, nullptr);
  HostName = SSVM_StringCreateByCString("func-add");
  SSVM_ImportObjectAddHostFunction(nullptr, HostName, HostFunc);
  EXPECT_TRUE(true);
  SSVM_ImportObjectAddHostFunction(ImpObj, HostName, nullptr);
  EXPECT_TRUE(true);
  SSVM_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  EXPECT_TRUE(true);
  SSVM_StringDelete(HostName);

  /// Add host function for binding "func-add-binding"
  HostFunc = SSVM_HostFunctionCreateBinding(
      nullptr, ExternWrap, reinterpret_cast<void *>(ExternAdd), 0);
  EXPECT_EQ(HostFunc, nullptr);
  HostFunc = SSVM_HostFunctionCreateBinding(
      HostFType, nullptr, reinterpret_cast<void *>(ExternAdd), 0);
  EXPECT_EQ(HostFunc, nullptr);
  HostFunc = SSVM_HostFunctionCreateBinding(
      HostFType, ExternWrap, reinterpret_cast<void *>(ExternAdd), 0);
  EXPECT_NE(HostFunc, nullptr);
  SSVM_HostFunctionDelete(HostFunc);
  SSVM_FunctionTypeDelete(HostFType);

  /// Add host table "table"
  SSVM_Limit TabLimit = {.HasMax = true, .Min = 10, .Max = 20};
  HostTable = SSVM_TableInstanceCreate(SSVM_RefType_FuncRef, TabLimit);
  HostName = SSVM_StringCreateByCString("table");
  SSVM_ImportObjectAddTable(nullptr, HostName, HostTable);
  EXPECT_TRUE(true);
  SSVM_ImportObjectAddTable(ImpObj, HostName, nullptr);
  EXPECT_TRUE(true);
  SSVM_ImportObjectAddTable(ImpObj, HostName, HostTable);
  EXPECT_TRUE(true);
  SSVM_StringDelete(HostName);

  /// Add host memory "memory"
  SSVM_Limit MemLimit = {.HasMax = true, .Min = 1, .Max = 2};
  HostMemory = SSVM_MemoryInstanceCreate(MemLimit);
  HostName = SSVM_StringCreateByCString("memory");
  SSVM_ImportObjectAddMemory(nullptr, HostName, HostMemory);
  EXPECT_TRUE(true);
  SSVM_ImportObjectAddMemory(ImpObj, HostName, nullptr);
  EXPECT_TRUE(true);
  SSVM_ImportObjectAddMemory(ImpObj, HostName, HostMemory);
  EXPECT_TRUE(true);
  SSVM_StringDelete(HostName);

  /// Add host global "global_i32": const 666
  HostGlobal =
      SSVM_GlobalInstanceCreate(SSVM_ValueGenI32(666), SSVM_Mutability_Const);
  HostName = SSVM_StringCreateByCString("global_i32");
  SSVM_ImportObjectAddGlobal(nullptr, HostName, HostGlobal);
  EXPECT_TRUE(true);
  SSVM_ImportObjectAddGlobal(ImpObj, HostName, nullptr);
  EXPECT_TRUE(true);
  SSVM_ImportObjectAddGlobal(ImpObj, HostName, HostGlobal);
  EXPECT_TRUE(true);
  SSVM_StringDelete(HostName);

  SSVM_ImportObjectDelete(ImpObj);

  /// Create WASI.
  ImpObj = SSVM_ImportObjectCreateWASI(Args, 2, Envs, 3, Dirs, 1, Preopens, 4);
  EXPECT_NE(ImpObj, nullptr);
  SSVM_ImportObjectDelete(ImpObj);
  ImpObj = SSVM_ImportObjectCreateWASI(nullptr, 0, nullptr, 0, nullptr, 0,
                                       nullptr, 0);
  EXPECT_NE(ImpObj, nullptr);
  SSVM_ImportObjectDelete(ImpObj);
  ImpObj = SSVM_ImportObjectCreateWASI(Args, 0, Envs, 3, Dirs, 1, Preopens, 4);
  EXPECT_NE(ImpObj, nullptr);
  SSVM_ImportObjectDelete(ImpObj);

  /// Initialize WASI in VM.
  SSVM_ConfigureContext *Conf = SSVM_ConfigureCreate();
  SSVM_ConfigureAddHostRegistration(Conf, SSVM_HostRegistration_Wasi);
  SSVM_VMContext *VM = SSVM_VMCreate(Conf, nullptr);
  SSVM_ConfigureDelete(Conf);
  ImpObj = SSVM_VMGetImportModuleContext(VM, SSVM_HostRegistration_Wasi);
  EXPECT_NE(ImpObj, nullptr);
  SSVM_ImportObjectInitWASI(nullptr, Args, 2, Envs, 3, Dirs, 1, Preopens, 4);
  EXPECT_TRUE(true);
  SSVM_ImportObjectInitWASI(ImpObj, Args, 2, Envs, 3, Dirs, 1, Preopens, 4);
  EXPECT_TRUE(true);
  SSVM_VMDelete(VM);
}

TEST(APICoreTest, VM) {
  SSVM_ConfigureContext *Conf = SSVM_ConfigureCreate();
  SSVM_ConfigureAddProposal(Conf, SSVM_Proposal_ReferenceTypes);
  SSVM_ConfigureAddHostRegistration(Conf, SSVM_HostRegistration_Wasi);
  SSVM_StoreContext *Store = SSVM_StoreCreate();
  SSVM_ImportObjectContext *ImpObj = createExternModule("extern");
  SSVM_String ModName, ModName2, FuncName, FuncName2, Names[15];
  SSVM_FunctionTypeContext *FuncType = nullptr;
  SSVM_Value P[10], R[10];
  SSVM_FunctionTypeContext *FuncTypes[15];

  /// WASM from file
  std::ifstream Wasm(TPath, std::ios::binary | std::ios::ate);
  std::vector<uint8_t> Buf(Wasm.tellg());
  Wasm.seekg(0, std::ios::beg);
  EXPECT_TRUE(Wasm.read(reinterpret_cast<char *>(Buf.data()), Buf.size()));
  Wasm.close();

  /// Load and validate to wasm AST
  SSVM_ASTModuleContext *Mod = loadModule(Conf);
  EXPECT_NE(Mod, nullptr);
  EXPECT_TRUE(validateModule(Conf, Mod));

  /// VM creation and deletion
  SSVM_VMContext *VM = SSVM_VMCreate(nullptr, nullptr);
  EXPECT_NE(VM, nullptr);
  SSVM_VMDelete(VM);
  EXPECT_TRUE(true);
  SSVM_VMDelete(nullptr);
  EXPECT_TRUE(true);
  VM = SSVM_VMCreate(Conf, nullptr);
  EXPECT_NE(VM, nullptr);
  SSVM_VMDelete(VM);
  VM = SSVM_VMCreate(nullptr, Store);
  EXPECT_NE(VM, nullptr);
  SSVM_VMDelete(VM);
  VM = SSVM_VMCreate(Conf, Store);
  SSVM_ConfigureDelete(Conf);

  /// VM register module from import
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMRegisterModuleFromImport(nullptr, ImpObj)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMRegisterModuleFromImport(VM, nullptr)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMRegisterModuleFromImport(VM, ImpObj)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMRegisterModuleFromImport(VM, ImpObj)));

  /// VM register module from file
  ModName = SSVM_StringCreateByCString("reg-wasm-file");
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_VMRegisterModuleFromFile(nullptr, ModName, TPath)));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_VMRegisterModuleFromFile(VM, ModName, "no_file")));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMRegisterModuleFromFile(VM, ModName, TPath)));
  SSVM_StringDelete(ModName);

  /// VM register module from buffer
  ModName = SSVM_StringCreateByCString("reg-wasm-buffer");
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMRegisterModuleFromBuffer(
      nullptr, ModName, Buf.data(), Buf.size())));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_VMRegisterModuleFromBuffer(VM, ModName, nullptr, 0)));
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_VMRegisterModuleFromBuffer(VM, ModName, Buf.data(), Buf.size())));
  SSVM_StringDelete(ModName);

  /// VM register module from AST module
  ModName = SSVM_StringCreateByCString("reg-wasm-ast");
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_VMRegisterModuleFromASTModule(nullptr, ModName, Mod)));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_VMRegisterModuleFromASTModule(VM, ModName, nullptr)));
  EXPECT_TRUE(
      SSVM_ResultOK(SSVM_VMRegisterModuleFromASTModule(VM, ModName, Mod)));
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_VMRegisterModuleFromASTModule(VM, ModName, Mod)));
  SSVM_StringDelete(ModName);

  ModName = SSVM_StringCreateByCString("reg-wasm-buffer");
  ModName2 = SSVM_StringCreateByCString("reg-wasm-error");
  FuncName = SSVM_StringCreateByCString("func-mul-2");
  FuncName2 = SSVM_StringCreateByCString("func-mul-3");
  P[0] = SSVM_ValueGenI32(123);
  P[1] = SSVM_ValueGenI32(456);

  /// VM run wasm from file
  R[0] = SSVM_ValueGenI32(0);
  R[1] = SSVM_ValueGenI32(0);
  EXPECT_TRUE(
      SSVM_ResultOK(SSVM_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  EXPECT_EQ(912, SSVM_ValueGetI32(R[1]));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromFile(nullptr, TPath, FuncName, P, 2, R, 2)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromFile(VM, "no_file", FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_VMRunWasmFromFile(VM, TPath, FuncName, P, 1, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromFile(VM, TPath, FuncName, nullptr, 0, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromFile(VM, TPath, FuncName, nullptr, 2, R, 2)));
  /// Function type mismatch
  P[0] = SSVM_ValueGenI64(123);
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, R, 2)));
  P[0] = SSVM_ValueGenI32(123);
  /// Function not found
  EXPECT_FALSE(
      SSVM_ResultOK(SSVM_VMRunWasmFromFile(VM, TPath, FuncName2, P, 2, R, 1)));
  /// Discard result
  R[0] = SSVM_ValueGenI32(0);
  EXPECT_TRUE(
      SSVM_ResultOK(SSVM_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_VMRunWasmFromFile(VM, TPath, FuncName, P, 2, nullptr, 1)));

  /// VM run wasm from buffer
  R[0] = SSVM_ValueGenI32(0);
  R[1] = SSVM_ValueGenI32(0);
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMRunWasmFromBuffer(VM, Buf.data(), Buf.size(),
                                                     FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  EXPECT_EQ(912, SSVM_ValueGetI32(R[1]));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMRunWasmFromBuffer(
      nullptr, Buf.data(), Buf.size(), FuncName, P, 2, R, 2)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromBuffer(VM, nullptr, 0, FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMRunWasmFromBuffer(
      VM, Buf.data(), Buf.size(), FuncName, P, 1, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMRunWasmFromBuffer(
      VM, Buf.data(), Buf.size(), FuncName, nullptr, 0, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMRunWasmFromBuffer(
      VM, Buf.data(), Buf.size(), FuncName, nullptr, 2, R, 2)));
  /// Function type mismatch
  P[0] = SSVM_ValueGenI64(123);
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMRunWasmFromBuffer(
      VM, Buf.data(), Buf.size(), FuncName, P, 2, R, 2)));
  P[0] = SSVM_ValueGenI32(123);
  /// Function not found
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMRunWasmFromBuffer(
      VM, Buf.data(), Buf.size(), FuncName2, P, 2, R, 2)));
  /// Discard result
  R[0] = SSVM_ValueGenI32(0);
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMRunWasmFromBuffer(VM, Buf.data(), Buf.size(),
                                                     FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMRunWasmFromBuffer(
      VM, Buf.data(), Buf.size(), FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMRunWasmFromBuffer(
      VM, Buf.data(), Buf.size(), FuncName, P, 2, nullptr, 1)));

  /// VM run wasm from AST module
  R[0] = SSVM_ValueGenI32(0);
  R[1] = SSVM_ValueGenI32(0);
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  EXPECT_EQ(912, SSVM_ValueGetI32(R[1]));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromASTModule(nullptr, Mod, FuncName, P, 2, R, 2)));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromASTModule(VM, nullptr, FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 1, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromASTModule(VM, Mod, FuncName, nullptr, 0, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromASTModule(VM, Mod, FuncName, nullptr, 2, R, 2)));
  /// Function type mismatch
  P[0] = SSVM_ValueGenI64(123);
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, R, 2)));
  P[0] = SSVM_ValueGenI32(123);
  /// Function not found
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMRunWasmFromASTModule(VM, Mod, FuncName2, P, 2, R, 2)));
  /// Discard result
  R[0] = SSVM_ValueGenI32(0);
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_VMRunWasmFromASTModule(VM, Mod, FuncName, P, 2, nullptr, 1)));

  /// VM load wasm from file
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMLoadWasmFromFile(VM, TPath)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMLoadWasmFromFile(nullptr, TPath)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMLoadWasmFromFile(VM, "file")));

  /// VM load wasm from buffer
  EXPECT_TRUE(
      SSVM_ResultOK(SSVM_VMLoadWasmFromBuffer(VM, Buf.data(), Buf.size())));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMLoadWasmFromBuffer(nullptr, Buf.data(), Buf.size())));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMLoadWasmFromBuffer(VM, nullptr, 0)));

  /// VM load wasm from AST module
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMLoadWasmFromASTModule(nullptr, Mod)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMLoadWasmFromASTModule(VM, nullptr)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMLoadWasmFromASTModule(nullptr, nullptr)));

  /// VM validate
  SSVM_VMCleanup(VM);
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMValidate(VM)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMValidate(nullptr)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMValidate(VM)));

  /// VM instantiate
  SSVM_VMCleanup(VM);
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMInstantiate(VM)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMInstantiate(VM)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMValidate(VM)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMInstantiate(nullptr)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMInstantiate(VM)));

  /// VM execute
  R[0] = SSVM_ValueGenI32(0);
  R[1] = SSVM_ValueGenI32(0);
  SSVM_VMCleanup(VM);
  /// Inited phase
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName, P, 2, R, 2)));
  /// Loaded phase
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName, P, 2, R, 2)));
  /// Validated phase
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMValidate(VM)));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName, P, 2, R, 2)));
  /// Instantiated phase
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMInstantiate(VM)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  EXPECT_EQ(912, SSVM_ValueGetI32(R[1]));
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMExecute(nullptr, FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName, P, 1, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName, nullptr, 0, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName, nullptr, 2, R, 2)));
  /// Function type mismatch
  P[0] = SSVM_ValueGenI64(123);
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName, P, 2, R, 2)));
  P[0] = SSVM_ValueGenI32(123);
  /// Function not found
  EXPECT_FALSE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName2, P, 2, R, 2)));
  /// Discard result
  R[0] = SSVM_ValueGenI32(0);
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMExecute(VM, FuncName, P, 2, nullptr, 1)));

  /// SSVM_VMExecuteRegistered
  R[0] = SSVM_ValueGenI32(0);
  R[1] = SSVM_ValueGenI32(0);
  SSVM_VMCleanup(VM);
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_VMExecuteRegistered(VM, ModName, FuncName, P, 2, R, 2)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  EXPECT_EQ(912, SSVM_ValueGetI32(R[1]));
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMExecuteRegistered(nullptr, ModName, FuncName, P, 2, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMExecuteRegistered(VM, ModName, FuncName, P, 1, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMExecuteRegistered(VM, ModName, FuncName, nullptr, 0, R, 2)));
  /// Function type mismatch
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMExecuteRegistered(VM, ModName, FuncName, nullptr, 2, R, 2)));
  /// Function type mismatch
  P[0] = SSVM_ValueGenI64(123);
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMExecuteRegistered(VM, ModName, FuncName, P, 2, R, 2)));
  P[0] = SSVM_ValueGenI32(123);
  /// Module not found
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMExecuteRegistered(VM, ModName2, FuncName, P, 2, R, 2)));
  /// Function not found
  EXPECT_FALSE(SSVM_ResultOK(
      SSVM_VMExecuteRegistered(VM, ModName, FuncName2, P, 2, R, 2)));
  /// Discard result
  R[0] = SSVM_ValueGenI32(0);
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_VMExecuteRegistered(VM, ModName, FuncName, P, 2, R, 1)));
  EXPECT_EQ(246, SSVM_ValueGetI32(R[0]));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_VMExecuteRegistered(VM, ModName, FuncName, P, 2, nullptr, 0)));
  /// Discard result
  EXPECT_TRUE(SSVM_ResultOK(
      SSVM_VMExecuteRegistered(VM, ModName, FuncName, P, 2, nullptr, 1)));

  /// VM get function type
  SSVM_VMCleanup(VM);
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMLoadWasmFromASTModule(VM, Mod)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMValidate(VM)));
  EXPECT_TRUE(SSVM_ResultOK(SSVM_VMInstantiate(VM)));
  FuncType = SSVM_VMGetFunctionType(VM, FuncName);
  EXPECT_NE(FuncType, nullptr);
  SSVM_FunctionTypeDelete(FuncType);
  FuncType = SSVM_VMGetFunctionType(nullptr, FuncName);
  EXPECT_EQ(FuncType, nullptr);
  FuncType = SSVM_VMGetFunctionType(VM, FuncName2);
  EXPECT_EQ(FuncType, nullptr);

  /// VM get function type registered
  FuncType = SSVM_VMGetFunctionTypeRegistered(VM, ModName, FuncName);
  EXPECT_NE(FuncType, nullptr);
  SSVM_FunctionTypeDelete(FuncType);
  FuncType = SSVM_VMGetFunctionTypeRegistered(nullptr, ModName, FuncName);
  EXPECT_EQ(FuncType, nullptr);
  FuncType = SSVM_VMGetFunctionTypeRegistered(VM, ModName2, FuncName);
  EXPECT_EQ(FuncType, nullptr);
  FuncType = SSVM_VMGetFunctionTypeRegistered(VM, ModName, FuncName2);
  EXPECT_EQ(FuncType, nullptr);

  SSVM_StringDelete(FuncName);
  SSVM_StringDelete(FuncName2);
  SSVM_StringDelete(ModName);
  SSVM_StringDelete(ModName2);

  /// VM get function list
  EXPECT_EQ(SSVM_VMGetFunctionListLength(VM), 11U);
  EXPECT_EQ(SSVM_VMGetFunctionListLength(nullptr), 0U);
  EXPECT_EQ(SSVM_VMGetFunctionList(nullptr, Names, FuncTypes, 15), 0U);
  EXPECT_EQ(SSVM_VMGetFunctionList(VM, nullptr, FuncTypes, 15), 11U);
  EXPECT_EQ(SSVM_VMGetFunctionList(VM, Names, nullptr, 15), 11U);
  for (uint32_t I = 0; I < 11; I++) {
    SSVM_StringDelete(Names[I]);
    SSVM_FunctionTypeDelete(FuncTypes[I]);
  }
  EXPECT_EQ(SSVM_VMGetFunctionList(VM, nullptr, nullptr, 15), 11U);
  EXPECT_EQ(SSVM_VMGetFunctionList(VM, Names, FuncTypes, 4), 11U);
  for (uint32_t I = 0; I < 4; I++) {
    SSVM_StringDelete(Names[I]);
    SSVM_FunctionTypeDelete(FuncTypes[I]);
  }
  EXPECT_EQ(SSVM_VMGetFunctionList(VM, Names, FuncTypes, 15), 11U);
  for (uint32_t I = 0; I < 11; I++) {
    SSVM_StringDelete(Names[I]);
    SSVM_FunctionTypeDelete(FuncTypes[I]);
  }

  /// VM cleanup
  SSVM_VMCleanup(VM);
  EXPECT_TRUE(true);
  SSVM_VMCleanup(nullptr);
  EXPECT_TRUE(true);

  /// VM get import module
  EXPECT_NE(SSVM_VMGetImportModuleContext(VM, SSVM_HostRegistration_Wasi),
            nullptr);
  EXPECT_EQ(
      SSVM_VMGetImportModuleContext(VM, SSVM_HostRegistration_SSVM_Process),
      nullptr);
  EXPECT_EQ(SSVM_VMGetImportModuleContext(nullptr, SSVM_HostRegistration_Wasi),
            nullptr);

  /// VM get store
  EXPECT_EQ(SSVM_VMGetStoreContext(VM), Store);
  EXPECT_EQ(SSVM_VMGetStoreContext(nullptr), nullptr);

  /// VM get statistics
  EXPECT_NE(SSVM_VMGetStatisticsContext(VM), nullptr);
  EXPECT_EQ(SSVM_VMGetStatisticsContext(nullptr), nullptr);

  SSVM_ASTModuleDelete(Mod);
  SSVM_ImportObjectDelete(ImpObj);
  SSVM_StoreDelete(Store);
  SSVM_VMDelete(VM);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  SSVM_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
