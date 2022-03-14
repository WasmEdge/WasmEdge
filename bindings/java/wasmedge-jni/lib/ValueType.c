//
// Created by Kenvi Zhu on 2022-01-13.
//
#include "wasmedge/wasmedge.h"
#include "jni.h"
#include "common.h"

WasmEdge_Value JavaValueToWasmEdgeValue(JNIEnv *env, jobject jVal) {
    jclass valueClass = (*env)->FindClass(env, "org/wasmedge/WasmEdgeValue");

    jmethodID getType = (*env)->GetMethodID(env, valueClass, "getType", "()Lorg/wasmedge/enums/ValueType;");

    jobject valType = (*env)->CallObjectMethod(env, jVal, getType);

    jclass typeClass = (*env)->GetObjectClass(env, valType);

    jmethodID getVal = (*env)->GetMethodID(env, typeClass, "getValue", "()I");

    jint jType = (*env)->CallIntMethod(env,valType, getVal);

    enum WasmEdge_ValType type = (enum WasmEdge_ValType)jType;

    WasmEdge_Value val;

    switch (type) {
        case WasmEdge_ValType_I32:
            return WasmEdge_ValueGenI32(getIntVal(env, jVal));
        case WasmEdge_ValType_I64:
            return WasmEdge_ValueGenI64(getLongVal(env, jVal));
        case WasmEdge_ValType_F32:
            return WasmEdge_ValueGenF32(getFloatVal(env, jVal));
        case WasmEdge_ValType_F64:
            return WasmEdge_ValueGenF64(getDoubleVal(env, jVal));
        case WasmEdge_ValType_V128:
            //TODO
            return WasmEdge_ValueGenV128(getLongVal(env, jVal));
        case WasmEdge_ValType_ExternRef:
            return WasmEdge_ValueGenExternRef(getStringVal(env, jVal));

        case WasmEdge_ValType_FuncRef:
            return WasmEdge_ValueGenFuncRef(getLongVal(env, jVal));
    }
}

jobject WasmEdgeValueToJavaValue(JNIEnv * env, WasmEdge_Value value) {
    const char* valClassName = NULL;
    char* key;
    switch (value.Type) {
        case WasmEdge_ValType_I32:
            valClassName = "org/wasmedge/WasmEdgeI32Value";
            break;
        case WasmEdge_ValType_I64:
            valClassName = "org/wasmedge/WasmEdgeI64Value";
            break;
        case WasmEdge_ValType_F32:
            valClassName = "org/wasmedge/WasmEdgeF32Value";
            break;
        case WasmEdge_ValType_F64:
            valClassName = "org/wasmedge/WasmEdgeF64Value";
            break;
        case WasmEdge_ValType_ExternRef:
            valClassName = "org/wasmedge/WasmEdgeExternRef";
            break;
        case WasmEdge_ValType_FuncRef:
            valClassName = "org/wasmedge/WasmEdgeFuncRef";
            break;
    }
    printf("find val class : %s\n", valClassName);
    jclass valClass = (*env)->FindClass(env, valClassName);


    jmethodID constructor = (*env)->GetMethodID(env, valClass, "<init>", "()V");


    printf("constructor is null ? %s\n", constructor == NULL ? "true" : "false");

    jobject jVal = (*env)->NewObject(env, valClass, constructor);
    printf("new object is null ? %s\n", jVal == NULL ? "true" : "false");
    setJavaValueObject(env, value, jVal);
    return jVal;
}


