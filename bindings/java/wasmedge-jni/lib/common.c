//
// Created by Kenvi Zhu on 2021-11-09.
//
#include "common.h"
#include "jni.h"
#include <stdlib.h>
#include <string.h>
#include "wasmedge/wasmedge.h"



void exitWithError(enum ErrorCode error, char* message, char* file, int line) {
     printf("Error with message: %s in file:%s at line %d.\n", message, file, line);
     exit(-1);
}

void throwNoClassDefError(JNIEnv *env, char * message) {
    jclass  exClass;
    char *className = "java/lang/NoClassDefFoundError";

    exClass = (*env)->FindClass(env, className);

    if(exClass == NULL) {
        exitWithError(JVM_ERROR, "Exception class not found.", __FILE_NAME__, __LINE__);
    }
    (*env)-> ThrowNew(env, exClass, message);

    exitWithError(JVM_ERROR, "Exception thrown for no class def", __FILE_NAME__, __LINE__);
}

void throwNoSuchMethodError(JNIEnv *env, char* methodName, char* sig) {
    jclass exClass;
    char *className = "java/lang/NoSuchMethodError";

    char message[1000];

    strcat(message, methodName);
    strcat(message, sig);

    if(exClass == NULL) {
        throwNoClassDefError(env, message);
    }

    (*env)->ThrowNew(env, exClass, methodName);
    exitWithError(JVM_ERROR, "Exception thrown for no such method", __FILE_NAME__, __LINE__);
}


jclass findJavaClass(JNIEnv* env, char * className) {
    jclass  class = (*env)->FindClass(env, className);
    if(class == NULL) {
        throwNoClassDefError(env, className);
    }
    return class;
}

jmethodID findJavaMethod(JNIEnv* env, jclass class, char* methodName, char* sig) {
    jmethodID jmethodId = (*env)->GetMethodID(env, class, methodName, sig);
    if(jmethodId == NULL) {
        throwNoSuchMethodError(env, methodName, sig);
    }
    return jmethodId;

}

void getClassName(JNIEnv* env, jobject obj, char* buff) {
    jclass cls = (*env)->GetObjectClass(env, obj);

// First get the class object
    jmethodID mid = (*env)->GetMethodID(env, cls, "getClass", "()Ljava/lang/Class;");
    jobject clsObj = (*env)->CallObjectMethod(env, obj, mid);

// Now get the class object's class descriptor
    cls = (*env)->GetObjectClass(env, clsObj);

// Find the getName() method on the class object
    mid = (*env)->GetMethodID(env, cls, "getName", "()Ljava/lang/String;");

// Call the getName() to get a jstring object back
    jstring strObj = (jstring)(*env)->CallObjectMethod(env, clsObj, mid);

// Now get the c string from the java jstring object
    const char* str = (*env)->GetStringUTFChars(env, strObj, NULL);

// Print the class name
    strcpy(buff, str);

// Release the memory pinned char array
    (*env)->ReleaseStringUTFChars(env, strObj, str);
}


long getPointer(JNIEnv* env, jobject obj) {
    printf("Start to get class");
    jclass cls = (*env)->GetObjectClass(env, obj);

    printf("Get class succeed.");
    if (cls == NULL) {
        exitWithError(JVM_ERROR, "class not found!", __FILE__, __LINE__);
    }

    printf("Start to get pointer field");
    jfieldID fidPointer = (*env)->GetFieldID(env, cls, "pointer", "J");
    if(fidPointer == NULL) {
        exitWithError(JVM_ERROR, "pointer filed not found!", __FILE__, __LINE__);
    }
    jlong value = (*env)->GetLongField(env, obj, fidPointer);
    char buf[216];
    getClassName(env, obj, buf);
    return value;
}

void setPointer(JNIEnv* env, jobject obj, jlong val) {
    jclass cls = (*env)->GetObjectClass(env, obj);
    jfieldID fidPointer = (*env)->GetFieldID(env, cls, "pointer", "J");
    char buf[216];
    getClassName(env, obj, buf);
    (*env)->SetLongField(env, obj, fidPointer, val);
}

void handleWasmEdgeResult(JNIEnv* env, WasmEdge_Result * result) {
    if(!WasmEdge_ResultOK(*result)) {
        char exceptionBuffer[1024];
        sprintf(exceptionBuffer, "Error occurred with message: %s.",
                WasmEdge_ResultGetMessage(*result));

        (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Exception"),
                         exceptionBuffer);
    }
}

int getIntVal(JNIEnv *env, jobject val) {
    jclass clazz = (*env)->GetObjectClass(env, val);
    jmethodID methodId = findJavaMethod(env, clazz, "getValue", "()I");

    jint value = (*env)->CallIntMethod(env, val, methodId);
    return value;
}

long getLongVal(JNIEnv *env, jobject val) {
    jclass clazz = (*env)->GetObjectClass(env, val);
    jmethodID methodId = findJavaMethod(env, clazz, "getValue", "()L");
    jlong value = (*env)->CallLongMethod(env, val, methodId);
    return value;
}

long getFloatVal(JNIEnv *env, jobject val) {
    jclass clazz = (*env)->GetObjectClass(env, val);
    jmethodID methodId = findJavaMethod(env, clazz, "getValue", "()F");
    jfloat value = (*env)->CallFloatMethod(env, val, methodId);
    return value;
}

double getDoubleVal(JNIEnv *env, jobject val) {
    jclass clazz = (*env)->GetObjectClass(env, val);
    jmethodID methodId = findJavaMethod(env, clazz, "getValue", "()D");
    jdouble value = (*env)->CallDoubleMethod(env, val, methodId);
    return value;
}


WasmEdge_Value *parseJavaParams(JNIEnv *env, jobjectArray params, jintArray paramTypes, jint paramSize) {

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
}

enum WasmEdge_ValType *parseValueTypes(JNIEnv *env, jintArray jValueTypes) {
    jint len = (*env)->GetArrayLength(env, jValueTypes);
    enum WasmEdge_ValType* valTypes = malloc(len * sizeof(enum  WasmEdge_ValType));
    jint* elements = (*env)->GetIntArrayElements(env, jValueTypes, false);
    for (int i = 0; i < len; ++i) {
        valTypes[i] = elements[i];
    }
    return valTypes;
}