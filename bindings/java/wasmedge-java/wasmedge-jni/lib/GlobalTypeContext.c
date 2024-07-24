//
// Created by Kenvi Zhu on 2021-12-07.
// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "../jni/org_wasmedge_GlobalTypeContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"

GETTER(GlobalTypeContext)

JNIEXPORT void JNICALL Java_org_wasmedge_GlobalTypeContext_nativeInit(
    JNIEnv *env, jobject thisObject, jint valueType, jint mutability) {

  // valueType conversion
  uint8_t type = valueType;
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

  WasmEdge_GlobalTypeContext *globalTypeContext = WasmEdge_GlobalTypeCreate(
      finalType, mutability);
  setPointer(env, thisObject, (jlong)globalTypeContext);
}

jobject
createJGlobalTypeContext(JNIEnv *env,
                         const WasmEdge_GlobalTypeContext *globalTypeContext) {

  jclass clazz = (*env)->FindClass(env, ORG_WASMEDGE_GLOBALTYPECONTEXT);
  jmethodID constructorId = (*env)->GetMethodID(env, clazz, DEFAULT_CONSTRUCTOR, LONG_VOID);
  return (*env)->NewObject(env, clazz, constructorId, (long)globalTypeContext);
}
JNIEXPORT void JNICALL
Java_org_wasmedge_GlobalTypeContext_close(JNIEnv *env, jobject thisObject) {
  WasmEdge_GlobalTypeContext *wasmEdgeGlobalTypeContext =
      getGlobalTypeContext(env, thisObject);
  setPointer(env, thisObject, 0);
  WasmEdge_GlobalTypeDelete(wasmEdgeGlobalTypeContext);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_GlobalTypeContext_nativeGetValueType(
    JNIEnv *env, jobject thisObject) {
  WasmEdge_GlobalTypeContext *globalTypeContext = getGlobalTypeContext(env, thisObject);
  WasmEdge_ValType valType = WasmEdge_GlobalTypeGetValType(globalTypeContext);

  // Convert WasmEdge_ValType to jint
  jint result;
  if(WasmEdge_ValTypeIsI32(valType)) {
      result = 0x7F;
  }else if (WasmEdge_ValTypeIsI64(valType)) {
      result = 0x7E;
  }else if (WasmEdge_ValTypeIsF32(valType)) {
      result = 0x7D;
  }else if (WasmEdge_ValTypeIsF64(valType)) {
      result = 0x7C;
  }else if (WasmEdge_ValTypeIsV128(valType)) {
      result = 0x7B;
  }else if (WasmEdge_ValTypeIsFuncRef(valType)) {
      result = 0x70;
  }else if (WasmEdge_ValTypeIsExternRef(valType)) {
      result = 0x6F;
  }else {
      // Handle unexpected type case
      jclass exceptionClass = (*env)->FindClass(env, "org/wasmedge/WasmEdgeException");
      if (exceptionClass != NULL) {
          (*env)->ThrowNew(env, exceptionClass, "Unknown WasmEdge_ValType encountered.");
      }
      return -1;
  }

  return result;
}
    
JNIEXPORT jint JNICALL Java_org_wasmedge_GlobalTypeContext_nativeGetMutability(
    JNIEnv *env, jobject thisObject) {
  return WasmEdge_GlobalTypeGetMutability(
      getGlobalTypeContext(env, thisObject));
}
