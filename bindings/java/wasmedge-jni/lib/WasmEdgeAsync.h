//
// Created by elfgum on 2022/8/1.
//

#ifndef WASMEDGE_JNI_WASMEDGEASYNC_H
#define WASMEDGE_JNI_WASMEDGEASYNC_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_Async * getAsync(JNIEnv* env, jobject thisObject);

#endif //WASMEDGE_JNI_WASMEDGEASYNC_H
