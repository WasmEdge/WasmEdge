//
// Created by Kenvi Zhu on 2021-09-28.
//
#include "../org_wasmedge_WasmEdgeString.h"
#include "wasmedge/wasmedge.h"


#ifdef __cplusplus
extern "C" {
#endif
/**
 * Get WasmEdge_String from java WasmEdgeString
 * @param env  jni env
 * @param obj WasmEdge obj
 * @return WasmEdge string
 */
WasmEdge_String * getWasmEdgeString(JNIEnv * env, jobject obj) {
    jclass clazz = (*env)->GetObjectClass(env, obj);
    jfieldID pointerId = (*env)->GetFieldID(env, clazz, "pointer", "J");
    jlong pointerVal = (*env)->GetLongField(env, obj, pointerId);
    WasmEdge_String* WasmEdgeString = (void*) pointerVal;
    return WasmEdgeString;
}


/**
 * Create WasmEdge string.
 * @param env jni env.
 * @param thisObject current obj
 * @param jstr java string
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeString_createInternal
        (JNIEnv * env, jobject thisObject, jstring jstr){
    jclass clazz = (*env)->GetObjectClass(env, thisObject);
    jfieldID pointerId = (*env)->GetFieldID(env, clazz, "pointer", "J");
    const char *nativeString = (*env)->GetStringUTFChars(env, jstr, 0);
    WasmEdge_String wasmStr = WasmEdge_StringCreateByCString(nativeString);
    jlong pointerVal = (long)&wasmStr;

    (*env)->SetLongField(env, thisObject, pointerId, pointerVal);
    return;
}

/**
 *
 * @param env
 * @param thisObject
 */
JNIEXPORT void JNICALL Java_org_wasmedge_WasmEdgeString_delete
        (JNIEnv *env, jobject thisObject) {
    WasmEdge_String* wasmEdgeString = getWasmEdgeString(env, thisObject);
    WasmEdge_StringDelete(*wasmEdgeString);
}

/**
 *
 * @param env
 * @param thisObject
 * @return
 */
JNIEXPORT jstring JNICALL Java_org_wasmedge_WasmEdgeString_toStringInternal
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_String * wasmEdgeString = getWasmEdgeString(env, thisObject);
    char* buf = malloc(wasmEdgeString->Length + 1);
    WasmEdge_StringCopy(*wasmEdgeString, buf, wasmEdgeString->Length + 1);
    jstring jstr = (*env)->NewStringUTF(env, buf);
    free(buf);

    return jstr;
}


JNIEXPORT jboolean JNICALL Java_org_wasmedge_WasmEdgeString_equalsInternal
        (JNIEnv * env, jobject thisObj, jobject thatObj) {
    WasmEdge_String* thisString = getWasmEdgeString(env, thisObj);
    WasmEdge_String* thatString = getWasmEdgeString(env, thatObj);

    return WasmEdge_StringIsEqual(*thisString, *thatString);
}

#ifdef __cplusplus
}
#endif

