// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifndef WASMEDGE_JNI_WASMEDGEASYNC_H
#define WASMEDGE_JNI_WASMEDGEASYNC_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_Async *getAsync(JNIEnv *env, jobject thisObject);

jobject createJAsyncObject(JNIEnv *env, const WasmEdge_Async *asyncObj);
#endif // WASMEDGE_JNI_WASMEDGEASYNC_H
