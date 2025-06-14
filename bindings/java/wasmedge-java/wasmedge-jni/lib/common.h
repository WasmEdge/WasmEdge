// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_COMMON_H
#define WASMEDGE_JAVA_COMMON_H

#include "constants.h"
#include "jni.h"
#include "wasmedge/wasmedge.h"

#define MAX_BUF_LEN 1024

enum ErrorCode { JVM_ERROR, WVM_ERROR };

void exitWithError(enum ErrorCode error, char *message);

void throwNoClassDefError(JNIEnv *env, char *message);

void throwNoSuchMethodError(JNIEnv *env, char *methodName, char *sig);

jclass findJavaClass(JNIEnv *env, char *className);

jmethodID findJavaMethod(JNIEnv *env, jclass class, char *methodName,
                         char *sig);

void getClassName(JNIEnv *env, jobject obj, char *buff);

long getPointer(JNIEnv *env, jobject obj);

void setPointer(JNIEnv *env, jobject obj, long val);

void handleWasmEdgeResult(JNIEnv *env, WasmEdge_Result *result);

int getIntVal(JNIEnv *env, jobject val);

long getLongVal(JNIEnv *env, jobject val);

long getFloatVal(JNIEnv *env, jobject val);

double getDoubleVal(JNIEnv *env, jobject val);

char *getStringVal(JNIEnv *env, jobject val);

void setJavaIntValue(JNIEnv *env, WasmEdge_Value val, jobject jobj);

void setJavaLongValue(JNIEnv *env, WasmEdge_Value val, jobject jobj);

void setJavaFloatValue(JNIEnv *env, WasmEdge_Value val, jobject jobj);

void setJavaDoubleValue(JNIEnv *env, WasmEdge_Value val, jobject jobj);

void setJavaStringValue(JNIEnv *env, char *val, jobject jobj);

enum WasmEdge_ValType *parseValueTypes(JNIEnv *env, jintArray jValueTypes);

bool checkAndHandleException(JNIEnv *env, const char *msg);

void setJavaValueObject(JNIEnv *env, WasmEdge_Value value, jobject j_val);

jobject WasmEdgeStringArrayToJavaList(JNIEnv *env, WasmEdge_String *wStrList,
                                      int32_t len);

jstring WasmEdgeStringToJString(JNIEnv *env, WasmEdge_String wStr);

jobject CreateJavaArrayList(JNIEnv *env, jint len);

bool AddElementToJavaList(JNIEnv *env, jobject jList, jobject ele);

WasmEdge_String JStringToWasmString(JNIEnv *env, jstring jstr);

const char **JStringArrayToPtr(JNIEnv *env, jarray jStrArray);

void ReleaseCString(JNIEnv *env, jarray jStrArray, const char **ptr);

jobject GetListElement(JNIEnv *env, jobject jList, jint idx);

jint GetListSize(JNIEnv *env, jobject jList);

#define GETTER(NAME)                                                           \
  WasmEdge_##NAME *get##NAME(JNIEnv *env, jobject j##NAME) {                   \
    if (j##NAME == NULL) {                                                     \
      return NULL;                                                             \
    }                                                                          \
    return (WasmEdge_##NAME *)getPointer(env, j##NAME);                        \
  }

#endif // WASMEDGE_JAVA_COMMON_H
