//
// Created by Kenvi Zhu on 2021-11-09.
//
#include "common.h"
#include "jni.h"
#include <stdlib.h>
#include <string.h>
#include "wasmedge/wasmedge.h"

bool checkAndHandleException(JNIEnv *env, const char* msg);

void exitWithError(enum ErrorCode error, char* message) {
     exit(-1);
}

void throwNoClassDefError(JNIEnv *env, char * message) {
    jclass  exClass;
    char *className = "java/lang/NoClassDefFoundError";

    exClass = (*env)->FindClass(env, className);

    if(exClass == NULL) {
        exitWithError(JVM_ERROR, "Exception class not found.");
    }
    (*env)-> ThrowNew(env, exClass, message);

    exitWithError(JVM_ERROR, "Exception thrown for no class def");
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
    exitWithError(JVM_ERROR, "Exception thrown for no such method");
}


jclass findJavaClass(JNIEnv* env, char * className) {
    jclass class = (*env)->FindClass(env, className);

    bool hasException = checkAndHandleException(env, "find class error");
    if(hasException) {
        return NULL;
    }

    if(class == NULL) {
        throwNoClassDefError(env, className);
    }
    return class;
}

jmethodID findJavaMethod(JNIEnv* env, jclass class, char* methodName, char* sig) {
    jmethodID jmethodId = (*env)->GetMethodID(env, class, methodName, sig);
    return jmethodId;

}

void getClassName(JNIEnv* env, jobject obj, char* buff) {
    jclass cls = (*env)->GetObjectClass(env, obj);

// First get the class object
    jmethodID mid = (*env)->GetMethodID(env, cls, "getClass", "()Ljava/lang/Class;");
    jobject clsObj = (*env)->CallObjectMethod(env, obj, mid);
    checkAndHandleException(env, "get class name error");

// Now get the class object's class descriptor
    cls = (*env)->GetObjectClass(env, clsObj);

// Find the getName() method on the class object
    mid = (*env)->GetMethodID(env, cls, "getName", "()Ljava/lang/String;");

// Call the getName() to get a jstring object back
    jstring strObj = (jstring)(*env)->CallObjectMethod(env, clsObj, mid);
    checkAndHandleException(env, "get name error");

// Now get the c string from the java jstring object
    const char* str = (*env)->GetStringUTFChars(env, strObj, NULL);

// Print the class name
    strcpy(buff, str);

// Release the memory pinned char array
    (*env)->ReleaseStringUTFChars(env, strObj, str);
}


long getPointer(JNIEnv* env, jobject obj) {
    jclass cls = (*env)->GetObjectClass(env, obj);

    if (cls == NULL) {
        exitWithError(JVM_ERROR, "class not found!");
    }

    jfieldID fidPointer = (*env)->GetFieldID(env, cls, "pointer", "J");
    if(fidPointer == NULL) {
        exitWithError(JVM_ERROR, "pointer filed not found!");
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
    checkAndHandleException(env, "Error get int value");
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

bool checkAndHandleException(JNIEnv *env, const char* msg) {
    if((*env)->ExceptionCheck(env)) {
        jthrowable e = (*env)->ExceptionOccurred(env);
        (*env)->ExceptionClear(env);

        jclass eclass = (*env)->GetObjectClass(env, e);

        jmethodID mid = (*env)->GetMethodID(env, eclass, "toString", "()Ljava/lang/String;");
        jstring jErrorMsg = (*env)->CallObjectMethod(env, e, mid);
        const char* cMsg = (*env)->GetStringUTFChars(env, jErrorMsg, NULL);


        (*env)->ReleaseStringUTFChars(env, jErrorMsg, cMsg);
        jclass newExcCls = (*env)->FindClass(env, "java/lang/RuntimeException");
        if (newExcCls == 0) { /* Unable to find the new exception class, give up. */
            return true;
        }
        (*env)->ThrowNew(env, newExcCls, msg);
    }
}