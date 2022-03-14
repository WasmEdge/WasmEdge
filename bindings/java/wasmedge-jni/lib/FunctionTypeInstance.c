//
// Created by Kenvi Zhu on 2022-03-14.
//
#include "wasmedge/wasmedge.h"
#include "jni.h"
#include "common.h"
#include "FunctionTypeContext.h"


WasmEdge_FunctionInstanceContext * getFunctionInstanceContext(JNIEnv* env, jobject jFuncInstance) {

    if(jFuncInstance == NULL) {
        return NULL;
    }
    WasmEdge_FunctionInstanceContext * funcInstance=
            (struct WasmEdge_FunctionInstanceContext *)getPointer(env, jFuncInstance);

    return funcInstance;
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_FunctionInstanceContext_getFunctionType
        (JNIEnv *env, jobject thisObject) {
    WasmEdge_FunctionInstanceContext * funcInstance = getFunctionInstanceContext(env, thisObject);
    WasmEdge_FunctionTypeContext* funcType = WasmEdge_FunctionInstanceGetFunctionType(funcInstance);
    return ConvertToJavaFunctionType(env, funcType, WasmEdge_StringCreateByCString(""));
}

/*
 * Class:     org_wasmedge_FunctionInstanceContext
 * Method:    delete
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_FunctionInstanceContext_delete
(JNIEnv *env, jobject thisObject) {
    WasmEdge_FunctionInstanceContext *funcInstance = getFunctionInstanceContext(env, thisObject);
    WasmEdge_FunctionInstanceDelete(funcInstance);
    setPointer(env, thisObject, 0);
}

