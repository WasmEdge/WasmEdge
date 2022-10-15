//
// Created by Kenvi Zhu on 2021-11-17.
//
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



JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listTable
        (JNIEnv *env , jobject thisObject) {
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    uint32_t tabLen = WasmEdge_StoreListTableLength(storeCxt);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String) * tabLen);
    uint32_t RealTabNum = WasmEdge_StoreListTable(storeCxt, nameList, tabLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealTabNum);

    free(nameList);

    return jNameList;
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_findTable
        (JNIEnv *env , jobject thisObject, jstring jTabName) {

    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);
    WasmEdge_String wTabName = JStringToWasmString(env, jTabName);

    WasmEdge_TableInstanceContext * tabInst = WasmEdge_StoreFindTable(storeCxt, wTabName);
    jobject jTabInst = createJTableInstanceContext(env, tabInst);

    WasmEdge_StringDelete(wTabName);

    return jTabInst;
}

/*
 * Class:     org_wasmedge_StoreContext
 * Method:    findTableRegistered
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Lorg/wasmedge/TableInstanceContext;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_findTableRegistered
        (JNIEnv *env, jobject thisObject, jstring jModName, jstring jTabName) {
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);
    WasmEdge_String wModName = JStringToWasmString(env, jModName);
    WasmEdge_String wTabName = JStringToWasmString(env, jTabName);

    WasmEdge_TableInstanceContext * tabInst = WasmEdge_StoreFindTableRegistered(storeCxt, wModName, wTabName);
    jobject jTabInst = createJTableInstanceContext(env, tabInst);


    WasmEdge_StringDelete(wTabName);
    WasmEdge_StringDelete(wModName);
    return jTabInst;
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

    uint32_t tabLen = WasmEdge_StoreListTableRegisteredLength(storeCxt, wModName);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String) * tabLen);
    uint32_t RealTabNum = WasmEdge_StoreListTableRegistered(storeCxt, wModName,nameList, tabLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealTabNum);

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

    uint32_t memLen = WasmEdge_StoreListMemoryLength(storeCxt);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String) * memLen);
    uint32_t RealMemNum = WasmEdge_StoreListMemory(storeCxt, nameList, memLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealMemNum);

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

    uint32_t memLen = WasmEdge_StoreListMemoryRegisteredLength(storeCxt, wModName);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String) * memLen);
    uint32_t RealModNum = WasmEdge_StoreListMemoryRegistered(storeCxt, wModName,nameList, memLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealModNum);

    free(nameList);

    return jNameList;
};




/*
 * Class:     org_wasmedge_StoreContext
 * Method:    findMemory
 * Signature: (Ljava/lang/String;)Lorg/wasmedge/MemoryInstanceContext;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_findMemory
        (JNIEnv *env , jobject thisObject, jstring jMemName) {
    WasmEdge_StoreContext* storeCxt = getStoreContext(env, thisObject);

    WasmEdge_String wMemName = JStringToWasmString(env, jMemName);

    WasmEdge_MemoryInstanceContext * memInst = WasmEdge_StoreFindMemory(storeCxt, wMemName);
    jobject jMemInst = createJMemoryInstanceContext(env, memInst);

    WasmEdge_StringDelete(wMemName);

    return jMemInst;
}

/*
 * Class:     org_wasmedge_StoreContext
 * Method:    findMemoryRegistered
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Lorg/wasmedge/MemoryInstanceContext;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_findMemoryRegistered
        (JNIEnv *env, jobject thisObject, jstring jModName, jstring jMemName) {

    WasmEdge_StoreContext* storeCxt = getStoreContext(env, thisObject);

    WasmEdge_String wModName = JStringToWasmString(env, jModName);
    WasmEdge_String wMemName = JStringToWasmString(env, jMemName);

    WasmEdge_MemoryInstanceContext * memInst = WasmEdge_StoreFindMemoryRegistered(storeCxt, wModName, wMemName);
    jobject jMemInst = createJMemoryInstanceContext(env, memInst);

    WasmEdge_StringDelete(wMemName);
    WasmEdge_StringDelete(wModName);

    return jMemInst;

}
/*
 * Class:     org_wasmedge_StoreContext
 * Method:    listGlobal
 * Signature: ()Ljava/util/List;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_listGlobal
        (JNIEnv *env , jobject thisObject) {
    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

    uint32_t globLen = WasmEdge_StoreListGlobalLength(storeCxt);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String) * globLen);
    uint32_t RealGlobNum = WasmEdge_StoreListGlobal(storeCxt, nameList, globLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealGlobNum);

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

    uint32_t modLen = WasmEdge_StoreListGlobalRegisteredLength(storeCxt, wModName);
    WasmEdge_String* nameList = (WasmEdge_String*)malloc(sizeof (struct WasmEdge_String) * modLen);
    uint32_t RealModNum = WasmEdge_StoreListGlobalRegistered(storeCxt, wModName,nameList, modLen);

    jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealModNum);

    free(nameList);

    return jNameList;
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_findGlobal
        (JNIEnv * env, jobject thisObject, jstring jGlobName) {

    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);
    WasmEdge_String wGlobName = JStringToWasmString(env, jGlobName);

    WasmEdge_GlobalInstanceContext * globInst = WasmEdge_StoreFindGlobal(storeCxt, wGlobName);
    return createJGlobalInstanceContext(env, globInst);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_StoreContext_findGlobalRegistered
        (JNIEnv *env, jobject thisObject, jstring jModName, jstring jGlobName) {

    WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);
    WasmEdge_String wModName = JStringToWasmString(env, jModName);
    WasmEdge_String wGlobName = JStringToWasmString(env, jGlobName);

    WasmEdge_GlobalInstanceContext * globInst = WasmEdge_StoreFindGlobalRegistered(storeCxt, wModName, wGlobName);
    jobject jGlob = createJGlobalInstanceContext(env, globInst);

    WasmEdge_StringDelete(wGlobName);
    WasmEdge_StringDelete(wModName);

    return jGlob;
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

