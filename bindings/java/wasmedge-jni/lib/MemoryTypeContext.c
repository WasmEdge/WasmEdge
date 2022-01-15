//
// Created by Kenvi Zhu on 2022-01-14.
//
#include "../jni/org_wasmedge_MemoryTypeContext.h"
#include "wasmedge/wasmedge.h"
#include "common.h"


WasmEdge_MemoryTypeContext * getMemoryTypeContext(JNIEnv* env, jobject jMemoryTypeContext) {

    if(jMemoryTypeContext == NULL) {
        return NULL;
    }
    WasmEdge_MemoryTypeContext * memoryTypeContext =
            (struct WasmEdge_MemoryTypeContext*)getPointer(env, jMemoryTypeContext);

    return memoryTypeContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_MemoryTypeContext_nativeInit
(JNIEnv * env, jobject thisObject, jobject jLimit) {


}
JNIEXPORT void JNICALL Java_org_wasmedge_MemoryTypeContext_delete
(JNIEnv *env , jobject thisObject) {
    WasmEdge_MemoryTypeContext * memoryTypeContext = getMemoryTypeContext(env, thisObject);
    WasmEdge_MemoryTypeDelete(memoryTypeContext);
    setPointer(env, thisObject, 0);

}