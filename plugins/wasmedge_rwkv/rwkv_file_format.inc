// Data types

#define TYPE_UNKNOWN TYPE_COUNT

enum rwkv_type {
    TYPE_FP32,
    TYPE_FP16,
    TYPE_Q4_0,
    TYPE_Q4_1,
    TYPE_Q4_1_O, // Unsupported
    TYPE_Q4_2, // Unsupported
    TYPE_Q4_3, // Unsupported
    TYPE_Q5_0,
    TYPE_Q5_1,
    TYPE_Q8_0,
    TYPE_Q8_1,
    TYPE_Q2_K,
    TYPE_Q3_K,
    TYPE_Q4_K,
    TYPE_Q5_K,
    TYPE_Q6_K,
    TYPE_Q8_K,
    TYPE_COUNT
};

#define GGML_TYPE_UNKNOWN GGML_TYPE_COUNT

static const enum ggml_type rwkv_type_to_ggml[TYPE_COUNT + 1] = {
    GGML_TYPE_F32,     /* FP32   */
    GGML_TYPE_F16,     /* FP16   */
    GGML_TYPE_Q4_0,    /* Q4_0   */
    GGML_TYPE_Q4_1,    /* Q4_1   */
    GGML_TYPE_UNKNOWN, /* Q4_1_O */
    GGML_TYPE_UNKNOWN, /* Q4_2   */
    GGML_TYPE_UNKNOWN, /* Q4_3   */
    GGML_TYPE_Q5_0,    /* Q5_0   */
    GGML_TYPE_Q5_1,    /* Q5_1   */
    GGML_TYPE_Q8_0,    /* Q8_0   */
    GGML_TYPE_Q8_1,    /* Q8_1   */
    GGML_TYPE_Q2_K,    /* Q2_K   */
    GGML_TYPE_Q3_K,    /* Q3_K   */
    GGML_TYPE_Q4_K,    /* Q4_K   */
    GGML_TYPE_Q5_K,    /* Q5_K   */
    GGML_TYPE_Q6_K,    /* Q6_K   */
    GGML_TYPE_Q8_K,    /* Q8_K   */
    GGML_TYPE_COUNT    /* COUNT  */
};

static const enum rwkv_type rwkv_type_from_ggml[GGML_TYPE_COUNT + 1] = {
    TYPE_FP32,   /* FP32  */
    TYPE_FP16,   /* FP16  */
    TYPE_Q4_0,   /* Q4_0  */
    TYPE_Q4_1,   /* Q4_1  */
    TYPE_Q4_2,   /* Q4_2  */
    TYPE_Q4_3,   /* Q4_3  */
    TYPE_Q5_0,   /* Q5_0  */
    TYPE_Q5_1,   /* Q5_1  */
    TYPE_Q8_0,   /* Q8_0  */
    TYPE_Q8_1,   /* Q8_1  */
    TYPE_Q2_K,   /* Q2_K  */
    TYPE_Q3_K,   /* Q3_K  */
    TYPE_Q4_K,   /* Q4_K  */
    TYPE_Q5_K,   /* Q5_K  */
    TYPE_Q6_K,   /* Q6_K  */
    TYPE_Q8_K,   /* Q8_K  */
    TYPE_COUNT,  /* COUNT */
};

static const char * rwkv_type_to_string[TYPE_COUNT + 1] = {
    "FP32",
    "FP16",
    "Q4_0",
    "Q4_1",
    "Q4_1_O",
    "Q4_2",
    "Q4_3",
    "Q5_0",
    "Q5_1",
    "Q8_0",
    "Q8_1",
    "Q2_K",
    "Q3_K",
    "Q4_K",
    "Q5_K",
    "Q6_K",
    "Q8_K",
    "unknown"
};

static enum rwkv_type rwkv_type_from_string(const char * str) {
    for (int i = 0; i < TYPE_COUNT; i++) {
        if (strcmp(str, rwkv_type_to_string[i]) == 0) {
            return (enum rwkv_type) i;
        }
    }

    return TYPE_UNKNOWN;
}

// rwkv_file_header

struct rwkv_file_header {
    uint32_t magic;
    uint32_t version;
    uint32_t n_vocab;
    uint32_t n_embed;
    uint32_t n_layer;
    uint32_t data_type;
};

static bool rwkv_is_file_version_in_range(const uint32_t version) {
    return version >= RWKV_FILE_VERSION_MIN && version <= RWKV_FILE_VERSION_MAX;
}

static bool rwkv_fread_file_header(FILE * file, struct rwkv_file_header & header) {
    RWKV_ASSERT_FALSE(RWKV_ERROR_FILE_READ, rwkv_fread_data(file, sizeof(struct rwkv_file_header), &header));
    RWKV_ASSERT_FALSE(RWKV_ERROR_FILE_MAGIC, header.magic == RWKV_FILE_MAGIC);
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_FILE_VERSION, rwkv_is_file_version_in_range(header.version), "Unsupported file version %" PRId32, header.version);
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_DATA_TYPE, header.data_type < TYPE_COUNT, "Model data type out of range (%" PRId32 " > %" PRId32 ")", header.data_type, TYPE_COUNT - 1);

    enum ggml_type ggml_type = rwkv_type_to_ggml[header.data_type];

    RWKV_ASSERT_FALSE_MSG(
        RWKV_ERROR_DATA_TYPE,
        ggml_type != GGML_TYPE_UNKNOWN,
        "Models in %s format cannot be loaded anymore because the format was removed.\n"
        "You need to quantize the model into another format or use an older version of rwkv.cpp.\n"
        "See https://github.com/saharNooby/rwkv.cpp#compatibility for more info",
        rwkv_type_to_string[header.data_type]
    );

    RWKV_ASSERT_FALSE_MSG(
        RWKV_ERROR_DATA_TYPE,
        (!ggml_is_quantized(ggml_type) || header.version == RWKV_FILE_VERSION_1),
        "The quantized model file in %s format was created with an old version of rwkv.cpp and can not be loaded anymore.\n"
        "You need to requantize the model or use an older version of rwkv.cpp.\n"
        "See https://github.com/saharNooby/rwkv.cpp#compatibility for more info",
        rwkv_type_to_string[header.data_type]
    );

    return true;
}

static bool rwkv_fwrite_file_header(FILE * file, const struct rwkv_file_header & header) {
    RWKV_ASSERT_FALSE(RWKV_ERROR_FILE_WRITE, rwkv_fwrite_data(file, &header, sizeof(struct rwkv_file_header)));

    return true;
}

// rwkv_tensor_header

struct rwkv_tensor_header {
    uint32_t dim_count;
    uint32_t key_length;
    uint32_t data_type;
    uint32_t size0;
    uint32_t size1;
    uint32_t size2;

    size_t size() const;
};

size_t rwkv_tensor_header::size() const {
    return rwkv_tensor_nbytes(rwkv_type_to_ggml[this->data_type], this->size0, this->size1, this->size2);
}

static bool rwkv_fread_tensor_header(FILE * file, struct rwkv_tensor_header & header) {
    RWKV_ASSERT_FALSE(RWKV_ERROR_FILE_READ, rwkv_fread_data(file, sizeof(struct rwkv_tensor_header) - sizeof(uint32_t) * 2, &header));
    header.size1 = 1;
    header.size2 = 1;

    RWKV_ASSERT_FALSE_MSG(
        RWKV_ERROR_SHAPE,
        header.dim_count == 1 || header.dim_count == 2 || header.dim_count == 3,
        "Tensor has an invalid shape (%" PRId32 " dimensions)",
        header.dim_count
    );

    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_DATA_TYPE, header.data_type < TYPE_COUNT, "Tensor data type out of range (%" PRId32 " > %" PRId32 ")", header.data_type, TYPE_COUNT - 1);

    RWKV_ASSERT_FALSE_MSG(
        RWKV_ERROR_DATA_TYPE,
        rwkv_type_to_ggml[header.data_type] != GGML_TYPE_UNKNOWN,
        "Tensor data type (%s) is no longer supported",
        rwkv_type_to_string[header.data_type]
    );

    if (header.dim_count >= 2) {
        RWKV_ASSERT_FALSE(RWKV_ERROR_FILE_READ, rwkv_fread_uint32(file, header.size1));
    }

    if (header.dim_count >= 3) {
        RWKV_ASSERT_FALSE(RWKV_ERROR_FILE_READ, rwkv_fread_uint32(file, header.size2));
    }

    return true;
}

static bool rwkv_fwrite_tensor_header(FILE * file, const struct rwkv_tensor_header & header) {
    size_t sub;

    if (header.dim_count == 1) {
        sub = sizeof(uint32_t) * 2;
    } else if (header.dim_count == 2) {
        sub = sizeof(uint32_t);
    } else {
        sub = 0;
    }

    RWKV_ASSERT_FALSE(RWKV_ERROR_FILE_WRITE, rwkv_fwrite_data(file, &header, sizeof(struct rwkv_tensor_header) - sub));

    return true;
}

static bool rwkv_fread_tensor_header_skip_name_and_data(FILE * file, struct rwkv_tensor_header & header) {
    RWKV_ENSURE_OR_FALSE(rwkv_fread_tensor_header(file, header));

    RWKV_ASSERT_FALSE(RWKV_ERROR_DATA, fseek(file, header.key_length + header.size(), SEEK_CUR) == 0);

    return true;
}

// rwkv_tensor

struct rwkv_tensor {
    struct rwkv_tensor_header header;
    std::string name;
    uint8_t * data;
};

static bool rwkv_fwrite_tensor(FILE * file, const struct rwkv_tensor & tensor) {
    RWKV_ENSURE_OR_FALSE(rwkv_fwrite_tensor_header(file, tensor.header));
    RWKV_ENSURE_OR_FALSE(rwkv_fwrite_string(file, tensor.name));
    RWKV_ENSURE_OR_FALSE(rwkv_fwrite_data(file, tensor.data, tensor.header.size()));
    return true;
}

// Reading ggml tensors

static bool rwkv_fread_ggml_tensor_info(FILE * file, struct ggml_context * ctx, std::string & name, struct ggml_tensor *& tensor) {
    struct rwkv_tensor_header header;
    RWKV_ENSURE_OR_FALSE_MSG(rwkv_fread_tensor_header(file, header), "Invalid tensor header");

    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_FILE_READ, rwkv_fread_string(file, header.key_length, name), "Failed to read tensor name");

    enum ggml_type ggml_type = rwkv_type_to_ggml[header.data_type];
    RWKV_ASSERT_FALSE_MSG(
        RWKV_ERROR_UNSUPPORTED,
        ggml_type != GGML_TYPE_UNKNOWN,
        "Unsupported data type %s in parameter %s",
        rwkv_type_to_string[header.data_type],
        name.c_str()
    );

    if (header.dim_count == 1) {
        tensor = ggml_new_tensor_1d(ctx, ggml_type, header.size0);
    } else if (header.dim_count == 2) {
        tensor = ggml_new_tensor_2d(ctx, ggml_type, header.size0, header.size1);
    } else {
        tensor = ggml_new_tensor_3d(ctx, ggml_type, header.size0, header.size1, header.size2);
    }

    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_ALLOC, tensor != NULL, "Failed to allocate tensor");

    ggml_set_name(tensor, name.c_str());

    RWKV_ASSERT_FALSE_MSG(
        RWKV_ERROR_FILE_READ,
        !fseek(file, rwkv_tensor_nbytes(tensor), SEEK_CUR),
        "Failed to seek to next tensor after parameter %s",
        name.c_str()
    );

    return true;
}

static bool rwkv_fread_ggml_tensor_data(FILE * file, std::unordered_map<std::string, struct ggml_tensor *> & parameters) {
    struct rwkv_tensor_header header;
    std::string name;
    RWKV_ENSURE_OR_FALSE_MSG(rwkv_fread_tensor_header(file, header), "Invalid tensor header");

    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_FILE_READ, rwkv_fread_string(file, header.key_length, name), "Failed to read tensor name");

    enum ggml_type ggml_type = rwkv_type_to_ggml[header.data_type];
    RWKV_ASSERT_FALSE_MSG(
        RWKV_ERROR_UNSUPPORTED,
        ggml_type != GGML_TYPE_UNKNOWN,
        "Unsupported data type %s in parameter %s",
        rwkv_type_to_string[header.data_type],
        name.c_str()
    );

    struct ggml_tensor * tensor;
    tensor = parameters[name];
    RWKV_ASSERT_FALSE_MSG(
        RWKV_ERROR_ALLOC,
        tensor != NULL,
        "Parameter %s not found in the model",
        name.c_str()
    );

    char * data = (char *) malloc(rwkv_tensor_nbytes(tensor));

    RWKV_ASSERT_FALSE_MSG(
        RWKV_ERROR_FILE_READ,
        rwkv_fread_data(file, rwkv_tensor_nbytes(tensor), data),
        "Failed to read data of parameter %s",
        name.c_str()
    );

    ggml_backend_tensor_set(tensor, data, 0, rwkv_tensor_nbytes(tensor));

    free(data);

    return true;
}
