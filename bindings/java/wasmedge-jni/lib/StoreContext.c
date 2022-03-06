//
// Created by Kenvi Zhu on 2021-11-17.
//
#include "jni.h"
#include "wasmedge/wasmedge.h"
#include "common.h"

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

JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listFunction
        (JNIEnv *env , jobject thisObject) {
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    uint32_t funcLen = WasmEdge_StoreListFunctionLength(storeCxt);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String));
    printf("-----\n");
    printf("size%d\n", funcLen);
    printf("-----\n");
    uint32_t RealFuncNum = WasmEdge_StoreListFunction(storeCxt, nameList, funcLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealFuncNum);

    free(nameList);

    return jNameList;
}


JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listFunctionRegistered
        (JNIEnv * env, jobject thisObject, jstring jModName) {

    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    const char* modName = (*env)->GetStringUTFChars(env, jModName, NULL);
    WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);

    uint32_t funcLen = WasmEdge_StoreListFunctionRegisteredLength(storeCxt, wModName);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String));
    uint32_t RealFuncNum = WasmEdge_StoreListFunctionRegistered(storeCxt, wModName,nameList, funcLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealFuncNum);

    free(nameList);

    return jNameList;
}


JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listTable
        (JNIEnv *env , jobject thisObject) {
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    uint32_t funcLen = WasmEdge_StoreListTableLength(storeCxt);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String));
    uint32_t RealFuncNum = WasmEdge_StoreListTable(storeCxt, nameList, funcLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealFuncNum);

    free(nameList);

    return jNameList;
}

/*
 * Class:     org_wasmedge_StoreContext
 * Method:    listTableRegistered
 * Signature: (Ljava/lang/String;)Ljava/util/List;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listTableRegistered
        (JNIEnv * env, jobject thisObject, jstring jModName) {

    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    const char* modName = (*env)->GetStringUTFChars(env, jModName, NULL);
    WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);

    uint32_t funcLen = WasmEdge_StoreListTableRegisteredLength(storeCxt, wModName);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String));
    uint32_t RealFuncNum = WasmEdge_StoreListTableRegistered(storeCxt, wModName,nameList, funcLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealFuncNum);

    free(nameList);

    return jNameList;
};

/*
 * Class:     org_wasmedge_StoreContext
 * Method:    listMemory
 * Signature: ()Ljava/util/List;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listMemory
        (JNIEnv *env , jobject thisObject) {
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    uint32_t funcLen = WasmEdge_StoreListMemoryLength(storeCxt);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String));
    uint32_t RealFuncNum = WasmEdge_StoreListMemory(storeCxt, nameList, funcLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealFuncNum);

    free(nameList);

    return jNameList;
};

/*
 * Class:     org_wasmedge_StoreContext
 * Method:    listMemoryRegistered
 * Signature: (Ljava/lang/String;)Ljava/util/List;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listMemoryRegistered
        (JNIEnv * env, jobject thisObject, jstring jModName) {

    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    const char* modName = (*env)->GetStringUTFChars(env, jModName, NULL);
    WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);

    uint32_t funcLen = WasmEdge_StoreListMemoryRegisteredLength(storeCxt, wModName);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String));
    uint32_t RealFuncNum = WasmEdge_StoreListMemoryRegistered(storeCxt, wModName,nameList, funcLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealFuncNum);

    free(nameList);

    return jNameList;
};

/*
 * Class:     org_wasmedge_StoreContext
 * Method:    listGlobal
 * Signature: ()Ljava/util/List;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listGlobal
        (JNIEnv *env , jobject thisObject) {
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    uint32_t funcLen = WasmEdge_StoreListGlobalLength(storeCxt);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String));
    uint32_t RealFuncNum = WasmEdge_StoreListGlobal(storeCxt, nameList, funcLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealFuncNum);

    free(nameList);

    return jNameList;
}

/*
 * Class:     org_wasmedge_StoreContext
 * Method:    listGlobalRegistered
 * Signature: (Ljava/lang/String;)Ljava/util/List;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listGlobalRegistered
        (JNIEnv * env, jobject thisObject, jstring jModName) {

    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    const char* modName = (*env)->GetStringUTFChars(env, jModName, NULL);
    WasmEdge_String wModName = WasmEdge_StringCreateByCString(modName);

    uint32_t funcLen = WasmEdge_StoreListGlobalRegisteredLength(storeCxt, wModName);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String));
    uint32_t RealFuncNum = WasmEdge_StoreListGlobalRegistered(storeCxt, wModName,nameList, funcLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealFuncNum);

    free(nameList);

    return jNameList;
}

/*
 * Class:     org_wasmedge_StoreContext
 * Method:    listModule
 * Signature: ()Ljava/util/List;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listModule
        (JNIEnv *env , jobject thisObject) {
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    uint32_t funcLen = WasmEdge_StoreListModuleLength(storeCxt);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String));
    uint32_t RealFuncNum = WasmEdge_StoreListModule(storeCxt, nameList, funcLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealFuncNum);

    free(nameList);

    return jNameList;
};

