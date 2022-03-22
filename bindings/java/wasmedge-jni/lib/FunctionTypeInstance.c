//
// Created by Kenvi Zhu on 2022-03-14.
//
#include "wasmedge/wasmedge.h"
#include "jni.h"
#include "common.h"
#include "FunctionTypeContext.h"
#include "FunctionTypeInstance.h"
#include "MemoryInstanceContext.h"

WasmEdge_Result HostFunc(void *Ptr, WasmEdge_MemoryInstanceContext * Mem,
                           const WasmEdge_Value *In, WasmEdge_Value *Out) {
    printf("host function called\n");

    HostFuncParam * param = (HostFuncParam*)Ptr;
    JNIEnv * env = param->env;
    char* funcKey = param->jFuncKey;

    jstring jFuncKey = (*env)->NewStringUTF(env, funcKey);

    printf("param parsed: %s\n", funcKey);
    jclass clazz = (*env)->FindClass(env, "org/wasmedge/WasmEdgeVM");
    jmethodID funcGetter = (*env)->GetStaticMethodID(env, clazz, "getHostFunc", "(Ljava/lang/String;)Lorg/wasmedge/HostFunction;");

    printf("parse host func\n");

    jobject jFunc = (*env)->CallStaticObjectMethod(env, clazz, funcGetter, jFuncKey);

    jclass jFuncClass = (*env)->GetObjectClass(env, jFunc);

    jmethodID funcMethod = (*env)->GetMethodID(env, jFuncClass, "apply", "(Lorg/wasmedge/MemoryInstanceContext;Ljava/util/List;Ljava/util/List;)Lorg/wasmedge/Result;");

    printf("call host func from java\n");
    if(jFunc == NULL || jFuncClass == NULL || funcMethod == NULL) {
        printf("invalid input\n");
    }

    jobject jMem = createJMemoryInstanceContext(env, Mem);

    jobject jParams = CreateJavaArrayList(env, 1);
    jobject jReturns = CreateJavaArrayList(env, 1);

    (*env)->CallObjectMethod(env, jFunc, funcMethod, jMem, jParams, jReturns);
    printf("return result\n");

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
        (JNIEnv *env, jobject thisObject, jobject jFuncType, jstring jHostFuncKey, jobject jData, jlong jCost) {
    WasmEdge_FunctionTypeContext* funcCxt = getFunctionTypeContext(env, jFuncType);
    HostFuncParam * params = malloc(sizeof(struct HostFuncParam));

    char* funcKey = (*env)->GetStringUTFChars(env, jHostFuncKey, NULL);
    params->jFuncKey= funcKey;
    params->env = env;
    WasmEdge_FunctionInstanceContext *funcInstance = WasmEdge_FunctionInstanceCreate(funcCxt, HostFunc, params, jCost);
    setPointer(env, thisObject, (long)funcInstance);
}

JNIEXPORT void JNICALL Java_org_wasmedge_FunctionInstanceContext_nativeCreateBinding
        (JNIEnv *env, jobject thisObject, jobject jWrapFuncType, jobject jWrapFunc, jobject jBinding, jobject jData, jlong jCost) {

}

jobject createJFunctionInstanceContext(JNIEnv* env, const WasmEdge_FunctionInstanceContext * funcInstance) {

    // FIXME add to all instances.
    if(funcInstance == NULL) {
        return NULL;
    }

    jclass clazz = (*env)->FindClass(env, "org/wasmedge/FunctionInstanceContext");
    jmethodID constructorId = (*env)->GetMethodID(env, clazz, "<init>", "(J)V");
    return (*env)->NewObject(env, clazz, constructorId, (long) funcInstance);
}

