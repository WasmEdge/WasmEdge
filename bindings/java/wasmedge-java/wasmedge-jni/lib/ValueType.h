// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_VALUETYPE_H
#define WASMEDGE_JAVA_VALUETYPE_H

WasmEdge_Value JavaValueToWasmEdgeValue(JNIEnv *env, jobject jVal);
jobject WasmEdgeValueToJavaValue(JNIEnv *env, WasmEdge_Value value);
char *u128toa(uint128_t n);

#endif // WASMEDGE_JAVA_VALUETYPE_H
