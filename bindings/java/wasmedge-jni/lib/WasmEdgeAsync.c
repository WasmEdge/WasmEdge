//
// Created by elfgum on 2022/8/1.
//
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include "../jni/org_wasmedge_WasmEdgeAsync.h"
#include "wasmedge/wasmedge.h"
#include "jni.h"
#include "common.h"
#include "ValueType.h"
#include "ExportTypeContext.h"
#include "ImportTypeContext.h"

// Warning, we need type cast in return.

WasmEdge_Async * getAsync(JNIEnv* env, jobject thisObject){
    if(thisObject == NULL) {
        return NULL;
    }
    printf("get async object\n");
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
    WasmEdge_AsyncWait(ctx);
}

JNIEXPORT jboolean JNICALL Java_org_wasmedge_WasmEdgeAsync_wasmEdge_1AsyncWaitFor
        (JNIEnv * env, jobject thisobject, jlong milliseconds){
    WasmEdge_Async * ctx = getAsync(env, thisobject);
    if (ctx == NULL) printf("async ctx is null\n");
    else printf("async ctx not null\n");
    sleep(5);
    WasmEdge_AsyncWaitFor(ctx, 10000);
    return 0;
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeAsync_wasmEdge_1AsyncCancel
        (JNIEnv * env, jobject thisobject){
    WasmEdge_Async * ctx = getAsync(env, thisobject);
    WasmEdge_AsyncCancel(ctx);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_WasmEdgeAsync_wasmEdge_1AsyncGetReturnsLength
        (JNIEnv * env, jobject thisobject){
    WasmEdge_Async * ctx = getAsync(env, thisobject);
    return (jint)WasmEdge_AsyncGetReturnsLength(ctx);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeAsync_wasmEdge_1AsyncGet
        (JNIEnv * env, jobject thisobject, jobjectArray jreturns, jintArray jReturnTypes){
    WasmEdge_Async * ctx = getAsync(env, thisobject);
    jsize returnsLen = (*env)->GetArrayLength(env, jreturns);
    WasmEdge_Value *returns = calloc(returnsLen, sizeof(WasmEdge_Value));
    int *type = (*env)->GetIntArrayElements(env, jReturnTypes, JNI_FALSE);
    for (int i = 0; i < returnsLen; i++) {
        WasmEdge_Value val;

        jobject val_object = (*env)->GetObjectArrayElement(env, jreturns, i);

        switch (type[i]) {

            case 0:
                val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
                break;
            case 1:
                val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
                break;
            case 2:
                val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
                break;
            case 3:
                val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
                break;
            default:
                break;
        }
        returns[i] = val;
    }
    // Warning: turn WasmEdge_Result to F
    WasmEdge_Result result= WasmEdge_AsyncGet(ctx, returns, returnsLen);
    handleWasmEdgeResult(env, &result);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeAsync_wasmEdge_1AsyncDelete
        (JNIEnv * env, jobject thisobject){
    WasmEdge_Async * ctx = getAsync(env, thisobject);
    WasmEdge_AsyncDelete(ctx);
}

jobject createJAsyncObject(JNIEnv* env, WasmEdge_Async * asyncObj) {

    jclass clazz = (*env)->FindClass(env, "org/wasmedge/WasmEdgeAsync");
    if (clazz != NULL) printf("class not null\n");
    jmethodID constructorId = (*env)->GetMethodID(env, clazz, "<init>", "(J)V");
    if (constructorId != NULL) printf("constructor not null\n");
    return (*env)->NewObject(env, clazz, constructorId, (long)asyncObj);
}
