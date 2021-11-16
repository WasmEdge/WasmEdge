//
// Created by Kenvi Zhu on 2021-11-09.
//

#ifndef WASMEDGE_JAVA_COMMON_H
#define WASMEDGE_JAVA_COMMON_H

#include "jni.h"


enum ErrorCode {
    JVM_ERROR,
    WVM_ERROR
};

void exitWithError(enum ErrorCode error, char* message, char* fileName, int line);

void throwNoClassDefError(JNIEnv *env, char * message);

void throwNoSuchMethodError(JNIEnv *env, char* methodName, char* sig);

jclass findJavaClass(JNIEnv* env, char * className);

jmethodID findJavaMethod(JNIEnv* env, jclass class, char* methodName, char* sig);

void getClassName(JNIEnv* env, jobject obj, char* buff);

#endif //WASMEDGE_JAVA_COMMON_H
