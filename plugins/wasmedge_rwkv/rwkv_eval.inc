// Copies state from an input buffer to the ggml tensor of the graph.
static void rwkv_set_inputs(const struct rwkv_context * ctx, const struct rwkv_computation_graph & graph, const float * state_in) {
    if (state_in) {
        ggml_backend_tensor_set(graph.input_state, state_in, 0, rwkv_tensor_nbytes(graph.input_state));
    } else {
        float * state_data = (float *) malloc(rwkv_tensor_nbytes(graph.input_state));
        rwkv_init_state(ctx, state_data);
        ggml_backend_tensor_set(graph.input_state, state_data, 0, rwkv_tensor_nbytes(graph.input_state));
        free(state_data);
    }
}

// Copies state and logits from ggml tensors of the graph to output buffers.
static void rwkv_get_outputs(const struct rwkv_computation_graph & graph, float * state_out, float * logits_out) {
    if (state_out) {
        ggml_backend_tensor_get(graph.output_state, state_out, 0, rwkv_tensor_nbytes(graph.output_state));
    }

    if (logits_out) {
        ggml_backend_tensor_get(graph.logits, logits_out, 0, rwkv_tensor_nbytes(graph.logits));
    }
}

// Evaluates a computation graph, optionally skipping logit computation.
static void rwkv_eval_graph(struct rwkv_computation_graph & graph, const bool compute_logits) {
    if (!compute_logits) {
        graph.cgraph->n_nodes = graph.pre_logits_nodes;
        graph.cgraph->n_leafs = graph.pre_logits_leafs;
    } else {
        graph.cgraph->n_nodes = graph.post_logits_nodes;
        graph.cgraph->n_leafs = graph.post_logits_leafs;
    }

    ggml_backend_sched_graph_compute(graph.sched, graph.cgraph);
}

// API function.
bool rwkv_eval(struct rwkv_context * ctx, const uint32_t token, const float * state_in, float * state_out, float * logits_out) {
    ctx->last_error = RWKV_ERROR_NONE;

    const struct rwkv_file_header & header = ctx->model->header;
    const size_t n_vocab = header.n_vocab;
    RWKV_CTX_ASSERT_FALSE_MSG(ctx, RWKV_ERROR_ARGS, token < n_vocab, "Token (%" PRId32 ") is out of range (0 .. %zu)", token, n_vocab - 1);

    if (!ctx->serial_graph.sched) {
        ctx->serial_graph.sched = ggml_backend_sched_new(ctx->model->backends.data(), NULL, ctx->model->backends.size(), RWKV_MAX_NODES, false);

        auto graph = ctx->serial_graph.cgraph;
        for (int i = 0; i < graph->n_nodes; i++) {
            auto node = graph->nodes[i];
            if (std::string(node->name).find(".in.") != std::string::npos ||
                std::string(node->name).find(".out.") != std::string::npos) {
                ggml_backend_sched_set_tensor_backend(ctx->serial_graph.sched, node, ctx->model->backends.back());
            }
        }
        for (int i = 0; i < graph->n_leafs; i++) {
            auto leaf = graph->leafs[i];
            if (std::string(leaf->name).find("state.in") != std::string::npos ||
                std::string(leaf->name).find("state.out") != std::string::npos) {
                ggml_backend_sched_set_tensor_backend(ctx->serial_graph.sched, leaf, ctx->model->backends.back());
            }
        }
        ggml_backend_sched_set_tensor_backend(ctx->serial_graph.sched, ctx->serial_graph.tokens, ctx->model->backends.back());

        ggml_backend_sched_alloc_graph(ctx->serial_graph.sched, ctx->serial_graph.cgraph);
    }

    rwkv_set_inputs(ctx, ctx->serial_graph, state_in);
    ggml_backend_tensor_set(ctx->serial_graph.tokens, &token, 0, rwkv_tensor_nbytes(ctx->serial_graph.tokens));

    rwkv_eval_graph(ctx->serial_graph, logits_out != NULL);

    rwkv_get_outputs(ctx->serial_graph, state_out, logits_out);

    return true;
}

// API function.
bool rwkv_eval_sequence(
    struct rwkv_context * ctx,
    const uint32_t * sequence,
    const size_t sequence_len,
    const float * state_in,
    float * state_out,
    float * logits_out
) {
    ctx->last_error = RWKV_ERROR_NONE;

    RWKV_CTX_ASSERT_FALSE_MSG(ctx, RWKV_ERROR_ARGS, sequence_len > 0, "Sequence length is 0");

    if (sequence_len == 1) {
        // Avoid building single-token sequence graph, we already have regular eval for this.
        return rwkv_eval(
            ctx,
            sequence[0],
            state_in,
            state_out,
            logits_out
        );
    }

    if (sequence) {
        const size_t n_vocab = ctx->model->header.n_vocab;

        for (size_t i = 0; i < sequence_len; i++) {
            const uint32_t token = sequence[i];

            RWKV_CTX_ASSERT_FALSE_MSG(ctx, RWKV_ERROR_ARGS, token < n_vocab, "Token at index %zu (%" PRId32 ") is out of range (0 .. %zu)", i, token, n_vocab - 1);
        }
    }

    if (ctx->last_used_sequence_length != sequence_len) {
        if (ctx->sequential_graph.sched) {
            ggml_backend_sched_free(ctx->sequential_graph.sched);
            ctx->sequential_graph.sched = NULL;
        }
        RWKV_ENSURE_OR_FALSE(rwkv_measure_and_build_sequential_context(*ctx->model, ctx->sequential_graph, sequence_len));

        ctx->last_used_sequence_length = sequence_len;
    }

    if (sequence) {
        if (!ctx->sequential_graph.sched) {
            ctx->sequential_graph.sched = ggml_backend_sched_new(ctx->model->backends.data(), NULL, ctx->model->backends.size(), RWKV_MAX_NODES, false);
            auto graph = ctx->sequential_graph.cgraph;

            for (int i = 0; i < graph->n_nodes; i++) {
                auto node = graph->nodes[i];
                if (std::string(node->name).find(".in.") != std::string::npos ||
                    std::string(node->name).find(".out.") != std::string::npos) {
                    ggml_backend_sched_set_tensor_backend(ctx->sequential_graph.sched, node, ctx->model->backends.back());
                }
            }
            for (int i = 0; i < graph->n_leafs; i++) {
                auto leaf = graph->leafs[i];
                if (std::string(leaf->name).find("state.in") != std::string::npos ||
                    std::string(leaf->name).find("state.out") != std::string::npos) {
                    ggml_backend_sched_set_tensor_backend(ctx->sequential_graph.sched, leaf, ctx->model->backends.back());
                }
            }
            ggml_backend_sched_set_tensor_backend(ctx->sequential_graph.sched, ctx->sequential_graph.tokens, ctx->model->backends.back());

            ggml_backend_sched_alloc_graph(ctx->sequential_graph.sched, ctx->sequential_graph.cgraph);
        }

        rwkv_set_inputs(ctx, ctx->sequential_graph, state_in);
        ggml_backend_tensor_set(ctx->sequential_graph.tokens, sequence, 0, sequence_len * sizeof(uint32_t));

        rwkv_eval_graph(ctx->sequential_graph, logits_out != NULL);

        rwkv_get_outputs(ctx->sequential_graph, state_out, logits_out);
    }

    return true;
}

// API function.
bool rwkv_eval_sequence_in_chunks(
    struct rwkv_context * ctx,
    const uint32_t * tokens,
    const size_t sequence_len,
    const size_t chunk_size,
    const float * state_in,
    float * state_out,
    float * logits_out
) {
    RWKV_CTX_ASSERT_FALSE_MSG(ctx, RWKV_ERROR_ARGS, sequence_len > 0, "Sequence length is 0");
    RWKV_CTX_ASSERT_FALSE_MSG(ctx, RWKV_ERROR_ARGS, chunk_size > 0, "Chunk size is 0");

    // Will be de-allocated automatically on return.
    std::unique_ptr<float[]> state{ new(std::nothrow) float[rwkv_get_state_len(ctx)] };

    if (state_in != NULL) {
        memcpy(state.get(), state_in, rwkv_get_state_len(ctx) * sizeof(float));
    } else {
        rwkv_init_state(ctx, state.get());
    }

    size_t chunk_count = sequence_len / chunk_size;
    size_t remainder = sequence_len % chunk_size;
    const uint32_t * tokens_offset = tokens;

    for (size_t c = 0; c < chunk_count; c++) {
        bool is_last_eval = c == chunk_count - 1 && remainder == 0;

        bool result = rwkv_eval_sequence(
            ctx,
            tokens_offset,
            chunk_size,
            state.get(),
            // On the last eval call, copy the state into the user-provided buffer.
            is_last_eval ? state_out : state.get(),
            // If this is not the last call, we don't have the use for logits and can skip their calculation.
            is_last_eval ? logits_out : NULL
        );

        if (!result) {
            return false;
        }

        tokens_offset += chunk_size;
    }

    if (remainder > 0) {
        bool result = rwkv_eval_sequence(
            ctx,
            tokens_offset,
            remainder,
            state.get(),
            // This eval call is always the last.
            state_out,
            logits_out
        );

        if (!result) {
            return false;
        }
    }

    return true;
}

// API function.
void rwkv_init_state(const struct rwkv_context * ctx, float * state) {
    memset(state, 0, rwkv_get_state_len(ctx) * sizeof(float));

    if (ctx->model->arch_version_major >= 5) {
        return;
    }

    const struct rwkv_file_header & header = ctx->model->header;
    const size_t layer_size = (size_t) header.n_embed * 5;
    const size_t layer_zero = (size_t) header.n_embed * 4;
    const size_t layers_size = (size_t) header.n_layer * layer_size;

    for (size_t start = 0; start < layers_size; start += layer_size) {
        for (size_t i = layer_zero; i < layer_size; i++) {
            state[start + i] = -1e30F;
        }
    }
}
