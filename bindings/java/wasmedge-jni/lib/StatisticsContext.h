//
// Created by Kenvi Zhu on 2022-01-09.
//

#ifndef WASMEDGE_JAVA_STATISTICSCONTEXT_H
#define WASMEDGE_JAVA_STATISTICSCONTEXT_H

WasmEdge_StatisticsContext *getStatisticsContext(JNIEnv *env, jobject jStatCxt);

jobject
CreateJavaStatisticsContext(JNIEnv *env,
                            WasmEdge_StatisticsContext *statisticsContext);

#endif // WASMEDGE_JAVA_STATISTICSCONTEXT_H
