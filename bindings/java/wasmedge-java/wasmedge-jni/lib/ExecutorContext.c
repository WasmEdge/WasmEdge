// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "../jni/org_wasmedge_ExecutorContext.h"
#include "AstModuleContext.h"
#include "ConfigureContext.h"
#include "FunctionTypeInstance.h"
#include "ModuleInstanceContext.h"
#include "StatisticsContext.h"
#include "StoreContext.h"
#include "ValueType.h"
#include "common.h"
#include "wasmedge/wasmedge.h"
#include <stdlib.h>

GETTER(ExecutorContext)

JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_nativeInit(
    JNIEnv *env, jobject thisObject, jobject jConfigContext, jobject jStatCxt) {
  WasmEdge_ConfigureContext *confCxt = getConfigureContext(env, jConfigContext);
  WasmEdge_StatisticsContext *statCxt = getStatisticsContext(env, jStatCxt);

  WasmEdge_ExecutorContext *exeCxt = WasmEdge_ExecutorCreate(confCxt, statCxt);
  setPointer(env, thisObject, (long)exeCxt);
}

/*
 * Class:     org_wasmedge_ExecutorContext
 * Method:    instantiate
 * Signature: (Lorg/wasmedge/StoreContext;Lorg/wasmedge/ASTModuleContext;)V
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_ExecutorContext_instantiate(
    JNIEnv *env, jobject thisObject, jobject jStoreCxt, jobject jAstModCxt) {
  WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);
  WasmEdge_StoreContext *storeCxt = getStoreContext(env, jStoreCxt);
  WasmEdge_ASTModuleContext *astModCxt = getASTModuleContext(env, jAstModCxt);

  WasmEdge_ModuleInstanceContext *modCxt = NULL;
  WasmEdge_ExecutorInstantiate(exeCxt, &modCxt, storeCxt, astModCxt);

  return createJModuleInstanceContext(env, modCxt);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_invoke(
    JNIEnv *env, jobject thisObject, jobject jFuncInstanceContext,
    jobject jParams, jobject jReturns) {
  WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);

  jsize paramLen = GetListSize(env, jParams);

  /* The parameters and returns arrays. */
  WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value));
  for (int i = 0; i < paramLen; i++) {
    wasm_params[i] =
        JavaValueToWasmEdgeValue(env, GetListElement(env, jParams, i));
  }

  WasmEdge_FunctionInstanceContext *function =
      getFunctionInstanceContext(env, jFuncInstanceContext);

  uint32_t returnLen = GetReturnLen(function);
  WasmEdge_Value *returns = malloc(sizeof(WasmEdge_Value) * returnLen);

  WasmEdge_Result result = WasmEdge_ExecutorInvoke(
      exeCxt, function, wasm_params, paramLen, returns, returnLen);

  // release resource
  handleWasmEdgeResult(env, &result);

  if (WasmEdge_ResultOK(result)) {
    for (int i = 0; i < returnLen; ++i) {
      AddElementToJavaList(env, jReturns,
                           WasmEdgeValueToJavaValue(env, returns[i]));
    }
  }
}

JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_registerImport(
    JNIEnv *env, jobject thisObject, jobject jStore, jobject jImpObj) {
  WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);
  WasmEdge_StoreContext *storeCxt = getStoreContext(env, jStore);
  WasmEdge_ModuleInstanceContext *impObj =
      getModuleInstanceContext(env, jImpObj);

  WasmEdge_Result result =
      WasmEdge_ExecutorRegisterImport(exeCxt, storeCxt, impObj);
  handleWasmEdgeResult(env, &result);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ExecutorContext_register(
    JNIEnv *env, jobject thisObject, jobject jStore, jobject jAstCxt,
    jstring jModName) {
  WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);
  WasmEdge_StoreContext *storeCxt = getStoreContext(env, jStore);
  WasmEdge_ASTModuleContext *astCxt = getASTModuleContext(env, jAstCxt);
  WasmEdge_String wModName = JStringToWasmString(env, jModName);

  WasmEdge_ModuleInstanceContext *instCxt = NULL;
  WasmEdge_Result result =
      WasmEdge_ExecutorRegister(exeCxt, &instCxt, storeCxt, astCxt, wModName);
  handleWasmEdgeResult(env, &result);

  return createJModuleInstanceContext(env, instCxt);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_ExecutorContext_close(JNIEnv *env, jobject thisObject) {
  WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);
  WasmEdge_ExecutorDelete(exeCxt);
}
