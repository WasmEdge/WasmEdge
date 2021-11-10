//
// Created by Kenvi Zhu on 2021-11-09.
//
#include "common.h"
#include "jni.h"
#include <stdlib.h>
#include <string.h>


void exitWithError(enum ErrorCode error, char* message, char* fileName, int line);

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
    printf("\nCalling class is: %s\n", str);

// Release the memory pinned char array
    (*env)->ReleaseStringUTFChars(env, strObj, str);
}

void exitWithError(enum ErrorCode error, char* message, char* fileName, int line) {
    printf("Error with message: %s at %s, %d", message, fileName, line);
    exit(-1);
}