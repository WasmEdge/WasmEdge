// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "../jni//org_wasmedge_LoaderContext.h"
#include "AstModuleContext.h"
#include "ConfigureContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"

GETTER(LoaderContext)

JNIEXPORT jobject JNICALL Java_org_wasmedge_LoaderContext_parseFromFile(
    JNIEnv *env, jobject thisObject, jstring jInputPath) {
  WasmEdge_LoaderContext *loader = getLoaderContext(env, thisObject);

  const char *inputPath = (*env)->GetStringUTFChars(env, jInputPath, NULL);

  WasmEdge_ASTModuleContext *mod = NULL;

  WasmEdge_Result result =
      WasmEdge_LoaderParseFromFile(loader, &mod, inputPath);
  (*env)->ReleaseStringUTFChars(env, jInputPath, inputPath);
  handleWasmEdgeResult(env, &result);

  if ((*env)->ExceptionOccurred(env)) {
    return NULL;
  }

  return createAstModuleContext(env, mod);
}

JNIEXPORT jobject JNICALL Java_org_wasmedge_LoaderContext_parseFromBuffer(
    JNIEnv *env, jobject thisObject, jbyteArray jBuf, jint jSize) {
  WasmEdge_LoaderContext *loader = getLoaderContext(env, thisObject);

  WasmEdge_ASTModuleContext *mod = NULL;

  jbyte *data = (*env)->GetByteArrayElements(env, jBuf, 0);

  WasmEdge_LoaderParseFromBuffer(loader, &mod, (uint8_t *)data, jSize);

  (*env)->ReleaseByteArrayElements(env, jBuf, data, jSize);

  return createAstModuleContext(env, mod);
}

JNIEXPORT void JNICALL Java_org_wasmedge_LoaderContext_nativeInit(
    JNIEnv *env, jobject thisObject, jobject jConfigContext) {
  WasmEdge_ConfigureContext *configureContext =
      getConfigureContext(env, jConfigContext);
  WasmEdge_LoaderContext *loaderContext =
      WasmEdge_LoaderCreate(configureContext);
  setPointer(env, thisObject, (long)loaderContext);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_LoaderContext_close(JNIEnv *env, jobject thisObject) {

  WasmEdge_LoaderContext *loader = getLoaderContext(env, thisObject);
  WasmEdge_LoaderDelete(loader);
}
