// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_FUNCTIONTYPECONTEXT_H
#define WASMEDGE_JAVA_FUNCTIONTYPECONTEXT_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_FunctionTypeContext *
getFunctionTypeContext(JNIEnv *env, jobject jFunctionTypeContext);
jobject ConvertToJavaFunctionList(JNIEnv *env, WasmEdge_String *nameList,
                                  const WasmEdge_FunctionTypeContext **funcList,
                                  int32_t len, jobject jFuncList);

jobject ConvertToJavaFunctionType(
    JNIEnv *env, const WasmEdge_FunctionTypeContext *functionTypeContext,
    WasmEdge_String name);
jobject createJFunctionTypeContext(
    JNIEnv *env, const WasmEdge_FunctionTypeContext *functionTypeContext);
#endif // WASMEDGE_JAVA_FUNCTIONTYPECONTEXT_H
