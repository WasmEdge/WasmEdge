//
// Created by Kenvi Zhu on 2021-11-19.
//

#include "../jni/org_wasmedge_VMContext.h"
#include "ConfigureContext.h"
#include "StoreContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"

JNIEXPORT void JNICALL Java_org_wasmedge_VMContext_initNative(
    JNIEnv *env, jobject jVmContext, jobject jConfigureContext,
    jobject jStoreContext) {

  WasmEdge_ConfigureContext *ConfigureContext =
      getConfigureContext(env, jConfigureContext);
  WasmEdge_StoreContext *StoreContext = getStoreContext(env, jStoreContext);

  WasmEdge_VMContext *VMContext =
      WasmEdge_VMCreate(ConfigureContext, StoreContext);

  setPointer(env, jVmContext, (jlong)VMContext);
}