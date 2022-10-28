//
// Created by Kenvi Zhu on 2021-12-07.
// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "../jni/org_wasmedge_GlobalTypeContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"

WasmEdge_GlobalTypeContext *getGlobalTypeContext(JNIEnv *env,
                                                 jobject jGlobalTypeContext) {

  if (jGlobalTypeContext == NULL) {
    return NULL;
  }
  WasmEdge_GlobalTypeContext *globalTypeContext =
      (struct WasmEdge_GlobalTypeContext *)getPointer(env, jGlobalTypeContext);

  return globalTypeContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_GlobalTypeContext_nativeInit(
    JNIEnv *env, jobject thisObject, jint valueType, jint mutability) {

  WasmEdge_GlobalTypeContext *globalTypeContext = WasmEdge_GlobalTypeCreate(
      (enum WasmEdge_ValType)valueType, (enum WasmEdge_Mutability)mutability);
  setPointer(env, thisObject, (jlong)globalTypeContext);
}

jobject
createJGlobalTypeContext(JNIEnv *env,
                         const WasmEdge_GlobalTypeContext *globalTypeContext) {

  jclass clazz = (*env)->FindClass(env, "org/wasmedge/GlobalTypeContext");
  jmethodID constructorId = (*env)->GetMethodID(env, clazz, "<init>", "(J)V");
  return (*env)->NewObject(env, clazz, constructorId, (long)globalTypeContext);
}
JNIEXPORT void JNICALL
Java_org_wasmedge_GlobalTypeContext_delete(JNIEnv *env, jobject thisObject) {
  WasmEdge_GlobalTypeContext *wasmEdgeGlobalTypeContext =
      getGlobalTypeContext(env, thisObject);
  setPointer(env, thisObject, 0);
  WasmEdge_GlobalTypeDelete(wasmEdgeGlobalTypeContext);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_GlobalTypeContext_nativeGetValueType(
    JNIEnv *env, jobject thisObject) {
  return WasmEdge_GlobalTypeGetValType(getGlobalTypeContext(env, thisObject));
}
JNIEXPORT jint JNICALL Java_org_wasmedge_GlobalTypeContext_nativeGetMutability(
    JNIEnv *env, jobject thisObject) {
  return WasmEdge_GlobalTypeGetMutability(
      getGlobalTypeContext(env, thisObject));
}