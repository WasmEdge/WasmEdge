// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JNI_ASYNC_H
#define WASMEDGE_JNI_ASYNC_H

#include "../jni/org_wasmedge_Async.h"
#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_Async *getAsync(JNIEnv *env, jobject thisObject);

jobject createJAsyncObject(JNIEnv *env, const WasmEdge_Async *asyncObj);
#endif // WASMEDGE_JNI_ASYNC_H
