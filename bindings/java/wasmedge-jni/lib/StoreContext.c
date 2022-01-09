//
// Created by Kenvi Zhu on 2021-11-17.
//
#include "jni.h"
#include "wasmedge/wasmedge.h"
#include "common.h"

JNIEXPORT void JNICALL Java_org_wasmedge_StoreContext_nativeInit
(JNIEnv *env, jobject thisObj) {
    WasmEdge_StoreContext *StoreContext = WasmEdge_StoreCreate();
    setPointer(env, thisObj, (jlong)StoreContext);
}

WasmEdge_StoreContext* getStoreContext(JNIEnv* env, jobject jStoreContext) {

    if(jStoreContext == NULL) {
        return NULL;
    }
    WasmEdge_StoreContext* StoreContext =  (WasmEdge_StoreContext*)getPointer(env, jStoreContext);

    return StoreContext;
}

jobject CreateJavaStoreContext(JNIEnv* env, WasmEdge_StoreContext* storeContext) {
    jclass storeClass = findJavaClass(env, "org/wasmedge/StoreContext");

    jmethodID constructor = (*env)->GetMethodID(env, storeClass, "<init>", "(L)V");

    jobject jStoreContext = (*env)->NewObject(env, storeClass, constructor, (long)storeContext);

    return jStoreContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_StoreContext_delete
        (JNIEnv * env, jobject thisObj) {
    WasmEdge_StoreDelete(getStoreContext(env, thisObj));
}

