// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "../jni/org_wasmedge_MemoryTypeContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"

GETTER(MemoryTypeContext)

JNIEXPORT void JNICALL Java_org_wasmedge_MemoryTypeContext_nativeInit(
    JNIEnv *env, jobject thisObject, jboolean jHasMax, jlong jMin, jlong jMax) {

  const WasmEdge_Limit limit = {.HasMax = jHasMax, .Min = jMin, .Max = jMax};

  WasmEdge_MemoryTypeContext *memCxt = WasmEdge_MemoryTypeCreate(limit);

  setPointer(env, thisObject, (long)memCxt);
}
JNIEXPORT void JNICALL
Java_org_wasmedge_MemoryTypeContext_close(JNIEnv *env, jobject thisObject) {
  WasmEdge_MemoryTypeContext *memoryTypeContext =
      getMemoryTypeContext(env, thisObject);
  WasmEdge_MemoryTypeDelete(memoryTypeContext);
  setPointer(env, thisObject, 0);
}

jobject
createJMemoryTypeContext(JNIEnv *env,
                         const WasmEdge_MemoryTypeContext *memTypeContext) {

  jclass clazz = (*env)->FindClass(env, ORG_WASMEDGE_MEMORYTYPECONTEXT);

  jmethodID constructorId =
      (*env)->GetMethodID(env, clazz, DEFAULT_CONSTRUCTOR, LONG_VOID);

  return (*env)->NewObject(env, clazz, constructorId, (long)memTypeContext);
}

JNIEXPORT jobject JNICALL
Java_org_wasmedge_MemoryTypeContext_getLimit(JNIEnv *env, jobject thisObject) {

  WasmEdge_MemoryTypeContext *memoryTypeContext =
      getMemoryTypeContext(env, thisObject);
  WasmEdge_Limit limit = WasmEdge_MemoryTypeGetLimit(memoryTypeContext);

  jclass limitClass = findJavaClass(env, ORG_WASMEDGE_LIMIT);

  jmethodID constructor =
      findJavaMethod(env, limitClass, DEFAULT_CONSTRUCTOR, BOOLLONGLONG_VOID);

  return (*env)->NewObject(env, limitClass, constructor, (jboolean)limit.HasMax,
                           (jlong)limit.Min, (jlong)limit.Max);
}
