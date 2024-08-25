// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "../jni/org_wasmedge_ConfigureContext.h"
#include "common.h"
#include "wasmedge/wasmedge.h"
#include <stdint.h>

GETTER(ConfigureContext)

JNIEXPORT void JNICALL
Java_org_wasmedge_ConfigureContext_nativeInit(JNIEnv *env, jobject thisObj) {
  WasmEdge_ConfigureContext *ConfigureContext = WasmEdge_ConfigureCreate();
  setPointer(env, thisObj, (jlong)ConfigureContext);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_ConfigureContext_close(JNIEnv *env, jobject thisObj) {
  WasmEdge_ConfigureDelete(getConfigureContext(env, thisObj));
}

JNIEXPORT void JNICALL Java_org_wasmedge_ConfigureContext_addProposal(
    JNIEnv *env, jobject thisObject, jint proposal) {
  WasmEdge_ConfigureAddProposal(getConfigureContext(env, thisObject),
                                (enum WasmEdge_Proposal)proposal);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ConfigureContext_removeProposal(
    JNIEnv *env, jobject thisObject, jint proposal) {
  WasmEdge_ConfigureRemoveProposal(getConfigureContext(env, thisObject),
                                   (enum WasmEdge_Proposal)proposal);
}

JNIEXPORT jboolean JNICALL Java_org_wasmedge_ConfigureContext_hasProposal(
    JNIEnv *env, jobject thisObject, jint proposal) {
  return WasmEdge_ConfigureHasProposal(getConfigureContext(env, thisObject),
                                       (enum WasmEdge_Proposal)proposal);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ConfigureContext_addHostRegistration(
    JNIEnv *env, jobject thisObject, jint hostRegistration) {
  WasmEdge_ConfigureAddHostRegistration(
      getConfigureContext(env, thisObject),
      (enum WasmEdge_HostRegistration)hostRegistration);
}

JNIEXPORT void JNICALL
Java_org_wasmedge_ConfigureContext_removeHostRegistration(
    JNIEnv *env, jobject thisObject, jint hostRegistration) {
  WasmEdge_ConfigureRemoveHostRegistration(
      getConfigureContext(env, thisObject),
      (enum WasmEdge_HostRegistration)hostRegistration);
}

JNIEXPORT jboolean JNICALL
Java_org_wasmedge_ConfigureContext_hasHostRegistration(JNIEnv *env,
                                                       jobject thisObject,
                                                       jint hostRegistration) {
  return WasmEdge_ConfigureHasHostRegistration(
      getConfigureContext(env, thisObject),
      (enum WasmEdge_HostRegistration)hostRegistration);
}

JNIEXPORT void JNICALL Java_org_wasmedge_ConfigureContext_setMaxMemoryPage(
    JNIEnv *env, jobject thisObject, jlong maxPage) {
  WasmEdge_ConfigureSetMaxMemoryPage(getConfigureContext(env, thisObject),
                                     (uint32_t)maxPage);
}

JNIEXPORT jlong JNICALL Java_org_wasmedge_ConfigureContext_getMaxMemoryPage(
    JNIEnv *env, jobject thisObject) {
  return (jlong)WasmEdge_ConfigureGetMaxMemoryPage(
      getConfigureContext(env, thisObject));
}

JNIEXPORT void JNICALL
Java_org_wasmedge_ConfigureContext_setCompilerOptimizationLevel(
    JNIEnv *env, jobject thisObject, jint optimizationLevel) {
  WasmEdge_ConfigureCompilerSetOptimizationLevel(
      getConfigureContext(env, thisObject),
      (enum WasmEdge_CompilerOptimizationLevel)optimizationLevel);
}

JNIEXPORT jint JNICALL
Java_org_wasmedge_ConfigureContext_nativeGetCompilerOptimizationLevel(
    JNIEnv *env, jobject thisObject) {
  return (jint)WasmEdge_ConfigureCompilerGetOptimizationLevel(
      getConfigureContext(env, thisObject));
}

JNIEXPORT void JNICALL
Java_org_wasmedge_ConfigureContext_setCompilerOutputFormat(JNIEnv *env,
                                                           jobject thisObject,
                                                           jint outputFormat) {
  WasmEdge_ConfigureCompilerSetOutputFormat(
      getConfigureContext(env, thisObject),
      (enum WasmEdge_CompilerOutputFormat)outputFormat);
}

JNIEXPORT jint JNICALL
Java_org_wasmedge_ConfigureContext_nativeGetCompilerOutputFormat(
    JNIEnv *env, jobject thisObject) {
  return (jint)WasmEdge_ConfigureCompilerGetOutputFormat(
      getConfigureContext(env, thisObject));
}

JNIEXPORT void JNICALL Java_org_wasmedge_ConfigureContext_setCompilerIsDumpIr(
    JNIEnv *env, jobject thisObject, jboolean isDumpIR) {
  WasmEdge_ConfigureCompilerSetDumpIR(getConfigureContext(env, thisObject),
                                      isDumpIR);
}

JNIEXPORT jboolean JNICALL
Java_org_wasmedge_ConfigureContext_getCompilerIsDumpIr(JNIEnv *env,
                                                       jobject thisObject) {
  return WasmEdge_ConfigureCompilerIsDumpIR(
      getConfigureContext(env, thisObject));
}

JNIEXPORT void JNICALL
Java_org_wasmedge_ConfigureContext_setCompilerIsGenericBinary(
    JNIEnv *env, jobject thisObject, jboolean isGenericBinary) {
  WasmEdge_ConfigureCompilerSetGenericBinary(
      getConfigureContext(env, thisObject), isGenericBinary);
}

JNIEXPORT jboolean JNICALL
Java_org_wasmedge_ConfigureContext_getCompilerIsGenericBinary(
    JNIEnv *env, jobject thisObject) {
  return WasmEdge_ConfigureCompilerIsGenericBinary(
      getConfigureContext(env, thisObject));
}

JNIEXPORT void JNICALL
Java_org_wasmedge_ConfigureContext_setStatisticsSetInstructionCounting(
    JNIEnv *env, jobject thisObject, jboolean instructionCounting) {
  WasmEdge_ConfigureStatisticsSetInstructionCounting(
      getConfigureContext(env, thisObject), instructionCounting);
}

JNIEXPORT jboolean JNICALL
Java_org_wasmedge_ConfigureContext_isStatisticsSetInstructionCounting(
    JNIEnv *env, jobject thisObject) {
  return WasmEdge_ConfigureStatisticsIsInstructionCounting(
      getConfigureContext(env, thisObject));
}

JNIEXPORT void JNICALL
Java_org_wasmedge_ConfigureContext_setStatisticsSetCostMeasuring(
    JNIEnv *env, jobject thisObject, jboolean costMeasuring) {
  WasmEdge_ConfigureStatisticsSetCostMeasuring(
      getConfigureContext(env, thisObject), costMeasuring);
}

JNIEXPORT jboolean JNICALL
Java_org_wasmedge_ConfigureContext_isStatisticsSetCostMeasuring(
    JNIEnv *env, jobject thisObject) {
  return WasmEdge_ConfigureStatisticsIsCostMeasuring(
      getConfigureContext(env, thisObject));
}

JNIEXPORT void JNICALL
Java_org_wasmedge_ConfigureContext_setStatisticsSetTimeMeasuring(
    JNIEnv *env, jobject thisObject, jboolean timeMeasuring) {
  WasmEdge_ConfigureStatisticsSetTimeMeasuring(
      getConfigureContext(env, thisObject), timeMeasuring);
}

JNIEXPORT jboolean JNICALL
Java_org_wasmedge_ConfigureContext_isStatisticsSetTimeMeasuring(
    JNIEnv *env, jobject thisObject) {
  return WasmEdge_ConfigureStatisticsIsTimeMeasuring(
      getConfigureContext(env, thisObject));
}
