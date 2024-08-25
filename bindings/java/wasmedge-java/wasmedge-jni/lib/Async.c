// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ExportTypeContext.h"
#include "ImportTypeContext.h"
#include "ValueType.h"
#include "common.h"
#include "jni.h"
#include "wasmedge/wasmedge.h"
#include <stdlib.h>

// Warning, we need type cast in return.

WasmEdge_Async *getAsync(JNIEnv *env, jobject thisObject) {
  if (thisObject == NULL) {
    return NULL;
  }
  return (WasmEdge_Async *)getPointer(env, thisObject);
}

JNIEXPORT void JNICALL Java_org_wasmedge_Async_asyncWait(JNIEnv *env,
                                                         jobject thisobject) {
  WasmEdge_Async *ctx = getAsync(env, thisobject);
  WasmEdge_AsyncWait(ctx);
}

JNIEXPORT jboolean JNICALL Java_org_wasmedge_Async_waitFor(JNIEnv *env,
                                                           jobject thisobject,
                                                           jlong milliseconds) {
  WasmEdge_Async *ctx = getAsync(env, thisobject);
  uint64_t Milliseconds = milliseconds;
  return WasmEdge_AsyncWaitFor(ctx, Milliseconds);
}

JNIEXPORT void JNICALL Java_org_wasmedge_Async_cancel(JNIEnv *env,
                                                      jobject thisobject) {
  WasmEdge_Async *ctx = getAsync(env, thisobject);
  WasmEdge_AsyncCancel(ctx);
}

JNIEXPORT jint JNICALL
Java_org_wasmedge_Async_getReturnsLength(JNIEnv *env, jobject thisobject) {
  WasmEdge_Async *ctx = getAsync(env, thisobject);
  return (jint)WasmEdge_AsyncGetReturnsLength(ctx);
}

JNIEXPORT void JNICALL Java_org_wasmedge_Async_get(JNIEnv *env,
                                                   jobject thisobject,
                                                   jobjectArray jreturns,
                                                   jintArray jReturnTypes) {
  WasmEdge_Async *ctx = getAsync(env, thisobject);
  jsize returnsLen = (*env)->GetArrayLength(env, jreturns);
  WasmEdge_Value *returns = calloc(returnsLen, sizeof(WasmEdge_Value));
  int *type = (*env)->GetIntArrayElements(env, jReturnTypes, JNI_FALSE);
  for (int i = 0; i < returnsLen; i++) {
    WasmEdge_Value val;

    jobject val_object = (*env)->GetObjectArrayElement(env, jreturns, i);

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
    returns[i] = val;
  }
  WasmEdge_Result result = WasmEdge_AsyncGet(ctx, returns, returnsLen);
  handleWasmEdgeResult(env, &result);

  for (int i = 0; i < returnsLen; ++i) {
    setJavaValueObject(env, returns[i],
                       (*env)->GetObjectArrayElement(env, jreturns, i));
  }
}

JNIEXPORT void JNICALL Java_org_wasmedge_Async_close(JNIEnv *env,
                                                     jobject thisobject) {
  WasmEdge_Async *ctx = getAsync(env, thisobject);
  WasmEdge_AsyncDelete(ctx);
}

jobject createJAsyncObject(JNIEnv *env, WasmEdge_Async *asyncObj) {

  jclass clazz = (*env)->FindClass(env, ORG_WASMEDGE_ASYNC);
  jmethodID constructorId =
      (*env)->GetMethodID(env, clazz, DEFAULT_CONSTRUCTOR, LONG_VOID);
  return (*env)->NewObject(env, clazz, constructorId, (long)asyncObj);
}
