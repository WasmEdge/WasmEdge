// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "../jni/org_wasmedge_TableTypeContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"

GETTER(TableTypeContext)

JNIEXPORT void JNICALL Java_org_wasmedge_TableTypeContext_nativeInit(
    JNIEnv *env, jobject thisObject, jint refType, jobject jLimit) {

  jclass cls = (*env)->GetObjectClass(env, jLimit);

  jmethodID hasMaxMid =
      (*env)->GetMethodID(env, cls, LIMIT_IS_HAS_MAX, VOID_BOOL);
  jboolean hasMax = (*env)->CallBooleanMethod(env, jLimit, hasMaxMid);

  jmethodID maxMid = (*env)->GetMethodID(env, cls, LIMIT_GET_MAX, VOID_LONG);
  jlong max = (*env)->CallLongMethod(env, jLimit, maxMid);

  jmethodID minMid = (*env)->GetMethodID(env, cls, LIMIT_GET_MIN, VOID_LONG);
  jlong min = (*env)->CallLongMethod(env, jLimit, minMid);

  WasmEdge_Limit tableLimit = {.HasMax = hasMax, .Min = min, .Max = max};
  WasmEdge_TableTypeContext *tableTypeContext =
      WasmEdge_TableTypeCreate((enum WasmEdge_RefType)refType, tableLimit);
  setPointer(env, thisObject, (long)tableTypeContext);
}

JNIEXPORT jobject JNICALL
Java_org_wasmedge_TableTypeContext_getLimit(JNIEnv *env, jobject thisObject) {
  WasmEdge_TableTypeContext *tableTypeContext =
      getTableTypeContext(env, thisObject);

  WasmEdge_Limit limit = WasmEdge_TableTypeGetLimit(tableTypeContext);

  jclass limitClass = findJavaClass(env, ORG_WASMEDGE_LIMIT);

  jmethodID constructor =
      findJavaMethod(env, limitClass, DEFAULT_CONSTRUCTOR, BOOLLONGLONG_VOID);

  return (*env)->NewObject(env, limitClass, constructor, (jboolean)limit.HasMax,
                           (jlong)limit.Min, (jlong)limit.Max);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_TableTypeContext_nativeGetRefType(
    JNIEnv *env, jobject thisObject) {
  WasmEdge_TableTypeContext *tableTypeContext =
      getTableTypeContext(env, thisObject);

  return WasmEdge_TableTypeGetRefType(tableTypeContext);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_TableTypeContext_close(JNIEnv *env, jobject thisObject) {
  WasmEdge_TableTypeContext *tableTypeContext =
      getTableTypeContext(env, thisObject);
  WasmEdge_TableTypeDelete(tableTypeContext);
}

jobject
createJTableTypeContext(JNIEnv *env,
                        const WasmEdge_TableTypeContext *tableTypeContext) {

  jclass clazz = (*env)->FindClass(env, ORG_WASMEDGE_TABLETYPECONTEXT);

  jmethodID constructorId =
      (*env)->GetMethodID(env, clazz, DEFAULT_CONSTRUCTOR, LONG_VOID);

  jobject table =
      (*env)->NewObject(env, clazz, constructorId, (long)tableTypeContext);

  return table;
}
