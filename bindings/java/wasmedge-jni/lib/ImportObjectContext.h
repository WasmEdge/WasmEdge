//
// Created by Kenvi Zhu on 2022-03-17.
//

#ifndef WASMEDGE_JAVA_IMPORTOBJECTCONTEXT_H
#define WASMEDGE_JAVA_IMPORTOBJECTCONTEXT_H
#include "jni.h"
#include "wasmedge/wasmedge.h"
jobject createJImportObject(JNIEnv* env, const WasmEdge_ImportObjectContext * impObj);
#endif //WASMEDGE_JAVA_IMPORTOBJECTCONTEXT_H
