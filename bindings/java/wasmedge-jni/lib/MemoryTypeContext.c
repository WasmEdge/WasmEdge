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
        (JNIEnv *env, jobject thisObject, jboolean jHasMax, jlong jMin, jlong jMax){

    const WasmEdge_Limit limit = {.HasMax = jHasMax, .Min = jMin, .Max = jMax};

    WasmEdge_MemoryTypeContext* memCxt = WasmEdge_MemoryTypeCreate(limit);

    setPointer(env, thisObject, (long)memCxt);
}
JNIEXPORT void JNICALL Java_org_wasmedge_MemoryTypeContext_delete
(JNIEnv *env , jobject thisObject) {
    WasmEdge_MemoryTypeContext * memoryTypeContext = getMemoryTypeContext(env, thisObject);
    WasmEdge_MemoryTypeDelete(memoryTypeContext);
    setPointer(env, thisObject, 0);

}


jobject createJMemoryTypeContext(JNIEnv* env, const WasmEdge_MemoryTypeContext * memTypeContext) {

    jclass clazz = (*env)->FindClass(env, "org/wasmedge/MemoryTypeContext");


    jmethodID constructorId = (*env)->GetMethodID(env, clazz, "<init>", "(J)V");

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
