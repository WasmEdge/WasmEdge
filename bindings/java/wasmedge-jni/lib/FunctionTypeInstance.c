//
// Created by Kenvi Zhu on 2022-03-14.
//
#include "FunctionTypeInstance.h"
#include "FunctionTypeContext.h"
#include "MemoryInstanceContext.h"
#include "ValueType.h"
#include "common.h"
#include "jni.h"
#include "wasmedge/wasmedge.h"
#include <stdlib.h>

WasmEdge_Result HostFuncWrap(void *This, void *Data,
                             const WasmEdge_CallingFrameContext *Mem,
                             const WasmEdge_Value *In, const unsigned int InLen,
                             WasmEdge_Value *Out, const unsigned int OutLen) {

  HostFuncParam *param = (HostFuncParam *)This;
  JNIEnv *env = param->env;
  const char *funcKey = param->jFuncKey;

  jstring jFuncKey = (*env)->NewStringUTF(env, funcKey);

  jclass clazz = (*env)->FindClass(env, "org/wasmedge/WasmEdgeVM");
  jmethodID funcGetter = (*env)->GetStaticMethodID(
      env, clazz, "getHostFunc",
      "(Ljava/lang/String;)Lorg/wasmedge/HostFunction;");

  jobject jFunc =
      (*env)->CallStaticObjectMethod(env, clazz, funcGetter, jFuncKey);

  jclass jFuncClass = (*env)->GetObjectClass(env, jFunc);

  jmethodID funcMethod =
      (*env)->GetMethodID(env, jFuncClass, "apply",
                          "(Lorg/wasmedge/MemoryInstanceContext;Ljava/util/"
                          "List;Ljava/util/List;)Lorg/wasmedge/Result;");

  // TODO replace with CallingFrameContext
  jobject jMem =
      createJMemoryInstanceContext(env, (WasmEdge_MemoryInstanceContext *)Mem);

  jobject jParams = CreateJavaArrayList(env, InLen);

  for (int i = 0; i < InLen; ++i) {
    AddElementToJavaList(env, jParams, WasmEdgeValueToJavaValue(env, In[i]));
  }

  jobject jReturns = CreateJavaArrayList(env, OutLen);

  (*env)->CallObjectMethod(env, jFunc, funcMethod, jMem, jParams, jReturns);

  for (int i = 0; i < OutLen; ++i) {
    Out[i] = JavaValueToWasmEdgeValue(env, GetListElement(env, jReturns, i));
  }

  return WasmEdge_Result_Success;
}

WasmEdge_FunctionInstanceContext *
getFunctionInstanceContext(JNIEnv *env, jobject jFuncInstance) {

  if (jFuncInstance == NULL) {
    return NULL;
  }
  WasmEdge_FunctionInstanceContext *funcInstance =
      (struct WasmEdge_FunctionInstanceContext *)getPointer(env, jFuncInstance);

  return funcInstance;
}

JNIEXPORT jobject JNICALL
Java_org_wasmedge_FunctionInstanceContext_getFunctionType(JNIEnv *env,
                                                          jobject thisObject) {
  WasmEdge_FunctionInstanceContext *funcInstance =
      getFunctionInstanceContext(env, thisObject);
  const WasmEdge_FunctionTypeContext *funcType =
      WasmEdge_FunctionInstanceGetFunctionType(funcInstance);
  return createJFunctionTypeContext(env, funcType);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_FunctionInstanceContext_nativeCreateFunction(
    JNIEnv *env, jobject thisObject, jobject jFuncType, jstring jHostFuncKey,
    jobject jData, jlong jCost) {
  WasmEdge_FunctionTypeContext *funcCxt =
      getFunctionTypeContext(env, jFuncType);
  HostFuncParam *params = malloc(sizeof(struct HostFuncParam));

  const char *funcKey = (*env)->GetStringUTFChars(env, jHostFuncKey, NULL);
  params->jFuncKey = funcKey;
  params->env = env;
  // WasmEdge_FunctionInstanceContext *funcInstance =
  // WasmEdge_FunctionInstanceCreate(funcCxt, HostFunc, params, jCost);

  WasmEdge_FunctionInstanceContext *funcInstance =
      WasmEdge_FunctionInstanceCreateBinding(funcCxt, HostFuncWrap, params,
                                             NULL, jCost);

  setPointer(env, thisObject, (long)funcInstance);
}

jobject createJFunctionInstanceContext(
    JNIEnv *env, const WasmEdge_FunctionInstanceContext *funcInstance) {

  // FIXME add to all instances.
  if (funcInstance == NULL) {
    return NULL;
  }

  jclass clazz = (*env)->FindClass(env, "org/wasmedge/FunctionInstanceContext");
  jmethodID constructorId = (*env)->GetMethodID(env, clazz, "<init>", "(J)V");
  return (*env)->NewObject(env, clazz, constructorId, (long)funcInstance);
}

uint32_t
GetReturnLen(WasmEdge_FunctionInstanceContext *functionInstanceContext) {
  const WasmEdge_FunctionTypeContext *type =
      WasmEdge_FunctionInstanceGetFunctionType(functionInstanceContext);
  return WasmEdge_FunctionTypeGetReturnsLength(type);
}