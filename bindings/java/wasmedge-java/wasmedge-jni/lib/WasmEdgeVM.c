// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "AstModuleContext.h"
#include "Async.h"
#include "ConfigureContext.h"
#include "FunctionTypeContext.h"
#include "ModuleInstanceContext.h"
#include "StatisticsContext.h"
#include "StoreContext.h"
#include "common.h"
#include "jni.h"
#include "string.h"
#include "wasmedge/wasmedge.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void setJavaIntValue(JNIEnv *env, WasmEdge_Value val, jobject jobj) {
  int int_val = WasmEdge_ValueGetI32(val);
  jclass val_clazz = (*env)->GetObjectClass(env, jobj);
  jmethodID val_setter =
      (*env)->GetMethodID(env, val_clazz, SET_VALUE_METHOD, INT_VOID);
  (*env)->CallIntMethod(env, jobj, val_setter, int_val);
}

void setJavaLongValue(JNIEnv *env, WasmEdge_Value val, jobject jobj) {
  int long_val = WasmEdge_ValueGetI64(val);
  jclass val_clazz = (*env)->GetObjectClass(env, jobj);
  jmethodID val_setter =
      (*env)->GetMethodID(env, val_clazz, SET_VALUE_METHOD, LONG_VOID);
  (*env)->CallLongMethod(env, jobj, val_setter, long_val);
}

void setJavaFloatValue(JNIEnv *env, WasmEdge_Value val, jobject jobj) {
  float float_val = WasmEdge_ValueGetF32(val);
  jclass val_clazz = (*env)->GetObjectClass(env, jobj);
  jmethodID val_setter =
      (*env)->GetMethodID(env, val_clazz, SET_VALUE_METHOD, FLOAT_VOID);
  (*env)->CallFloatMethod(env, jobj, val_setter, float_val);
}

void setJavaDoubleValue(JNIEnv *env, WasmEdge_Value val, jobject jobj) {
  float double_val = WasmEdge_ValueGetF64(val);
  jclass val_clazz = (*env)->GetObjectClass(env, jobj);
  jmethodID val_setter =
      (*env)->GetMethodID(env, val_clazz, SET_VALUE_METHOD, DOUBLE_VOID);
  (*env)->CallFloatMethod(env, jobj, val_setter, double_val);
}

void setJavaStringValue(JNIEnv *env, char *val, jobject jobj) {
  jclass val_clazz = (*env)->GetObjectClass(env, jobj);

  jmethodID val_setter =
      (*env)->GetMethodID(env, val_clazz, SET_VALUE_METHOD, STRING_VOID);

  jstring jkey = (*env)->NewStringUTF(env, val);
  (*env)->CallObjectMethod(env, jobj, val_setter, jkey);
}

WasmEdge_VMContext *getVmContext(JNIEnv *env, jobject vmContextObj) {
  long pointerVal = getPointer(env, vmContextObj);
  return (WasmEdge_VMContext *)pointerVal;
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_runWasmFromFile(
    JNIEnv *env, jobject this_object, jstring file_path, jstring func_name,
    jobjectArray params, jint param_size, jintArray param_types,
    jobjectArray returns, jint return_size, jintArray return_types) {

  /* The configure and store context to the VM creation can be NULL. */
  WasmEdge_VMContext *VMCxt = getVmContext(env, this_object);

  /* The parameters and returns arrays. */
  WasmEdge_Value *wasm_params = calloc(param_size, sizeof(WasmEdge_Value));
  int *type = (*env)->GetIntArrayElements(env, param_types, JNI_FALSE);
  for (int i = 0; i < param_size; i++) {
    WasmEdge_Value val;

    jobject val_object = (*env)->GetObjectArrayElement(env, params, i);

    switch (type[i]) {

    case 0:
      val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
      break;
    case 1:
      val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
      break;
    case 2:
      val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
      break;
    case 3:
      val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
      break;
    default:
      break;
    }
    wasm_params[i] = val;
  }

  //    WasmEdge_Value* WasmRetuns = calloc(return_size, sizeof
  //    (WasmEdge_Value));
  //    /* Function name. */
  const char *c_func_name = (*env)->GetStringUTFChars(env, func_name, NULL);
  const char *c_file_path = (*env)->GetStringUTFChars(env, file_path, NULL);

  /* The parameters and returns arrays. */
  // WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(5) };
  WasmEdge_Value *Returns = malloc(sizeof(WasmEdge_Value) * return_size);
  /* Function name. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString(c_func_name);
  /* Run the WASM function from file. */
  WasmEdge_Result Res =
      WasmEdge_VMRunWasmFromFile(VMCxt, c_file_path, FuncName, wasm_params,
                                 param_size, Returns, return_size);

  if (WasmEdge_ResultOK(Res)) {
    for (int i = 0; i < return_size; ++i) {
      setJavaValueObject(env, Returns[i],
                         (*env)->GetObjectArrayElement(env, returns, i));
    }
  } else {
    char exceptionBuffer[1024];
    sprintf(exceptionBuffer, ERR_RUN_FROM_FILE_TEMPLATE, c_file_path,
            WasmEdge_ResultGetMessage(Res));

    (*env)->ThrowNew(env, (*env)->FindClass(env, JAVA_LANG_EXCEPTION),
                     exceptionBuffer);
  }

  /* Resources deallocations. */
  WasmEdge_StringDelete(FuncName);
  (*env)->ReleaseStringUTFChars(env, func_name, c_func_name);
  (*env)->ReleaseStringUTFChars(env, file_path, c_file_path);
  free(wasm_params);
  free(Returns);
  return;
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_nativeInit(
    JNIEnv *env, jobject thisObject, jobject jConfigureContext,
    jobject jStoreContext) {
  WasmEdge_ConfigureContext *ConfigureContext =
      getConfigureContext(env, jConfigureContext);
  WasmEdge_StoreContext *StoreContext = getStoreContext(env, jStoreContext);

  WasmEdge_VMContext *VMContext =
      WasmEdge_VMCreate(ConfigureContext, StoreContext);

  setPointer(env, thisObject, (jlong)VMContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_loadWasmFromFile(
    JNIEnv *env, jobject thisObject, jstring filePath) {
  const char *c_file_path = (*env)->GetStringUTFChars(env, filePath, NULL);
  WasmEdge_Result res =
      WasmEdge_VMLoadWasmFromFile(getVmContext(env, thisObject), c_file_path);
  handleWasmEdgeResult(env, &res);

  (*env)->ReleaseStringUTFChars(env, filePath, c_file_path);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_WasmEdgeVm_validate(JNIEnv *env, jobject thisObject) {
  WasmEdge_Result result = WasmEdge_VMValidate(getVmContext(env, thisObject));
  handleWasmEdgeResult(env, &result);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_WasmEdgeVm_instantiate(JNIEnv *env, jobject thisObject) {
  WasmEdge_Result result =
      WasmEdge_VMInstantiate(getVmContext(env, thisObject));
  handleWasmEdgeResult(env, &result);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_execute(
    JNIEnv *env, jobject thisObject, jstring funcName, jobjectArray params,
    jint paramSize, jintArray paramTypes, jobjectArray returns, jint returnSize,
    jintArray returnTypes) {

  WasmEdge_VMContext *VMCxt = getVmContext(env, thisObject);

  /* The parameters and returns arrays. */
  WasmEdge_Value *wasm_params = calloc(paramSize, sizeof(WasmEdge_Value));
  int *type = (*env)->GetIntArrayElements(env, paramTypes, JNI_FALSE);
  for (int i = 0; i < paramSize; i++) {
    WasmEdge_Value val;

    jobject val_object = (*env)->GetObjectArrayElement(env, params, i);

    switch (type[i]) {

    case 0:
      val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
      break;
    case 1:
      val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
      break;
    case 2:
      val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
      break;
    case 3:
      val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
      break;
    default:
      break;
    }
    wasm_params[i] = val;
  }

  //    WasmEdge_Value* WasmRetuns = calloc(return_size, sizeof
  //    (WasmEdge_Value));
  //    /* Function name. */
  const char *c_func_name = (*env)->GetStringUTFChars(env, funcName, NULL);

  /* The parameters and returns arrays. */
  // WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(5) };
  WasmEdge_Value *Returns = malloc(sizeof(WasmEdge_Value) * returnSize);
  /* Function name. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString(c_func_name);
  /* Run the WASM function from file. */
  WasmEdge_Result Res = WasmEdge_VMExecute(VMCxt, FuncName, wasm_params,
                                           paramSize, Returns, returnSize);

  handleWasmEdgeResult(env, &Res);
  if (WasmEdge_ResultOK(Res)) {
    for (int i = 0; i < returnSize; ++i) {
      setJavaValueObject(env, Returns[i],
                         (*env)->GetObjectArrayElement(env, returns, i));
    }
  }

  /* Resources deallocations. */
  WasmEdge_StringDelete(FuncName);
  (*env)->ReleaseStringUTFChars(env, funcName, c_func_name);
  free(wasm_params);
  free(Returns);
  return;
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_close(JNIEnv *env,
                                                          jobject thisObj) {
  WasmEdge_VMDelete(getVmContext(env, thisObj));
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_getFunctionList(
    JNIEnv *env, jobject thisObject, jobject jFuncList) {

  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);

  uint32_t funcLen = WasmEdge_VMGetFunctionListLength(vmContext);
  const WasmEdge_FunctionTypeContext **funcList =
      (const WasmEdge_FunctionTypeContext **)malloc(
          sizeof(WasmEdge_FunctionTypeContext *));
  WasmEdge_String *nameList =
      (WasmEdge_String *)malloc(sizeof(struct WasmEdge_String));
  uint32_t RealFuncNum =
      WasmEdge_VMGetFunctionList(vmContext, nameList, funcList, funcLen);

  ConvertToJavaFunctionList(env, nameList, funcList, RealFuncNum, jFuncList);

  free(funcList);
  free(nameList);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVm_getFunctionType(
    JNIEnv *env, jobject thisObject, jstring jFuncName) {
  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);

  const char *funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);
  WasmEdge_String wFuncName = WasmEdge_StringCreateByCString(funcName);
  const WasmEdge_FunctionTypeContext *functionTypeContext =
      WasmEdge_VMGetFunctionType(vmContext, wFuncName);

  (*env)->ReleaseStringUTFChars(env, jFuncName, funcName);

  if (functionTypeContext == NULL) {
    WasmEdge_StringDelete(wFuncName);
    return NULL;
  }

  jobject jFuncType =
      ConvertToJavaFunctionType(env, functionTypeContext, wFuncName);

  WasmEdge_StringDelete(wFuncName);
  return jFuncType;
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_registerModuleFromFile(
    JNIEnv *env, jobject thisObject, jstring jModName, jstring jFilePath) {
  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);

  const char *modName = (*env)->GetStringUTFChars(env, jModName, NULL);
  WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);
  const char *filePath = (*env)->GetStringUTFChars(env, jFilePath, NULL);

  WasmEdge_Result result =
      WasmEdge_VMRegisterModuleFromFile(vmContext, wModName, filePath);
  (*env)->ReleaseStringUTFChars(env, jModName, modName);
  (*env)->ReleaseStringUTFChars(env, jFilePath, filePath);
  WasmEdge_StringDelete(wModName);

  handleWasmEdgeResult(env, &result);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_registerModuleFromBuffer(
    JNIEnv *env, jobject thisObject, jstring jModName, jbyteArray jBuff) {

  WasmEdge_VMContext *vm = getVmContext(env, thisObject);

  jbyte *data = (*env)->GetByteArrayElements(env, jBuff, 0);
  jsize size = (*env)->GetArrayLength(env, jBuff);

  const char *modName = (*env)->GetStringUTFChars(env, jModName, NULL);

  WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);

  WasmEdge_VMRegisterModuleFromBuffer(vm, wModName, (unsigned char *)data,
                                      size);
  (*env)->ReleaseByteArrayElements(env, jBuff, data, size);
  (*env)->ReleaseStringUTFChars(env, jModName, modName);
  WasmEdge_StringDelete(wModName);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_registerModuleFromAstModule(
    JNIEnv *env, jobject thisObject, jstring jModName,
    jobject jAstModuleContext) {
  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);

  WasmEdge_ASTModuleContext *mod = getASTModuleContext(env, jAstModuleContext);

  const char *modName = (*env)->GetStringUTFChars(env, jModName, NULL);
  WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);

  WasmEdge_Result result =
      WasmEdge_VMRegisterModuleFromASTModule(vmContext, wModName, mod);

  (*env)->ReleaseStringUTFChars(env, jModName, modName);
  WasmEdge_StringDelete(wModName);

  handleWasmEdgeResult(env, &result);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_runWasmFromBuffer(
    JNIEnv *env, jobject thisObject, jbyteArray jBuff, jstring jFuncName,
    jobjectArray jParams, jintArray jParamTypes, jobjectArray jReturns,
    jintArray jReturnTypes) {

  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);

  jbyte *buff = (*env)->GetByteArrayElements(env, jBuff, 0);
  jsize size = (*env)->GetArrayLength(env, jBuff);

  const char *funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);
  WasmEdge_String wFuncName = WasmEdge_StringCreateByCString(funcName);

  jsize paramLen = (*env)->GetArrayLength(env, jParams);

  /* The parameters and returns arrays. */
  WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value));
  int *type = (*env)->GetIntArrayElements(env, jParamTypes, JNI_FALSE);
  for (int i = 0; i < paramLen; i++) {
    WasmEdge_Value val;

    jobject val_object = (*env)->GetObjectArrayElement(env, jParams, i);

    switch (type[i]) {

    case 0:
      val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
      break;
    case 1:
      val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
      break;
    case 2:
      val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
      break;
    case 3:
      val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
      break;
    default:
      break;
    }
    wasm_params[i] = val;
  }

  jsize returnLen = (*env)->GetArrayLength(env, jReturns);
  WasmEdge_Value *returns = malloc(sizeof(WasmEdge_Value) * returnLen);

  //
  WasmEdge_Result result =
      WasmEdge_VMRunWasmFromBuffer(vmContext, (uint8_t *)buff, size, wFuncName,
                                   wasm_params, paramLen, returns, returnLen);

  if (WasmEdge_ResultOK(result)) {
    for (int i = 0; i < returnLen; ++i) {
      setJavaValueObject(env, returns[i],
                         (*env)->GetObjectArrayElement(env, jReturns, i));
    }
  }

  // release resources
  (*env)->ReleaseByteArrayElements(env, jBuff, buff, size);
  (*env)->ReleaseStringUTFChars(env, jFuncName, funcName);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_runWasmFromAstModule(
    JNIEnv *env, jobject thisObject, jobject jAstMod, jstring jFuncName,
    jobjectArray jParams, jintArray jParamTypes, jobjectArray jReturns,
    jintArray jReturnTypes) {

  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);
  WasmEdge_ASTModuleContext *mod = getASTModuleContext(env, jAstMod);

  const char *funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);
  WasmEdge_String wFuncName = WasmEdge_StringCreateByCString(funcName);

  jsize paramLen = (*env)->GetArrayLength(env, jParams);

  /* The parameters and returns arrays. */
  WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value));
  int *type = (*env)->GetIntArrayElements(env, jParamTypes, JNI_FALSE);
  for (int i = 0; i < paramLen; i++) {
    WasmEdge_Value val;

    jobject val_object = (*env)->GetObjectArrayElement(env, jParams, i);

    switch (type[i]) {

    case 0:
      val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
      break;
    case 1:
      val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
      break;
    case 2:
      val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
      break;
    case 3:
      val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
      break;
    default:
      break;
    }
    wasm_params[i] = val;
  }

  jsize returnLen = (*env)->GetArrayLength(env, jReturns);
  WasmEdge_Value *returns = malloc(sizeof(WasmEdge_Value) * returnLen);

  //
  WasmEdge_Result result = WasmEdge_VMRunWasmFromASTModule(
      vmContext, mod, wFuncName, wasm_params, paramLen, returns, returnLen);

  if (WasmEdge_ResultOK(result)) {
    for (int i = 0; i < returnLen; ++i) {
      setJavaValueObject(env, returns[i],
                         (*env)->GetObjectArrayElement(env, jReturns, i));
    }
  }

  // release resources
  (*env)->ReleaseStringUTFChars(env, jFuncName, funcName);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_executeRegistered(
    JNIEnv *env, jobject thisObject, jstring jModName, jstring jFuncName,
    jobjectArray jParams, jintArray jParamTypes, jobjectArray jReturns,
    jintArray jReturnTypes) {
  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);

  const char *modName = (*env)->GetStringUTFChars(env, jModName, NULL);
  const char *funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);

  // wasm string
  WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);
  WasmEdge_String wFuncName = WasmEdge_StringCreateByCString(funcName);

  jsize paramLen = (*env)->GetArrayLength(env, jParams);

  /* The parameters and returns arrays. */
  WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value));
  int *type = (*env)->GetIntArrayElements(env, jParamTypes, JNI_FALSE);
  for (int i = 0; i < paramLen; i++) {
    WasmEdge_Value val;

    jobject val_object = (*env)->GetObjectArrayElement(env, jParams, i);

    switch (type[i]) {

    case 0:
      val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
      break;
    case 1:
      val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
      break;
    case 2:
      val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
      break;
    case 3:
      val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
      break;
    default:
      break;
    }
    wasm_params[i] = val;
  }

  jsize returnLen = (*env)->GetArrayLength(env, jReturns);
  WasmEdge_Value *returns = malloc(sizeof(WasmEdge_Value) * returnLen);

  //
  WasmEdge_Result result =
      WasmEdge_VMExecuteRegistered(vmContext, wModName, wFuncName, wasm_params,
                                   paramLen, returns, returnLen);

  if (WasmEdge_ResultOK(result)) {
    for (int i = 0; i < returnLen; ++i) {
      setJavaValueObject(env, returns[i],
                         (*env)->GetObjectArrayElement(env, jReturns, i));
    }
  }

  // release resources

  (*env)->ReleaseStringUTFChars(env, jModName, modName);
  (*env)->ReleaseStringUTFChars(env, jFuncName, funcName);
  WasmEdge_StringDelete(wModName);
  WasmEdge_StringDelete(wFuncName);
}

JNIEXPORT jobject JNICALL
Java_org_wasmedge_WasmEdgeVm_getStoreContext(JNIEnv *env, jobject thisObject) {
  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);
  WasmEdge_StoreContext *storeContext = WasmEdge_VMGetStoreContext(vmContext);
  return CreateJavaStoreContext(env, storeContext);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVm_getStatisticsContext(
    JNIEnv *env, jobject thisObject) {

  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);
  WasmEdge_StatisticsContext *statCxt =
      WasmEdge_VMGetStatisticsContext(vmContext);
  return CreateJavaStatisticsContext(env, statCxt);
}

JNIEXPORT jobject JNICALL
Java_org_wasmedge_WasmEdgeVm_nativeGetImportModuleContext(JNIEnv *env,
                                                          jobject thisObject,
                                                          jint reg) {
  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);
  WasmEdge_ModuleInstanceContext *imp = WasmEdge_VMGetImportModuleContext(
      vmContext, (enum WasmEdge_HostRegistration)reg);

  return createJModuleInstanceContext(env, imp);
}

JNIEXPORT jobject JNICALL
Java_org_wasmedge_WasmEdgeVm_getFunctionTypeRegistered(JNIEnv *env,
                                                       jobject thisObject,
                                                       jstring jModName,
                                                       jstring jFuncName) {
  WasmEdge_VMContext *vmCxt = getVmContext(env, thisObject);
  WasmEdge_String wModName = JStringToWasmString(env, jModName);
  WasmEdge_String wFuncName = JStringToWasmString(env, jFuncName);

  const WasmEdge_FunctionTypeContext *functionTypeContext =
      WasmEdge_VMGetFunctionTypeRegistered(vmCxt, wModName, wModName);

  WasmEdge_StringDelete(wModName);
  WasmEdge_StringDelete(wFuncName);

  return createJFunctionTypeContext(env, functionTypeContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVm_registerModuleFromImport(
    JNIEnv *env, jobject thisObject, jobject jImport) {
  WasmEdge_ModuleInstanceContext *impObj =
      getModuleInstanceContext(env, jImport);

  WasmEdge_VMContext *vm = getVmContext(env, thisObject);

  WasmEdge_Result result = WasmEdge_VMRegisterModuleFromImport(vm, impObj);
  handleWasmEdgeResult(env, &result);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVm_asyncRunWasmFromFile(
    JNIEnv *env, jobject thisObject, jstring jPath, jstring jFuncName,
    jobjectArray jParams, jintArray jParamTypes) {
  /* The configure and store context to the VM creation can be NULL. */
  WasmEdge_VMContext *VMCxt = getVmContext(env, thisObject);

  jsize jParamLen = (*env)->GetArrayLength(env, jParams);
  /* The parameters */
  WasmEdge_Value *wasm_params = calloc(jParamLen, sizeof(WasmEdge_Value));

  int *type = (*env)->GetIntArrayElements(env, jParamTypes, JNI_FALSE);
  for (int i = 0; i < jParamLen; i++) {
    WasmEdge_Value val;

    jobject val_object = (*env)->GetObjectArrayElement(env, jParams, i);

    switch (type[i]) {

    case 0:
      val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
      break;
    case 1:
      val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
      break;
    case 2:
      val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
      break;
    case 3:
      val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
      break;
    default:
      break;
    }
    wasm_params[i] = val;
  }

  //    /* Function name. */
  const char *c_func_name = (*env)->GetStringUTFChars(env, jFuncName, NULL);
  const char *c_file_path = (*env)->GetStringUTFChars(env, jPath, NULL);

  /* Function name. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString(c_func_name);
  /* Run the WASM function from file. */
  // Warning : need uint32_t but jParamLen is jint (long)
  WasmEdge_Async *async = WasmEdge_VMAsyncRunWasmFromFile(
      VMCxt, c_file_path, FuncName, wasm_params, jParamLen);

  WasmEdge_StringDelete(FuncName);
  (*env)->ReleaseStringUTFChars(env, jFuncName, c_func_name);
  (*env)->ReleaseStringUTFChars(env, jPath, c_file_path);
  free(wasm_params);
  return createJAsyncObject(env, async);
}

// Similar Warning as before
JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVm_asyncRunWasmFromBuffer(
    JNIEnv *env, jobject thisObject, jbyteArray jBuff, jstring jFuncName,
    jobjectArray jParams, jintArray jParamTypes) {

  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);

  jbyte *buff = (*env)->GetByteArrayElements(env, jBuff, 0);
  jsize size = (*env)->GetArrayLength(env, jBuff);

  const char *funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);
  WasmEdge_String wFuncName = WasmEdge_StringCreateByCString(funcName);

  jsize paramLen = (*env)->GetArrayLength(env, jParams);

  /* The parameters and returns arrays. */
  WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value));
  int *type = (*env)->GetIntArrayElements(env, jParamTypes, JNI_FALSE);
  for (int i = 0; i < paramLen; i++) {
    WasmEdge_Value val;

    jobject val_object = (*env)->GetObjectArrayElement(env, jParams, i);

    switch (type[i]) {

    case 0:
      val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
      break;
    case 1:
      val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
      break;
    case 2:
      val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
      break;
    case 3:
      val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
      break;
    default:
      break;
    }
    wasm_params[i] = val;
  }

  WasmEdge_Async *async = WasmEdge_VMAsyncRunWasmFromBuffer(
      vmContext, (unsigned char *)buff, size, wFuncName, wasm_params, paramLen);

  return createJAsyncObject(env, async);
}

JNIEXPORT jobject JNICALL
Java_org_wasmedge_WasmEdgeVm_asyncRunWasmFromAstModule(
    JNIEnv *env, jobject thisObject, jobject jAstMod, jstring jFuncName,
    jobjectArray jParams, jintArray jParamTypes) {

  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);
  WasmEdge_ASTModuleContext *mod = getASTModuleContext(env, jAstMod);

  const char *funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);
  WasmEdge_String wFuncName = WasmEdge_StringCreateByCString(funcName);

  jsize paramLen = (*env)->GetArrayLength(env, jParams);

  /* The parameters and returns arrays. */
  WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value));
  int *type = (*env)->GetIntArrayElements(env, jParamTypes, JNI_FALSE);
  for (int i = 0; i < paramLen; i++) {
    WasmEdge_Value val;

    jobject val_object = (*env)->GetObjectArrayElement(env, jParams, i);

    switch (type[i]) {

    case 0:
      val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
      break;
    case 1:
      val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
      break;
    case 2:
      val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
      break;
    case 3:
      val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
      break;
    default:
      break;
    }
    wasm_params[i] = val;
  }

  WasmEdge_Async *async = WasmEdge_VMAsyncRunWasmFromASTModule(
      vmContext, mod, wFuncName, wasm_params, paramLen);

  return createJAsyncObject(env, async);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVm_asyncExecute(
    JNIEnv *env, jobject thisObject, jstring jFuncName, jobjectArray jParams,
    jintArray jParamTypes) {

  WasmEdge_VMContext *VMCxt = getVmContext(env, thisObject);

  /* The parameters and returns arrays. */
  jsize paramLen = (*env)->GetArrayLength(env, jParams);
  WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value));
  int *type = (*env)->GetIntArrayElements(env, jParamTypes, JNI_FALSE);
  for (int i = 0; i < paramLen; i++) {
    WasmEdge_Value val;

    jobject val_object = (*env)->GetObjectArrayElement(env, jParams, i);

    switch (type[i]) {

    case 0:
      val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
      break;
    case 1:
      val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
      break;
    case 2:
      val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
      break;
    case 3:
      val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
      break;
    default:
      break;
    }
    wasm_params[i] = val;
  }

  //    WasmEdge_Value* WasmRetuns = calloc(return_size, sizeof
  //    (WasmEdge_Value));
  //    /* Function name. */
  const char *c_func_name = (*env)->GetStringUTFChars(env, jFuncName, NULL);

  /* Function name. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString(c_func_name);
  /* Run the WASM function from file. */
  WasmEdge_Async *async =
      WasmEdge_VMAsyncExecute(VMCxt, FuncName, wasm_params, paramLen);

  return createJAsyncObject(env, async);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVm_asyncExecuteRegistered(
    JNIEnv *env, jobject thisObject, jstring jModName, jstring jFuncName,
    jobjectArray jParams, jintArray jParamTypes) {
  WasmEdge_VMContext *vmContext = getVmContext(env, thisObject);

  const char *modName = (*env)->GetStringUTFChars(env, jModName, NULL);
  const char *funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);

  // wasm string
  WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);
  WasmEdge_String wFuncName = WasmEdge_StringCreateByCString(funcName);

  jsize paramLen = (*env)->GetArrayLength(env, jParams);

  /* The parameters and returns arrays. */
  WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value));
  int *type = (*env)->GetIntArrayElements(env, jParamTypes, JNI_FALSE);
  for (int i = 0; i < paramLen; i++) {
    WasmEdge_Value val;

    jobject val_object = (*env)->GetObjectArrayElement(env, jParams, i);

    switch (type[i]) {

    case 0:
      val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
      break;
    case 1:
      val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
      break;
    case 2:
      val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
      break;
    case 3:
      val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
      break;
    default:
      break;
    }
    wasm_params[i] = val;
  }

  WasmEdge_Async *async = WasmEdge_VMAsyncExecuteRegistered(
      vmContext, wModName, wFuncName, wasm_params, paramLen);

  return createJAsyncObject(env, async);
}
