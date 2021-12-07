//
// Created by Kenvi Zhu on 2021-12-07.
//

#include "../jni/org_wasmedge_GlobalTypeContext.h"
#include "wasmedge/wasmedge.h"
#include "common.h"

WasmEdge_GlobalTypeContext * getGlobalTypeContext(JNIEnv* env, jobject jGlobalTypeContext) {

    if(jGlobalTypeContext== NULL) {
        return NULL;
    }
    WasmEdge_GlobalTypeContext * globalTypeContext =
            (struct WasmEdge_GlobalTypeContext*)getPointer(env, jGlobalTypeContext);

    return globalTypeContext;
}


JNIEXPORT void JNICALL Java_org_wasmedge_GlobalTypeContext_nativeInit
        (JNIEnv * env, jobject thisObject, jint valueType, jint mutability) {

    WasmEdge_GlobalTypeContext* globalTypeContext  =
            WasmEdge_GlobalTypeCreate((enum WasmEdge_ValType)valueType, (enum WasmEdge_Mutability) mutability);
   setPointer(env, thisObject, (jlong)globalTypeContext);

}

JNIEXPORT void JNICALL Java_org_wasmedge_GlobalTypeContext_delete
        (JNIEnv *env, jobject thisObject) {
    WasmEdge_GlobalTypeContext* wasmEdgeGlobalTypeContext = getGlobalTypeContext(env, thisObject);
    setPointer(env, thisObject, 0);
    WasmEdge_GlobalTypeDelete(wasmEdgeGlobalTypeContext);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_GlobalTypeContext_nativeGetValueType
        (JNIEnv * env, jobject thisObject) {
    return WasmEdge_GlobalTypeGetValType(getGlobalTypeContext(env, thisObject));

}
JNIEXPORT jint JNICALL Java_org_wasmedge_GlobalTypeContext_nativeGetMutability
        (JNIEnv * env, jobject thisObject) {
    return WasmEdge_GlobalTypeGetMutability(getGlobalTypeContext(env, thisObject));
}