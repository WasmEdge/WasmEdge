// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_TABLEINSTANCECONTEXT_H
#define WASMEDGE_JAVA_TABLEINSTANCECONTEXT_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

jobject
createJTableInstanceContext(JNIEnv *env,
                            const WasmEdge_TableInstanceContext *tabInstance);

WasmEdge_TableInstanceContext *
getTableInstanceContext(JNIEnv *env, jobject jTableInstanceContext);

#endif // WASMEDGE_JAVA_TABLEINSTANCECONTEXT_H
