//
// Created by Kenvi Zhu on 2022-01-13.
//

#ifndef WASMEDGE_JAVA_VALUETYPE_H
#define WASMEDGE_JAVA_VALUETYPE_H

WasmEdge_Value JavaValueToWasmEdgeValue(JNIEnv *env, jobject jVal);
jobject WasmEdgeValueToJavaValue(JNIEnv * env, WasmEdge_Value value);

#endif //WASMEDGE_JAVA_VALUETYPE_H
