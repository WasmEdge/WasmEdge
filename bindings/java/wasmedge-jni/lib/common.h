//
// Created by Kenvi Zhu on 2021-11-09.
//

#ifndef WASMEDGE_JAVA_COMMON_H
#define WASMEDGE_JAVA_COMMON_H

#include "jni.h"
#include "wasmedge/wasmedge.h"


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

long getPointer(JNIEnv* env, jobject obj);

void setPointer(JNIEnv* env, jobject obj, long val);

void handleWasmEdgeResult(JNIEnv* env, WasmEdge_Result * result);

int getIntVal(JNIEnv *env, jobject val);

long getLongVal(JNIEnv *env, jobject val);

long getFloatVal(JNIEnv *env, jobject val);

double getDoubleVal(JNIEnv *env, jobject val);

void setJavaIntValue(JNIEnv *env, WasmEdge_Value val, jobject jobj);

void setJavaLongValue(JNIEnv *env, WasmEdge_Value val, jobject jobj);

void setJavaFloatValue(JNIEnv *env, WasmEdge_Value val, jobject jobj);

void setJavaDoubleValue(JNIEnv *env, WasmEdge_Value val, jobject jobj);

enum WasmEdge_ValType *parseValueTypes(JNIEnv *env, jintArray jValueTypes) ;

#endif //WASMEDGE_JAVA_COMMON_H
