// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include <string.h>
#include "common.h"
#include "jni.h"
#include "wasmedge/wasmedge.h"

__int128_t atoint128_t(const char *s)
{
    const char *p = s;
    __int128_t val = 0;

    if (*p == '-' || *p == '+') {
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        val = (10 * val) + (*p - '0');
        p++;
    }
    if (*s == '-') val = val * -1;
    return val;
}

char* u128toa(uint128_t n) {
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

  jmethodID getType = (*env)->GetMethodID(env, valueClass, GET_TYPE, VOID_VALUETYPE);

  jobject valType = (*env)->CallObjectMethod(env, jVal, getType);

  jclass typeClass = (*env)->GetObjectClass(env, valType);

  jmethodID getVal = (*env)->GetMethodID(env, typeClass, GET_VALUE, VOID_INT);

  jint jType = (*env)->CallIntMethod(env, valType, getVal);

  WasmEdge_Value val;

  if (jType == 0x7F) {
      val = WasmEdge_ValueGenI32(getIntVal(env, jVal));
  } else if (jType == 0x7E) {
      val = WasmEdge_ValueGenI64(getLongVal(env, jVal));
  } else if (jType == 0x7D) {
      val = WasmEdge_ValueGenF32(getFloatVal(env, jVal));
  } else if (jType == 0x7C) {
      val = WasmEdge_ValueGenF64(getDoubleVal(env, jVal));
  } else if (jType == 0x7B) {
      val = WasmEdge_ValueGenV128(atoint128_t(getStringVal(env, jVal)));
  } else if (jType == 0x70) {
      val = WasmEdge_ValueGenExternRef(getStringVal(env, jVal));
  } else if (jType == 0x6F) {
      val = WasmEdge_ValueGenFuncRef((WasmEdge_FunctionInstanceContext *)getLongVal(env, jVal));
  } 
  return val;
}

jobject WasmEdgeValueToJavaValue(JNIEnv *env, WasmEdge_Value value) {
  const char *valClassName = NULL;
 if (WasmEdge_ValTypeIsI32(value.Type)) {
        valClassName = ORG_WASMEDGE_I32VALUE;
    } else if (WasmEdge_ValTypeIsI64(value.Type)) {
        valClassName = ORG_WASMEDGE_I64VALUE;
    } else if (WasmEdge_ValTypeIsF32(value.Type)) {
        valClassName = ORG_WASMEDGE_F32VALUE;
    } else if (WasmEdge_ValTypeIsF64(value.Type)) {
        valClassName = ORG_WASMEDGE_F64VALUE;
    } else if (WasmEdge_ValTypeIsV128(value.Type)) {
        valClassName = ORG_WASMEDGE_V128VALUE;
    } else if (WasmEdge_ValTypeIsExternRef(value.Type)) {
        valClassName = ORG_WASMEDGE_EXTERNREF;
    } else if (WasmEdge_ValTypeIsFuncRef(value.Type)) {
        valClassName = ORG_WASMEDGE_FUNCREF;
    }
  jclass valClass = (*env)->FindClass(env, valClassName);

  jmethodID constructor = (*env)->GetMethodID(env, valClass, DEFAULT_CONSTRUCTOR, VOID_VOID);

  jobject jVal = (*env)->NewObject(env, valClass, constructor);

  setJavaValueObject(env, value, jVal);
  return jVal;
}

