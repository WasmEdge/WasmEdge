//
// Created by Kenvi Zhu on 2021-12-04.
//
#include "../jni/org_wasmedge_FunctionTypeContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"
#include <stdlib.h>

jobject ConvertToJavaFunctionType(
    JNIEnv *env, const WasmEdge_FunctionTypeContext *functionTypeContext,
    const WasmEdge_String name);

WasmEdge_FunctionTypeContext *
getFunctionTypeContext(JNIEnv *env, jobject jFunctionTypeContext) {
  if (jFunctionTypeContext == NULL) {
    return NULL;
  }
  return (WasmEdge_FunctionTypeContext *)getPointer(env, jFunctionTypeContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_FunctionTypeContext_nativeInit(
    JNIEnv *env, jobject thisObject, jintArray paramTypes,
    jintArray returnTypes) {
  int paramLen =
      paramTypes == NULL ? 0 : (*env)->GetArrayLength(env, paramTypes);
  int returnLen =
      returnTypes == NULL ? 0 : (*env)->GetArrayLength(env, returnTypes);
  enum WasmEdge_ValType *paramList = parseValueTypes(env, paramTypes);
  enum WasmEdge_ValType *returnList = parseValueTypes(env, returnTypes);

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
  enum WasmEdge_ValType *params =
      malloc(sizeof(enum WasmEdge_ValType) * paramLen);
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
  enum WasmEdge_ValType *returns =
      malloc(sizeof(enum WasmEdge_ValType) * returnLen);
  WasmEdge_FunctionTypeGetReturns(functionTypeContext, returns, returnLen);

  jintArray types = (*env)->NewIntArray(env, returnLen);
  (*env)->SetIntArrayRegion(env, types, 0, returnLen, (jint *)returns);
  free(returns);
  return types;
}

/*
 * Class:     org_wasmedge_FunctionTypeContext
 * Method:    delete
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_wasmedge_FunctionTypeContext_delete(JNIEnv *env, jobject thisObject) {
  WasmEdge_FunctionTypeContext *functionTypeContext =
      getFunctionTypeContext(env, thisObject);
  WasmEdge_FunctionTypeDelete(functionTypeContext);
}

jobject ConvertToJavaValueType(JNIEnv *env, enum WasmEdge_ValType *valType) {

  jclass valueTypeCalss = findJavaClass(env, "org/wasmedge/enums/ValueType");
  if (valueTypeCalss == NULL) {
    return NULL;
  }

  jmethodID jmethodId = (*env)->GetStaticMethodID(
      env, valueTypeCalss, "parseType", "(I)Lorg/wasmedge/enums/ValueType;");

  if (jmethodId == NULL) {
    return NULL;
  }

  jobject valueType = (*env)->CallStaticObjectMethod(env, valueTypeCalss,
                                                     jmethodId, (jint)*valType);

  if (checkAndHandleException(env, "Error when creating value type")) {
    return NULL;
  }

  return valueType;
}

jobject ConvertToValueTypeList(JNIEnv *env, enum WasmEdge_ValType *list,
                               int32_t len) {
  jclass listClass = findJavaClass(env, "java/util/ArrayList");

  if (listClass == NULL) {
    return NULL;
  }

  jmethodID listConstructor = findJavaMethod(env, listClass, "<init>", "(I)V");

  if (listConstructor == NULL) {
    return NULL;
  }

  jobject jList = (*env)->NewObject(env, listClass, listConstructor, len);

  if (jList == NULL) {
    return NULL;
  }

  if (checkAndHandleException(env, "Error when creating value type list\n")) {
    return NULL;
  }

  char buf[256];
  getClassName(env, jList, buf);

  jmethodID addMethod =
      findJavaMethod(env, listClass, "add", "(Ljava/lang/Object;)Z");

  if (addMethod == NULL) {
    return NULL;
  }

  enum WasmEdge_ValType *ptr = list;
  for (int i = 0; i < len; ++i) {
    jobject valueType = ConvertToJavaValueType(env, ptr);

    (*env)->CallBooleanMethod(env, jList, addMethod, valueType);

    if (checkAndHandleException(env, "Error when adding value type\n")) {
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
      findJavaMethod(env, funcListClass, "add", "(Ljava/lang/Object;)Z");

  if (addMethod == NULL) {
    return;
  }

  for (int i = 0; i < len; ++i) {
    jobject jFunc = ConvertToJavaFunctionType(env, funcList[i], nameList[i]);

    char buf[256];

    getClassName(env, jFuncList, buf);

    getClassName(env, jFunc, buf);

    (*env)->CallBooleanMethod(env, jFuncList, addMethod, jFunc);

    sprintf(buf, "Error when converting to java function list with index: %d",
            i);

    if (checkAndHandleException(env, buf)) {
      return;
    }
  }
}

jobject ConvertToJavaFunctionType(
    JNIEnv *env, const WasmEdge_FunctionTypeContext *functionTypeContext,
    const WasmEdge_String name) {
  int retLen = WasmEdge_FunctionTypeGetReturnsLength(functionTypeContext);
  enum WasmEdge_ValType *list =
      (enum WasmEdge_ValType *)malloc(sizeof(enum WasmEdge_ValType) * retLen);

  int actualLen =
      WasmEdge_FunctionTypeGetReturns(functionTypeContext, list, retLen);

  jobject jReturnList = ConvertToValueTypeList(env, list, actualLen);

  if (jReturnList == NULL) {
    return NULL;
  }

  free(list);

  int paramLen = WasmEdge_FunctionTypeGetParametersLength(functionTypeContext);
  enum WasmEdge_ValType *paramList =
      (enum WasmEdge_ValType *)malloc(sizeof(enum WasmEdge_ValType) * paramLen);

  int actualParamLen = WasmEdge_FunctionTypeGetParameters(functionTypeContext,
                                                          paramList, paramLen);

  jobject jParamList = ConvertToValueTypeList(env, paramList, actualParamLen);

  if (jParamList == NULL) {
    return NULL;
  }

  free(paramList);

  jclass functionTypeClass =
      findJavaClass(env, "org/wasmedge/FunctionTypeContext");
  if (functionTypeClass == NULL) {
    return NULL;
  }

  jmethodID constructor = findJavaMethod(env, functionTypeClass, "<init>",
                                         "(Ljava/util/List;Ljava/util/List;)V");

  jobject jFunc = (*env)->NewObject(env, functionTypeClass, constructor,
                                    jParamList, jReturnList);

  if (checkAndHandleException(env,
                              "Error when creating function type context.\n")) {
    return NULL;
  }

  jmethodID nameSetter = (*env)->GetMethodID(env, functionTypeClass, "setName",
                                             "(Ljava/lang/String;)V");

  uint32_t len = 256;
  char BUF[len];
  WasmEdge_StringCopy(name, BUF, len);
  jstring jstr = (*env)->NewStringUTF(env, BUF);

  (*env)->CallVoidMethod(env, jFunc, nameSetter, jstr);

  if (checkAndHandleException(
          env, "Error when setting function type context name.\n")) {
    return NULL;
  }

  return jFunc;
}

jobject createJFunctionTypeContext(
    JNIEnv *env, const WasmEdge_FunctionTypeContext *functionTypeContext) {

  jclass clazz = (*env)->FindClass(env, "org/wasmedge/FunctionTypeContext");
  jmethodID constructorId = (*env)->GetMethodID(env, clazz, "<init>", "(J)V");
  return (*env)->NewObject(env, clazz, constructorId,
                           (long)functionTypeContext);
}
