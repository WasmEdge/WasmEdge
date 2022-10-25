//
// Created by Kenvi Zhu on 2021-11-19.
//

#ifndef WASMEDGE_JAVA_CONFIGURECONTEXT_H
#define WASMEDGE_JAVA_CONFIGURECONTEXT_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_ConfigureContext *getConfigureContext(JNIEnv *env,
                                               jobject jConfigureContext);

#endif // WASMEDGE_JAVA_CONFIGURECONTEXT_H
