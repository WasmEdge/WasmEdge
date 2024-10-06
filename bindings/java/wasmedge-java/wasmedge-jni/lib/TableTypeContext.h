// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_TABLETYPECONTEXT_H
#define WASMEDGE_JAVA_TABLETYPECONTEXT_H
#include "wasmedge/wasmedge.h"

WasmEdge_TableTypeContext *getTableTypeContext(JNIEnv *env,
                                               jobject jTableTypeContext);

jobject
createJTableTypeContext(JNIEnv *env,
                        const WasmEdge_TableTypeContext *tableTypeContext);

#endif // WASMEDGE_JAVA_TABLETYPECONTEXT_H
