#include <jni.h>
#include <string>
#include <array>
#include <wasmedge/wasmedge.h>

extern "C"
JNIEXPORT jint JNICALL
Java_org_wasmedge_native_1lib_NativeLib_nativeWasmFibonacci(JNIEnv *env, jobject, jbyteArray image_bytes,
                                                            jint idx) {
    jsize buffer_size = env->GetArrayLength(image_bytes);
    jbyte *buffer = env->GetByteArrayElements(image_bytes, nullptr);

    WasmEdge_ConfigureContext *conf = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureAddHostRegistration(conf, WasmEdge_HostRegistration_Wasi);

    WasmEdge_VMContext *vm_ctx = WasmEdge_VMCreate(conf, nullptr);

    const WasmEdge_String &func_name = WasmEdge_StringCreateByCString("fib");
    std::array<WasmEdge_Value, 1> params{WasmEdge_ValueGenI32(idx)};
    std::array<WasmEdge_Value, 1> ret_val{};

    const WasmEdge_Result &res = WasmEdge_VMRunWasmFromBuffer(vm_ctx, (uint8_t *) buffer,
                                                              buffer_size,
                                                              func_name, params.data(), params.size(),
                                                              ret_val.data(), ret_val.size());

    WasmEdge_VMDelete(vm_ctx);
    WasmEdge_ConfigureDelete(conf);
    WasmEdge_StringDelete(func_name);

    env->ReleaseByteArrayElements(image_bytes, buffer, 0);
    if (!WasmEdge_ResultOK(res)) {
        return -1;
    }
    return WasmEdge_ValueGetI32(ret_val[0]);
}
