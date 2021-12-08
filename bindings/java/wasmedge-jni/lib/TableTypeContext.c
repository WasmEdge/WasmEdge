//
// Created by Kenvi Zhu on 2021-12-08.
//

#include "../jni/org_wasmedge_TableTypeContext.h"
#include "wasmedge/wasmedge.h"
#include "common.h"

WasmEdge_TableTypeContext * getTableTypeContext(JNIEnv* env, jobject jTableTypeContext) {

    if(jTableTypeContext == NULL) {
        return NULL;
    }
    WasmEdge_TableTypeContext * tableTypeContext=  (WasmEdge_TableTypeContext *)getPointer(env, jTableTypeContext);

    return tableTypeContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_TableTypeContext_nativeInit
        (JNIEnv *env, jobject thisObject, jint refType, jobject jLimit){
    jclass cls = (*env)->GetObjectClass(env, thisObject);
    jfieldID hasMaxFid = (*env)->GetFieldID(env, cls, "hasMax", "B");
    jboolean hasMax = (*env)->GetBooleanField(env, thisObject, hasMaxFid);

    jfieldID maxFid = (*env)->GetFieldID(env, cls, "max", "J");
    jlong max = (*env)->GetLongField(env, thisObject, maxFid);

    jfieldID minFid = (*env)->GetFieldID(env, cls, "min", "J");
    jlong min = (*env)->GetLongField(env, thisObject, minFid);

    WasmEdge_Limit  tableLimit = {.HasMax = hasMax, .Min = min, .Max = max};
    WasmEdge_TableTypeContext * tableTypeContext = WasmEdge_TableTypeCreate((enum WasmEdge_RefType) refType, tableLimit);
    setPointer(env, thisObject, (long)tableTypeContext);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_TableTypeContext_getLimit
        (JNIEnv *env, jobject thisObject) {
    WasmEdge_TableTypeContext* tableTypeContext = getTableTypeContext(env, thisObject);

    WasmEdge_Limit limit = WasmEdge_TableTypeGetLimit(tableTypeContext);

    jclass limitClass = findJavaClass(env, "org.wasmedge.WasmEdgeLimit");
    jmethodID constructor = findJavaMethod(env, limitClass, "<init>", "(BJJ)");
    return (*env)->NewObject(env, limitClass, constructor,
                             (jboolean)limit.HasMax, (jlong)limit.Min, (jlong)limit.Max);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_TableTypeContext_nativeGetRefType
        (JNIEnv *env, jobject thisObject) {
    WasmEdge_TableTypeContext* tableTypeContext = getTableTypeContext(env, thisObject);

    return WasmEdge_TableTypeGetRefType(tableTypeContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_TableTypeContext_delete
        (JNIEnv *env, jobject thisObject) {
    WasmEdge_TableTypeContext* tableTypeContext = getTableTypeContext(env, thisObject);
    WasmEdge_TableTypeDelete(tableTypeContext);
}

