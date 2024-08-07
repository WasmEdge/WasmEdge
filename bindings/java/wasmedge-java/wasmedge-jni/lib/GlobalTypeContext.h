// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_GLOBALTYPECONTEXT_H
#define WASMEDGE_JAVA_GLOBALTYPECONTEXT_H
#include "wasmedge/wasmedge.h"

WasmEdge_GlobalTypeContext *getGlobalTypeContext(JNIEnv *env,
                                                 jobject jGlobalTypeContext);

jobject
createJGlobalTypeContext(JNIEnv *env,
                         const WasmEdge_GlobalTypeContext *globalTypeContext);
#endif // WASMEDGE_JAVA_GLOBALTYPECONTEXT_H
