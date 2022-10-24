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
            return WasmEdge_ValueGenV128(0);
        case WasmEdge_ValType_ExternRef:
            //TODO
            return WasmEdge_ValueGenFuncRef(-1);
        case WasmEdge_ValType_FuncRef:
            return WasmEdge_ValueGenFuncRef(-1);
    }
}

jobject WasmEdgeValueToJavaValue(JNIEnv * env, WasmEdge_Value value) {
    const char* valClass = NULL;
    switch (value.Type) {
        case WasmEdge_ValType_I32:
            valClass = "org/wasmedge/WasmEdgeI32Value";
            break;
        case WasmEdge_ValType_I64:
            valClass = "org/wasmedge/WasmEdgeI64Value";
            break;
        case WasmEdge_ValType_F32:
            valClass = "org/wasmedge/WasmEdgeF32Value";
            break;
        case WasmEdge_ValType_F64:
            valClass = "org/wasmedge/WasmEdgeF64Value";
            break;
    }

    jmethodID constructor = (*env)->GetMethodID(env, valClass, "<init>", "()V");

    jobject jVal = (*env)->NewObject(env, valClass, constructor);

    setJavaValueObject(env, value, jVal);
    return jVal;
}


