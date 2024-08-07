// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ExportTypeContext.h"
#include "AstModuleContext.h"
#include "FunctionTypeContext.h"
#include "GlobalTypeContext.h"
#include "MemoryTypeContext.h"
#include "TableTypeContext.h"
#include "common.h"
#include "jni.h"
#include "wasmedge/wasmedge.h"

GETTER(ExportTypeContext)

/*
 * Class:     org_wasmedge_ExportTypeContext
 * Method:    getExternalName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_wasmedge_ExportTypeContext_getExternalName(
    JNIEnv *env, jobject thisObject) {
  WasmEdge_ExportTypeContext *expType = getExportTypeContext(env, thisObject);

  WasmEdge_String wName = WasmEdge_ExportTypeGetExternalName(expType);

  return WasmEdgeStringToJString(env, wName);
}

/*
 * Class:     org_wasmedge_ExportTypeContext
 * Method:    getExternalType
 * Signature: ()Lorg/wasmedge/enums/ExternalType;
 */
JNIEXPORT jint JNICALL
Java_org_wasmedge_ExportTypeContext_nativeGetExternalType(JNIEnv *env,
                                                          jobject thisObject) {
  WasmEdge_ExportTypeContext *expType = getExportTypeContext(env, thisObject);

  enum WasmEdge_ExternalType type = WasmEdge_ExportTypeGetExternalType(expType);
  return type;
}

/*
 * Class:     org_wasmedge_ExportTypeContext
 * Method:    getFunctionType
 * Signature: ()Lorg/wasmedge/FunctionTypeContext;
 */
JNIEXPORT jobject JNICALL
Java_org_wasmedge_ExportTypeContext_nativeGetFunctionType(JNIEnv *env,
                                                          jobject thisObject,
                                                          jobject jAstCxt) {
  WasmEdge_ExportTypeContext *expType = getExportTypeContext(env, thisObject);
  WasmEdge_ASTModuleContext *astCxt = getASTModuleContext(env, jAstCxt);
  const WasmEdge_FunctionTypeContext *functionTypeContext =
      WasmEdge_ExportTypeGetFunctionType(astCxt, expType);
  return createJFunctionTypeContext(env, functionTypeContext);
}

/*
 * Class:     org_wasmedge_ExportTypeContext
 * Method:    getTableType
 * Signature: ()Lorg/wasmedge/TableTypeContext;
 */
JNIEXPORT jobject JNICALL
Java_org_wasmedge_ExportTypeContext_nativeGetTableType(JNIEnv *env,
                                                       jobject thisObject,
                                                       jobject jAstCxt) {
  WasmEdge_ExportTypeContext *expType = getExportTypeContext(env, thisObject);
  WasmEdge_ASTModuleContext *astCxt = getASTModuleContext(env, jAstCxt);
  const WasmEdge_TableTypeContext *tableCxt =
      WasmEdge_ExportTypeGetTableType(astCxt, expType);

  return createJTableTypeContext(env, tableCxt);
}
/*
 * Class:     org_wasmedge_ExportTypeContext
 * Method:    getMemoryType
 * Signature: ()Lorg/wasmedge/MemoryTypeContext;
 */
JNIEXPORT jobject JNICALL
Java_org_wasmedge_ExportTypeContext_nativeGetMemoryType(JNIEnv *env,
                                                        jobject thisObject,
                                                        jobject jAstCxt) {
  WasmEdge_ExportTypeContext *expType = getExportTypeContext(env, thisObject);
  WasmEdge_ASTModuleContext *astCxt = getASTModuleContext(env, jAstCxt);

  const WasmEdge_MemoryTypeContext *memCxt =
      WasmEdge_ExportTypeGetMemoryType(astCxt, expType);
  return createJMemoryTypeContext(env, memCxt);
}
/*
 * Class:     org_wasmedge_ExportTypeContext
 * Method:    getGlobalType
 * Signature: ()Lorg/wasmedge/GlobalTypeContext;
 */
JNIEXPORT jobject JNICALL
Java_org_wasmedge_ExportTypeContext_nativeGetGlobalType(JNIEnv *env,
                                                        jobject thisObject,
                                                        jobject jAstCxt) {
  WasmEdge_ExportTypeContext *expType = getExportTypeContext(env, thisObject);
  WasmEdge_ASTModuleContext *astCxt = getASTModuleContext(env, jAstCxt);

  const WasmEdge_GlobalTypeContext *globalCxt =
      WasmEdge_ExportTypeGetGlobalType(astCxt, expType);

  return createJGlobalTypeContext(env, globalCxt);
}

jobject createExportTypeContext(JNIEnv *env,
                                const WasmEdge_ExportTypeContext *cxt,
                                jobject jAstMod) {

  jclass cls = findJavaClass(env, ORG_WASMEDGE_EXPORTTYPECONTEXT);
  jmethodID constructor =
      findJavaMethod(env, cls, DEFAULT_CONSTRUCTOR, ASTMODULECONTEXT_VOID);
  jobject obj = (*env)->NewObject(env, cls, constructor, (long)cxt, jAstMod);
  return obj;
}
