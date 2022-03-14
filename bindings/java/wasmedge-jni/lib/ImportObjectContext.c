//
// Created by Kenvi Zhu on 2022-03-14.
//

#include "wasmedge/wasmedge.h"
#include "jni.h"
#include "common.h"
#include "FunctionTypeInstance.h"
#include "MemoryInstanceContext.h"
#include "GlobalInstanceContext.h"
#include "TableInstanceContext.h"

WasmEdge_ImportObjectContext * getImportObjectContext(JNIEnv* env, jobject jImpObjCxt) {

    if(jImpObjCxt == NULL) {
        return NULL;
    }
    WasmEdge_ImportObjectContext * importObjectContext=
            (struct WasmEdge_ImportObjectContext *)getPointer(env, jImpObjCxt);

    return importObjectContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_ImportObjectContext_nativeInit
(JNIEnv * env, jobject thisObject, jstring moduleName) {
    WasmEdge_ImportObjectContext * impCxt = WasmEdge_ImportObjectCreate(JStringToWasmString(env, moduleName));
    setPointer(env, thisObject, (long)impCxt);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ImportObjectContext_initWASI
(JNIEnv * env, jobject thisObject, jobjectArray a, jobjectArray b, jobjectArray c) {

}

JNIEXPORT jint JNICALL Java_org_wasmedge_ImportObjectContext_getWASIExitCode
        (JNIEnv * env, jobject thisObject) {
    return WasmEdge_ImportObjectWASIGetExitCode(getImportObjectContext(env, thisObject));
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ImportObjectContext_createWasmEdgeProcess
        (JNIEnv * env, jclass thisClass, jobjectArray a, jboolean b) {

}

JNIEXPORT void JNICALL Java_org_wasmedge_ImportObjectContext_initWasmEdgeProcess
(JNIEnv *env, jobject thisObject, jobjectArray a, jboolean b) {

}

JNIEXPORT void JNICALL Java_org_wasmedge_ImportObjectContext_addFunction
(JNIEnv * env, jobject thisObject, jstring jFuncName, jobject jFunc) {
    WasmEdge_ImportObjectContext * impObjCxt = getImportObjectContext(env, thisObject);
    WasmEdge_FunctionInstanceContext *funcInst = getFunctionInstanceContext(env, jFunc);

    WasmEdge_ImportObjectAddFunction(impObjCxt, JStringToWasmString(env, jFuncName), funcInst);

}

JNIEXPORT void JNICALL Java_org_wasmedge_ImportObjectContext_addTable
(JNIEnv * env, jobject thisObject, jstring jTabName, jobject jTable) {
    WasmEdge_ImportObjectContext * impObjCxt = getImportObjectContext(env, thisObject);
    WasmEdge_TableInstanceContext *tabIns = getTableInstanceContext(env, jTable);
    WasmEdge_ImportObjectAddTable(impObjCxt, JStringToWasmString(env, jTabName), tabIns);

}

JNIEXPORT void JNICALL Java_org_wasmedge_ImportObjectContext_addMemory
(JNIEnv * env, jobject thisObject, jstring jMemName, jobject jMem) {

    WasmEdge_ImportObjectContext * impObjCxt = getImportObjectContext(env, thisObject);
    WasmEdge_MemoryInstanceContext *memCxt = getMemoryInstanceContext(env, jMem);

    WasmEdge_ImportObjectAddMemory(impObjCxt, JStringToWasmString(env, jMemName), memCxt);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ImportObjectContext_addGlobal
(JNIEnv * env, jobject thisObject, jstring jGlobalName, jobject jGlobal) {

    WasmEdge_ImportObjectContext * impObjCxt = getImportObjectContext(env, thisObject);
    WasmEdge_GlobalInstanceContext *globalInstance = getGlobalInstanceContext(env, jGlobal);
    WasmEdge_ImportObjectAddGlobal(impObjCxt, JStringToWasmString(env, jGlobalName), globalInstance);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ImportObjectContext_delete
(JNIEnv * env, jobject thisObject) {
    WasmEdge_ImportObjectContext *impObjCxt = getImportObjectContext(env, thisObject);
    WasmEdge_ImportObjectDelete(impObjCxt);
    setPointer(env, thisObject, 0);

}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ImportObjectContext_CreateWASI
        (JNIEnv *env , jclass thisClass, jobjectArray a, jobjectArray b, jobjectArray c) {

}
