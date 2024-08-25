// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_STORECONTEXT_H
#define WASMEDGE_JAVA_STORECONTEXT_H
#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_StoreContext *getStoreContext(JNIEnv *env, jobject jStoreContext);

jobject CreateJavaStoreContext(JNIEnv *env,
                               WasmEdge_StoreContext *storeContext);

#endif // WASMEDGE_JAVA_STORECONTEXT_H
