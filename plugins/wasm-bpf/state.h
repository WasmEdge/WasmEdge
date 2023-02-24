#ifndef _WASM_BPF_STATE
#define _WASM_BPF_STATE

#include <cinttypes>
#include <map>
#include <memory>
#include <shared_mutex>
#include "bpf-api.h"
struct WasmBpfState {
    std::map<handle_t, wasm_bpf_program*> handles;
    handle_t next_handle = 1;
    std::shared_mutex lock;
    virtual ~WasmBpfState() {
        for (auto p = handles.begin(); p != handles.end(); p++) {
            delete p->second;
        }
    }
};

using state_t = std::shared_ptr<WasmBpfState>;

#endif