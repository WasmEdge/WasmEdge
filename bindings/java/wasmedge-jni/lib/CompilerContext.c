//
// Created by Kenvi Zhu on 2021-12-16.
//
#include "../jni/org_wasmedge_CompilerContext.h"
#include "wasmedge/wasmedge.h"
#include "ConfigureContext.h"
#include "common.h"

WasmEdge_CompilerContext * getCompilerContext(JNIEnv* env, jobject jCompilerContext) {
    if(jCompilerContext == NULL) {
        return NULL;
    }
    return (WasmEdge_CompilerContext *)getPointer(env, jCompilerContext);
}

JNIEXPORT void JNICALL Java_org_wasmedge_CompilerContext_nativeInit
        (JNIEnv * env, jobject thisObject, jobject jConfigContext) {

   WasmEdge_ConfigureContext* configureContext = getConfigureContext(env, jConfigContext);

   WasmEdge_CompilerContext* compilerContext =  WasmEdge_CompilerCreate(configureContext);

   setPointer(env, thisObject, (long)compilerContext);
}


JNIEXPORT void JNICALL Java_org_wasmedge_CompilerContext_compile
        (JNIEnv * env, jobject thisObject, jstring jInputPath, jstring jOutputPath) {
    WasmEdge_CompilerContext * compilerContext = getCompilerContext(env, thisObject);


    const char* inputPath = (*env)->GetStringUTFChars(env, jInputPath, NULL);
    const char* outputPath = (*env)->GetStringUTFChars(env, jOutputPath, NULL);

    WasmEdge_CompilerCompile(compilerContext, inputPath, outputPath);

    (*env)->ReleaseStringUTFChars(env, jInputPath, inputPath);
    (*env)->ReleaseStringUTFChars(env, jOutputPath, outputPath);
}

JNIEXPORT void JNICALL Java_org_wasmedge_CompilerContext_delete
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_CompilerContext * compilerContext = getCompilerContext(env, thisObject);
    WasmEdge_CompilerDelete(compilerContext);
}

