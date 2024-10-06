// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_FUNCTIONTYPEINSTANCE_H
#define WASMEDGE_JAVA_FUNCTIONTYPEINSTANCE_H
#include "jni.h"
#include "wasmedge/wasmedge.h"

typedef struct HostFuncParam {
  JNIEnv *env;
  const char *jFuncKey;
} HostFuncParam;

WasmEdge_FunctionInstanceContext *
getFunctionInstanceContext(JNIEnv *env, jobject jFuncInstance);

jobject createJFunctionInstanceContext(
    JNIEnv *env, const WasmEdge_FunctionInstanceContext *funcInstance);

uint32_t
GetReturnLen(WasmEdge_FunctionInstanceContext *functionInstanceContext);

#endif // WASMEDGE_JAVA_FUNCTIONTYPEINSTANCE_H
