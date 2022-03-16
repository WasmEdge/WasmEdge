//
// Created by Kenvi Zhu on 2021-11-09.
//
#include "common.h"
#include "jni.h"
#include <stdlib.h>
#include <string.h>
#include "wasmedge/wasmedge.h"

bool checkAndHandleException(JNIEnv *env, const char* msg);

void exitWithError(enum ErrorCode error, char* message) {
     exit(-1);
}

void throwNoClassDefError(JNIEnv *env, char * message) {
    jclass  exClass;
    char *className = "java/lang/NoClassDefFoundError";

    exClass = (*env)->FindClass(env, className);

    if(exClass == NULL) {
        exitWithError(JVM_ERROR, "Exception class not found.");
    }
    (*env)-> ThrowNew(env, exClass, message);

    exitWithError(JVM_ERROR, "Exception thrown for no class def");
}

void throwNoSuchMethodError(JNIEnv *env, char* methodName, char* sig) {
    jclass exClass;
    char *className = "java/lang/NoSuchMethodError";

    char message[1000];

    strcat(message, methodName);
    strcat(message, sig);

    if(exClass == NULL) {
        throwNoClassDefError(env, message);
    }

    (*env)->ThrowNew(env, exClass, methodName);
    exitWithError(JVM_ERROR, "Exception thrown for no such method");
}


jclass findJavaClass(JNIEnv* env, char * className) {
    jclass class = (*env)->FindClass(env, className);

    bool hasException = checkAndHandleException(env, "find class error");
    if(hasException) {
        return NULL;
    }

    if(class == NULL) {
        throwNoClassDefError(env, className);
    }
    return class;
}

jmethodID findJavaMethod(JNIEnv* env, jclass class, char* methodName, char* sig) {
    jmethodID jmethodId = (*env)->GetMethodID(env, class, methodName, sig);
    return jmethodId;

}

void getClassName(JNIEnv* env, jobject obj, char* buff) {
    jclass cls = (*env)->GetObjectClass(env, obj);

// First get the class object
    jmethodID mid = (*env)->GetMethodID(env, cls, "getClass", "()Ljava/lang/Class;");
    jobject clsObj = (*env)->CallObjectMethod(env, obj, mid);
    checkAndHandleException(env, "get class name error");

// Now get the class object's class descriptor
    cls = (*env)->GetObjectClass(env, clsObj);

// Find the getName() method on the class object
    mid = (*env)->GetMethodID(env, cls, "getName", "()Ljava/lang/String;");

// Call the getName() to get a jstring object back
    jstring strObj = (jstring)(*env)->CallObjectMethod(env, clsObj, mid);
    checkAndHandleException(env, "get name error");

// Now get the c string from the java jstring object
    const char* str = (*env)->GetStringUTFChars(env, strObj, NULL);

// Print the class name
    strcpy(buff, str);

// Release the memory pinned char array
    (*env)->ReleaseStringUTFChars(env, strObj, str);
}


long getPointer(JNIEnv* env, jobject obj) {
    jclass cls = (*env)->GetObjectClass(env, obj);

    if (cls == NULL) {
        exitWithError(JVM_ERROR, "class not found!");
    }

    jfieldID fidPointer = (*env)->GetFieldID(env, cls, "pointer", "J");
    if(fidPointer == NULL) {
        exitWithError(JVM_ERROR, "pointer filed not found!");
    }
    jlong value = (*env)->GetLongField(env, obj, fidPointer);
    return value;
}

void setPointer(JNIEnv* env, jobject obj, jlong val) {
    jclass cls = (*env)->GetObjectClass(env, obj);
    jfieldID fidPointer = (*env)->GetFieldID(env, cls, "pointer", "J");
    (*env)->SetLongField(env, obj, fidPointer, val);
}

void handleWasmEdgeResult(JNIEnv* env, WasmEdge_Result * result) {
    if(!WasmEdge_ResultOK(*result)) {
        char exceptionBuffer[1024];
        sprintf(exceptionBuffer, "Error occurred with message: %s.",
                WasmEdge_ResultGetMessage(*result));

        (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Exception"),
                         exceptionBuffer);
    }
}

int getIntVal(JNIEnv *env, jobject val) {
    jclass clazz = (*env)->GetObjectClass(env, val);
    jmethodID methodId = findJavaMethod(env, clazz, "getValue", "()I");

    jint value = (*env)->CallIntMethod(env, val, methodId);
    checkAndHandleException(env, "Error get int value");
    return value;
}

long getLongVal(JNIEnv *env, jobject val) {
    jclass clazz = (*env)->GetObjectClass(env, val);

    jmethodID methodId = (*env)->GetMethodID(env, clazz, "getValue", "()J");
    jlong value = (*env)->CallLongMethod(env, val, methodId);
    return value;
}

long getFloatVal(JNIEnv *env, jobject val) {
    jclass clazz = (*env)->GetObjectClass(env, val);
    jmethodID methodId = findJavaMethod(env, clazz, "getValue", "()F");
    jfloat value = (*env)->CallFloatMethod(env, val, methodId);
    return value;
}

double getDoubleVal(JNIEnv *env, jobject val) {
    jclass clazz = (*env)->GetObjectClass(env, val);
    jmethodID methodId = findJavaMethod(env, clazz, "getValue", "()D");
    jdouble value = (*env)->CallDoubleMethod(env, val, methodId);
    return value;
}

char* getStringVal(JNIEnv *env, jobject val) {
    jclass clazz = (*env)->GetObjectClass(env, val);

    if(clazz == NULL) {
        printf("class not found\n");
    }

    jmethodID methodId = findJavaMethod(env, clazz, "getValue", "()Ljava/lang/String;");

    if(methodId == NULL) {
        printf("method not found \n");
    }
    jstring value = (jstring)(*env)->CallObjectMethod(env, val, methodId);
    if(value == NULL)  {
        printf("value not found\n");
    }

    const char* c_str = (*env)->GetStringUTFChars(env, value, NULL);
    size_t len = (*env)->GetStringUTFLength(env, value);
    char * buf = malloc(sizeof(char) * len);

    memcpy(buf, c_str, len);

    (*env)->ReleaseStringUTFChars(env, val, c_str);
    return buf;
}


WasmEdge_Value *parseJavaParams(JNIEnv *env, jobjectArray params, jintArray paramTypes, jint paramSize) {

    WasmEdge_Value *wasm_params = calloc(paramSize, sizeof(WasmEdge_Value));
    int *type = (*env)->GetIntArrayElements(env, paramTypes, JNI_FALSE);
    for (int i = 0; i < paramSize; i++) {
        WasmEdge_Value val;

        jobject val_object = (*env)->GetObjectArrayElement(env, params, i);

        switch (type[i]) {

            case 0:
                val = WasmEdge_ValueGenI32(getIntVal(env, val_object));
                break;
            case 1:
                val = WasmEdge_ValueGenI64(getLongVal(env, val_object));
                break;
            case 2:
                val = WasmEdge_ValueGenF32(getFloatVal(env, val_object));
                break;
            case 3:
                val = WasmEdge_ValueGenF64(getDoubleVal(env, val_object));
                break;
            case 4:
                //TODO
                val = WasmEdge_ValueGenV128(getLongVal(env, val_object));
                break;
            case 5:
                //TODO
                val = WasmEdge_ValueGenFuncRef(getLongVal(env, val_object));
                break;
            case 6:
                //TODO
                val = WasmEdge_ValueGenExternRef(&val_object);
                break;
            default:
                break;
        }
        wasm_params[i] = val;
    }
}

enum WasmEdge_ValType *parseValueTypes(JNIEnv *env, jintArray jValueTypes) {
    jint len = (*env)->GetArrayLength(env, jValueTypes);
    enum WasmEdge_ValType* valTypes = malloc(len * sizeof(enum  WasmEdge_ValType));
    jint* elements = (*env)->GetIntArrayElements(env, jValueTypes, false);
    for (int i = 0; i < len; ++i) {
        valTypes[i] = elements[i];
    }
    return valTypes;
}

bool checkAndHandleException(JNIEnv *env, const char* msg) {
    if((*env)->ExceptionCheck(env)) {
        jthrowable e = (*env)->ExceptionOccurred(env);
        (*env)->ExceptionClear(env);

        jclass eclass = (*env)->GetObjectClass(env, e);

        jmethodID mid = (*env)->GetMethodID(env, eclass, "toString", "()Ljava/lang/String;");
        jstring jErrorMsg = (*env)->CallObjectMethod(env, e, mid);
        const char* cMsg = (*env)->GetStringUTFChars(env, jErrorMsg, NULL);


        (*env)->ReleaseStringUTFChars(env, jErrorMsg, cMsg);
        jclass newExcCls = (*env)->FindClass(env, "java/lang/RuntimeException");
        if (newExcCls == 0) { /* Unable to find the new exception class, give up. */
            return true;
        }
        (*env)->ThrowNew(env, newExcCls, msg);
        return true;
    }
    return false;
}


void setJavaValueObject(JNIEnv *env, WasmEdge_Value value, jobject j_val) {
    switch (value.Type) {
        case WasmEdge_ValType_I32:
            setJavaIntValue(env, value, j_val);
            break;
        case WasmEdge_ValType_I64:
        case WasmEdge_ValType_FuncRef:
        case WasmEdge_ValType_V128:
            setJavaLongValue(env, value, j_val);
            break;
        case WasmEdge_ValType_F32:
            setJavaFloatValue(env, value, j_val);
            break;
        case WasmEdge_ValType_F64:
            setJavaDoubleValue(env, value, j_val);
            break;
        case WasmEdge_ValType_ExternRef:
            setJavaStringValue(env, value, j_val);
            break;
        default:
            break;
    }
}



jobject WasmEdgeStringArrayToJavaList(JNIEnv* env, WasmEdge_String* wStrList, int32_t len) {
    jclass listClass = findJavaClass(env, "java/util/ArrayList");

    if (listClass == NULL) {
        return NULL;
    }

    jmethodID listConstructor = findJavaMethod(env, listClass, "<init>", "(I)V");

    if(listConstructor == NULL) {
        return NULL;
    }

    jobject jList = (*env)->NewObject(env, listClass, listConstructor, len);

    if(jList == NULL) {
        return NULL;
    }

    if(checkAndHandleException(env, "Error when creating java list\n")) {
        return NULL;
    }


    jmethodID addMethod = findJavaMethod(env, listClass, "add", "(Ljava/lang/Object;)Z");

    if(addMethod == NULL) {
        return NULL;
    }

    WasmEdge_String * ptr = wStrList;
    char buf[MAX_BUF_LEN];
    for (int i = 0; i < len; ++i) {


        memset(buf, 0, MAX_BUF_LEN);
        WasmEdge_StringCopy(*ptr, buf, MAX_BUF_LEN);
        jobject jStr = (*env)->NewStringUTF(env, buf);

        (*env)->CallBooleanMethod(env, jList, addMethod, jStr);

        if(checkAndHandleException(env, "Error when adding jstr\n")) {
            return NULL;
        }

        ptr++;
    }

    return jList;

}

jstring WasmEdgeStringToJString(JNIEnv* env, WasmEdge_String wStr) {
     char buf[MAX_BUF_LEN];
     memset(buf, 0, MAX_BUF_LEN);
    WasmEdge_StringCopy(wStr, buf, MAX_BUF_LEN);

    jobject jStr = (*env)->NewStringUTF(env, buf);

    return jStr;
}

jobject CreateJavaArrayList(JNIEnv* env, jint len) {
    jclass listClass = findJavaClass(env, "java/util/ArrayList");

    if (listClass == NULL) {
        return NULL;
    }

    jmethodID listConstructor = findJavaMethod(env, listClass, "<init>", "(I)V");

    if(listConstructor == NULL) {
        return NULL;
    }

    jobject jList = (*env)->NewObject(env, listClass, listConstructor, len);

    if(jList == NULL) {
        return NULL;
    }

    if(checkAndHandleException(env, "Error when creating java list\n")) {
        return NULL;
    }
    return jList;
}

bool AddElementToJavaList(JNIEnv* env, jobject jList, jobject ele) {
    jclass listClass = findJavaClass(env, "java/util/ArrayList");

    if (listClass == NULL) {
        return false;
    }

    jmethodID addMethod = findJavaMethod(env, listClass, "add", "(Ljava/lang/Object;)Z");

    return (*env)->CallBooleanMethod(env, jList, addMethod, ele);
}

WasmEdge_String JStringToWasmString(JNIEnv* env, jstring jstr) {
     uint32_t len = (*env)->GetStringUTFLength(env, jstr);
     const char* strPtr = (*env)->GetStringUTFChars(env, jstr, NULL);

    WasmEdge_String wStr = WasmEdge_StringCreateByBuffer(strPtr, len);

    (*env)->ReleaseStringUTFChars(env, jstr, strPtr);

    return wStr;
}

const char** JStringArrayToPtr(JNIEnv* env, jarray jStrArray) {
    int len = (*env)->GetArrayLength(env, jStrArray);

    const char** ptr = malloc(sizeof(char*));

    for(int i = 0; i < len; i++) {
        jstring  jStr = (*env)->GetObjectArrayElement(env, jStrArray, i);
        const* strPtr = (*env)->GetStringUTFChars(env, jStr, NULL);
        ptr[i] = strPtr;
    }
    return ptr;
}

void ReleaseCString(JNIEnv* env, jarray jStrArray, const char** ptr) {
    int len = (*env)->GetArrayLength(env, jStrArray);

    for(int i = 0; i < len; i++) {
        jstring  jStr = (*env)->GetObjectArrayElement(env, jStrArray, i);
        (*env)->ReleaseStringUTFChars(env, jStr, ptr[i]);
    }
}