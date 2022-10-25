//
// Created by Kenvi Zhu on 2022-03-06.
//

#ifndef WASMEDGE_JAVA_EXPORTTYPECONTEXT_H
#define WASMEDGE_JAVA_EXPORTTYPECONTEXT_H
#include "jni.h"
#include "wasmedge/wasmedge.h"

jobject createExportTypeContext(JNIEnv *env,
                                const WasmEdge_ExportTypeContext *cxt,
                                jobject jAstMo);
#endif // WASMEDGE_JAVA_EXPORTTYPECONTEXT_H
