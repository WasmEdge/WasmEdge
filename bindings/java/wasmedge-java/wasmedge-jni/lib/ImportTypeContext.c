// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ImportTypeContext.h"
#include "AstModuleContext.h"
#include "FunctionTypeContext.h"
#include "GlobalTypeContext.h"
#include "MemoryTypeContext.h"
#include "TableTypeContext.h"
#include "common.h"
#include "jni.h"
#include "wasmedge/wasmedge.h"

GETTER(ImportTypeContext)

JNIEXPORT jstring JNICALL Java_org_wasmedge_ImportTypeContext_getModuleName(
    JNIEnv *env, jobject thisObject) {

  WasmEdge_ImportTypeContext *impType = getImportTypeContext(env, thisObject);

  WasmEdge_String wModName = WasmEdge_ImportTypeGetModuleName(impType);
  return WasmEdgeStringToJString(env, wModName);
}

/*
 * Class:     org_wasmedge_ImportTypeContext
 * Method:    getExternalName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_wasmedge_ImportTypeContext_getExternalName(
    JNIEnv *env, jobject thisObject) {
  WasmEdge_ImportTypeContext *expType = getImportTypeContext(env, thisObject);

  WasmEdge_String wName = WasmEdge_ImportTypeGetExternalName(expType);

  return WasmEdgeStringToJString(env, wName);
}

/*
 * Class:     org_wasmedge_ImportTypeContext
 * Method:    getExternalType
 * Signature: ()Lorg/wasmedge/enums/ExternalType;
 */
JNIEXPORT jint JNICALL
Java_org_wasmedge_ImportTypeContext_nativeGetExternalType(JNIEnv *env,
                                                          jobject thisObject) {
  WasmEdge_ImportTypeContext *expType = getImportTypeContext(env, thisObject);

  enum WasmEdge_ExternalType type = WasmEdge_ImportTypeGetExternalType(expType);
  return type;
}

/*
 * Class:     org_wasmedge_ImportTypeContext
 * Method:    getFunctionType
 * Signature: ()Lorg/wasmedge/FunctionTypeContext;
 */
JNIEXPORT jobject JNICALL
Java_org_wasmedge_ImportTypeContext_nativeGetFunctionType(JNIEnv *env,
                                                          jobject thisObject,
                                                          jobject jAstCxt) {
  WasmEdge_ImportTypeContext *expType = getImportTypeContext(env, thisObject);
  WasmEdge_ASTModuleContext *astCxt = getASTModuleContext(env, jAstCxt);
  const WasmEdge_FunctionTypeContext *functionTypeContext =
      WasmEdge_ImportTypeGetFunctionType(astCxt, expType);
  return createJFunctionTypeContext(env, functionTypeContext);
}

/*
 * Class:     org_wasmedge_ImportTypeContext
 * Method:    getTableType
 * Signature: ()Lorg/wasmedge/TableTypeContext;
 */
JNIEXPORT jobject JNICALL
Java_org_wasmedge_ImportTypeContext_nativeGetTableType(JNIEnv *env,
                                                       jobject thisObject,
                                                       jobject jAstCxt) {
  WasmEdge_ImportTypeContext *expType = getImportTypeContext(env, thisObject);
  WasmEdge_ASTModuleContext *astCxt = getASTModuleContext(env, jAstCxt);
  const WasmEdge_TableTypeContext *tableCxt =
      WasmEdge_ImportTypeGetTableType(astCxt, expType);

  return createJTableTypeContext(env, tableCxt);
}
/*
 * Class:     org_wasmedge_ImportTypeContext
 * Method:    getMemoryType
 * Signature: ()Lorg/wasmedge/MemoryTypeContext;
 */
JNIEXPORT jobject JNICALL
Java_org_wasmedge_ImportTypeContext_nativeGetMemoryType(JNIEnv *env,
                                                        jobject thisObject,
                                                        jobject jAstCxt) {
  WasmEdge_ImportTypeContext *expType = getImportTypeContext(env, thisObject);
  WasmEdge_ASTModuleContext *astCxt = getASTModuleContext(env, jAstCxt);

  const WasmEdge_MemoryTypeContext *memCxt =
      WasmEdge_ImportTypeGetMemoryType(astCxt, expType);
  return createJMemoryTypeContext(env, memCxt);
}
/*
 * Class:     org_wasmedge_ImportTypeContext
 * Method:    getGlobalType
 * Signature: ()Lorg/wasmedge/GlobalTypeContext;
 */
JNIEXPORT jobject JNICALL
Java_org_wasmedge_ImportTypeContext_nativeGetGlobalType(JNIEnv *env,
                                                        jobject thisObject,
                                                        jobject jAstCxt) {
  WasmEdge_ImportTypeContext *expType = getImportTypeContext(env, thisObject);
  WasmEdge_ASTModuleContext *astCxt = getASTModuleContext(env, jAstCxt);

  const WasmEdge_GlobalTypeContext *globalCxt =
      WasmEdge_ImportTypeGetGlobalType(astCxt, expType);

  return createJGlobalTypeContext(env, globalCxt);
}

jobject createImportTypeContext(JNIEnv *env,
                                const WasmEdge_ImportTypeContext *cxt,
                                jobject jAstMod) {

  jclass cls = findJavaClass(env, ORG_WASMEDGE_IMPORTTYPECONTEXT);
  jmethodID constructor =
      findJavaMethod(env, cls, DEFAULT_CONSTRUCTOR, ASTMODULECONTEXT_VOID);
  jobject obj = (*env)->NewObject(env, cls, constructor, (long)cxt, jAstMod);
  return obj;
}
