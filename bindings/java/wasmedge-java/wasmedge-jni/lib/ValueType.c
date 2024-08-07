// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common.h"
#include "jni.h"
#include "wasmedge/wasmedge.h"
#include <string.h>

__int128_t atoint128_t(const char *s) {
  const char *p = s;
  __int128_t val = 0;

  if (*p == '-' || *p == '+') {
    p++;
  }
  while (*p >= '0' && *p <= '9') {
    val = (10 * val) + (*p - '0');
    p++;
  }
  if (*s == '-')
    val = val * -1;
  return val;
}

char *u128toa(uint128_t n) {
  static char buf[40];
  unsigned int i, j, m = 39;
  memset(buf, 0, 40);
  for (i = 128; i-- > 0;) {
    int carry = !!(n & ((uint128_t)1 << i));
    for (j = 39; j-- > m + 1 || carry;) {
      int d = 2 * buf[j] + carry;
      carry = d > 9;
      buf[j] = carry ? d - 10 : d;
    }
    m = j;
  }
  for (i = 0; i < 38; i++) {
    if (buf[i]) {
      break;
    }
  }
  for (j = i; j < 39; j++) {
    buf[j] += '0';
  }
  return buf + i;
}

WasmEdge_Value JavaValueToWasmEdgeValue(JNIEnv *env, jobject jVal) {
  jclass valueClass = (*env)->FindClass(env, ORG_WASMEDGE_VALUE);

  jmethodID getType =
      (*env)->GetMethodID(env, valueClass, GET_TYPE, VOID_VALUETYPE);

  jobject valType = (*env)->CallObjectMethod(env, jVal, getType);

  jclass typeClass = (*env)->GetObjectClass(env, valType);

  jmethodID getVal = (*env)->GetMethodID(env, typeClass, GET_VALUE, VOID_INT);

  jint jType = (*env)->CallIntMethod(env, valType, getVal);

  enum WasmEdge_ValType type = (enum WasmEdge_ValType)jType;

  WasmEdge_Value val;

  switch (type) {
  case WasmEdge_ValType_I32:
    return WasmEdge_ValueGenI32(getIntVal(env, jVal));
  case WasmEdge_ValType_I64:
    return WasmEdge_ValueGenI64(getLongVal(env, jVal));
  case WasmEdge_ValType_F32:
    return WasmEdge_ValueGenF32(getFloatVal(env, jVal));
  case WasmEdge_ValType_F64:
    return WasmEdge_ValueGenF64(getDoubleVal(env, jVal));
  case WasmEdge_ValType_V128:
    return WasmEdge_ValueGenV128(atoint128_t(getStringVal(env, jVal)));
  case WasmEdge_ValType_ExternRef:
    return WasmEdge_ValueGenExternRef(getStringVal(env, jVal));
  case WasmEdge_ValType_FuncRef:
    return WasmEdge_ValueGenFuncRef(
        (WasmEdge_FunctionInstanceContext *)getLongVal(env, jVal));
  }
}

jobject WasmEdgeValueToJavaValue(JNIEnv *env, WasmEdge_Value value) {
  const char *valClassName = NULL;
  switch (value.Type) {
  case WasmEdge_ValType_I32:
    valClassName = ORG_WASMEDGE_I32VALUE;
    break;
  case WasmEdge_ValType_I64:
    valClassName = ORG_WASMEDGE_I64VALUE;
    break;
  case WasmEdge_ValType_F32:
    valClassName = ORG_WASMEDGE_F32VALUE;
    break;
  case WasmEdge_ValType_V128:
    valClassName = ORG_WASMEDGE_V128VALUE;
    break;
  case WasmEdge_ValType_F64:
    valClassName = ORG_WASMEDGE_F64VALUE;
    break;
  case WasmEdge_ValType_ExternRef:
    valClassName = ORG_WASMEDGE_EXTERNREF;
    break;
  case WasmEdge_ValType_FuncRef:
    valClassName = ORG_WASMEDGE_FUNCREF;
    break;
  }
  jclass valClass = (*env)->FindClass(env, valClassName);

  jmethodID constructor =
      (*env)->GetMethodID(env, valClass, DEFAULT_CONSTRUCTOR, VOID_VOID);

  jobject jVal = (*env)->NewObject(env, valClass, constructor);

  setJavaValueObject(env, value, jVal);
  return jVal;
}
