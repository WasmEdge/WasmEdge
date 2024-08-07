// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_EXPORTTYPECONTEXT_H
#define WASMEDGE_JAVA_EXPORTTYPECONTEXT_H
#include "jni.h"
#include "wasmedge/wasmedge.h"

jobject createExportTypeContext(JNIEnv *env,
                                const WasmEdge_ExportTypeContext *cxt,
                                jobject jAstMo);
#endif // WASMEDGE_JAVA_EXPORTTYPECONTEXT_H
