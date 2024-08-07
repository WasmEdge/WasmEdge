// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "FunctionTypeInstance.h"
#include "GlobalInstanceContext.h"
#include "MemoryInstanceContext.h"
#include "TableInstanceContext.h"
#include "common.h"
#include "jni.h"
#include "wasmedge/wasmedge.h"
#include <stdlib.h>

GETTER(StoreContext)

JNIEXPORT void JNICALL
Java_org_wasmedge_StoreContext_nativeInit(JNIEnv *env, jobject thisObj) {
  WasmEdge_StoreContext *StoreContext = WasmEdge_StoreCreate();
  setPointer(env, thisObj, (jlong)StoreContext);
}

jobject CreateJavaStoreContext(JNIEnv *env,
                               WasmEdge_StoreContext *storeContext) {
  jclass storeClass = findJavaClass(env, ORG_WASMEDGE_STORECONTEXT);

  jmethodID constructor =
      (*env)->GetMethodID(env, storeClass, DEFAULT_CONSTRUCTOR, LONG_VOID);

  jobject jStoreContext =
      (*env)->NewObject(env, storeClass, constructor, (long)storeContext);

  return jStoreContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_StoreContext_close(JNIEnv *env,
                                                            jobject thisObj) {
  WasmEdge_StoreDelete(getStoreContext(env, thisObj));
}

/*
 * Class:     org_wasmedge_StoreContext
 * Method:    listModule
 * Signature: ()Ljava/util/List;
 */
JNIEXPORT jobject JNICALL
Java_org_wasmedge_StoreContext_listModule(JNIEnv *env, jobject thisObject) {
  WasmEdge_StoreContext *storeCxt = getStoreContext(env, thisObject);

  uint32_t modLen = WasmEdge_StoreListModuleLength(storeCxt);
  WasmEdge_String *nameList =
      (WasmEdge_String *)malloc(sizeof(struct WasmEdge_String) * modLen);
  uint32_t RealModNum = WasmEdge_StoreListModule(storeCxt, nameList, modLen);

  jobject jNameList = WasmEdgeStringArrayToJavaList(env, nameList, RealModNum);

  free(nameList);

  return jNameList;
};
