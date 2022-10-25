//
// Created by Kenvi Zhu on 2022-01-12.
//

#ifndef WASMEDGE_JAVA_GLOBALTYPECONTEXT_H
#define WASMEDGE_JAVA_GLOBALTYPECONTEXT_H
#include "wasmedge/wasmedge.h"

WasmEdge_GlobalTypeContext *getGlobalTypeContext(JNIEnv *env,
                                                 jobject jGlobalTypeContext);

jobject
createJGlobalTypeContext(JNIEnv *env,
                         const WasmEdge_GlobalTypeContext *globalTypeContext);
#endif // WASMEDGE_JAVA_GLOBALTYPECONTEXT_H
