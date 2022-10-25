//
// Created by Kenvi Zhu on 2022-03-17.
//

#ifndef WASMEDGE_JAVA_MODULEINSTANCECONTEXT_H
#define WASMEDGE_JAVA_MODULEINSTANCECONTEXT_H
#include "jni.h"
#include "wasmedge/wasmedge.h"

jobject
createJModuleInstanceContext(JNIEnv *env,
                             const WasmEdge_ModuleInstanceContext *impObj);

WasmEdge_ModuleInstanceContext *getModuleInstanceContext(JNIEnv *env,
                                                         jobject jImpObjCxt);
#endif // WASMEDGE_JAVA_MODULEINSTANCECONTEXT_H
