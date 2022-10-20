//
// Created by Kenvi Zhu on 2022-03-06.
//

#ifndef WASMEDGE_JAVA_IMPORTTYPECONTEXT_H
#define WASMEDGE_JAVA_IMPORTTYPECONTEXT_H
#include "jni.h"
#include "wasmedge/wasmedge.h"

jobject createImportTypeContext(JNIEnv *env,
                                const WasmEdge_ImportTypeContext *cxt,
                                jobject jAstMo);
#endif // WASMEDGE_JAVA_IMPORTTYPECONTEXT_H
