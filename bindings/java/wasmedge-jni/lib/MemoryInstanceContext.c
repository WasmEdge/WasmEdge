//
// Created by Kenvi Zhu on 2022-01-14.
//
#include "../jni/org_wasmedge_MemoryInstanceContext.h"
#include "wasmedge/wasmedge.h"
#include "common.h"
#include "MemoryTypeContext.h"


WasmEdge_MemoryInstanceContext * getMemoryInstanceContext(JNIEnv* env, jobject jMemoryInstanceContext) {

    if(jMemoryInstanceContext == NULL) {
        return NULL;
    }
    WasmEdge_MemoryInstanceContext * memoryInstanceContext =
            (struct WasmEdge_MemoryInstanceContext *)getPointer(env, jMemoryInstanceContext);

    return memoryInstanceContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_MemoryInstanceContext_nativeInit
(JNIEnv * env, jobject thisObject, jobject jMemoryTypeContext) {
    WasmEdge_MemoryTypeContext * memoryTypeContext = getMemoryTypeContext(env, jMemoryTypeContext);
    WasmEdge_MemoryInstanceContext * memInstance = WasmEdge_MemoryInstanceCreate(memoryTypeContext);
    setPointer(env, thisObject, (long)memInstance);
}

/*
 * Class:     org_wasmedge_MemoryInstanceContext
 * Method:    setData
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_MemoryInstanceContext_setData
(JNIEnv *env, jobject thisObject, jbyteArray jData, jint jOffSet, jint jLength) {
    WasmEdge_MemoryInstanceContext * memoryInstanceContext = getMemoryInstanceContext(env, thisObject);

    jbyte * buff = (*env)->GetByteArrayElements(env, jData, NULL);

    WasmEdge_MemoryInstanceSetData(memoryInstanceContext, buff, jOffSet, jLength);

    (*env)->ReleaseByteArrayElements(env, jData, buff, jLength);
}


JNIEXPORT jbyteArray JNICALL Java_org_wasmedge_MemoryInstanceContext_getData
        (JNIEnv * env, jobject thisObject, jint jOffSet, jint jSize) {
    WasmEdge_MemoryInstanceContext *memoryInstanceContext = getMemoryInstanceContext(env, thisObject);
    uint8_t *data = (uint8_t*)malloc(sizeof(uint8_t) * jSize);
    WasmEdge_Result result = WasmEdge_MemoryInstanceGetData(memoryInstanceContext, data, jOffSet, jSize);
    if(!WasmEdge_ResultOK(result)) {
        free(data);
    }
    handleWasmEdgeResult(env, &result);

    jbyteArray jBytes = (*env)->NewByteArray(env, jSize);
    (*env)->SetByteArrayRegion(env, jBytes,0, jSize, data);

    free(data);
    return jBytes;


}

/*
 * Class:     org_wasmedge_MemoryInstanceContext
 * Method:    getPageSize
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_wasmedge_MemoryInstanceContext_getPageSize
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_MemoryInstanceContext * memInstance = getMemoryInstanceContext(env, thisObject);
    return WasmEdge_MemoryInstanceGetPageSize(memInstance);
}

/*
 * Class:     org_wasmedge_MemoryInstanceContext
 * Method:    growPage
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_MemoryInstanceContext_growPage
(JNIEnv *env , jobject thisObject, jint jSize) {
    WasmEdge_MemoryInstanceContext * memInstance = getMemoryInstanceContext(env, thisObject);
    WasmEdge_Result result = WasmEdge_MemoryInstanceGrowPage(memInstance, jSize);
    handleWasmEdgeResult(env, &result);
}

JNIEXPORT void JNICALL Java_org_wasmedge_MemoryInstanceContext_delete
(JNIEnv * env, jobject thisObject) {
    WasmEdge_MemoryInstanceDelete(getMemoryInstanceContext(env, thisObject));
    setPointer(env, thisObject, 0);
}

