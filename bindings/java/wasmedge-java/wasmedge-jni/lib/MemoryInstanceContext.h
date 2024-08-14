// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_MEMORYINSTANCECONTEXT_H
#define WASMEDGE_JAVA_MEMORYINSTANCECONTEXT_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_MemoryInstanceContext *
getMemoryInstanceContext(JNIEnv *env, jobject jMemoryInstanceContext);
jobject
createJMemoryInstanceContext(JNIEnv *env,
                             const WasmEdge_MemoryInstanceContext *memInstance);

#endif // WASMEDGE_JAVA_MEMORYINSTANCECONTEXT_H
