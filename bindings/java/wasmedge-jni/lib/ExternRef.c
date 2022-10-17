//
// Created by Kenvi Zhu on 2022-01-16.
//

#include <stdlib.h>
#include "../jni/org_wasmedge_WasmEdgeExternRef.h"
#include "common.h"
#include <string.h>


JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeExternRef_nativeInit
        (JNIEnv * env, jobject thisObject, jobject jVal) {
    jint len = (*env)->GetStringUTFLength(env, jVal);
    char *ptr = (char*)malloc(sizeof(char ) * len);

    const char* val = (*env)->GetStringUTFChars(env, jVal, NULL);

    memcpy(ptr, val, len);

    (*env)->ReleaseStringUTFChars(env, jVal, val);
    WasmEdge_Value ref = WasmEdge_ValueGenExternRef(ptr);

    setPointer(env, thisObject, (long)&ref);
}

