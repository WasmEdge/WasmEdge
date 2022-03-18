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
#include "ImportObjectContext.h"
#include "common.h"

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
        (JNIEnv * env, jobject thisObject, jobject jStoreContext, jstring jFuncName, jobjectArray jParams,
         jintArray jParamTypes, jobjectArray jReturns, jintArray jReturnTypes) {
    WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, jStoreContext);

    const char* funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);
    WasmEdge_String wFuncName = WasmEdge_StringCreateByCString(funcName);

    jsize paramLen = (*env)->GetArrayLength(env, jParams);

    /* The parameters and returns arrays. */
    WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value));
    int *type = (*env)->GetIntArrayElements(env, jParamTypes, JNI_FALSE);
    for (int i = 0; i < paramLen; i++) {
        WasmEdge_Value val;

        jobject val_object = (*env)->GetObjectArrayElement(env, jParams, i);

        switch (type[i]) {

            case 0:
                val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
                break;
            case 1:
                val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
                break;
            case 2:
                val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
                break;
            case 3:
                val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
                break;
            default:
                break;
        }
        wasm_params[i] = val;
    }

    jsize returnLen = (*env)->GetArrayLength(env, jReturns);
    WasmEdge_Value *returns = malloc(sizeof(WasmEdge_Value) * returnLen);

    //
    WasmEdge_Result result = WasmEdge_ExecutorInvoke(exeCxt, storeCxt, wFuncName, wasm_params, paramLen, returns, returnLen);

    //release resource
    (*env)->ReleaseStringUTFChars(env, jFuncName, funcName);
    WasmEdge_StringDelete(wFuncName);

    handleWasmEdgeResult(env, & result);

    if (WasmEdge_ResultOK(result)) {
        for (int i = 0; i < returnLen; ++i) {
            setJavaValueObject(env, returns[i], (*env)->GetObjectArrayElement(env, jReturns, i));
        }
    }


}

/*
 * Class:     org_wasmedge_ExecutorContext
 * Method:    invokeRegistered
 * Signature: (Lorg/wasmedge/StoreContext;Ljava/lang/String;Ljava/lang/String;[Lorg/wasmedge/WasmEdgeValue;[I[Lorg/wasmedge/WasmEdgeValue;[I)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_invokeRegistered
        (JNIEnv * env, jobject thisObject, jobject jStoreCxt, jstring jModName, jstring jFuncName,
         jobjectArray jParams, jintArray jParamTypes, jobjectArray jReturns, jintArray jReturnTypes) {

    WasmEdge_ExecutorContext * exeCxt = getExecutorContext(env, thisObject);
    WasmEdge_StoreContext * storeCxt = getStoreContext(env, jStoreCxt);

    const char* modName = (*env)->GetStringUTFChars(env, jModName, NULL);
    const char* funcName = (*env)->GetStringUTFChars(env, jFuncName, NULL);

    WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);
    WasmEdge_String wFuncName = WasmEdge_StringCreateByCString(funcName);

    jsize paramLen = (*env)->GetArrayLength(env, jParams);

    /* The parameters and returns arrays. */
    WasmEdge_Value *wasm_params = calloc(paramLen, sizeof(WasmEdge_Value));
    int *type = (*env)->GetIntArrayElements(env, jParamTypes, JNI_FALSE);
    for (int i = 0; i < paramLen; i++) {
        WasmEdge_Value val;

        jobject val_object = (*env)->GetObjectArrayElement(env, jParams, i);

        switch (type[i]) {

            case 0:
                val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
                break;
            case 1:
                val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
                break;
            case 2:
                val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
                break;
            case 3:
                val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
                break;
            default:
                break;
        }
        wasm_params[i] = val;
    }

    jsize returnLen = (*env)->GetArrayLength(env, jReturns);
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
            setJavaValueObject(env, returns[i], (*env)->GetObjectArrayElement(env, jReturns, i));
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

JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_registerImport
        (JNIEnv *env, jobject thisObject, jobject jStore, jobject jImpObj) {
    WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, jStore);
    WasmEdge_ImportObjectContext *impObj = getImportObjectContext(env, jImpObj);

    WasmEdge_ExecutorRegisterImport(exeCxt, storeCxt, impObj);

}

JNIEXPORT void JNICALL Java_org_wasmedge_ExecutorContext_delete
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_ExecutorContext *exeCxt = getExecutorContext(env, thisObject);
    WasmEdge_ExecutorDelete(exeCxt);
}
