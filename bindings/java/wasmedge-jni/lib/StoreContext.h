//
// Created by Kenvi Zhu on 2021-11-19.
//

#ifndef WASMEDGE_JAVA_STORECONTEXT_H
#define WASMEDGE_JAVA_STORECONTEXT_H
#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_StoreContext *getStoreContext(JNIEnv *env, jobject jStoreContext);

jobject CreateJavaStoreContext(JNIEnv *env,
                               WasmEdge_StoreContext *storeContext);

#endif // WASMEDGE_JAVA_STORECONTEXT_H
