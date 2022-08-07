//
// Created by elfgum on 2022/8/1.
//

#include <malloc.h>
#include "../jni/org_wasmedge_WasmEdgeAsync.h"
#include "wasmedge/wasmedge.h"
#include "jni.h"
#include "common.h"
#include "ValueType.h"
#include "ExportTypeContext.h"
#include "ImportTypeContext.h"

WasmEdge_Async * getAsync(JNIEnv* env, jobject thisObject){
    if(thisObject == NULL) {
        return NULL;
    }
    return (WasmEdge_Async*) getPointer(env, thisObject);
}
/*
 * it seems that we don't need this file
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeAsync_nativeInit
        (JNIEnv * env, jobject thisobject, jobjectArray jargs){

    WasmEdge_Async* configureContext = getAsync(env, thisobject);

    WasmEdge_Async * async = (WasmEdge_Async *)malloc(sizeof(struct WasmEdge_Async));

    WasmeWasmEdge_Async(jargs);

    setPointer(env, thisObject, (long)compilerContext);
}
*/

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeAsync_wasmEdge_1AsyncWait
        (JNIEnv * env, jobject thisobject){
    WasmEdge_Async * ctx = getAsync(env, thisobject);
    return WasmEdge_AsyncWait(ctx);
}

JNIEXPORT jboolean JNICALL Java_org_wasmedge_WasmEdgeAsync_wasmEdge_1AsyncWaitFor
        (JNIEnv * env, jobject thisobject, jlong milliseconds){
    WasmEdge_Async * ctx = getAsync(env, thisobject);
    return WasmEdge_AsyncWaitFor(ctx, (uint64_t)milliseconds);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeAsync_wasmEdge_1AsyncCancel
        (JNIEnv * env, jobject thisobject){
    WasmEdge_Async * ctx = getAsync(env, thisobject);
    return WasmEdge_AsyncCancel(ctx);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_WasmEdgeAsync_wasmEdge_1AsyncGetReturnsLength
        (JNIEnv * env, jobject thisobject){
    WasmEdge_Async * ctx = getAsync(env, thisobject);
    return (jint)WasmEdge_AsyncGetReturnsLength(ctx);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeAsync_wasmEdge_1AsyncGet
        (JNIEnv * env, jobject thisobject, jobject jreturns, jint jreturnlen){
    WasmEdge_Async * ctx = getAsync(env, thisobject);
    WasmEdge_Value Returns = JavaValueToWasmEdgeValue(env, jreturns);
    const uint32_t ReturnLen = jreturnlen;
    // mind here, we have some type problem to address
    return (struct jobject)WasmEdge_AsyncGet(ctx, &Returns, ReturnLen);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeAsync_wasmEdge_1AsyncDelete
        (JNIEnv * env, jobject thisobject){
    WasmEdge_Async * ctx = getAsync(env, thisobject);
    WasmEdge_AsyncDelete(ctx);
}
