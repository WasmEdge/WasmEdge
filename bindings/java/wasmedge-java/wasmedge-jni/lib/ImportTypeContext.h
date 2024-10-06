// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_IMPORTTYPECONTEXT_H
#define WASMEDGE_JAVA_IMPORTTYPECONTEXT_H
#include "jni.h"
#include "wasmedge/wasmedge.h"

jobject createImportTypeContext(JNIEnv *env,
                                const WasmEdge_ImportTypeContext *cxt,
                                jobject jAstMo);
#endif // WASMEDGE_JAVA_IMPORTTYPECONTEXT_H
