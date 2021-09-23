#include "org_wasmedge_WasmEdgeVM.h"
#include "wasmedge/wasmedge.h"

JNIEXPORT jstring JNICALL Java_org_wasmedge_WasmEdgeVM_getVersion
  (JNIEnv * env, jobject thisObject) {
    
    const char* Version = WasmEdge_VersionGet();
    jstring result = (*env)->NewStringUTF(env, Version);
    return result;
}

JNIEXPORT jlong JNICALL Java_org_wasmedge_WasmEdgeVM_getMajorVersion
  (JNIEnv * env, jobject thisObject) {
    return WasmEdge_VersionGetMajor();

  }

JNIEXPORT jlong JNICALL Java_org_wasmedge_WasmEdgeVM_getMinorVersion
  (JNIEnv * env, jobject thisObject) {
    return WasmEdge_VersionGetMinor();
  }

JNIEXPORT jlong JNICALL Java_org_wasmedge_WasmEdgeVM_getPatchVersion
  (JNIEnv * env, jobject thisObject) {
    return WasmEdge_VersionGetPatch();
  }
