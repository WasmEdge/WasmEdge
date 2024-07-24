// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "../jni/org_wasmedge_FunctionTypeContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"
#include <stdlib.h>

jobject ConvertToJavaFunctionType(
    JNIEnv *env, const WasmEdge_FunctionTypeContext *functionTypeContext,
    const WasmEdge_String name);

GETTER(FunctionTypeContext)

JNIEXPORT void JNICALL Java_org_wasmedge_FunctionTypeContext_nativeInit(
    JNIEnv *env, jobject thisObject, jintArray paramTypes,
    jintArray returnTypes) {
  int paramLen =
      paramTypes == NULL ? 0 : (*env)->GetArrayLength(env, paramTypes);
  int returnLen =
      returnTypes == NULL ? 0 : (*env)->GetArrayLength(env, returnTypes);
  WasmEdge_ValType *paramList = parseValueTypes(env, paramTypes);
  WasmEdge_ValType *returnList = parseValueTypes(env, returnTypes);

  WasmEdge_FunctionTypeContext *functionTypeContext =
      WasmEdge_FunctionTypeCreate(paramList, paramLen, returnList, returnLen);
  setPointer(env, thisObject, (jlong)functionTypeContext);
}

JNIEXPORT jintArray JNICALL
Java_org_wasmedge_FunctionTypeContext_nativeGetParameters(JNIEnv *env,
                                                          jobject thisObject) {
  WasmEdge_FunctionTypeContext *functionTypeContext =
      getFunctionTypeContext(env, thisObject);
  uint32_t paramLen =
      WasmEdge_FunctionTypeGetParametersLength(functionTypeContext);
  WasmEdge_ValType *params =
      malloc(sizeof(WasmEdge_ValType) * paramLen);
  WasmEdge_FunctionTypeGetParameters(functionTypeContext, params, paramLen);

  jintArray types = (*env)->NewIntArray(env, paramLen);
  (*env)->SetIntArrayRegion(env, types, 0, paramLen, (jint *)params);
  free(params);
  return types;
}

JNIEXPORT jintArray JNICALL
Java_org_wasmedge_FunctionTypeContext_nativeGetReturns(JNIEnv *env,
                                                       jobject thisObject) {

  WasmEdge_FunctionTypeContext *functionTypeContext =
      getFunctionTypeContext(env, thisObject);
  uint32_t returnLen =
      WasmEdge_FunctionTypeGetReturnsLength(functionTypeContext);
  WasmEdge_ValType *returns =
      malloc(sizeof(WasmEdge_ValType) * returnLen);
  WasmEdge_FunctionTypeGetReturns(functionTypeContext, returns, returnLen);

  jintArray types = (*env)->NewIntArray(env, returnLen);
  (*env)->SetIntArrayRegion(env, types, 0, returnLen, (jint *)returns);
  free(returns);
  return types;
}

JNIEXPORT void JNICALL
Java_org_wasmedge_FunctionTypeContext_close(JNIEnv *env, jobject thisObject) {
  WasmEdge_FunctionTypeContext *functionTypeContext =
      getFunctionTypeContext(env, thisObject);
  WasmEdge_FunctionTypeDelete(functionTypeContext);
}

jobject ConvertToJavaValueType(JNIEnv *env, WasmEdge_ValType *valType) {
    if (valType == NULL) {
        return NULL;
    }

    // Find the Java class for value type
    jclass valueTypeClass = findJavaClass(env, ORG_WASMEDGE_ENUMS_VALUETYPE);
    if (valueTypeClass == NULL) {
        return NULL;
    }

    // Get the static method ID for the method that converts C type to Java enum
    jmethodID jmethodId = (*env)->GetStaticMethodID(
        env, valueTypeClass, PARSE_TYPE, INT_VALUETYPE);

    if (jmethodId == NULL) {
        return NULL;
    }

    // Extract type code from WasmEdge_ValType
    // TODO
    uint32_t typeCode;
    typeCode= valType->Data[2];
    if(valType->Data[1])
      typeCode = valType->Data[3];
    // Call the static method in Java to get the Java representation of the value type
    jobject valueType = (*env)->CallStaticObjectMethod(env, valueTypeClass, jmethodId, (jint)typeCode);

    // Check for exceptions
    if (checkAndHandleException(env, "Error when creating value type")) {
        return NULL;
    }

    return valueType;
}

jobject ConvertToValueTypeList(JNIEnv *env, WasmEdge_ValType *list,
                               int32_t len) {
  jclass listClass = findJavaClass(env, JAVA_UTIL_ARRAYLIST);

  if (listClass == NULL) {
    return NULL;
  }

  jmethodID listConstructor = findJavaMethod(env, listClass, DEFAULT_CONSTRUCTOR, INT_VOID);

  if (listConstructor == NULL) {
    return NULL;
  }

  jobject jList = (*env)->NewObject(env, listClass, listConstructor, len);

  if (jList == NULL) {
    return NULL;
  }

  if (checkAndHandleException(env, ERR_CREATE_VALUE_TYPE_LIST_FAILED)) {
    return NULL;
  }

  char buf[256];
  getClassName(env, jList, buf);

  jmethodID addMethod =
      findJavaMethod(env, listClass, ADD_ELEMENT, OBJECT_BOOL);

  if (addMethod == NULL) {
    return NULL;
  }

  WasmEdge_ValType *ptr = list;
  for (int i = 0; i < len; ++i) {
    jobject valueType = ConvertToJavaValueType(env, ptr);

    (*env)->CallBooleanMethod(env, jList, addMethod, valueType);

    if (checkAndHandleException(env, ERR_ADD_VALUE_TYPE)) {
      return NULL;
    }

    ptr++;
  }

  return jList;
}

void ConvertToJavaFunctionList(JNIEnv *env, WasmEdge_String *nameList,
                               const WasmEdge_FunctionTypeContext **funcList,
                               int32_t len, jobject jFuncList) {

  jclass funcListClass = (*env)->GetObjectClass(env, jFuncList);

  jmethodID addMethod =
      findJavaMethod(env, funcListClass, ADD_ELEMENT, OBJECT_BOOL);

  if (addMethod == NULL) {
    return;
  }

  for (int i = 0; i < len; ++i) {
    jobject jFunc = ConvertToJavaFunctionType(env, funcList[i], nameList[i]);

    char buf[256];

    getClassName(env, jFuncList, buf);

    getClassName(env, jFunc, buf);

    (*env)->CallBooleanMethod(env, jFuncList, addMethod, jFunc);

    if (checkAndHandleException(env, buf)) {
      return;
    }
  }
}

jobject ConvertToJavaFunctionType(
    JNIEnv *env, const WasmEdge_FunctionTypeContext *functionTypeContext,
    const WasmEdge_String name) {
  int retLen = WasmEdge_FunctionTypeGetReturnsLength(functionTypeContext);
  WasmEdge_ValType *list =
      (WasmEdge_ValType *)malloc(sizeof(WasmEdge_ValType) * retLen);

  int actualLen =
      WasmEdge_FunctionTypeGetReturns(functionTypeContext, list, retLen);

  jobject jReturnList = ConvertToValueTypeList(env, list, actualLen);

  if (jReturnList == NULL) {
    return NULL;
  }

  free(list);

  int paramLen = WasmEdge_FunctionTypeGetParametersLength(functionTypeContext);
  WasmEdge_ValType *paramList =
      (WasmEdge_ValType *)malloc(sizeof(WasmEdge_ValType) * paramLen);

  int actualParamLen = WasmEdge_FunctionTypeGetParameters(functionTypeContext,
                                                          paramList, paramLen);

  jobject jParamList = ConvertToValueTypeList(env, paramList, actualParamLen);

  if (jParamList == NULL) {
    return NULL;
  }

  free(paramList);

  jclass functionTypeClass =
      findJavaClass(env, ORG_WASMEDGE_FUNCTIONTYPECONTEXT);
  if (functionTypeClass == NULL) {
    return NULL;
  }

  jmethodID constructor = findJavaMethod(env, functionTypeClass, DEFAULT_CONSTRUCTOR,
                                         LISTLIST_VOID);

  jobject jFunc = (*env)->NewObject(env, functionTypeClass, constructor,
                                    jParamList, jReturnList);

  if (checkAndHandleException(env,
                              ERROR_CREATE_FUNCTION_TYPE_FAILED)) {
    return NULL;
  }

  jmethodID nameSetter = (*env)->GetMethodID(env, functionTypeClass, SET_NAME,
                                             STRING_VOID);

  uint32_t len = 256;
  char BUF[len];
  WasmEdge_StringCopy(name, BUF, len);
  jstring jstr = (*env)->NewStringUTF(env, BUF);

  (*env)->CallVoidMethod(env, jFunc, nameSetter, jstr);

  if (checkAndHandleException(
          env, ERR_SET_FUNCTION_TYPE_FAILED)) {
    return NULL;
  }

  return jFunc;
}

jobject createJFunctionTypeContext(
    JNIEnv *env, const WasmEdge_FunctionTypeContext *functionTypeContext) {

  jclass clazz = (*env)->FindClass(env, ORG_WASMEDGE_FUNCTIONTYPECONTEXT);
  jmethodID constructorId = (*env)->GetMethodID(env, clazz, DEFAULT_CONSTRUCTOR, LONG_VOID);
  return (*env)->NewObject(env, clazz, constructorId,
                           (long)functionTypeContext);
}
