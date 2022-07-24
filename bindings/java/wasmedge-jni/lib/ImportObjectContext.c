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

jobject createJImportObject(JNIEnv* env, const WasmEdge_ImportObjectContext * impObj);

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
        (JNIEnv *env , jobject thisObject, jobjectArray jArgs, jobjectArray jEnvs, jobjectArray jPreopens) {

    const char** args = JStringArrayToPtr(env, jArgs);
    const char** envs = JStringArrayToPtr(env, jEnvs);
    const char** preopens = JStringArrayToPtr(env, jPreopens);

    WasmEdge_ImportObjectContext* impCxt = getImportObjectContext(env, thisObject);
    WasmEdge_ImportObjectInitWASI(impCxt, args, (*env)->GetArrayLength(env, jArgs),
                                  envs, (*env)->GetArrayLength(env, jEnvs),
                                  preopens, (*env)->GetArrayLength(env, jPreopens));
    ReleaseCString(env, jArgs, args);
    ReleaseCString(env, jEnvs, envs);
    ReleaseCString(env, jPreopens, preopens);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_ImportObjectContext_getWASIExitCode
        (JNIEnv * env, jobject thisObject) {
    return WasmEdge_ImportObjectWASIGetExitCode(getImportObjectContext(env, thisObject));
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_ImportObjectContext_createWasmEdgeProcess
        (JNIEnv * env, jclass thisClass, jobjectArray jAllowedCmds, jboolean jAllowAll) {

    const char** allowedCmds = JStringArrayToPtr(env, jAllowedCmds);

    WasmEdge_ImportObjectContext* impCxt = WasmEdge_ImportObjectCreateWasmEdgeProcess(allowedCmds, (*env)->GetArrayLength(env, jAllowedCmds), jAllowAll);

    ReleaseCString(env, jAllowedCmds, allowedCmds);

    return createJImportObject(env, impCxt);

}

JNIEXPORT void JNICALL Java_org_wasmedge_ImportObjectContext_initWasmEdgeProcess
(JNIEnv *env, jobject thisObject, jobjectArray jAllowedCmds, jboolean jAllowAll) {
    const char** allowedCmds = JStringArrayToPtr(env, jAllowedCmds);

    WasmEdge_ImportObjectContext* impCxt = getImportObjectContext(env, thisObject);
    WasmEdge_ImportObjectInitWasmEdgeProcess(impCxt, allowedCmds, (*env)->GetArrayLength(env, jAllowedCmds), jAllowAll);

    ReleaseCString(env, jAllowedCmds, allowedCmds);
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
        (JNIEnv *env , jclass thisClass, jobjectArray jArgs, jobjectArray jEnvs, jobjectArray jPreopens) {

    const char** args = JStringArrayToPtr(env, jArgs);
    const char** envs = JStringArrayToPtr(env, jEnvs);
    const char** preopens = JStringArrayToPtr(env, jPreopens);

    WasmEdge_ImportObjectContext * importObjectContext = WasmEdge_ImportObjectCreateWASI(args, (*env)->GetArrayLength(env, jArgs),
                                    envs, (*env)->GetArrayLength(env,jEnvs),
                                    preopens, (*env)->GetArrayLength(env,jPreopens));

    ReleaseCString(env, jArgs, args);
    ReleaseCString(env, jEnvs, envs);
    ReleaseCString(env, jPreopens, preopens);

    return createJImportObject(env, importObjectContext);
}


jobject createJImportObject(JNIEnv* env, const WasmEdge_ImportObjectContext * impObj) {

    jclass clazz = (*env)->FindClass(env, "org/wasmedge/ImportObjectContext");

    jmethodID constructorId = (*env)->GetMethodID(env, clazz, "<init>", "(J)V");

    return (*env)->NewObject(env, clazz, constructorId, (long) impObj);
}
