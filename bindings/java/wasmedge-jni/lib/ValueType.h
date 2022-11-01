// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifndef WASMEDGE_JAVA_VALUETYPE_H
#define WASMEDGE_JAVA_VALUETYPE_H

WasmEdge_Value JavaValueToWasmEdgeValue(JNIEnv *env, jobject jVal);
jobject WasmEdgeValueToJavaValue(JNIEnv *env, WasmEdge_Value value);

#endif // WASMEDGE_JAVA_VALUETYPE_H
