//
// Created by Kenvi Zhu on 2022-01-14.
//

#ifndef WASMEDGE_JAVA_TABLETYPECONTEXT_H
#define WASMEDGE_JAVA_TABLETYPECONTEXT_H

WasmEdge_TableTypeContext * getTableTypeContext(JNIEnv* env, jobject jTableTypeContext);

jobject createJTableTypeContext(JNIEnv* env, const WasmEdge_TableTypeContext * tableTypeContext);

#endif //WASMEDGE_JAVA_TABLETYPECONTEXT_H
