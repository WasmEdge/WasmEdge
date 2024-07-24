// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "../jni/org_wasmedge_TableTypeContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"

GETTER(TableTypeContext)

JNIEXPORT void JNICALL Java_org_wasmedge_TableTypeContext_nativeInit(
    JNIEnv *env, jobject thisObject, jint refType, jobject jLimit) {

  jclass cls = (*env)->GetObjectClass(env, jLimit);

  jmethodID hasMaxMid = (*env)->GetMethodID(env, cls, LIMIT_IS_HAS_MAX, VOID_BOOL);
  jboolean hasMax = (*env)->CallBooleanMethod(env, jLimit, hasMaxMid);

  jmethodID maxMid = (*env)->GetMethodID(env, cls, LIMIT_GET_MAX, VOID_LONG);
  jlong max = (*env)->CallLongMethod(env, jLimit, maxMid);

  jmethodID minMid = (*env)->GetMethodID(env, cls, LIMIT_GET_MIN, VOID_LONG);
  jlong min = (*env)->CallLongMethod(env, jLimit, minMid);

  WasmEdge_Limit tableLimit = {.HasMax = hasMax, .Min = min, .Max = max};
  //jint refType to WasmEdge_ValType
  uint8_t type = refType;
  WasmEdge_ValType finalType;
  switch(type){
    case 0x7F:
        finalType = WasmEdge_ValTypeGenI32();
        break;
    case 0x7E:
        finalType = WasmEdge_ValTypeGenI64();
        break;
    case 0x7D:
        finalType = WasmEdge_ValTypeGenF32();
        break;
    case 0x7C:
        finalType = WasmEdge_ValTypeGenF64();
        break;
    case 0x7B:
        finalType = WasmEdge_ValTypeGenV128();
        break;
    case 0x70:
        finalType = WasmEdge_ValTypeGenFuncRef();
        break;
    case 0x6F:
        finalType = WasmEdge_ValTypeGenExternRef();
        break;
    }
  WasmEdge_TableTypeContext *tableTypeContext =
      WasmEdge_TableTypeCreate(finalType, tableLimit);
  setPointer(env, thisObject, (long)tableTypeContext);
}

JNIEXPORT jobject JNICALL
Java_org_wasmedge_TableTypeContext_getLimit(JNIEnv *env, jobject thisObject) {
  WasmEdge_TableTypeContext *tableTypeContext =
      getTableTypeContext(env, thisObject);

  WasmEdge_Limit limit = WasmEdge_TableTypeGetLimit(tableTypeContext);

  jclass limitClass = findJavaClass(env, ORG_WASMEDGE_LIMIT);

  jmethodID constructor = findJavaMethod(env, limitClass, DEFAULT_CONSTRUCTOR, BOOLLONGLONG_VOID);

  return (*env)->NewObject(env, limitClass, constructor, (jboolean)limit.HasMax,
                           (jlong)limit.Min, (jlong)limit.Max);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_TableTypeContext_nativeGetRefType(
    JNIEnv *env, jobject thisObject) {
WasmEdge_TableTypeContext *tableTypeContext = getTableTypeContext(env, thisObject);
  WasmEdge_ValType valType = WasmEdge_TableTypeGetRefType(tableTypeContext);

  // Convert WasmEdge_ValType to jint
  jint result;
  if (WasmEdge_ValTypeIsI32(valType)) {
      result = 0x7F;
  } else if (WasmEdge_ValTypeIsI64(valType)) {
      result = 0x7E;
  } else if (WasmEdge_ValTypeIsF32(valType)) {
      result = 0x7D;
  } else if (WasmEdge_ValTypeIsF64(valType)) {
      result = 0x7C;
  } else if (WasmEdge_ValTypeIsV128(valType)) {
      result = 0x7B;
  } else if (WasmEdge_ValTypeIsFuncRef(valType)) {
      result = 0x70;
  } else if (WasmEdge_ValTypeIsExternRef(valType)) {
      result = 0x6F;
  } else {
      // Handle unexpected type case
      jclass exceptionClass = (*env)->FindClass(env, "org/wasmedge/WasmEdgeException");
      if (exceptionClass != NULL) {
          (*env)->ThrowNew(env, exceptionClass, "Unknown WasmEdge_ValType encountered.");
      }
      return -1;
  }

  return result;  
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

  jmethodID constructorId = (*env)->GetMethodID(env, clazz, DEFAULT_CONSTRUCTOR, LONG_VOID);

  jobject table =
      (*env)->NewObject(env, clazz, constructorId, (long)tableTypeContext);

  return table;
}
