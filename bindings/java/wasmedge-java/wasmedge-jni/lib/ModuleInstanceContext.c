// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "FunctionTypeInstance.h"
#include "GlobalInstanceContext.h"
#include "MemoryInstanceContext.h"
#include "TableInstanceContext.h"
#include "common.h"
#include "jni.h"
#include "wasmedge/wasmedge.h"
#include <stdlib.h>

GETTER(ModuleInstanceContext)

jobject
createJModuleInstanceContext(JNIEnv *env,
                             const WasmEdge_ModuleInstanceContext *impObj);

JNIEXPORT void JNICALL Java_org_wasmedge_ModuleInstanceContext_nativeInit(
    JNIEnv *env, jobject thisObject, jstring moduleName) {
  WasmEdge_ModuleInstanceContext *impCxt =
      WasmEdge_ModuleInstanceCreate(JStringToWasmString(env, moduleName));
  setPointer(env, thisObject, (long)impCxt);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ModuleInstanceContext_initWasi(
    JNIEnv *env, jobject thisObject, jobjectArray jArgs, jobjectArray jEnvs,
    jobjectArray jPreopens) {

  const char **args = JStringArrayToPtr(env, jArgs);
  const char **envs = JStringArrayToPtr(env, jEnvs);
  const char **preopens = JStringArrayToPtr(env, jPreopens);

  WasmEdge_ModuleInstanceContext *impCxt =
      getModuleInstanceContext(env, thisObject);
  WasmEdge_ModuleInstanceInitWASI(impCxt, args,
                                  (*env)->GetArrayLength(env, jArgs), envs,
                                  (*env)->GetArrayLength(env, jEnvs), preopens,
                                  (*env)->GetArrayLength(env, jPreopens));
  ReleaseCString(env, jArgs, args);
  ReleaseCString(env, jEnvs, envs);
  ReleaseCString(env, jPreopens, preopens);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_ModuleInstanceContext_getWasiExitCode(
    JNIEnv *env, jobject thisObject) {
  return WasmEdge_ModuleInstanceWASIGetExitCode(
      getModuleInstanceContext(env, thisObject));
}

JNIEXPORT void JNICALL Java_org_wasmedge_ModuleInstanceContext_addFunction(
    JNIEnv *env, jobject thisObject, jstring jFuncName, jobject jFunc) {
  WasmEdge_ModuleInstanceContext *impObjCxt =
      getModuleInstanceContext(env, thisObject);
  WasmEdge_FunctionInstanceContext *funcInst =
      getFunctionInstanceContext(env, jFunc);

  WasmEdge_ModuleInstanceAddFunction(
      impObjCxt, JStringToWasmString(env, jFuncName), funcInst);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ModuleInstanceContext_addTable(
    JNIEnv *env, jobject thisObject, jstring jTabName, jobject jTable) {
  WasmEdge_ModuleInstanceContext *impObjCxt =
      getModuleInstanceContext(env, thisObject);
  WasmEdge_TableInstanceContext *tabIns = getTableInstanceContext(env, jTable);
  WasmEdge_ModuleInstanceAddTable(impObjCxt, JStringToWasmString(env, jTabName),
                                  tabIns);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ModuleInstanceContext_addMemory(
    JNIEnv *env, jobject thisObject, jstring jMemName, jobject jMem) {

  WasmEdge_ModuleInstanceContext *impObjCxt =
      getModuleInstanceContext(env, thisObject);
  WasmEdge_MemoryInstanceContext *memCxt = getMemoryInstanceContext(env, jMem);

  WasmEdge_ModuleInstanceAddMemory(impObjCxt,
                                   JStringToWasmString(env, jMemName), memCxt);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ModuleInstanceContext_addGlobal(
    JNIEnv *env, jobject thisObject, jstring jGlobalName, jobject jGlobal) {

  WasmEdge_ModuleInstanceContext *impObjCxt =
      getModuleInstanceContext(env, thisObject);
  WasmEdge_GlobalInstanceContext *globalInstance =
      getGlobalInstanceContext(env, jGlobal);
  WasmEdge_ModuleInstanceAddGlobal(
      impObjCxt, JStringToWasmString(env, jGlobalName), globalInstance);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_ModuleInstanceContext_close(JNIEnv *env, jobject thisObject) {
  WasmEdge_ModuleInstanceContext *impObjCxt =
      getModuleInstanceContext(env, thisObject);
  WasmEdge_ModuleInstanceDelete(impObjCxt);
  setPointer(env, thisObject, 0);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ModuleInstanceContext_createWasi(
    JNIEnv *env, jclass thisClass, jobjectArray jArgs, jobjectArray jEnvs,
    jobjectArray jPreopens) {

  const char **args = JStringArrayToPtr(env, jArgs);
  const char **envs = JStringArrayToPtr(env, jEnvs);
  const char **preopens = JStringArrayToPtr(env, jPreopens);

  WasmEdge_ModuleInstanceContext *importObjectContext =
      WasmEdge_ModuleInstanceCreateWASI(
          args, (*env)->GetArrayLength(env, jArgs), envs,
          (*env)->GetArrayLength(env, jEnvs), preopens,
          (*env)->GetArrayLength(env, jPreopens));

  ReleaseCString(env, jArgs, args);
  ReleaseCString(env, jEnvs, envs);
  ReleaseCString(env, jPreopens, preopens);

  return createJModuleInstanceContext(env, importObjectContext);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ModuleInstanceContext_listFunction(
    JNIEnv *env, jobject thisObject) {
  WasmEdge_ModuleInstanceContext *moduleInstanceContext =
      getModuleInstanceContext(env, thisObject);

  uint32_t funcLen =
      WasmEdge_ModuleInstanceListFunctionLength(moduleInstanceContext);
  WasmEdge_String *nameList =
      (WasmEdge_String *)malloc(sizeof(struct WasmEdge_String) * funcLen);
  uint32_t RealFuncNum = WasmEdge_ModuleInstanceListFunction(
      moduleInstanceContext, nameList, funcLen);

  jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealFuncNum);

  free(nameList);

  return jNameList;
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ModuleInstanceContext_findFunction(
    JNIEnv *env, jobject thisObject, jstring jFuncName) {
  WasmEdge_ModuleInstanceContext *moduleInstanceContext =
      getModuleInstanceContext(env, thisObject);
  WasmEdge_String wFuncName = JStringToWasmString(env, jFuncName);

  WasmEdge_FunctionInstanceContext *funcInstance =
      WasmEdge_ModuleInstanceFindFunction(moduleInstanceContext, wFuncName);

  return createJFunctionInstanceContext(env, funcInstance);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ModuleInstanceContext_listTable(
    JNIEnv *env, jobject thisObject) {
  WasmEdge_ModuleInstanceContext *moduleInstanceContext =
      getModuleInstanceContext(env, thisObject);

  uint32_t tabLen =
      WasmEdge_ModuleInstanceListTableLength(moduleInstanceContext);
  WasmEdge_String *nameList =
      (WasmEdge_String *)malloc(sizeof(struct WasmEdge_String) * tabLen);
  uint32_t RealTabNum =
      WasmEdge_ModuleInstanceListTable(moduleInstanceContext, nameList, tabLen);

  jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealTabNum);

  free(nameList);

  return jNameList;
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ModuleInstanceContext_findTable(
    JNIEnv *env, jobject thisObject, jstring jTabName) {
  WasmEdge_ModuleInstanceContext *moduleInstanceContext =
      getModuleInstanceContext(env, thisObject);
  WasmEdge_String wTabName = JStringToWasmString(env, jTabName);

  WasmEdge_TableInstanceContext *tabInst =
      WasmEdge_ModuleInstanceFindTable(moduleInstanceContext, wTabName);
  jobject jTabInst = createJTableInstanceContext(env, tabInst);

  WasmEdge_StringDelete(wTabName);

  return jTabInst;
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ModuleInstanceContext_listMemory(
    JNIEnv *env, jobject thisObject) {
  WasmEdge_ModuleInstanceContext *moduleInstanceContext =
      getModuleInstanceContext(env, thisObject);

  uint32_t memLen =
      WasmEdge_ModuleInstanceListMemoryLength(moduleInstanceContext);
  WasmEdge_String *nameList =
      (WasmEdge_String *)malloc(sizeof(struct WasmEdge_String) * memLen);
  uint32_t RealMemNum = WasmEdge_ModuleInstanceListMemory(moduleInstanceContext,
                                                          nameList, memLen);

  jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealMemNum);

  free(nameList);

  return jNameList;
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ModuleInstanceContext_findMemory(
    JNIEnv *env, jobject thisObject, jstring jMemName) {
  WasmEdge_ModuleInstanceContext *moduleInstanceContext =
      getModuleInstanceContext(env, thisObject);

  WasmEdge_String wMemName = JStringToWasmString(env, jMemName);

  WasmEdge_MemoryInstanceContext *memInst =
      WasmEdge_ModuleInstanceFindMemory(moduleInstanceContext, wMemName);
  jobject jMemInst = createJMemoryInstanceContext(env, memInst);

  WasmEdge_StringDelete(wMemName);

  return jMemInst;
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ModuleInstanceContext_listGlobal(
    JNIEnv *env, jobject thisObject) {
  WasmEdge_ModuleInstanceContext *moduleInstanceContext =
      getModuleInstanceContext(env, thisObject);
  uint32_t globLen =
      WasmEdge_ModuleInstanceListGlobalLength(moduleInstanceContext);
  WasmEdge_String *nameList =
      (WasmEdge_String *)malloc(sizeof(struct WasmEdge_String) * globLen);
  uint32_t RealGlobNum = WasmEdge_ModuleInstanceListGlobal(
      moduleInstanceContext, nameList, globLen);

  jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealGlobNum);

  free(nameList);

  return jNameList;
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ModuleInstanceContext_findGlobal(
    JNIEnv *env, jobject thisObject, jstring jGlobalName) {
  WasmEdge_ModuleInstanceContext *moduleInstanceContext =
      getModuleInstanceContext(env, thisObject);
  WasmEdge_String wGlobName = JStringToWasmString(env, jGlobalName);

  WasmEdge_GlobalInstanceContext *globInst =
      WasmEdge_ModuleInstanceFindGlobal(moduleInstanceContext, wGlobName);
  return createJGlobalInstanceContext(env, globInst);
}

jobject
createJModuleInstanceContext(JNIEnv *env,
                             const WasmEdge_ModuleInstanceContext *impObj) {

  jclass clazz = (*env)->FindClass(env, ORG_WASMEDGE_MODULEINSTANCECONTEXT);

  jmethodID constructorId =
      (*env)->GetMethodID(env, clazz, DEFAULT_CONSTRUCTOR, LONG_VOID);

  return (*env)->NewObject(env, clazz, constructorId, (long)impObj);
}
