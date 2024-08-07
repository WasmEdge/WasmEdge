// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasmedge/wasmedge.h"
#include "jni.h"

JNIEXPORT jstring JNICALL
Java_org_wasmedge_WasmEdge_getVersion(JNIEnv *env, jobject thisObject) {

  const char *Version = WasmEdge_VersionGet();
  jstring result = (*env)->NewStringUTF(env, Version);
  return result;
}

JNIEXPORT jlong JNICALL
Java_org_wasmedge_WasmEdge_getMajorVersion(JNIEnv *env, jobject thisObject) {

  return WasmEdge_VersionGetMajor();
}

JNIEXPORT jlong JNICALL
Java_org_wasmedge_WasmEdge_getMinorVersion(JNIEnv *env, jobject thisObject) {
  return WasmEdge_VersionGetMinor();
}

JNIEXPORT jlong JNICALL
Java_org_wasmedge_WasmEdge_getPatchVersion(JNIEnv *env, jobject thisObject) {
  return WasmEdge_VersionGetPatch();
}

JNIEXPORT void JNICALL
Java_org_wasmedge_WasmEdge_setErrorLevel(JNIEnv *env, jobject thisObject) {
  WasmEdge_LogSetErrorLevel();
}

/*
 * Class:     org_wasmedge_WasmEdge
 * Method:    setDebugLevel
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_wasmedge_WasmEdge_setDebugLevel(JNIEnv *env, jobject thisObject) {
  WasmEdge_LogSetDebugLevel();
}
