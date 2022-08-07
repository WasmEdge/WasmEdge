//
// Created by elfgum on 2022/8/1.
//

#include "../jni/org_wasmedge_Async.h"
#include "wasmedge/wasmedge.h"
#include "jni.h"
#include "common.h"
#include "ExportTypeContext.h"
#include "ImportTypeContext.h"

WasmEdge_Async * getAsync(JNIEnv* env, jobject thisObject){
    if(thisObject == NULL) {
        return NULL;
    }
    return (WasmEdge_Async*) getPointer(env, thisObject);
}

