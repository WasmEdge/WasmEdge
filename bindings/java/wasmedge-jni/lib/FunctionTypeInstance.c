//
// Created by Kenvi Zhu on 2022-03-14.
//
#include "wasmedge/wasmedge.h"
#include "jni.h"
#include "common.h"
#include "FunctionTypeContext.h"
#include "FunctionTypeInstance.h"

WasmEdge_Result HostFunc(void *Ptr, WasmEdge_MemoryInstanceContext * Mem,
                           const WasmEdge_Value *In, WasmEdge_Value *Out) {
    printf("host function called\n");

    return WasmEdge_Result_Success;

}

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
    const WasmEdge_FunctionTypeContext* funcType = WasmEdge_FunctionInstanceGetFunctionType(funcInstance);
    return createJFunctionTypeContext(env, funcType);
}

JNIEXPORT void JNICALL Java_org_wasmedge_FunctionInstanceContext_nativeCreateFunction
        (JNIEnv *env, jobject thisObject, jobject jFuncType, jobject jHostFunc, jobject jData, jlong jCost) {
    WasmEdge_FunctionTypeContext* funcCxt = getFunctionTypeContext(env, jFuncType);
    HostFuncParam * params = malloc(sizeof(struct HostFuncParam));
    params->jfunc = jHostFunc;
    params->env = env;
    WasmEdge_FunctionInstanceContext *funcInstance = WasmEdge_FunctionInstanceCreate(funcCxt, HostFunc, params, jCost);
    setPointer(env, thisObject, (long)funcInstance);
}

JNIEXPORT void JNICALL Java_org_wasmedge_FunctionInstanceContext_nativeCreateBinding
        (JNIEnv *env, jobject thisObject, jobject jWrapFuncType, jobject jWrapFunc, jobject jBinding, jobject jData, jlong jCost) {

}
