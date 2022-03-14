//
// Created by Kenvi Zhu on 2022-03-14.
//

#ifndef WASMEDGE_JAVA_FUNCTIONTYPEINSTANCE_H
#define WASMEDGE_JAVA_FUNCTIONTYPEINSTANCE_H
#include "wasmedge/wasmedge.h"
#include "jni.h"
WasmEdge_GlobalInstanceContext * getGlobalInstanceContext(JNIEnv* env, jobject jGlobalInstanceContext);

#endif //WASMEDGE_JAVA_FUNCTIONTYPEINSTANCE_H
