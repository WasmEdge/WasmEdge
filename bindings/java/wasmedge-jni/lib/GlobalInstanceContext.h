//
// Created by Kenvi Zhu on 2022-03-14.
//

#ifndef WASMEDGE_JAVA_GLOBALINSTANCECONTEXT_H
#define WASMEDGE_JAVA_GLOBALINSTANCECONTEXT_H

#include "jni.h"
#include "wasmedge/wasmedge.h"
WasmEdge_GlobalInstanceContext * getGlobalInstanceContext(JNIEnv* env, jobject jGlobalInstanceContext);
jobject createJGlobalInstanceContext(JNIEnv* env, const WasmEdge_GlobalInstanceContext * globInstance);

#endif //WASMEDGE_JAVA_GLOBALINSTANCECONTEXT_H
