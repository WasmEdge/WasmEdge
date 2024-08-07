// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_ASTMODULECONTEXT_H
#define WASMEDGE_JAVA_ASTMODULECONTEXT_H

#include "jni.h"
#include "wasmedge/wasmedge.h"

WasmEdge_ASTModuleContext *getASTModuleContext(JNIEnv *env, jobject thisObject);
jobject createAstModuleContext(JNIEnv *env, WasmEdge_ASTModuleContext *mod);

#endif // WASMEDGE_JAVA_ASTMODULECONTEXT_H
