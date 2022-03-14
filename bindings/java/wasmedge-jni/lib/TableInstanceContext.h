//
// Created by Kenvi Zhu on 2022-03-14.
//

#ifndef WASMEDGE_JAVA_TABLEINSTANCECONTEXT_H
#define WASMEDGE_JAVA_TABLEINSTANCECONTEXT_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_TableInstanceContext * getTableInstanceContext(JNIEnv* env, jobject jTableInstanceContext);

#endif //WASMEDGE_JAVA_TABLEINSTANCECONTEXT_H
