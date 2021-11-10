//
// Created by Kenvi Zhu on 2021-10-10.
//
#include "../org_wasmedge_WasmEdge.h"
#include "wasmedge/wasmedge.h"

/**
 * Get WasmEdge_String from java WasmEdgeString
 * @param env  jni env
 * @param obj WasmEdge obj
 * @return WasmEdge string
 */
WasmEdge_Result * getWasmEdgeResult(JNIEnv * env, jobject obj) {
    jclass clazz = (*env)->GetObjectClass(env, obj);
    jfieldID pointerId = (*env)->GetFieldID(env, clazz, "pointer", "J");
    jlong pointerVal = (*env)->GetLongField(env, obj, pointerId);
    WasmEdge_Result* wasmEdgeResult = (void*) pointerVal;
    return wasmEdgeResult;
}


JNIEXPORT jint JNICALL Java_org_wasmedge_WasmEdgeResult_getResultCode
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_Result* result = getWasmEdgeResult(env, thisObject);

    return WasmEdge_ResultGetCode(*result);


}

JNIEXPORT jboolean JNICALL Java_org_wasmedge_WasmEdgeResult_isOk
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_Result* result = getWasmEdgeResult(env, thisObject);
    return WasmEdge_ResultOK(*result);
}

/*
 * Class:     org_wasmedge_WasmEdgeResult
 * Method:    getMessage
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_wasmedge_WasmEdgeResult_getMessage
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_Result* result = getWasmEdgeResult(env, thisObject);
    const char* message =  WasmEdge_ResultGetMessage(*result);
    jstring result_str= (*env)->NewStringUTF(env, message);

    return result_str;
}