// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_CONFIGURECONTEXT_H
#define WASMEDGE_JAVA_CONFIGURECONTEXT_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_ConfigureContext *getConfigureContext(JNIEnv *env,
                                               jobject jConfigureContext);

#endif // WASMEDGE_JAVA_CONFIGURECONTEXT_H
