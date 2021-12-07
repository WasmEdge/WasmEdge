//
// Created by Kenvi Zhu on 2021-12-04.
//
#include "common.h"
#include "../jni/org_wasmedge_FunctionTypeContext.h"


WasmEdge_FunctionTypeContext * getFunctionTypeContext(JNIEnv* env, jobject jFunctionTypeContext) {
    if (jFunctionTypeContext == NULL) {
        return NULL;
    }
    return (WasmEdge_FunctionTypeContext*)getPointer(env, jFunctionTypeContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_FunctionTypeContext_nativeInit
        (JNIEnv * env, jobject thisObject, jintArray paramTypes, jintArray returnTypes) {
    int paramLen = (*env)->GetArrayLength(env, paramTypes);
    int returnLen = (*env)->GetArrayLength(env, returnTypes);
    enum WasmEdge_ValType* paramList = parseValueTypes(env, paramTypes);
    enum WasmEdge_ValType* returnList = parseValueTypes(env, returnTypes);

    WasmEdge_FunctionTypeContext * functionTypeContext =
            WasmEdge_FunctionTypeCreate(paramList, paramLen, returnList, returnLen);
    setPointer(env, thisObject, (jlong)functionTypeContext);
}

JNIEXPORT jintArray JNICALL Java_org_wasmedge_FunctionTypeContext_nativeGetParameters
        (JNIEnv *env, jobject thisObject) {
    WasmEdge_FunctionTypeContext* functionTypeContext = getFunctionTypeContext(env, thisObject);
    uint32_t paramLen = WasmEdge_FunctionTypeGetParametersLength(functionTypeContext);
    enum WasmEdge_ValType* params = malloc(sizeof (enum WasmEdge_ValType) * paramLen);
    WasmEdge_FunctionTypeGetParameters(functionTypeContext, params, paramLen);

    jintArray types = (*env)->NewIntArray(env, paramLen);
    (*env)->SetIntArrayRegion(env, types, 0, paramLen, (jint*) params);
    free(params);
    return types;
}



JNIEXPORT jintArray JNICALL Java_org_wasmedge_FunctionTypeContext_nativeGetReturns
        (JNIEnv * env, jobject thisObject){

    WasmEdge_FunctionTypeContext* functionTypeContext = getFunctionTypeContext(env, thisObject);
    uint32_t returnLen = WasmEdge_FunctionTypeGetReturnsLength(functionTypeContext);
    enum WasmEdge_ValType* returns = malloc(sizeof (enum  WasmEdge_ValType) * returnLen);
    WasmEdge_FunctionTypeGetReturns(functionTypeContext, returns, returnLen);

    jintArray types = (*env)->NewIntArray(env, returnLen);
    (*env)->SetIntArrayRegion(env, types, 0, returnLen, (jint*) returns);
    free(returns);
    return types;
}

/*
 * Class:     org_wasmedge_FunctionTypeContext
 * Method:    delete
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_FunctionTypeContext_delete
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_FunctionTypeContext* functionTypeContext =  getFunctionTypeContext(env, thisObject);
    WasmEdge_FunctionTypeDelete(functionTypeContext);
}

