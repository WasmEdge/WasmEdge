//
// Created by Kenvi Zhu on 2021-11-17.
//
#include "jni.h"
#include "wasmedge/wasmedge.h"
#include "common.h"

JNIEXPORT void JNICALL Java_org_wasmedge_StoreContext_initNative
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
