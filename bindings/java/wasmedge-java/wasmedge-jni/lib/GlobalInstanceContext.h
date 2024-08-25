// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_GLOBALINSTANCECONTEXT_H
#define WASMEDGE_JAVA_GLOBALINSTANCECONTEXT_H

#include "jni.h"
#include "wasmedge/wasmedge.h"
WasmEdge_GlobalInstanceContext *
getGlobalInstanceContext(JNIEnv *env, jobject jGlobalInstanceContext);
jobject createJGlobalInstanceContext(
    JNIEnv *env, const WasmEdge_GlobalInstanceContext *globInstance);

#endif // WASMEDGE_JAVA_GLOBALINSTANCECONTEXT_H
