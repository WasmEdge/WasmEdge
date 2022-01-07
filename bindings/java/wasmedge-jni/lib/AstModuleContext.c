//
// Created by Kenvi Zhu on 2021-12-20.
//

#include "../jni/org_wasmedge_ASTModuleContext.h"
#include "wasmedge/wasmedge.h"
#include "jni.h"
#include "common.h"

WasmEdge_ASTModuleContext * getASTModuleContext(JNIEnv* env, jobject thisObject) {
    return (WasmEdge_ASTModuleContext*) getPointer(env, thisObject);
}


JNIEXPORT jobject JNICALL Java_org_wasmedge_ASTModuleContext_listImports
        (JNIEnv * env, jobject thisObject) {

}



jobject createAstModuleContext(JNIEnv * env, WasmEdge_ASTModuleContext* mod) {

    jclass cls = findJavaClass(env, "org/wasmedge/ASTModuleContext");
    jmethodID constructor = findJavaMethod(env, cls, "<init>", "()V");
    jobject obj = (*env)->NewObject(env, cls, constructor);
    setPointer(env, obj, (long)mod);
    return obj;
}

JNIEXPORT void JNICALL Java_org_wasmedge_ASTModuleContext_delete
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_ASTModuleContext *mod  = getASTModuleContext(env, thisObject);
    WasmEdge_ASTModuleDelete(mod);
}