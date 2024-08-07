// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "../jni/org_wasmedge_GlobalInstanceContext.h"
#include "TableTypeContext.h"
#include "ValueType.h"
#include "common.h"
#include "wasmedge/wasmedge.h"

GETTER(TableInstanceContext)

JNIEXPORT void JNICALL Java_org_wasmedge_TableInstanceContext_nativeInit(
    JNIEnv *env, jobject thisObject, jobject jTableTypeContext) {
  WasmEdge_TableTypeContext *tableTypeContext =
      getTableTypeContext(env, jTableTypeContext);
  WasmEdge_TableInstanceContext *tableInstanceContext =
      WasmEdge_TableInstanceCreate(tableTypeContext);
  setPointer(env, thisObject, (long)tableInstanceContext);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_TableInstanceContext_close(JNIEnv *env, jobject thisObject) {
  WasmEdge_TableInstanceContext *tableInstanceContext =
      getTableInstanceContext(env, thisObject);
  WasmEdge_TableInstanceDelete(tableInstanceContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_TableInstanceContext_setData(
    JNIEnv *env, jobject thisObject, jobject jVal, jint jOffSet) {
  WasmEdge_TableInstanceContext *tableInstanceContext =
      getTableInstanceContext(env, thisObject);
  WasmEdge_Value data = JavaValueToWasmEdgeValue(env, jVal);
  WasmEdge_Result result =
      WasmEdge_TableInstanceSetData(tableInstanceContext, data, jOffSet);
  handleWasmEdgeResult(env, &result);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_TableInstanceContext_getData(
    JNIEnv *env, jobject thisObject, jobject jValType, jint jOffSet) {
  // TODO fixme
  WasmEdge_TableInstanceContext *tableInstanceContext =
      getTableInstanceContext(env, thisObject);

  jclass typeClass = (*env)->GetObjectClass(env, jValType);
  jmethodID typeGetter =
      (*env)->GetMethodID(env, typeClass, GET_VALUE, VOID_INT);

  jint valType = (*env)->CallIntMethod(env, jValType, typeGetter);

  WasmEdge_Value val;

  switch (valType) {
  case WasmEdge_ValType_I32:
    val = WasmEdge_ValueGenI32(0);
    break;
  case WasmEdge_ValType_I64:
    val = WasmEdge_ValueGenF64(0);
    break;
  case WasmEdge_ValType_F32:
    val = WasmEdge_ValueGenF32(0.0);
    break;
  case WasmEdge_ValType_F64:
    val = WasmEdge_ValueGenF64(0.0);
    break;
  case WasmEdge_ValType_FuncRef:
    val = WasmEdge_ValueGenNullRef(WasmEdge_RefType_FuncRef);
    break;
  case WasmEdge_ValType_ExternRef:
    val = WasmEdge_ValueGenNullRef(WasmEdge_RefType_ExternRef);
    break;
  }

  WasmEdge_TableInstanceGetData(tableInstanceContext, &val, jOffSet);
  return WasmEdgeValueToJavaValue(env, val);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_TableInstanceContext_getSize(
    JNIEnv *env, jobject thisObject) {
  WasmEdge_TableInstanceContext *tableInstanceContext =
      getTableInstanceContext(env, thisObject);
  return WasmEdge_TableInstanceGetSize(tableInstanceContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_TableInstanceContext_grow(
    JNIEnv *env, jobject thisObject, jint jSize) {
  WasmEdge_TableInstanceContext *tableInstanceContext =
      getTableInstanceContext(env, thisObject);
  WasmEdge_Result result =
      WasmEdge_TableInstanceGrow(tableInstanceContext, jSize);
  handleWasmEdgeResult(env, &result);
}

jobject
createJTableInstanceContext(JNIEnv *env,
                            const WasmEdge_TableInstanceContext *tabInstance) {

  // FIXME add to all instances.
  if (tabInstance == NULL) {
    return NULL;
  }

  jclass clazz = (*env)->FindClass(env, ORG_WASMEDGE_TABLEINSTANCECONTEXT);
  jmethodID constructorId =
      (*env)->GetMethodID(env, clazz, DEFAULT_CONSTRUCTOR, LONG_VOID);
  return (*env)->NewObject(env, clazz, constructorId, (long)tabInstance);
}
