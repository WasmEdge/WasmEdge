//
// Created by Kenvi Zhu on 2022-01-16.
//

#include "../jni/org_wasmedge_WasmEdgeExternRef.h"
#include "common.h"

WasmEdge_Value * getExternRef(JNIEnv* env, jobject jExternRef) {
    if (jExternRef == NULL) {
        return NULL;
    }
    return (WasmEdge_Value *)getPointer(env, jExternRef);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeExternRef_nativeInit
        (JNIEnv * env, jobject thisObject, jobject jVal) {
    WasmEdge_Value ref = WasmEdge_ValueGenExternRef(&jVal);
    setPointer(env, thisObject, (long)&ref);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeExternRef_getExternRefVal
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_Value *ref = getExternRef(env, thisObject);
    jobject * ptr = WasmEdge_ValueGetExternRef(*ref);
    return *ptr;

}
