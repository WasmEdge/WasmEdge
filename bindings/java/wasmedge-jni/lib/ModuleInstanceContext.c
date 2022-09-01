//
// Created by Kenvi Zhu on 2022-03-14.
//

#include "wasmedge/wasmedge.h"
#include "../jni/org_wasmedge_ModuleInstanceContext.h"
#include "jni.h"
#include "common.h"
#include "FunctionTypeInstance.h"
#include "MemoryInstanceContext.h"
#include "GlobalInstanceContext.h"
#include "TableInstanceContext.h"

jobject createJModuleInstanceObject(JNIEnv* env, const WasmEdge_ModuleInstanceContext * impObj);

WasmEdge_ModuleInstanceContext * getModuleInstanceContext(JNIEnv* env, jobject jImpObjCxt) {

    if(jImpObjCxt == NULL) {
        return NULL;
    }
    WasmEdge_ModuleInstanceContext * moduleInstanceContext=
            (struct WasmEdge_ModuleInstanceContext *)getPointer(env, jImpObjCxt);

    return moduleInstanceContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_ModuleInstanceContext_nativeInit
(JNIEnv * env, jobject thisObject, jstring moduleName) {
    WasmEdge_ModuleInstanceContext * impCxt = WasmEdge_ModuleInstanceCreate(JStringToWasmString(env, moduleName));
    setPointer(env, thisObject, (long)impCxt);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ModuleInstanceContext_initWASI
        (JNIEnv *env , jobject thisObject, jobjectArray jArgs, jobjectArray jEnvs, jobjectArray jPreopens) {

    const char** args = JStringArrayToPtr(env, jArgs);
    const char** envs = JStringArrayToPtr(env, jEnvs);
    const char** preopens = JStringArrayToPtr(env, jPreopens);

    WasmEdge_ModuleInstanceContext* impCxt = getModuleInstanceContext(env, thisObject);
    WasmEdge_ModuleInstanceInitWASI(impCxt, args, (*env)->GetArrayLength(env, jArgs),
                                  envs, (*env)->GetArrayLength(env, jEnvs),
                                  preopens, (*env)->GetArrayLength(env, jPreopens));
    ReleaseCString(env, jArgs, args);
    ReleaseCString(env, jEnvs, envs);
    ReleaseCString(env, jPreopens, preopens);
}
//TO DO, Exit Code
JNIEXPORT jint JNICALL Java_org_wasmedge_ModuleInstanceContext_getWASIExitCode
        (JNIEnv * env, jobject thisObject) {
    return WasmEdge_ImportObjectWASIGetExitCode(getImportObjectContext(env, thisObject));
}

JNIEXPORT jobject JNICALL Java_org_wasmEdge_ModuleInstanceContext_createWasmEdgeProcess
        (JNIEnv * env, jclass thisClass, jobjectArray jAllowedCmds, jboolean jAllowAll) {

    const char** allowedCmds = JStringArrayToPtr(env, jAllowedCmds);

    WasmEdge_ModuleInstanceContext* impCxt = WasmEdge_ModuleInstanceCreateWasmEdgeProcess(allowedCmds, (*env)->GetArrayLength(env, jAllowedCmds), jAllowAll);

    ReleaseCString(env, jAllowedCmds, allowedCmds);

    return createJModuleInstanceObject(env, impCxt);

}

JNIEXPORT void JNICALL Java_org_wasmedge_ModuleInstanceContext_initWasmEdgeProcess
(JNIEnv *env, jobject thisObject, jobjectArray jAllowedCmds, jboolean jAllowAll) {
    const char** allowedCmds = JStringArrayToPtr(env, jAllowedCmds);

    WasmEdge_ModuleInstanceContext* impCxt = getModuleInstanceContext(env, thisObject);
    WasmEdge_ModuleInstanceInitWasmEdgeProcess(impCxt, allowedCmds, jAllowAll);

    ReleaseCString(env, jAllowedCmds, allowedCmds);
}

JNIEXPORT void JNICALL Java_org_wasmEdge_ModuleInstanceContext_addFunction
(JNIEnv * env, jobject thisObject, jstring jFuncName, jobject jFunc) {
    WasmEdge_ModuleInstanceContext * impObjCxt = getModuleInstanceContext(env, thisObject);
    WasmEdge_FunctionInstanceContext *funcInst = getFunctionInstanceContext(env, jFunc);

    WasmEdge_ModuleInstanceAddFunction(impObjCxt, JStringToWasmString(env, jFuncName), funcInst);

}

JNIEXPORT void JNICALL Java_org_wasmEdge_ModuleInstanceContext_addTable
(JNIEnv * env, jobject thisObject, jstring jTabName, jobject jTable) {
    WasmEdge_ModuleInstanceContext * impObjCxt = getModuleInstanceContext(env, thisObject);
    WasmEdge_TableInstanceContext *tabIns = getTableInstanceContext(env, jTable);
    WasmEdge_ModuleInstanceAddTable(impObjCxt, JStringToWasmString(env, jTabName), tabIns);

}

JNIEXPORT void JNICALL Java_org_wasmEdge_ModuleInstanceContext_addMemory
(JNIEnv * env, jobject thisObject, jstring jMemName, jobject jMem) {

    WasmEdge_ModuleInstanceContext * impObjCxt = getModuleInstanceContext(env, thisObject);
    WasmEdge_MemoryInstanceContext *memCxt = getMemoryInstanceContext(env, jMem);

    WasmEdge_ModuleInstanceAddMemory(impObjCxt, JStringToWasmString(env, jMemName), memCxt);
}

JNIEXPORT void JNICALL Java_org_wasmEdge_ModuleInstanceContext_addGlobal
(JNIEnv * env, jobject thisObject, jstring jGlobalName, jobject jGlobal) {

    WasmEdge_ModuleInstanceContext * impObjCxt = getModuleInstanceContext(env, thisObject);
    WasmEdge_GlobalInstanceContext *globalInstance = getGlobalInstanceContext(env, jGlobal);
    WasmEdge_ModuleInstanceAddGlobal(impObjCxt, JStringToWasmString(env, jGlobalName), globalInstance);
}

JNIEXPORT void JNICALL Java_org_wasmEdge_ModuleInstanceContext_delete
(JNIEnv * env, jobject thisObject) {
    WasmEdge_ModuleInstanceContext *impObjCxt = getModuleInstanceContext(env, thisObject);
    WasmEdge_ModuleInstanceDelete(impObjCxt);
    setPointer(env, thisObject, 0);

}

JNIEXPORT jobject JNICALL Java_org_wasmEdge_ModuleInstanceContext_CreateWASI
        (JNIEnv *env , jclass thisClass, jobjectArray jArgs, jobjectArray jEnvs, jobjectArray jPreopens) {

    const char** args = JStringArrayToPtr(env, jArgs);
    const char** envs = JStringArrayToPtr(env, jEnvs);
    const char** preopens = JStringArrayToPtr(env, jPreopens);

    WasmEdge_ModuleInstanceContext * importObjectContext = WasmEdge_ModuleInstanceCreateWASI(args, (*env)->GetArrayLength(env, jArgs),
                                    envs, (*env)->GetArrayLength(env,jEnvs),
                                    preopens, (*env)->GetArrayLength(env,jPreopens));

    ReleaseCString(env, jArgs, args);
    ReleaseCString(env, jEnvs, envs);
    ReleaseCString(env, jPreopens, preopens);

    return createJModuleInstanceObject(env, importObjectContext);
}


jobject createJModuleInstanceObject(JNIEnv* env, const WasmEdge_ModuleInstanceContext * impObj) {

    jclass clazz = (*env)->FindClass(env, "org/wasmedge/ModuleInstanceObject");

    jmethodID constructorId = (*env)->GetMethodID(env, clazz, "<init>", "(J)V");

    return (*env)->NewObject(env, clazz, constructorId, (long) impObj);
}
