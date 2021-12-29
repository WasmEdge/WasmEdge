//
// Created by Kenvi Zhu on 2021-12-04.
//

#ifndef WASMEDGE_JAVA_FUNCTIONTYPECONTEXT_H
#define WASMEDGE_JAVA_FUNCTIONTYPECONTEXT_H

#include "wasmedge/wasmedge.h"
#include "jni.h"

WasmEdge_FunctionTypeContext * getFunctionTypeContext(JNIEnv* env, jobject jFunctionTypeContext);
jobject ConvertToJavaFunctionList(JNIEnv * env, WasmEdge_String* nameList, const WasmEdge_FunctionTypeContext** funcList, int32_t len);

jobject ConvertToJavaFunctionType(JNIEnv* env, const WasmEdge_FunctionTypeContext* functionTypeContext);
#endif //WASMEDGE_JAVA_FUNCTIONTYPECONTEXT_H
