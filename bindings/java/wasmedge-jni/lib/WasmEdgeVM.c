//
// Created by Kenvi Zhu on 2021-10-10.
//

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "wasmedge/wasmedge.h"
#include "jni.h"
#include "common.h"
#include "StoreContext.h"
#include "ConfigureContext.h"



void setJavaIntValue(JNIEnv *env, WasmEdge_Value val, jobject jobj) {
    int int_val = WasmEdge_ValueGetI32(val);
    jclass val_clazz = (*env)->GetObjectClass(env, jobj);
    jmethodID val_setter = (*env)->GetMethodID(env, val_clazz, "setValue", "(I)V");
    (*env)->CallIntMethod(env, jobj, val_setter, int_val);
}

void setJavaLongValue(JNIEnv *env, WasmEdge_Value val, jobject jobj) {
    int long_val = WasmEdge_ValueGetI64(val);
    jclass val_clazz = (*env)->GetObjectClass(env, jobj);
    jmethodID val_setter = (*env)->GetMethodID(env, val_clazz, "setValue", "(J)V");
    (*env)->CallLongMethod(env, jobj, val_setter, long_val);
}

void setJavaFloatValue(JNIEnv *env, WasmEdge_Value val, jobject jobj) {
    float float_val = WasmEdge_ValueGetF32(val);
    jclass val_clazz = (*env)->GetObjectClass(env, jobj);
    jmethodID val_setter = (*env)->GetMethodID(env, val_clazz, "setValue", "(F)V");
    (*env)->CallFloatMethod(env, jobj, val_setter, float_val);
}

void setJavaDoubleValue(JNIEnv *env, WasmEdge_Value val, jobject jobj) {
    float double_val = WasmEdge_ValueGetF64(val);
    jclass val_clazz = (*env)->GetObjectClass(env, jobj);
    jmethodID val_setter = (*env)->GetMethodID(env, val_clazz, "setValue", "(D)V");
    (*env)->CallFloatMethod(env, jobj, val_setter, double_val);
}

jobject createDoubleJavaLongValueObject(JNIEnv *env, WasmEdge_Value val) {
    float double_val = WasmEdge_ValueGetF64(val);
    jclass val_clazz = (*env)->FindClass(env, "org/wasmedge/WasmEdgeF64Value");
    jmethodID val_constructor = (*env)->GetMethodID(env, val_clazz, "<init>", "(D)V");
    jobject j_val = (*env)->NewObject(env, val_clazz, val_constructor, double_val);
    return j_val;
}


void setJavaValueObject(JNIEnv *env, WasmEdge_Value value, jobject j_val) {
    switch (value.Type) {
        case WasmEdge_ValType_I32:
            setJavaIntValue(env, value, j_val);
            break;
        case WasmEdge_ValType_I64:
            setJavaLongValue(env, value, j_val);
            break;
        case WasmEdge_ValType_F32:
            setJavaFloatValue(env, value, j_val);
            break;
        case WasmEdge_ValType_F64:
            setJavaDoubleValue(env, value, j_val);
            break;
        default:
            break;
    }
}

WasmEdge_VMContext* getVmContext(JNIEnv* env, jobject vmContextObj) {
    long pointerVal = getPointer(env, vmContextObj);
    return (WasmEdge_VMContext*) pointerVal;
}


JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_runWasmFromFile
        (JNIEnv *env, jobject this_object, jstring file_path, jstring func_name,
         jobjectArray params, jint param_size, jintArray param_types, jobjectArray returns, jint return_size,
         jintArray return_types) {

    printf("Run wasm file\n");

    /* The configure and store context to the VM creation can be NULL. */
    WasmEdge_VMContext *VMCxt = getVmContext(env, this_object);
    printf("Params done");


    /* The parameters and returns arrays. */
    WasmEdge_Value *wasm_params = calloc(param_size, sizeof(WasmEdge_Value));
    int *type = (*env)->GetIntArrayElements(env, param_types, JNI_FALSE);
    for (int i = 0; i < param_size; i++) {
        WasmEdge_Value val;

        jobject val_object = (*env)->GetObjectArrayElement(env, params, i);

        switch (type[i]) {

            case 0:
                val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
                break;
            case 1:
                val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
                break;
            case 2:
                val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
                break;
            case 3:
                val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
                break;
            default:
                break;
        }
        wasm_params[i] = val;
    }


//    WasmEdge_Value* WasmRetuns = calloc(return_size, sizeof (WasmEdge_Value));
//    /* Function name. */
    const char *c_func_name = (*env)->GetStringUTFChars(env, func_name, NULL);
    const char *c_file_path = (*env)->GetStringUTFChars(env, file_path, NULL);

    /* The parameters and returns arrays. */
    //WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(5) };
    WasmEdge_Value *Returns = malloc(sizeof(WasmEdge_Value) * return_size);
    /* Function name. */
    WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
    /* Run the WASM function from file. */
    WasmEdge_Result Res = WasmEdge_VMRunWasmFromFile(VMCxt, c_file_path, FuncName, wasm_params, param_size, Returns, return_size);

    printf("Run wasm finished\n");
    if (WasmEdge_ResultOK(Res)) {
        for (int i = 0; i < return_size; ++i) {
            setJavaValueObject(env, Returns[i], (*env)->GetObjectArrayElement(env, returns, i));
        }
    } else {
        char exceptionBuffer[1024];
        sprintf(exceptionBuffer, "Error running wasm from file %s, error message: %s.", c_file_path,
                WasmEdge_ResultGetMessage(Res));

        (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Exception"),
                         exceptionBuffer);
    }

    /* Resources deallocations. */
    WasmEdge_StringDelete(FuncName);
    free(c_func_name);
    free(c_file_path);
    free(wasm_params);
    free(Returns);
    return;
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_nativeInit
        (JNIEnv * env, jobject thisObject, jobject jConfigureContext, jobject jStoreContext) {
    printf("Get configure context\n");
    WasmEdge_ConfigureContext* ConfigureContext = getConfigureContext(env, jConfigureContext);
    WasmEdge_StoreContext * StoreContext = getStoreContext(env, jStoreContext);


    WasmEdge_VMContext* VMContext = WasmEdge_VMCreate(ConfigureContext, StoreContext);

    setPointer(env, thisObject, (jlong)VMContext);

}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_loadWasmFromFile
        (JNIEnv * env, jobject thisObject, jstring filePath) {
    const char *c_file_path = (*env)->GetStringUTFChars(env, filePath, NULL);
    WasmEdge_Result res = WasmEdge_VMLoadWasmFromFile(getVmContext(env, thisObject), c_file_path);
    handleWasmEdgeResult(env, &res);
    free(c_file_path);
}


JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_validate
        (JNIEnv *env, jobject thisObject) {
    WasmEdge_Result result =  WasmEdge_VMValidate(getVmContext(env, thisObject));
    handleWasmEdgeResult(env, &result);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_instantiate
        (JNIEnv *env, jobject thisObject) {
    WasmEdge_Result result = WasmEdge_VMInstantiate(getVmContext(env, thisObject));
    handleWasmEdgeResult(env, &result);
}

JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_execute
        (JNIEnv *env, jobject thisObject, jstring funcName, jobjectArray params, jint paramSize,
         jintArray paramTypes, jobjectArray retuns, jint returnSize, jintArray returnTypes) {

    WasmEdge_VMContext *VMCxt = getVmContext(env, thisObject);
    printf("Params done");


    /* The parameters and returns arrays. */
    WasmEdge_Value *wasm_params = calloc(paramSize, sizeof(WasmEdge_Value));
    int *type = (*env)->GetIntArrayElements(env, paramTypes, JNI_FALSE);
    for (int i = 0; i < paramSize; i++) {
        WasmEdge_Value val;

        jobject val_object = (*env)->GetObjectArrayElement(env, params, i);

        switch (type[i]) {

            case 0:
                val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
                break;
            case 1:
                val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
                break;
            case 2:
                val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
                break;
            case 3:
                val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
                break;
            default:
                break;
        }
        wasm_params[i] = val;
    }


//    WasmEdge_Value* WasmRetuns = calloc(return_size, sizeof (WasmEdge_Value));
//    /* Function name. */
    const char *c_func_name = (*env)->GetStringUTFChars(env, funcName, NULL);

    /* The parameters and returns arrays. */
    //WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(5) };
    WasmEdge_Value *Returns = malloc(sizeof(WasmEdge_Value) * returnSize);
    /* Function name. */
    WasmEdge_String FuncName = WasmEdge_StringCreateByCString(c_func_name);
    /* Run the WASM function from file. */
    printf("Start to run wasm function: %s\n", c_func_name);
    WasmEdge_Result Res = WasmEdge_VMExecute(VMCxt,  FuncName, wasm_params, paramSize, Returns, returnSize);

    printf("Run wasm finished\n");

    handleWasmEdgeResult(env, &Res);
    if (WasmEdge_ResultOK(Res)) {
        for (int i = 0; i < returnSize; ++i) {
            setJavaValueObject(env, Returns[i], (*env)->GetObjectArrayElement(env, retuns, i));
        }
    }

    /* Resources deallocations. */
    WasmEdge_StringDelete(FuncName);
    free(c_func_name);
    free(wasm_params);
    free(Returns);
    return;
}


JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_delete
        (JNIEnv * env, jobject thisObj) {
    WasmEdge_VMDelete(getVmContext(env, thisObj));

}