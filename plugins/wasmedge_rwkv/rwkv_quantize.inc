static bool rwkv_tensor_needs_quant(std::string name) {
    return name != "emb.weight" &&
            name != "head.weight" &&
            name.find("att.v1") == std::string::npos &&
            name.find("att.v2") == std::string::npos &&
            name.find("att.g1") == std::string::npos &&
            name.find("att.g2") == std::string::npos &&
            name.find("att.a1") == std::string::npos &&
            name.find("att.a2") == std::string::npos &&
            name.find("att.w1") == std::string::npos &&
            name.find("att.w2") == std::string::npos &&
            name.find("att.r_k") == std::string::npos;
}

// API function.
bool rwkv_quantize_model_file(const char * in_path, const char * out_path, const char * type_name) {
    global_last_error = RWKV_ERROR_NONE;

    enum ggml_type out_type = rwkv_type_to_ggml[rwkv_type_from_string(type_name)];
    RWKV_ASSERT_FALSE_MSG(
        RWKV_ERROR_ARGS | RWKV_ERROR_DATA_TYPE,
        ggml_is_quantized(out_type),
        "Unsupported output data type (%s)",
        rwkv_type_to_string[rwkv_type_from_ggml[out_type]]
    );

    RWKV_MSG("Loading model from '%s'\n", in_path);

    struct stat in_stat;

    struct rwkv_file in_file(fopen(in_path, "rb"));
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_FILE | RWKV_ERROR_FILE_OPEN, in_file.file, "Failed to open %s for reading", in_path);

    // Be very careful when changing this code. It must support files larger than 2 GB by using 64-bit functions to the get file length.
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_FILE | RWKV_ERROR_FILE_STAT, fstat(fileno(in_file.file), &in_stat) == 0, "failed to stat file %s", in_path);

    struct rwkv_file out_file(fopen(out_path, "wb"));
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_FILE | RWKV_ERROR_FILE_OPEN, out_file.file, "Failed to open %s for writing", out_path);

    struct rwkv_file_header in_header;
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_FILE, rwkv_fread_file_header(in_file.file, in_header), "Invalid file header");

    enum ggml_type in_type = rwkv_type_to_ggml[in_header.data_type];
    RWKV_ASSERT_FALSE_MSG(
        RWKV_ERROR_FILE,
        in_type == GGML_TYPE_F32 || in_type == GGML_TYPE_F16,
        "Unsupported input data type (%s); needs to be FP32 or FP16",
        rwkv_type_to_string[rwkv_type_from_ggml[in_type]]
    );

    struct rwkv_file_header out_header = in_header;
    out_header.version = RWKV_FILE_VERSION;
    out_header.data_type = rwkv_type_from_ggml[out_type];
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_FILE, rwkv_fwrite_file_header(out_file.file, out_header), "Failed to write file header");

    // Process parameters.
    size_t orig_total_size = 0;
    size_t new_total_size = 0;

    // Required to init the F16 tables.
    // Doesn't crash if ggml_init fails.
    ggml_free(ggml_init({ 0, NULL, true }));

    size_t max_in_size = 0;
    size_t max_out_size = 0;
    size_t max_key_length = 0;

    while (ftell(in_file.file) < in_stat.st_size) {
        struct rwkv_tensor_header header;
        RWKV_ASSERT_FALSE(RWKV_ERROR_FILE, rwkv_fread_tensor_header_skip_name_and_data(in_file.file, header));

        size_t in_size = header.size();

        if (in_size > max_in_size) {
            max_in_size = in_size;
        }

        if (header.data_type == TYPE_FP16) {
            if (in_size > max_out_size) {
                max_out_size = in_size;
            }

            size_t f32_size = rwkv_tensor_nbytes(GGML_TYPE_F32, header.size0, header.size1, header.size2);

            if (f32_size > max_in_size) {
                max_in_size = f32_size;
            }
        }

        size_t out_size = rwkv_tensor_nbytes(out_type, header.size0, header.size1, header.size2);

        if (out_size > max_out_size) {
            max_out_size = out_size;
        }

        if (header.key_length > max_key_length) {
            max_key_length = header.key_length;
        }
    }

    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_FILE | RWKV_ERROR_FILE_READ, fseek(in_file.file, sizeof(struct rwkv_file_header), SEEK_SET) == 0, "Failed to seek in file");

    std::unique_ptr<uint8_t[]> scratch(new(std::nothrow) uint8_t[max_in_size + max_out_size]);
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_ALLOC, scratch.get(), "Failed to allocate buffer");

    uint8_t * in_buf = scratch.get();
    uint8_t * out_buf = in_buf + max_in_size;

    struct rwkv_tensor tensor;
    struct rwkv_tensor_header & header = tensor.header;
    std::string & name = tensor.name;
    uint8_t *& data = tensor.data;

    while (ftell(in_file.file) < in_stat.st_size) {
        RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_MODEL_PARAMS, rwkv_fread_tensor_header(in_file.file, header), "Failed to read tensor header");
        RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_MODEL_PARAMS, rwkv_fread_string(in_file.file, header.key_length, name), "Failed to read tensor name");

        const char * name_str = name.c_str();
        RWKV_MSG(
            "%*s - [%5" PRId32 ", %5" PRId32 ", %5" PRId32 "], type = %6s ",
            (int) max_key_length,
            name_str,
            header.size0,
            header.size1,
            header.size2,
            rwkv_type_to_string[header.data_type]
        );

        data = header.data_type == TYPE_FP16 ? out_buf : in_buf;
        size_t orig_size = header.size(), new_size = orig_size;
        RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_MODEL_PARAMS, rwkv_fread_data(in_file.file, orig_size, data), "\nFailed to read tensor data of %s", name_str);

        // Quantize only 2D tensors, except embedding and head matrices.
        // Embedding and head take not too much space, especially in bigger models;
        // but they significantly increase perplexity when quantized.
        // In RWKV v5, time_decay and time_first/time_faaaa are 3D tensors, so they are not quantized.
        if ((header.data_type == TYPE_FP32 || header.data_type == TYPE_FP16) &&
            header.dim_count == 2 &&
            rwkv_tensor_needs_quant(name)
        ) {
            RWKV_MSG("-> %6s ", rwkv_type_to_string[rwkv_type_from_ggml[out_type]]);

            size_t nelements = (size_t) header.size0 * (size_t) header.size1 * (size_t) header.size2;

            if (header.data_type == TYPE_FP16) {
                ggml_fp16_to_fp32_row((const ggml_fp16_t *) out_buf, (float *) in_buf, nelements);
            }

            new_size = ggml_quantize_chunk(out_type, (const float *) in_buf, out_buf, 0, header.size1, header.size0, NULL);
            header.data_type = rwkv_type_from_ggml[out_type];
            data = out_buf;

            RWKV_MSG("size = %8.2f MB -> %8.2f MB", orig_size / 1024.0 / 1024.0, new_size / 1024.0 / 1024.0);

            RWKV_MSG("\n");
        } else {
            RWKV_MSG("size = %8.3f MB\n", orig_size / 1024.0 / 1024.0);
        }

        RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_FILE_WRITE, rwkv_fwrite_tensor(out_file.file, tensor), "Failed to write tensor %s", name_str);
        orig_total_size += orig_size;
        new_total_size += new_size;
    }

    RWKV_MSG("original size     = %8.2f MB\n", orig_total_size / 1024.0 / 1024.0);
    RWKV_MSG("quantized size    = %8.2f MB\n", new_total_size / 1024.0 / 1024.0);
    RWKV_MSG("compression ratio = %8.2f\n", orig_total_size / float(new_total_size));


    return true;
}
