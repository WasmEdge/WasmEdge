/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_wasmedge_WasmEdgeVM */

#ifndef _Included_org_wasmedge_WasmEdgeVM
#define _Included_org_wasmedge_WasmEdgeVM
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    nativeInit
 * Signature: (Lorg/wasmedge/ConfigureContext;Lorg/wasmedge/StoreContext;)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_nativeInit
  (JNIEnv *, jobject, jobject, jobject);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    runWasmFromFile
 * Signature: (Ljava/lang/String;Ljava/lang/String;[Lorg/wasmedge/WasmEdgeValue;I[I[Lorg/wasmedge/WasmEdgeValue;I[I)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_runWasmFromFile
  (JNIEnv *, jobject, jstring, jstring, jobjectArray, jint, jintArray, jobjectArray, jint, jintArray);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    runWasmFromBuffer
 * Signature: ([BLjava/lang/String;[Lorg/wasmedge/WasmEdgeValue;[I[Lorg/wasmedge/WasmEdgeValue;[I)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_runWasmFromBuffer
  (JNIEnv *, jobject, jbyteArray, jstring, jobjectArray, jintArray, jobjectArray, jintArray);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    runWasmFromASTModule
 * Signature: (Lorg/wasmedge/ASTModuleContext;Ljava/lang/String;[Lorg/wasmedge/WasmEdgeValue;[I[Lorg/wasmedge/WasmEdgeValue;[I)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_runWasmFromASTModule
  (JNIEnv *, jobject, jobject, jstring, jobjectArray, jintArray, jobjectArray, jintArray);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    loadWasmFromFile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_loadWasmFromFile
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    loadWasmFromBuffer
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_loadWasmFromBuffer
  (JNIEnv *, jobject, jbyteArray);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    loadWasmFromASTModule
 * Signature: (Lorg/wasmedge/ASTModuleContext;)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_loadWasmFromASTModule
  (JNIEnv *, jobject, jobject);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    validate
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_validate
  (JNIEnv *, jobject);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    instantiate
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_instantiate
  (JNIEnv *, jobject);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    execute
 * Signature: (Ljava/lang/String;[Lorg/wasmedge/WasmEdgeValue;I[I[Lorg/wasmedge/WasmEdgeValue;I[I)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_execute
  (JNIEnv *, jobject, jstring, jobjectArray, jint, jintArray, jobjectArray, jint, jintArray);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    registerModuleFromFile
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_registerModuleFromFile
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    registerModuleFromBuffer
 * Signature: (Ljava/lang/String;[B)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_registerModuleFromBuffer
  (JNIEnv *, jobject, jstring, jbyteArray);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    registerModuleFromImport
 * Signature: (Lorg/wasmedge/ModuleInstanceContext;)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_registerModuleFromImport
  (JNIEnv *, jobject, jobject);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    registerModuleFromASTModule
 * Signature: (Ljava/lang/String;Lorg/wasmedge/ASTModuleContext;)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_registerModuleFromASTModule
  (JNIEnv *, jobject, jstring, jobject);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    executeRegistered
 * Signature: (Ljava/lang/String;Ljava/lang/String;[Lorg/wasmedge/WasmEdgeValue;[I[Lorg/wasmedge/WasmEdgeValue;[I)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_executeRegistered
  (JNIEnv *, jobject, jstring, jstring, jobjectArray, jintArray, jobjectArray, jintArray);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    getFunctionList
 * Signature: (Ljava/util/List;)V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_getFunctionList
  (JNIEnv *, jobject, jobject);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    getFunctionType
 * Signature: (Ljava/lang/String;)Lorg/wasmedge/FunctionTypeContext;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVM_getFunctionType
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    nativeGetImportModuleContext
 * Signature: (I)Lorg/wasmedge/ModuleInstanceContext;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVM_nativeGetImportModuleContext
  (JNIEnv *, jobject, jint);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    getStoreContext
 * Signature: ()Lorg/wasmedge/StoreContext;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVM_getStoreContext
  (JNIEnv *, jobject);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    getStatisticsContext
 * Signature: ()Lorg/wasmedge/StatisticsContext;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVM_getStatisticsContext
  (JNIEnv *, jobject);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    getFunctionTypeRegistered
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Lorg/wasmedge/FunctionTypeContext;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVM_getFunctionTypeRegistered
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    cleanUp
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_cleanUp
  (JNIEnv *, jobject);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    delete
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeVM_delete
  (JNIEnv *, jobject);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    wasmEdgeVMAsyncRunWasmFromFile
 * Signature: (Ljava/lang/String;Ljava/lang/String;[Lorg/wasmedge/WasmEdgeValue;[I)Lorg/wasmedge/WasmEdgeAsync;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVM_wasmEdgeVMAsyncRunWasmFromFile
  (JNIEnv *, jobject, jstring, jstring, jobjectArray, jintArray);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    wasmEdgeVMAsyncRunWasmFromBuffer
 * Signature: ([BLjava/lang/String;[Lorg/wasmedge/WasmEdgeValue;[I)Lorg/wasmedge/WasmEdgeAsync;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVM_wasmEdgeVMAsyncRunWasmFromBuffer
  (JNIEnv *, jobject, jbyteArray, jstring, jobjectArray, jintArray);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    wasmEdgeVMAsyncRunWasmFromASTModule
 * Signature: (Lorg/wasmedge/ASTModuleContext;Ljava/lang/String;[Lorg/wasmedge/WasmEdgeValue;[I)Lorg/wasmedge/WasmEdgeAsync;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVM_wasmEdgeVMAsyncRunWasmFromASTModule
  (JNIEnv *, jobject, jobject, jstring, jobjectArray, jintArray);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    wasmEdgeVMAsyncExecute
 * Signature: (Ljava/lang/String;[Lorg/wasmedge/WasmEdgeValue;[I)Lorg/wasmedge/WasmEdgeAsync;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVM_wasmEdgeVMAsyncExecute
  (JNIEnv *, jobject, jstring, jobjectArray, jintArray);

/*
 * Class:     org_wasmedge_WasmEdgeVM
 * Method:    wasmEdgeVMAsyncExecuteRegistered
 * Signature: (Ljava/lang/String;Ljava/lang/String;[Lorg/wasmedge/WasmEdgeValue;[I)Lorg/wasmedge/WasmEdgeAsync;
 */
JNIEXPORT jobject JNICALL Java_org_wasmedge_WasmEdgeVM_wasmEdgeVMAsyncExecuteRegistered
  (JNIEnv *, jobject, jstring, jstring, jobjectArray, jintArray);

#ifdef __cplusplus
}
#endif
#endif
