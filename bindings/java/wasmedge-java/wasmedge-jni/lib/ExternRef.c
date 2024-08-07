// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common.h"
#include <stdlib.h>
#include <string.h>

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeExternRef_nativeInit(
    JNIEnv *env, jobject thisObject, jobject jVal) {
  jint len = (*env)->GetStringUTFLength(env, jVal);
  char *ptr = (char *)malloc(sizeof(char) * len);

  const char *val = (*env)->GetStringUTFChars(env, jVal, NULL);

  memcpy(ptr, val, len);

  (*env)->ReleaseStringUTFChars(env, jVal, val);
  WasmEdge_Value ref = WasmEdge_ValueGenExternRef(ptr);

  setPointer(env, thisObject, (long)&ref);
}
