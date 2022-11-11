## Python Bindings for WasmEdge

- [Docs](https://satacker.github.io/WasmEdge/)

## API Status Logging

Typedefs
--
| Typedef | Status |
| -- | -- |
| WasmEdge_Value | [✅] |
| WasmEdge_String | <!--- ❌ ✅ --> [ ] |
| WasmEdge_Result | [✅] |
| WasmEdge_Limit | [✅] |
| WasmEdge_ConfigureContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_StatisticsContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_ASTModuleContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_FunctionTypeContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_MemoryTypeContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_TableTypeContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_GlobalTypeContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_ImportTypeContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_ExportTypeContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_CompilerContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_LoaderContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_ValidatorContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_ExecutorContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_StoreContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_FunctionInstanceContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_TableInstanceContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_MemoryInstanceContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_GlobalInstanceContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_ImportObjectContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_Async | <!--- ❌ ✅ --> [✅] |
| WasmEdge_VMContext | <!--- ❌ ✅ --> [✅] |
| WasmEdge_HostFunc_t | <!--- ❌ ✅ --> [ ] |
| WasmEdge_WrapFunc_t | <!--- ❌ ✅ --> [ ] |

Function
--
| Function | Status |
| -- | -- |
| const  char * WasmEdge_VersionGet(void) | <!--- ❌ ✅ --> [✅ ] |
| uint32_t WasmEdge_VersionGetMajor(void) | <!--- ❌ ✅ --> [✅] |
| uint32_t WasmEdge_VersionGetMinor(void) | <!--- ❌ ✅ --> [✅] |
| uint32_t WasmEdge_VersionGetPatch(void) | <!--- ❌ ✅ --> [✅] |
| void WasmEdge_LogSetErrorLevel(void) | <!--- ❌ ✅ --> [✅] |
| void WasmEdge_LogSetDebugLevel(void) | <!--- ❌ ✅ --> [✅] |
| WasmEdge_Value WasmEdge_ValueGenI32(const int32_t Val) | <!--- ❌ ✅ --> [✅] |
| WasmEdge_Value WasmEdge_ValueGenI64(const int64_t Val) | <!--- ❌ ✅ --> [✅] | 
| WasmEdge_Value WasmEdge_ValueGenF32(const float Val) | <!--- ❌ ✅ --> [✅] | 
| WasmEdge_Value WasmEdge_ValueGenF64(const double Val) | <!--- ❌ ✅ --> [✅] | 
| WasmEdge_Value WasmEdge_ValueGenV128(const int128_t Val) | <!--- ❌ ✅ --> [✅] | 
| WasmEdge_Value WasmEdge_ValueGenNullRef(const enum WasmEdge_RefType T) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Value WasmEdge_ValueGenFuncRef(const uint32_t Index) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Value WasmEdge_ValueGenExternRef(void *Ref) | <!--- ❌ ✅ --> [✅] |
|int32_t WasmEdge_ValueGetI32(const WasmEdge_Value Val) | <!--- ❌ ✅ --> [✅] |
|int64_t WasmEdge_ValueGetI64(const WasmEdge_Value Val) | <!--- ❌ ✅ --> [✅] |
|float WasmEdge_ValueGetF32(const WasmEdge_Value Val) | <!--- ❌ ✅ --> [✅] |
|double WasmEdge_ValueGetF64(const WasmEdge_Value Val) | <!--- ❌ ✅ --> [✅] |
|int128_t WasmEdge_ValueGetV128(const WasmEdge_Value Val) | <!--- ❌ ✅ --> [✅] |
|bool WasmEdge_ValueIsNullRef(const WasmEdge_Value Val) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_ValueGetFuncIdx(const WasmEdge_Value Val) | <!--- ❌ ✅ --> [✅] |
|void * WasmEdge_ValueGetExternRef(const WasmEdge_Value Val) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_String WasmEdge_StringCreateByCString(const char *Str) | <!--- ❌ ✅ --> [ ] |
|WasmEdge_String WasmEdge_StringCreateByBuffer(const char *Buf, const uint32_t Len) | <!--- ❌ ✅ --> [ ] |
|WasmEdge_String WasmEdge_StringWrap(const char *Buf, const uint32_t Len) | <!--- ❌ ✅ --> [ ] |
|bool WasmEdge_StringIsEqual(const WasmEdge_String Str1, const WasmEdge_String Str2) | <!--- ❌ ✅ --> [ ] |
|uint32_t WasmEdge_StringCopy(const WasmEdge_String Str, char *Buf, const uint32_t Len) | <!--- ❌ ✅ --> [ ] |
|void WasmEdge_StringDelete(WasmEdge_String Str) | <!--- ❌ ✅ --> [ ] |
|bool WasmEdge_ResultOK(const WasmEdge_Result Res) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_ResultGetCode(const WasmEdge_Result Res) | <!--- ❌ ✅ --> [✅] |
| const  char * WasmEdge_ResultGetMessage(const WasmEdge_Result Res) | <!--- ❌ ✅ --> [✅] |
|bool WasmEdge_LimitIsEqual(const WasmEdge_Limit Lim1, const WasmEdge_Limit Lim2) | <!--- ❌ ✅ --> [ ] |
|WasmEdge_ConfigureContext * WasmEdge_ConfigureCreate(void) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureAddProposal(WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_Proposal Prop) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureRemoveProposal(WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_Proposal Prop) | <!--- ❌ ✅ --> [✅] |
|bool WasmEdge_ConfigureHasProposal(const WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_Proposal Prop) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureAddHostRegistration(WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_HostRegistration Host) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureRemoveHostRegistration(WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_HostRegistration Host) | <!--- ❌ ✅ --> [✅] |
|bool WasmEdge_ConfigureHasHostRegistration(const WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_HostRegistration Host) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureSetMaxMemoryPage(WasmEdge_ConfigureContext *Cxt, const uint32_t Page) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_ConfigureGetMaxMemoryPage(const WasmEdge_ConfigureContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureCompilerSetOptimizationLevel(WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_CompilerOptimizationLevel Level) | <!--- ❌ ✅ --> [✅] |
|enum WasmEdge_CompilerOptimizationLevel WasmEdge_ConfigureCompilerGetOptimizationLevel(const WasmEdge_ConfigureContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureCompilerSetOutputFormat(WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_CompilerOutputFormat Format) | <!--- ❌ ✅ --> [✅] |
|enum WasmEdge_CompilerOutputFormat WasmEdge_ConfigureCompilerGetOutputFormat(const WasmEdge_ConfigureContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureCompilerSetDumpIR(WasmEdge_ConfigureContext *Cxt, const bool IsDump) | <!--- ❌ ✅ --> [✅] |
|bool WasmEdge_ConfigureCompilerIsDumpIR(const WasmEdge_ConfigureContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureCompilerSetGenericBinary(WasmEdge_ConfigureContext *Cxt, const bool IsGeneric) | <!--- ❌ ✅ --> [✅] |
|bool WasmEdge_ConfigureCompilerIsGenericBinary(const WasmEdge_ConfigureContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureCompilerSetInterruptible(WasmEdge_ConfigureContext *Cxt, const bool IsInterruptible) | <!--- ❌ ✅ --> [✅] |
|bool WasmEdge_ConfigureCompilerIsInterruptible(const WasmEdge_ConfigureContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureStatisticsSetInstructionCounting(WasmEdge_ConfigureContext *Cxt, const bool IsCount) | <!--- ❌ ✅ --> [✅] |
|bool WasmEdge_ConfigureStatisticsIsInstructionCounting(const WasmEdge_ConfigureContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureStatisticsSetCostMeasuring(WasmEdge_ConfigureContext *Cxt, const bool IsMeasure) | <!--- ❌ ✅ --> [✅] |
|bool WasmEdge_ConfigureStatisticsIsCostMeasuring(const WasmEdge_ConfigureContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureStatisticsSetTimeMeasuring(WasmEdge_ConfigureContext *Cxt, const bool IsMeasure) | <!--- ❌ ✅ --> [✅] |
|bool WasmEdge_ConfigureStatisticsIsTimeMeasuring(const WasmEdge_ConfigureContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ConfigureDelete(WasmEdge_ConfigureContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_StatisticsContext * WasmEdge_StatisticsCreate(void) | <!--- ❌ ✅ --> [✅] |
|uint64_t WasmEdge_StatisticsGetInstrCount(const WasmEdge_StatisticsContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|double WasmEdge_StatisticsGetInstrPerSecond(const WasmEdge_StatisticsContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint64_t WasmEdge_StatisticsGetTotalCost(const WasmEdge_StatisticsContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_StatisticsSetCostTable(WasmEdge_StatisticsContext *Cxt, uint64_t *CostArr, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_StatisticsSetCostLimit(WasmEdge_StatisticsContext *Cxt, const uint64_t Limit) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_StatisticsDelete(WasmEdge_StatisticsContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_ASTModuleListImportsLength(const WasmEdge_ASTModuleContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_ASTModuleListImports(const WasmEdge_ASTModuleContext *Cxt, const WasmEdge_ImportTypeContext **Imports, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_ASTModuleListExportsLength(const WasmEdge_ASTModuleContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_ASTModuleListExports(const WasmEdge_ASTModuleContext *Cxt, const WasmEdge_ExportTypeContext **Exports, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ASTModuleDelete(WasmEdge_ASTModuleContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_FunctionTypeContext * WasmEdge_FunctionTypeCreate(const enum WasmEdge_ValType *ParamList, const uint32_t ParamLen, const enum WasmEdge_ValType *ReturnList, const uint32_t ReturnLen) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_FunctionTypeGetParametersLength(const WasmEdge_FunctionTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_FunctionTypeGetParameters(const WasmEdge_FunctionTypeContext *Cxt, enum WasmEdge_ValType *List, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_FunctionTypeGetReturnsLength(const WasmEdge_FunctionTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_FunctionTypeGetReturns(const WasmEdge_FunctionTypeContext *Cxt, enum WasmEdge_ValType *List, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_FunctionTypeDelete(WasmEdge_FunctionTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_TableTypeContext * WasmEdge_TableTypeCreate(const enum WasmEdge_RefType RefType, const WasmEdge_Limit Limit) | <!--- ❌ ✅ --> [✅] |
|enum WasmEdge_RefType WasmEdge_TableTypeGetRefType(const WasmEdge_TableTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Limit WasmEdge_TableTypeGetLimit(const WasmEdge_TableTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_TableTypeDelete(WasmEdge_TableTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_MemoryTypeContext * WasmEdge_MemoryTypeCreate(const WasmEdge_Limit Limit) | <!--- ❌ ✅ --> [ ] |
|WasmEdge_Limit WasmEdge_MemoryTypeGetLimit(const WasmEdge_MemoryTypeContext *Cxt) | <!--- ❌ ✅ --> [ ] |
|void WasmEdge_MemoryTypeDelete(WasmEdge_MemoryTypeContext *Cxt) | <!--- ❌ ✅ --> [ ] |
|WasmEdge_GlobalTypeContext * WasmEdge_GlobalTypeCreate(const enum WasmEdge_ValType ValType, const enum WasmEdge_Mutability Mut) | <!--- ❌ ✅ --> [✅] |
|enum WasmEdge_ValType WasmEdge_GlobalTypeGetValType(const WasmEdge_GlobalTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|enum WasmEdge_Mutability WasmEdge_GlobalTypeGetMutability(const WasmEdge_GlobalTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_GlobalTypeDelete(WasmEdge_GlobalTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|enum WasmEdge_ExternalType WasmEdge_ImportTypeGetExternalType(const WasmEdge_ImportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_String WasmEdge_ImportTypeGetModuleName(const WasmEdge_ImportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_String WasmEdge_ImportTypeGetExternalName(const WasmEdge_ImportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_FunctionTypeContext * WasmEdge_ImportTypeGetFunctionType(const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_ImportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_TableTypeContext * WasmEdge_ImportTypeGetTableType(const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_ImportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_MemoryTypeContext * WasmEdge_ImportTypeGetMemoryType(const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_ImportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_GlobalTypeContext * WasmEdge_ImportTypeGetGlobalType(const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_ImportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|enum WasmEdge_ExternalType WasmEdge_ExportTypeGetExternalType(const WasmEdge_ExportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_String WasmEdge_ExportTypeGetExternalName(const WasmEdge_ExportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_FunctionTypeContext * WasmEdge_ExportTypeGetFunctionType(const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_ExportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_TableTypeContext * WasmEdge_ExportTypeGetTableType(const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_ExportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_MemoryTypeContext * WasmEdge_ExportTypeGetMemoryType(const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_ExportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_GlobalTypeContext * WasmEdge_ExportTypeGetGlobalType(const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_ExportTypeContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_CompilerContext * WasmEdge_CompilerCreate(const WasmEdge_ConfigureContext *ConfCxt) | <!--- ❌ ✅ --> [ ] |
|WasmEdge_Result WasmEdge_CompilerCompile(WasmEdge_CompilerContext *Cxt, const char *InPath, const char *OutPath) | <!--- ❌ ✅ --> [ ] |
|void WasmEdge_CompilerDelete(WasmEdge_CompilerContext *Cxt) | <!--- ❌ ✅ --> [ ] |
|WasmEdge_LoaderContext * WasmEdge_LoaderCreate(const WasmEdge_ConfigureContext *ConfCxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_LoaderParseFromFile(WasmEdge_LoaderContext *Cxt, WasmEdge_ASTModuleContext **Module, const char *Path) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_LoaderParseFromBuffer(WasmEdge_LoaderContext *Cxt, WasmEdge_ASTModuleContext **Module, const uint8_t *Buf, const uint32_t BufLen) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_LoaderDelete(WasmEdge_LoaderContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_ValidatorContext * WasmEdge_ValidatorCreate(const WasmEdge_ConfigureContext *ConfCxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_ValidatorValidate(WasmEdge_ValidatorContext *Cxt, const WasmEdge_ASTModuleContext *ModuleCxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ValidatorDelete(WasmEdge_ValidatorContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_ExecutorContext * WasmEdge_ExecutorCreate(const WasmEdge_ConfigureContext *ConfCxt, WasmEdge_StatisticsContext *StatCxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_ExecutorInstantiate(WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt, const WasmEdge_ASTModuleContext *ASTCxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_ExecutorRegisterImport(WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt, const WasmEdge_ImportObjectContext *ImportCxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_ExecutorRegisterModule(WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt, const WasmEdge_ASTModuleContext *ASTCxt, WasmEdge_String ModuleName) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_ExecutorInvoke(WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen, WasmEdge_Value *Returns, const uint32_t ReturnLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_ExecutorInvokeRegistered(WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt, const WasmEdge_String ModuleName, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen, WasmEdge_Value *Returns, const uint32_t ReturnLen) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ExecutorDelete(WasmEdge_ExecutorContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_StoreContext * WasmEdge_StoreCreate(void) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_FunctionInstanceContext * WasmEdge_StoreFindFunction(WasmEdge_StoreContext *Cxt, const WasmEdge_String Name) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_FunctionInstanceContext * WasmEdge_StoreFindFunctionRegistered(WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName, const WasmEdge_String FuncName) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_TableInstanceContext * WasmEdge_StoreFindTable(WasmEdge_StoreContext *Cxt, const WasmEdge_String Name) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_TableInstanceContext * WasmEdge_StoreFindTableRegistered(WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName, const WasmEdge_String TableName) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_MemoryInstanceContext * WasmEdge_StoreFindMemory(WasmEdge_StoreContext *Cxt, const WasmEdge_String Name) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_MemoryInstanceContext * WasmEdge_StoreFindMemoryRegistered(WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName, const WasmEdge_String MemoryName) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_GlobalInstanceContext * WasmEdge_StoreFindGlobal(WasmEdge_StoreContext *Cxt, const WasmEdge_String Name) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_GlobalInstanceContext * WasmEdge_StoreFindGlobalRegistered(WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName, const WasmEdge_String GlobalName) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListFunctionLength(const WasmEdge_StoreContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListFunction(const WasmEdge_StoreContext *Cxt, WasmEdge_String *Names, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListFunctionRegisteredLength(const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListFunctionRegistered(const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName, WasmEdge_String *Names, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListTableLength(const WasmEdge_StoreContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListTable(const WasmEdge_StoreContext *Cxt, WasmEdge_String *Names, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListTableRegisteredLength(const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListTableRegistered(const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName, WasmEdge_String *Names, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListMemoryLength(const WasmEdge_StoreContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListMemory(const WasmEdge_StoreContext *Cxt, WasmEdge_String *Names, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListMemoryRegisteredLength(const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListMemoryRegistered(const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName, WasmEdge_String *Names, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListGlobalLength(const WasmEdge_StoreContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListGlobal(const WasmEdge_StoreContext *Cxt, WasmEdge_String *Names, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListGlobalRegisteredLength(const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListGlobalRegistered(const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName, WasmEdge_String *Names, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListModuleLength(const WasmEdge_StoreContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_StoreListModule(const WasmEdge_StoreContext *Cxt, WasmEdge_String *Names, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_StoreDelete(WasmEdge_StoreContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_FunctionInstanceContext * WasmEdge_FunctionInstanceCreate(const WasmEdge_FunctionTypeContext *Type, WasmEdge_HostFunc_t HostFunc, void *Data, const uint64_t Cost) | <!--- ❌ ✅ --> [ ] |
|WasmEdge_FunctionInstanceContext * WasmEdge_FunctionInstanceCreateBinding(const WasmEdge_FunctionTypeContext *Type, WasmEdge_WrapFunc_t WrapFunc, void *Binding, void *Data, const uint64_t Cost) | <!--- ❌ ✅ --> [ ] |
| const  WasmEdge_FunctionTypeContext * WasmEdge_FunctionInstanceGetFunctionType(const WasmEdge_FunctionInstanceContext *Cxt) | <!--- ❌ ✅ --> [ ] |
|void WasmEdge_FunctionInstanceDelete(WasmEdge_FunctionInstanceContext *Cxt) | <!--- ❌ ✅ --> [ ] |
|WasmEdge_TableInstanceContext * WasmEdge_TableInstanceCreate(const WasmEdge_TableTypeContext *TabType) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_TableTypeContext * WasmEdge_TableInstanceGetTableType(const WasmEdge_TableInstanceContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_TableInstanceGetData(const WasmEdge_TableInstanceContext *Cxt, WasmEdge_Value *Data, const uint32_t Offset) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_TableInstanceSetData(WasmEdge_TableInstanceContext *Cxt, WasmEdge_Value Data, const uint32_t Offset) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_TableInstanceGetSize(const WasmEdge_TableInstanceContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_TableInstanceGrow(WasmEdge_TableInstanceContext *Cxt, const uint32_t Size) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_TableInstanceDelete(WasmEdge_TableInstanceContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_MemoryInstanceContext * WasmEdge_MemoryInstanceCreate(const WasmEdge_MemoryTypeContext *MemType) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_MemoryTypeContext * WasmEdge_MemoryInstanceGetMemoryType(const WasmEdge_MemoryInstanceContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_MemoryInstanceGetData(const WasmEdge_MemoryInstanceContext *Cxt, uint8_t *Data, const uint32_t Offset, const uint32_t Length) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_MemoryInstanceSetData(WasmEdge_MemoryInstanceContext *Cxt, const uint8_t *Data, const uint32_t Offset, const uint32_t Length) | <!--- ❌ ✅ --> [✅] |
|uint8_t * WasmEdge_MemoryInstanceGetPointer(WasmEdge_MemoryInstanceContext *Cxt, const uint32_t Offset, const uint32_t Length) | <!--- ❌ ✅ --> [ ] |
| const  uint8_t * WasmEdge_MemoryInstanceGetPointerConst(const WasmEdge_MemoryInstanceContext *Cxt, const uint32_t Offset, const uint32_t Length) | <!--- ❌ ✅ --> [ ] |
|uint32_t WasmEdge_MemoryInstanceGetPageSize(const WasmEdge_MemoryInstanceContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_MemoryInstanceGrowPage(WasmEdge_MemoryInstanceContext *Cxt, const uint32_t Page) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_MemoryInstanceDelete(WasmEdge_MemoryInstanceContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_GlobalInstanceContext * WasmEdge_GlobalInstanceCreate(const WasmEdge_GlobalTypeContext *GlobType, const WasmEdge_Value Value) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_GlobalTypeContext * WasmEdge_GlobalInstanceGetGlobalType(const WasmEdge_GlobalInstanceContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Value WasmEdge_GlobalInstanceGetValue(const WasmEdge_GlobalInstanceContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_GlobalInstanceSetValue(WasmEdge_GlobalInstanceContext *Cxt, const WasmEdge_Value Value) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_GlobalInstanceDelete(WasmEdge_GlobalInstanceContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_ImportObjectContext * WasmEdge_ImportObjectCreate(const WasmEdge_String ModuleName) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_ImportObjectContext * WasmEdge_ImportObjectCreateWASI (const char *const *Args, const uint32_t ArgLen, const char *const  *Envs, const uint32_t EnvLen, const char *const *Preopens, const  uint32_t PreopenLen) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ImportObjectInitWASI(WasmEdge_ImportObjectContext  *Cxt, const char *const *Args, const uint32_t ArgLen, const char *const  *Envs, const uint32_t EnvLen, const char *const *Preopens, const  uint32_t PreopenLen) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_ImportObjectWASIGetExitCode(WasmEdge_ImportObjectContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_ImportObjectContext * WasmEdge_ImportObjectCreateWasmEdgeProcess(const char *const *AllowedCmds, const uint32_t CmdsLen, const bool AllowAll) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ImportObjectInitWasmEdgeProcess(WasmEdge_ImportObjectContext *Cxt, const char *const *AllowedCmds, const uint32_t CmdsLen, const bool AllowAll) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ImportObjectAddFunction(WasmEdge_ImportObjectContext *Cxt, const WasmEdge_String Name, WasmEdge_FunctionInstanceContext *FuncCxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ImportObjectAddTable(WasmEdge_ImportObjectContext *Cxt, const WasmEdge_String Name, WasmEdge_TableInstanceContext *TableCxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ImportObjectAddMemory(WasmEdge_ImportObjectContext *Cxt, const WasmEdge_String Name, WasmEdge_MemoryInstanceContext *MemoryCxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ImportObjectAddGlobal(WasmEdge_ImportObjectContext *Cxt, const WasmEdge_String Name, WasmEdge_GlobalInstanceContext *GlobalCxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_ImportObjectDelete(WasmEdge_ImportObjectContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_AsyncWait(WasmEdge_Async *Cxt) | <!--- ❌ ✅ --> [✅] |
|bool WasmEdge_AsyncWaitFor(WasmEdge_Async *Cxt, uint64_t Milliseconds) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_AsyncCancel(WasmEdge_Async *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_AsyncGetReturnsLength(WasmEdge_Async *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_AsyncGet(WasmEdge_Async *Cxt, WasmEdge_Value *Returns, const uint32_t ReturnLen) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_AsyncDelete(WasmEdge_Async *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_VMContext * WasmEdge_VMCreate(const WasmEdge_ConfigureContext *ConfCxt, WasmEdge_StoreContext *StoreCxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMRegisterModuleFromFile(WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName, const char *Path) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMRegisterModuleFromBuffer(WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName, const uint8_t *Buf, const uint32_t BufLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMRegisterModuleFromImport(WasmEdge_VMContext *Cxt, const WasmEdge_ImportObjectContext *ImportCxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMRegisterModuleFromASTModule(WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName, const WasmEdge_ASTModuleContext *ASTCxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMRunWasmFromFile(WasmEdge_VMContext *Cxt, const char *Path, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen, WasmEdge_Value *Returns, const uint32_t ReturnLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMRunWasmFromBuffer(WasmEdge_VMContext *Cxt, const uint8_t *Buf, const uint32_t BufLen, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen, WasmEdge_Value *Returns, const uint32_t ReturnLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMRunWasmFromASTModule(WasmEdge_VMContext *Cxt, const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen, WasmEdge_Value *Returns, const uint32_t ReturnLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Async * WasmEdge_VMAsyncRunWasmFromFile(WasmEdge_VMContext *Cxt, const char *Path, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Async * WasmEdge_VMAsyncRunWasmFromBuffer(WasmEdge_VMContext *Cxt, const uint8_t *Buf, const uint32_t BufLen, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Async * WasmEdge_VMAsyncRunWasmFromASTModule(WasmEdge_VMContext *Cxt, const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMLoadWasmFromFile(WasmEdge_VMContext *Cxt, const char *Path) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMLoadWasmFromBuffer(WasmEdge_VMContext *Cxt, const uint8_t *Buf, const uint32_t BufLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMLoadWasmFromASTModule(WasmEdge_VMContext *Cxt, const WasmEdge_ASTModuleContext *ASTCxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMValidate(WasmEdge_VMContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMInstantiate(WasmEdge_VMContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMExecute(WasmEdge_VMContext *Cxt, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen, WasmEdge_Value *Returns, const uint32_t ReturnLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Result WasmEdge_VMExecuteRegistered(WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen, WasmEdge_Value *Returns, const uint32_t ReturnLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Async * WasmEdge_VMAsyncExecute(WasmEdge_VMContext *Cxt, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_Async * WasmEdge_VMAsyncExecuteRegistered(WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName, const WasmEdge_String FuncName, const WasmEdge_Value *Params, const uint32_t ParamLen) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_FunctionTypeContext * WasmEdge_VMGetFunctionType(WasmEdge_VMContext *Cxt, const WasmEdge_String FuncName) | <!--- ❌ ✅ --> [✅] |
| const  WasmEdge_FunctionTypeContext * WasmEdge_VMGetFunctionTypeRegistered(WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName, const WasmEdge_String FuncName) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_VMCleanup(WasmEdge_VMContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_VMGetFunctionListLength(WasmEdge_VMContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|uint32_t WasmEdge_VMGetFunctionList(WasmEdge_VMContext *Cxt, WasmEdge_String *Names, const WasmEdge_FunctionTypeContext **FuncTypes, const uint32_t Len) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_ImportObjectContext * WasmEdge_VMGetImportModuleContext(WasmEdge_VMContext *Cxt, const enum WasmEdge_HostRegistration Reg) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_StoreContext * WasmEdge_VMGetStoreContext(WasmEdge_VMContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|WasmEdge_StatisticsContext * WasmEdge_VMGetStatisticsContext(WasmEdge_VMContext *Cxt) | <!--- ❌ ✅ --> [✅] |
|void WasmEdge_VMDelete(WasmEdge_VMContext *Cxt) | <!--- ❌ ✅ --> [✅] |
