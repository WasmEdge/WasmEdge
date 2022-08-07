//
// Created by elfgum on 2022/8/1.
//

#ifndef WASMEDGE_JNI_ASYNC_H
#define WASMEDGE_JNI_ASYNC_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_Async * getAsync(JNIEnv* env, jobject thisObject);
// jobject createAstModuleContext(JNIEnv * env, WasmEdge_ASTModuleContext* mod);

#endif //WASMEDGE_JNI_ASYNC_H
