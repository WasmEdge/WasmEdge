// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "../jni/org_wasmedge_CompilerContext.h"
#include "ConfigureContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"

GETTER(CompilerContext)

JNIEXPORT void JNICALL Java_org_wasmedge_CompilerContext_nativeInit(
    JNIEnv *env, jobject thisObject, jobject jConfigContext) {

  WasmEdge_ConfigureContext *configureContext =
      getConfigureContext(env, jConfigContext);

  WasmEdge_CompilerContext *compilerContext =
      WasmEdge_CompilerCreate(configureContext);

  setPointer(env, thisObject, (long)compilerContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_CompilerContext_compile(
    JNIEnv *env, jobject thisObject, jstring jInputPath, jstring jOutputPath) {
  WasmEdge_CompilerContext *compilerContext =
      getCompilerContext(env, thisObject);

  const char *inputPath = (*env)->GetStringUTFChars(env, jInputPath, NULL);
  const char *outputPath = (*env)->GetStringUTFChars(env, jOutputPath, NULL);

  WasmEdge_Result result =
      WasmEdge_CompilerCompile(compilerContext, inputPath, outputPath);

  (*env)->ReleaseStringUTFChars(env, jInputPath, inputPath);
  (*env)->ReleaseStringUTFChars(env, jOutputPath, outputPath);

  handleWasmEdgeResult(env, &result);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_CompilerContext_close(JNIEnv *env, jobject thisObject) {
  WasmEdge_CompilerContext *compilerContext =
      getCompilerContext(env, thisObject);
  WasmEdge_CompilerDelete(compilerContext);
}
