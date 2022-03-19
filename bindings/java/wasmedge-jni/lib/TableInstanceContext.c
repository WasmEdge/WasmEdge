//
// Created by Kenvi Zhu on 2022-01-14.
//
#include "../jni/org_wasmedge_GlobalInstanceContext.h"
#include "wasmedge/wasmedge.h"
#include "TableTypeContext.h"
#include "common.h"
#include "ValueType.h"


WasmEdge_TableInstanceContext * getTableInstanceContext(JNIEnv* env, jobject jTableInstanceContext) {

    if(jTableInstanceContext == NULL) {
        return NULL;
    }
    WasmEdge_TableInstanceContext * tableInstanceContext =
            (struct WasmEdge_TableInstanceContext*)getPointer(env, jTableInstanceContext);

    return tableInstanceContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_TableInstanceContext_nativeInit
        (JNIEnv * env, jobject thisObject, jobject jTableTypeContext) {
    WasmEdge_TableTypeContext *tableTypeContext = getTableTypeContext(env, jTableTypeContext);
    WasmEdge_TableInstanceContext * tableInstanceContext = WasmEdge_TableInstanceCreate(tableTypeContext);
    setPointer(env, thisObject, (long)tableInstanceContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_TableInstanceContext_delete
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_TableInstanceContext * tableInstanceContext = getTableInstanceContext(env, thisObject);
    WasmEdge_TableInstanceDelete(tableInstanceContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_TableInstanceContext_setData
        (JNIEnv * env, jobject thisObject, jobject jVal, jint jOffSet) {
    WasmEdge_TableInstanceContext * tableInstanceContext = getTableInstanceContext(env, thisObject);
    WasmEdge_Value data = JavaValueToWasmEdgeValue(env, jVal);
    WasmEdge_Result result = WasmEdge_TableInstanceSetData(tableInstanceContext, data, jOffSet);
    handleWasmEdgeResult(env, &result);
}


JNIEXPORT jobject JNICALL Java_org_wasmedge_TableInstanceContext_getData
        (JNIEnv * env, jobject thisObject, jobject jValType, jint jOffSet) {
    //TODO fixme
    WasmEdge_TableInstanceContext * tableInstanceContext = getTableInstanceContext(env, thisObject);

    jclass typeClass = (*env)->GetObjectClass(env, jValType);
    jmethodID typeGetter = (*env)->GetMethodID(env, typeClass, "getValue", "()I");

    jint valType = (*env)->CallIntMethod(env, jValType, typeGetter);

    WasmEdge_Value val;

    switch (valType) {
        case WasmEdge_ValType_I32:
            val = WasmEdge_ValueGenI32(0);
            break;
        case WasmEdge_ValType_I64:
            val = WasmEdge_ValueGenF64(0);
            break;
        case WasmEdge_ValType_F32:
            val = WasmEdge_ValueGenF32(0.0);
            break;
        case WasmEdge_ValType_F64:
            val = WasmEdge_ValueGenF64(0.0);
            break;
        case WasmEdge_ValType_FuncRef:
            val = WasmEdge_ValueGenNullRef(WasmEdge_RefType_FuncRef);
            break;
        case WasmEdge_ValType_ExternRef:
            val = WasmEdge_ValueGenNullRef(WasmEdge_RefType_ExternRef);
            break;
    }

    WasmEdge_TableInstanceGetData(tableInstanceContext, &val, jOffSet);
    return WasmEdgeValueToJavaValue(env, val);

}

JNIEXPORT jint JNICALL Java_org_wasmedge_TableInstanceContext_getSize
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_TableInstanceContext * tableInstanceContext = getTableInstanceContext(env, thisObject);
    return WasmEdge_TableInstanceGetSize(tableInstanceContext);
}


JNIEXPORT jint JNICALL Java_org_wasmedge_TableInstanceContext_grow
        (JNIEnv * env, jobject thisObject, jint jSize) {
    WasmEdge_TableInstanceContext * tableInstanceContext = getTableInstanceContext(env, thisObject);
    WasmEdge_Result result = WasmEdge_TableInstanceGrow(tableInstanceContext, jSize);
    handleWasmEdgeResult(env, &result);

}

jobject createJTableInstanceContext(JNIEnv* env, const WasmEdge_TableInstanceContext * tabInstance) {

    // FIXME add to all instances.
    if(tabInstance == NULL) {
        return NULL;
    }

    jclass clazz = (*env)->FindClass(env, "org/wasmedge/TableInstanceContext");
    jmethodID constructorId = (*env)->GetMethodID(env, clazz, "<init>", "(J)V");
    return (*env)->NewObject(env, clazz, constructorId, (long) tabInstance);
}
