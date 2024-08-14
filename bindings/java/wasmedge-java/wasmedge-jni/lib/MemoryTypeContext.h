// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_MEMORYTYPECONTEXT_H
#define WASMEDGE_JAVA_MEMORYTYPECONTEXT_H
#include "wasmedge/wasmedge.h"

WasmEdge_MemoryTypeContext *getMemoryTypeContext(JNIEnv *env,
                                                 jobject jMemoryTypeContext);
jobject
createJMemoryTypeContext(JNIEnv *env,
                         const WasmEdge_MemoryTypeContext *memTypeContext);

#endif // WASMEDGE_JAVA_MEMORYTYPECONTEXT_H
