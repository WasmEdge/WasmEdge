//
// Created by Kenvi Zhu on 2022-01-12.
//
// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "../jni/org_wasmedge_GlobalInstanceContext.h"
#include "GlobalTypeContext.h"
#include "ValueType.h"
#include "common.h"

GETTER(GlobalInstanceContext)

JNIEXPORT void JNICALL Java_org_wasmedge_GlobalInstanceContext_nativeInit(
    JNIEnv *env, jobject thisObject, jobject jGlobalTypeCxt, jobject jVal) {
  WasmEdge_GlobalTypeContext *globalTypeContext =
      getGlobalTypeContext(env, jGlobalTypeCxt);

  WasmEdge_GlobalInstanceContext *globalInstanceContext =
      WasmEdge_GlobalInstanceCreate(globalTypeContext,
                                    JavaValueToWasmEdgeValue(env, jVal));
  setPointer(env, thisObject, (long)globalInstanceContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_GlobalInstanceContext_nativeSetValue(
    JNIEnv *env, jobject thisObject, jobject jVal) {
  WasmEdge_GlobalInstanceContext *globalInstanceContext =
      getGlobalInstanceContext(env, thisObject);

  WasmEdge_Value value = JavaValueToWasmEdgeValue(env, jVal);

  WasmEdge_GlobalInstanceSetValue(globalInstanceContext, value);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_GlobalInstanceContext_close(JNIEnv *env, jobject thisObject) {
  WasmEdge_GlobalInstanceContext *globalInstanceContext =
      getGlobalInstanceContext(env, thisObject);
  WasmEdge_GlobalInstanceDelete(globalInstanceContext);
}

jobject createJGlobalInstanceContext(
    JNIEnv *env, const WasmEdge_GlobalInstanceContext *globInstance) {

  // FIXME add to all instances.
  if (globInstance == NULL) {
    return NULL;
  }

  jclass clazz = (*env)->FindClass(env, ORG_WASMEDGE_GLOBALINSTANCECONTEXT);
  jmethodID constructorId =
      (*env)->GetMethodID(env, clazz, DEFAULT_CONSTRUCTOR, LONG_VOID);
  return (*env)->NewObject(env, clazz, constructorId, (long)globInstance);
}
