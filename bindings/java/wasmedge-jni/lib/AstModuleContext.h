//
// Created by Kenvi Zhu on 2021-12-20.
//

#ifndef WASMEDGE_JAVA_ASTMODULECONTEXT_H
#define WASMEDGE_JAVA_ASTMODULECONTEXT_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_ASTModuleContext * getASTModuleContext(JNIEnv* env, jobject thisObject);
jobject createAstModuleContext(JNIEnv * env, WasmEdge_ASTModuleContext* mod);

#endif //WASMEDGE_JAVA_ASTMODULECONTEXT_H
