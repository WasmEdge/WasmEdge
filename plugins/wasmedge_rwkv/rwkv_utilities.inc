static size_t rwkv_tensor_nbytes(const enum ggml_type type, const int64_t size0, const int64_t size1, const int64_t size2) {
    return (ggml_type_size(type) * size0 * size1 * size2) / ggml_blck_size(type);
}

// For some reason, ggml_nbytes calculates the size in a way
// incompatible with rwkv.cpp; we need our own function for that.
static size_t rwkv_tensor_nbytes(const struct ggml_tensor * tensor) {
    return rwkv_tensor_nbytes(tensor->type, tensor->ne[0], tensor->ne[1], tensor->ne[2]);
}

// Minimum amount of memory required for a ggml context, not counting the tensor data.
static size_t rwkv_ggml_overhead() {
    return ggml_tensor_overhead() * RWKV_MAX_NODES + ggml_graph_overhead();
}

static struct ggml_context * rwkv_init_ggml_context(const size_t memory_size, const bool no_alloc) {
    struct ggml_init_params init_params = {
        memory_size,
        NULL,
        no_alloc
    };

    return ggml_init(init_params);
}

// IO utilities

// Reads a single uint32 value from a file.
static bool rwkv_fread_uint32(FILE * file, uint32_t & dest) {
    return fread((void *) &dest, sizeof(uint32_t), 1, file) == 1;
}

// Reads a single string value from a file.
static bool rwkv_fread_string(FILE * file, const size_t length, std::string & dest) {
    char * buffer = new(std::nothrow) char[length];
    if (!buffer) {
        return false;
    }
    int ret = fread(buffer, length, 1, file);
    if (ret == 1) {
        dest.assign(buffer, length);
    }
    delete[] buffer;
    return ret == 1;
}

// Reads a single data buffer from a file.
static bool rwkv_fread_data(FILE * file, const size_t length, void * dest) {
    return fread(dest, length, 1, file) == 1;
}

// Writes a single string value to a file.
static bool rwkv_fwrite_string(FILE * file, const std::string & value) {
    return fwrite((const void *) value.data(), value.length(), 1, file) == 1;
}

// Writes a single data buffer to a file.
static bool rwkv_fwrite_data(FILE * file, const void * data, const size_t length) {
    return fwrite(data, length, 1, file) == 1;
}
