//
// Created by Kenvi Zhu on 2021-12-04.
//
#include "common.h"
#include "../jni/org_wasmedge_FunctionTypeContext.h"
#include "wasmedge/wasmedge.h"

jobject ConvertToJavaFunctionType(JNIEnv* env, const WasmEdge_FunctionTypeContext* functionTypeContext);

WasmEdge_FunctionTypeContext * getFunctionTypeContext(JNIEnv* env, jobject jFunctionTypeContext) {
    if (jFunctionTypeContext == NULL) {
        return NULL;
    }
    return (WasmEdge_FunctionTypeContext*)getPointer(env, jFunctionTypeContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_FunctionTypeContext_nativeInit
        (JNIEnv * env, jobject thisObject, jintArray paramTypes, jintArray returnTypes) {
    int paramLen = (*env)->GetArrayLength(env, paramTypes);
    int returnLen = (*env)->GetArrayLength(env, returnTypes);
    enum WasmEdge_ValType* paramList = parseValueTypes(env, paramTypes);
    enum WasmEdge_ValType* returnList = parseValueTypes(env, returnTypes);

    WasmEdge_FunctionTypeContext * functionTypeContext =
            WasmEdge_FunctionTypeCreate(paramList, paramLen, returnList, returnLen);
    setPointer(env, thisObject, (jlong)functionTypeContext);
}

JNIEXPORT jintArray JNICALL Java_org_wasmedge_FunctionTypeContext_nativeGetParameters
        (JNIEnv *env, jobject thisObject) {
    WasmEdge_FunctionTypeContext* functionTypeContext = getFunctionTypeContext(env, thisObject);
    uint32_t paramLen = WasmEdge_FunctionTypeGetParametersLength(functionTypeContext);
    enum WasmEdge_ValType* params = malloc(sizeof (enum WasmEdge_ValType) * paramLen);
    WasmEdge_FunctionTypeGetParameters(functionTypeContext, params, paramLen);

    jintArray types = (*env)->NewIntArray(env, paramLen);
    (*env)->SetIntArrayRegion(env, types, 0, paramLen, (jint*) params);
    free(params);
    return types;
}


JNIEXPORT jintArray JNICALL Java_org_wasmedge_FunctionTypeContext_nativeGetReturns
        (JNIEnv * env, jobject thisObject){

    WasmEdge_FunctionTypeContext* functionTypeContext = getFunctionTypeContext(env, thisObject);
    uint32_t returnLen = WasmEdge_FunctionTypeGetReturnsLength(functionTypeContext);
    enum WasmEdge_ValType* returns = malloc(sizeof (enum  WasmEdge_ValType) * returnLen);
    WasmEdge_FunctionTypeGetReturns(functionTypeContext, returns, returnLen);

    jintArray types = (*env)->NewIntArray(env, returnLen);
    (*env)->SetIntArrayRegion(env, types, 0, returnLen, (jint*) returns);
    free(returns);
    return types;
}

/*
 * Class:     org_wasmedge_FunctionTypeContext
 * Method:    delete
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_FunctionTypeContext_delete
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_FunctionTypeContext* functionTypeContext =  getFunctionTypeContext(env, thisObject);
    WasmEdge_FunctionTypeDelete(functionTypeContext);
}

jobject ConvertToJavaValueType(JNIEnv* env, enum WasmEdge_ValType* valType) {
    jclass cls = findJavaClass(env, "org/wasmedge/enums/ValueType");

    if(cls == NULL) {
        return NULL;
    }
    jmethodID jmethodId = (*env)->GetMethodID(env, cls, "parseType", "(I)Lorg/wasmedge/enums/ValueType;");

    if(jmethodId == NULL) {
        return NULL;
    }

    return (*env)->CallStaticObjectMethod(env, cls, jmethodId, (jint)*valType);
}



jobject ConvertToValueTypeList(JNIEnv* env, enum WasmEdge_ValType* list, int32_t len) {
    jclass cls = findJavaClass(env, "java/util/ArrayList");

    if(cls == NULL) {
        return NULL;
    }
    jmethodID listConstructor = findJavaMethod(env, cls, "<init>", "()V");

    if(listConstructor == NULL) {
        return NULL;
    }

    jobject jList = (*env)->NewObject(env, cls, listConstructor);

    jmethodID addMethod = findJavaMethod(env, cls, "add", "(Ljava/lang/Object;)Z");

    if(addMethod == NULL) {
        return NULL;
    }

    enum WasmEdge_ValType* ptr = list;
    for (int i = 0; i < len; ++i) {
        jobject valueType = ConvertToJavaValueType(env, ptr);
        (*env)->CallBooleanMethod(env, jList, addMethod, valueType);
        ptr++;
    }

    return jList;

}

jobject ConvertToJavaFunctionList(JNIEnv * env, WasmEdge_String* nameList, const WasmEdge_FunctionTypeContext** funcList, int32_t len) {
    jclass cls = findJavaClass(env, "java/util/ArrayList");

    if(cls == NULL) {
        return NULL;
    }
    jmethodID listConstructor = findJavaMethod(env, cls, "<init>", "()V");

    if(listConstructor == NULL) {
        return NULL;
    }

    jobject jList = (*env)->NewObject(env, cls, listConstructor);

    jmethodID addMethod = findJavaMethod(env, cls, "add", "(Ljava/lang/Object;)Z");

    if(addMethod == NULL) {
        return NULL;
    }


    for (int i = 0; i < len; ++i) {
        jobject jFunc = ConvertToJavaFunctionType(env, funcList[i]);
        (*env)->CallBooleanMethod(env, jList, addMethod, jFunc);
    }
    return jList;
}


jobject ConvertToJavaFunctionType(JNIEnv* env, const WasmEdge_FunctionTypeContext* functionTypeContext) {
    int retLen = WasmEdge_FunctionTypeGetReturnsLength(functionTypeContext);
    enum WasmEdge_ValType* list = (enum WasmEdge_ValType*)malloc(sizeof (enum WasmEdge_ValType) * retLen);

    int actualLen = WasmEdge_FunctionTypeGetReturns(functionTypeContext, list, retLen);

    jobjectArray retArray = ConvertToValueTypeList(env,  list, actualLen);

    free(list);

    int paramLen = WasmEdge_FunctionTypeGetParametersLength(functionTypeContext);
    enum WasmEdge_ValType* paramList = (enum WasmEdge_ValType*) malloc(sizeof (enum WasmEdge_ValType) * paramLen);

    int actualParamLen = WasmEdge_FunctionTypeGetParameters(functionTypeContext, paramList, paramLen);

    jobject paramArray = ConvertToValueTypeList(env,  paramList, actualParamLen);

    free(paramList);

    jclass cls = findJavaClass(env, "org/wasmedge/FunctionType");

    if(cls == NULL) {
        return NULL;
    }

    jmethodID constructor  = findJavaMethod(env, cls, "<init>", "([Lorg/wasmedge/ValueType;[Lorg/wasmedge/ValueType)V");

    if(constructor == NULL) {
        return NULL;
    }

    return (*env)->NewObject(env, cls, constructor, paramArray, retArray);
}


