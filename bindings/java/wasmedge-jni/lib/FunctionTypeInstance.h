//
// Created by Kenvi Zhu on 2022-03-14.
//

#ifndef WASMEDGE_JAVA_FUNCTIONTYPEINSTANCE_H
#define WASMEDGE_JAVA_FUNCTIONTYPEINSTANCE_H
#include "wasmedge/wasmedge.h"
#include "jni.h"

typedef struct HostFuncParam {
    JNIEnv * env;
    char* jFuncKey;
} HostFuncParam;


WasmEdge_FunctionInstanceContext * getFunctionInstanceContext(JNIEnv* env, jobject jFuncInstance);

jobject createJFunctionInstanceContext(JNIEnv* env, const WasmEdge_FunctionInstanceContext * funcInstance);

#endif //WASMEDGE_JAVA_FUNCTIONTYPEINSTANCE_H
