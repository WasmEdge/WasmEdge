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


jobject createJMemoryTypeContext(JNIEnv* env, const WasmEdge_MemoryTypeContext * memTypeContext) {

    jclass clazz = (*env)->FindClass(env, "org/wasmedge/MemoryTypeContext");

    if(clazz == NULL) {
        printf("memory type not found \n");
    }

    jmethodID constructorId = (*env)->GetMethodID(env, clazz, "<init>", "(J)V");

    if(constructorId == NULL) {
        printf("memory type constructor not found\n");
    }
    return (*env)->NewObject(env, clazz, constructorId, (long) memTypeContext);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_MemoryTypeContext_getLimit
        (JNIEnv *env, jobject thisObject) {

    WasmEdge_MemoryTypeContext * memoryTypeContext = getMemoryTypeContext(env, thisObject);
    WasmEdge_Limit limit = WasmEdge_MemoryTypeGetLimit(memoryTypeContext);

    jclass limitClass = findJavaClass(env, "org/wasmedge/WasmEdgeLimit");


    jmethodID constructor = findJavaMethod(env, limitClass, "<init>", "(ZJJ)V");

    return (*env)->NewObject(env, limitClass, constructor,
                             (jboolean)limit.HasMax, (jlong)limit.Min, (jlong)limit.Max);

}
