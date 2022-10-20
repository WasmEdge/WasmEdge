//
// Created by Kenvi Zhu on 2022-01-14.
//

#ifndef WASMEDGE_JAVA_MEMORYTYPECONTEXT_H
#define WASMEDGE_JAVA_MEMORYTYPECONTEXT_H
#include "wasmedge/wasmedge.h"

WasmEdge_MemoryTypeContext *getMemoryTypeContext(JNIEnv *env,
                                                 jobject jMemoryTypeContext);
jobject
createJMemoryTypeContext(JNIEnv *env,
                         const WasmEdge_MemoryTypeContext *memTypeContext);

#endif // WASMEDGE_JAVA_MEMORYTYPECONTEXT_H
