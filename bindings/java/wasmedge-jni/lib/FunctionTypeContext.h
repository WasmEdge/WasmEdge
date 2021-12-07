//
// Created by Kenvi Zhu on 2021-12-04.
//

#ifndef WASMEDGE_JAVA_FUNCTIONTYPECONTEXT_H
#define WASMEDGE_JAVA_FUNCTIONTYPECONTEXT_H

#include "wasmedge/wasmedge.h"

WasmEdge_FunctionTypeContext * getFunctionTypeContext(JNIEnv* env, jobject jFunctionTypeContext);

#endif //WASMEDGE_JAVA_FUNCTIONTYPECONTEXT_H
