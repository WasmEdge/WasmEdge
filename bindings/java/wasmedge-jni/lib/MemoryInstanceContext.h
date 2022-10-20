//
// Created by Kenvi Zhu on 2022-03-14.
//

#ifndef WASMEDGE_JAVA_MEMORYINSTANCECONTEXT_H
#define WASMEDGE_JAVA_MEMORYINSTANCECONTEXT_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_MemoryInstanceContext *
getMemoryInstanceContext(JNIEnv *env, jobject jMemoryInstanceContext);
jobject
createJMemoryInstanceContext(JNIEnv *env,
                             const WasmEdge_MemoryInstanceContext *memInstance);

#endif // WASMEDGE_JAVA_MEMORYINSTANCECONTEXT_H
