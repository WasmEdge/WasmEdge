//
// Created by Kenvi Zhu on 2021-11-17.
//
#include <stdlib.h>
#include "jni.h"
#include "wasmedge/wasmedge.h"
#include "common.h"
#include "FunctionTypeInstance.h"
#include "GlobalInstanceContext.h"
#include "TableInstanceContext.h"
#include "MemoryInstanceContext.h"

JNIEXPORT void JNICALL Java_org_wasmedge_StoreContext_nativeInit
(JNIEnv *env, jobject thisObj) {
    WasmEdge_StoreContext *StoreContext = WasmEdge_StoreCreate();
    setPointer(env, thisObj, (jlong)StoreContext);
}

WasmEdge_StoreContext* getStoreContext(JNIEnv* env, jobject jStoreContext) {

    if(jStoreContext == NULL) {
        return NULL;
    }
    WasmEdge_StoreContext* StoreContext =  (WasmEdge_StoreContext*)getPointer(env, jStoreContext);

    return StoreContext;
}

jobject CreateJavaStoreContext(JNIEnv* env, WasmEdge_StoreContext* storeContext) {
    jclass storeClass = findJavaClass(env, "org/wasmedge/StoreContext");

    jmethodID constructor = (*env)->GetMethodID(env, storeClass, "<init>", "(J)V");

    jobject jStoreContext = (*env)->NewObject(env, storeClass, constructor, (long)storeContext);

    return jStoreContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_StoreContext_delete
        (JNIEnv * env, jobject thisObj) {
    WasmEdge_StoreDelete(getStoreContext(env, thisObj));
}


/*
 * Class:     org_wasmedge_StoreContext
 * Method:    listModule
 * Signature: ()Ljava/util/List;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listModule
        (JNIEnv *env , jobject thisObject) {
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    uint32_t modLen = WasmEdge_StoreListModuleLength(storeCxt);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String) * modLen);
    uint32_t RealModNum = WasmEdge_StoreListModule(storeCxt, nameList, modLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealModNum);

    free(nameList);

    return jNameList;
};

