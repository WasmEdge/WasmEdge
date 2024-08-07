// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_STATISTICSCONTEXT_H
#define WASMEDGE_JAVA_STATISTICSCONTEXT_H

WasmEdge_StatisticsContext *getStatisticsContext(JNIEnv *env, jobject jStatCxt);

jobject
CreateJavaStatisticsContext(JNIEnv *env,
                            WasmEdge_StatisticsContext *statisticsContext);

#endif // WASMEDGE_JAVA_STATISTICSCONTEXT_H
