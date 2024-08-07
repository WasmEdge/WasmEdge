// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
