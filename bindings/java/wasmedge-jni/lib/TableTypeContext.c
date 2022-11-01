// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "../jni/org_wasmedge_TableTypeContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"

WasmEdge_TableTypeContext *getTableTypeContext(JNIEnv *env,
                                               jobject jTableTypeContext) {

  if (jTableTypeContext == NULL) {
    return NULL;
  }
  WasmEdge_TableTypeContext *tableTypeContext =
      (WasmEdge_TableTypeContext *)getPointer(env, jTableTypeContext);

  return tableTypeContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_TableTypeContext_nativeInit(
    JNIEnv *env, jobject thisObject, jint refType, jobject jLimit) {

  jclass cls = (*env)->GetObjectClass(env, jLimit);

  jmethodID hasMaxMid = (*env)->GetMethodID(env, cls, "isHasMax", "()Z");
  jboolean hasMax = (*env)->CallBooleanMethod(env, jLimit, hasMaxMid);

  jmethodID maxMid = (*env)->GetMethodID(env, cls, "getMax", "()J");
  jlong max = (*env)->CallLongMethod(env, jLimit, maxMid);

  jmethodID minMid = (*env)->GetMethodID(env, cls, "getMin", "()J");
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

  jclass limitClass = findJavaClass(env, "org/wasmedge/WasmEdgeLimit");

  jmethodID constructor = findJavaMethod(env, limitClass, "<init>", "(ZJJ)V");

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
Java_org_wasmedge_TableTypeContext_delete(JNIEnv *env, jobject thisObject) {
  WasmEdge_TableTypeContext *tableTypeContext =
      getTableTypeContext(env, thisObject);
  WasmEdge_TableTypeDelete(tableTypeContext);
}

jobject
createJTableTypeContext(JNIEnv *env,
                        const WasmEdge_TableTypeContext *tableTypeContext) {

  jclass clazz = (*env)->FindClass(env, "org/wasmedge/TableTypeContext");

  jmethodID constructorId = (*env)->GetMethodID(env, clazz, "<init>", "(J)V");

  jobject table =
      (*env)->NewObject(env, clazz, constructorId, (long)tableTypeContext);

  return table;
}
