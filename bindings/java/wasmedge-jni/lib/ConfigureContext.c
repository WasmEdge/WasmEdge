//
// Created by Kenvi Zhu on 2021-11-19.
//

#include "../jni/org_wasmedge_ConfigureContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"

JNIEXPORT void JNICALL Java_org_wasmedge_ConfigureContext_nativeInit
        (JNIEnv * env, jobject thisObj) {

    WasmEdge_ConfigureContext *ConfigureContext = WasmEdge_ConfigureCreate();
    setPointer(env, thisObj, (jlong)ConfigureContext);
}

WasmEdge_ConfigureContext* getConfigureContext(JNIEnv* env, jobject jConfigureContext) {
    if (jConfigureContext == NULL) {
        return NULL;
    }
    return (WasmEdge_ConfigureContext *)getPointer(env, jConfigureContext);
}
