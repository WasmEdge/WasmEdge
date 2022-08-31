//
// Created by Kenvi Zhu on 2022-01-09.
//

#include "../jni/org_wasmedge_ExecutorContext.h"
#include "wasmedge/wasmedge.h"
#include "StatisticsContext.h"
#include "ConfigureContext.h"
#include "common.h"
#include "AstModuleContext.h"
#include "StoreContext.h"
#include "ModuleInstanceContext.h"
#include "common.h"
#include "ValueType.h"
#include "FunctionTypeInstance.h"

WasmEdge_ExecutorContext *getExecutorContext(JNIEnv * env, jobject jExeCtx) {
    if(jExeCtx == NULL) {
        return NULL;
    }

    return (WasmEdge_ExecutorContext*)getPointer(env, jExeCtx);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_nativeInit
        (JNIEnv * env, jobject thisObject, jobject jConfigContext, jobject jStatCxt) {
    WasmEdge_ConfigureContext * confCxt = getConfigureContext(env, jConfigContext);
    WasmEdge_StatisticsContext * statCxt = getStatisticsContext(env, jStatCxt);

    WasmEdge_ExecutorContext * exeCxt = WasmEdge_ExecutorCreate(confCxt, statCxt);
    setPointer(env, thisObject, (long)exeCxt);
}

/*
 * Class:     org_wasmedge_ExecutorContext
 * Method:    instantiate
 * Signature: (Lorg/wasmedge/StoreContext;Lorg/wasmedge/ASTModuleContext;)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_instantiate__Lorg_wasmedge_StoreContext_2Lorg_wasmedge_ASTModuleContext_2
        (JNIEnv * env, jobject thisObject, jobject jStoreCxt, jobject jAstModCxt) {
    WasmEdge_ExecutorContext * exeCxt = getExecutorContext(env, thisObject);
    WasmEdge_StoreContext * storeCxt = getStoreContext(env, jStoreCxt);
    WasmEdge_ASTModuleContext * astModCxt = getASTModuleContext(env, jAstModCxt);

    WasmEdge_ExecutorInstantiate(exeCxt, storeCxt, astModCxt);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_invoke
        (JNIEnv * env, jobject thisObject, jobject jStoreContext, jstring jFuncName, jobject jParams, jobject jReturns) {
    WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, jStoreContext);

    const char* funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);
    WasmEdge_String wFuncName = WasmEdge_StringCreateByCString(funcName);

    jsize paramLen = GetListSize(env, jParams);

    /* The parameters and returns arrays. */
    WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value));
    for (int i = 0; i < paramLen; i++) {
        wasm_params[i] = JavaValueToWasmEdgeValue(env, GetListElement(env, jParams, i));
    }

    uint32_t returnLen = GetReturnLen(WasmEdge_StoreFindFunction(storeCxt, wFuncName));
    WasmEdge_Value *returns = malloc(sizeof(WasmEdge_Value) * returnLen);

    WasmEdge_Result result = WasmEdge_ExecutorInvoke(exeCxt, storeCxt, wFuncName, wasm_params, paramLen, returns, returnLen);

    //release resource
    (*env)->ReleaseStringUTFChars(env, jFuncName, funcName);
    WasmEdge_StringDelete(wFuncName);

    handleWasmEdgeResult(env, & result);


    if (WasmEdge_ResultOK(result)) {
        for (int i = 0; i < returnLen; ++i) {
            AddElementToJavaList(env, jReturns, WasmEdgeValueToJavaValue(env, returns[i]));
        }
    }

}

/*
 * Class:     org_wasmedge_ExecutorContext
 * Method:    invokeRegistered
 * Signature: (Lorg/wasmedge/StoreContext;Ljava/lang/String;Ljava/lang/String;[Lorg/wasmedge/WasmEdgeValue;[I[Lorg/wasmedge/WasmEdgeValue;[I)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_invokeRegistered
        (JNIEnv *env, jobject thisObject, jobject jStoreCxt, jstring jModName, jstring jFuncName, jobject jParams, jobject jReturns) {


    WasmEdge_ExecutorContext * exeCxt = getExecutorContext(env, thisObject);
    WasmEdge_StoreContext * storeCxt = getStoreContext(env, jStoreCxt);

    const char* modName = (*env)->GetStringUTFChars(env, jModName, NULL);
    const char* funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);

    WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);
    WasmEdge_String wFuncName = WasmEdge_StringCreateByCString(funcName);

    jsize paramLen = GetListSize(env, jParams);

    /* The parameters and returns arrays. */
    WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value) * paramLen);
    for (int i = 0; i < paramLen; i++) {

        jobject val_object = GetListElement(env, jParams, i);

        wasm_params[i] = JavaValueToWasmEdgeValue(env, val_object);
    }

    jsize returnLen = GetListSize(env, jReturns);
    WasmEdge_Value *returns = malloc(sizeof(WasmEdge_Value) * returnLen);

    //
    WasmEdge_Result result = WasmEdge_ExecutorInvoke(exeCxt, storeCxt, wFuncName, wasm_params, paramLen, returns, returnLen);

    //release resource
    (*env)->ReleaseStringUTFChars(env, jFuncName, funcName);
    (*env)->ReleaseStringUTFChars(env, jModName, modName);
    WasmEdge_StringDelete(wFuncName);
    WasmEdge_StringDelete(wModName);

    handleWasmEdgeResult(env, & result);

    if (WasmEdge_ResultOK(result)) {
        for (int i = 0; i < returnLen; ++i) {
//            setJavaValueObject(env, returns[i], (*env)->GetObjectArrayElement(env, jReturns, i));
        }
    }

}

JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_registerModule
        (JNIEnv * env, jobject thisObject, jobject jStoreCxt, jobject jAstModCxt, jstring jModName) {
    WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, jStoreCxt);

    WasmEdge_ASTModuleContext * astModCxt = getASTModuleContext(env, jAstModCxt);

    const char* modName = (*env)->GetStringUTFChars(env, jModName, NULL);
    WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);

    WasmEdge_Result result = WasmEdge_ExecutorRegisterModule(exeCxt, storeCxt, astModCxt, wModName);

    // release resources
    (*env)->ReleaseStringUTFChars(env, jModName, modName);
    WasmEdge_StringDelete(wModName);

    handleWasmEdgeResult(env, &result);

}
//TO DO here
JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_registerImport
        (JNIEnv *env, jobject thisObject, jobject jStore, jobject jImpObj) {
    WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, jStore);
    WasmEdge_ModuleInstanceContext *impObj = getModuleInstanceContext(env, jImpObj);

    WasmEdge_ExecutorRegisterImport(exeCxt, storeCxt, impObj);

}

JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_delete
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);
    WasmEdge_ExecutorDelete(exeCxt);
}
