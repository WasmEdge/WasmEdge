//
// Created by Kenvi Zhu on 2022-01-09.
//

#include "../jni/org_wasmedge_StatisticsContext.h"
#include "jni.h"
#include "wasmedge/wasmedge.h"
#include "common.h"

WasmEdge_StatisticsContext *getStatisticsContext(JNIEnv * env, jobject jStatCxt) {
    if(jStatCxt == NULL) {
        return NULL;
    }
    return (WasmEdge_StatisticsContext *)getPointer(env, jStatCxt);
}

jobject CreateJavaStatisticsContext(JNIEnv *env, WasmEdge_StatisticsContext * statisticsContext) {
    jclass statClass = findJavaClass(env, "org/wasmedge/StatisticsContext");

    jmethodID constructor = (*env)->GetMethodID(env, statClass, "<init>", "(J)V");

    jobject jStatContext = (*env)->NewObject(env, statClass, constructor, (long)statisticsContext);
    checkAndHandleException(env, "error creating stat context");

    return jStatContext;
}

JNIEXPORT void JNICALL Java_org_wasmedge_StatisticsContext_nativeInit
        (JNIEnv * env, jobject thisObject) {

    WasmEdge_StatisticsContext *statCxt = WasmEdge_StatisticsCreate();
    setPointer(env, thisObject, (long)statCxt);
}

JNIEXPORT jint JNICALL Java_org_wasmedge_StatisticsContext_getInstrCount
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_StatisticsContext* statCxt = getStatisticsContext(env, thisObject);
    return WasmEdge_StatisticsGetInstrCount(statCxt);

}

JNIEXPORT jdouble JNICALL Java_org_wasmedge_StatisticsContext_getInstrPerSecond
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_StatisticsContext* statCxt = getStatisticsContext(env, thisObject);
    return WasmEdge_StatisticsGetInstrPerSecond(statCxt);

}

JNIEXPORT void JNICALL Java_org_wasmedge_StatisticsContext_setCostTable
        (JNIEnv * env, jobject thisObject, jlongArray jCostTable) {

    WasmEdge_StatisticsContext* statCxt = getStatisticsContext(env, thisObject);

    int len = (*env)->GetArrayLength(env, jCostTable);

    long* data = (*env)->GetLongArrayElements(env, jCostTable, NULL);
    uint64_t* CostTable = malloc(sizeof (uint64_t) * len);

    WasmEdge_StatisticsSetCostTable(statCxt, data, len);

    (*env)->ReleaseLongArrayElements(env, jCostTable, data, len);
}

JNIEXPORT void JNICALL Java_org_wasmedge_StatisticsContext_setCostLimit
        (JNIEnv * env, jobject thisObject, jlong costLimit) {
    WasmEdge_StatisticsContext* statCxt = getStatisticsContext(env, thisObject);
    WasmEdge_StatisticsSetCostLimit(statCxt, costLimit);
}

JNIEXPORT jlong JNICALL Java_org_wasmedge_StatisticsContext_getTotalCost
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_StatisticsContext* statCxt = getStatisticsContext(env, thisObject);
    return WasmEdge_StatisticsGetTotalCost(statCxt);

}

JNIEXPORT void JNICALL Java_org_wasmedge_StatisticsContext_delete
        (JNIEnv * env, jobject thisObject) {
    WasmEdge_StatisticsContext* statCxt = getStatisticsContext(env, thisObject);
    WasmEdge_StatisticsDelete(statCxt);
    setPointer(env, thisObject, 0);
}

