// View tensors of a state of a single layer.
struct rwkv_layer_state {
    struct ggml_tensor * ffn_xx;
    struct ggml_tensor * att_xx;
    // Used in RWKV v4.
    struct ggml_tensor * att_aa;
    struct ggml_tensor * att_bb;
    struct ggml_tensor * att_pp;
    // Used in RWKV v5+.
    struct ggml_tensor * att_heads;
};


// The computation graph holds ggml context and the ggml cgraph.
// It can be either a serial or a sequential graph.
struct rwkv_computation_graph {
    struct ggml_context * ggml_ctx;
    struct ggml_cgraph * cgraph = nullptr;
    ggml_backend_sched_t sched;

    // Input tensors.
    struct ggml_tensor * tokens;
    struct ggml_tensor * input_state;
    std::unique_ptr<struct rwkv_layer_state[]> input_layers;

    // Output tensors.
    struct ggml_tensor * output_state;
    std::unique_ptr<struct rwkv_layer_state[]> output_layers;
    struct ggml_tensor * logits;

    // ggml graph counters before the graph was extended with logits tensor.
    int pre_logits_nodes;
    int pre_logits_leafs;
    // ggml graph counters after the graph was extended with logits tensor.
    int post_logits_nodes;
    int post_logits_leafs;
};

// The context holds the model and both serial and sequential computation graphs.
struct rwkv_context {
    struct rwkv_model * model;

    // The serial graph implements the traditional RNN mode that processes only one token at a time (serial mode).
    struct rwkv_computation_graph serial_graph;
    // The sequence graph implements the "sequence mode" (or transformer/GPT mode) that processes multiple tokens at a time.
    // This can be an order of magnitude or so faster than serial execution if used properly.
    struct rwkv_computation_graph sequential_graph;
    size_t last_used_sequence_length;

    uint32_t n_threads;

    enum rwkv_error_flags last_error;
    bool print_errors;
};

static void rwkv_carry_x(
    struct ggml_context * ctx,
    struct ggml_tensor * weight,
    struct ggml_tensor * bias,
    struct ggml_tensor *& x,
    struct ggml_tensor *& x_prev,
    struct ggml_tensor *& carry
) {
    const size_t n_embed = x->ne[0];
    const size_t sequence_len = x->ne[1];

    // self.layer_norm(x, self.w.blocks[i].ln2)
    x = rwkv_layer_norm(ctx, x, weight, bias);

    if (sequence_len == 1) {
        x_prev = carry;
    } else {
        x_prev = ggml_concat(
            ctx,
            ggml_view_2d(ctx, carry, n_embed, 1, carry->nb[1], 0),
            ggml_view_2d(ctx, x, n_embed, sequence_len - 1, x->nb[1], 0),
            1
        );
    }

    carry = ggml_view_1d(ctx, x, n_embed, n_embed * (sequence_len - 1) * sizeof(float));
}

static void rwkv_att_rkv_v4(
    struct ggml_context * ctx,
    struct rwkv_layer layer,
    struct ggml_tensor * x,
    struct ggml_tensor * x_prev,
    struct ggml_tensor *& r,
    struct ggml_tensor *& k,
    struct ggml_tensor *& v
) {
    // xk = x * time_mix_k + state[5 * i + 1] * (1 - time_mix_k)
    struct ggml_tensor * xk = ggml_add(ctx,
        ggml_mul(ctx, x, layer.att_time_mix_k),
        ggml_sub(ctx, x_prev, ggml_mul(ctx, x_prev, layer.att_time_mix_k))
    );

    // xv = x * time_mix_v + state[5 * i + 1] * (1 - time_mix_v)
    struct ggml_tensor * xv = ggml_add(ctx,
        ggml_mul(ctx, x, layer.att_time_mix_v),
        ggml_sub(ctx, x_prev, ggml_mul(ctx, x_prev, layer.att_time_mix_v))
    );

    // xr = x * time_mix_r + state[5 * i + 1] * (1 - time_mix_r)
    struct ggml_tensor * xr = ggml_add(ctx,
        ggml_mul(ctx, x, layer.att_time_mix_r),
        ggml_sub(ctx, x_prev, ggml_mul(ctx, x_prev, layer.att_time_mix_r))
    );

    // r = torch.sigmoid(rw @ xr)
    r = ggml_sigmoid(ctx, ggml_mul_mat(ctx, layer.att_receptance, xr));
    // k = kw @ xk
    k = ggml_mul_mat(ctx, layer.att_key, xk);
    // v = vw @ xv
    v = ggml_mul_mat(ctx, layer.att_value, xv);
}

static struct ggml_tensor * rwkv_att_wkv_v4(
    struct ggml_context * ctx,
    struct ggml_tensor * att_time_first,
    struct ggml_tensor * att_time_decay,
    struct ggml_tensor * k,
    struct ggml_tensor * v,
    struct ggml_tensor *& aa,
    struct ggml_tensor *& bb,
    struct ggml_tensor *& pp
) {
    // ww = time_first + k
    struct ggml_tensor * ww = ggml_add(ctx, att_time_first, k);
    // qq = torch.maximum(pp, ww)
    struct ggml_tensor * qq = rwkv_max(ctx, pp, ww);
    // e1 = torch.exp(pp - qq)
    struct ggml_tensor * e1 = ggml_exp(ctx, ggml_sub(ctx, pp, qq));
    // e2 = torch.exp(ww - qq)
    struct ggml_tensor * e2 = ggml_exp(ctx, ggml_sub(ctx, ww, qq));

    // a = e1 * aa + e2 * v
    struct ggml_tensor * a = ggml_add(ctx, ggml_mul(ctx, e1, aa), ggml_mul(ctx, e2, v));
    // b = e1 * bb + e2
    struct ggml_tensor * b = ggml_add(ctx, ggml_mul(ctx, e1, bb), e2);

    // ww = pp + time_decay
    ww = ggml_add(ctx, pp, att_time_decay);
    // qq = torch.maximum(ww, k)
    qq = rwkv_max(ctx, ww, k);
    // e1 = torch.exp(ww - qq)
    e1 = ggml_exp(ctx, ggml_sub(ctx, ww, qq));
    // e2 = torch.exp(k[t] - qq)
    e2 = ggml_exp(ctx, ggml_sub(ctx, k, qq));

    // state[5 * i + 2] = e1 * aa + e2 * v
    // state[5 * i + 3] = e1 * bb + e2
    // state[5 * i + 4] = qq
    aa = ggml_add(ctx, ggml_mul(ctx, e1, aa), ggml_mul(ctx, e2, v));
    bb = ggml_add(ctx, ggml_mul(ctx, e1, bb), e2);
    pp = qq;

    // wkv = a / b
    return ggml_div(ctx, a, b);
}

static struct ggml_tensor * rwkv_att_v4(
    struct ggml_context * ctx,
    struct ggml_tensor * x,
    struct rwkv_layer layer,
    struct rwkv_layer_state & state,
    struct rwkv_computation_graph & graph
) {
    size_t n_embed = x->ne[0];
    size_t sequence_length = x->ne[1];
    struct ggml_tensor * x0 = x, * x_prev;
    rwkv_carry_x(ctx, layer.ln1_weight, layer.ln1_bias, x0, x_prev, state.att_xx);

    struct ggml_tensor * r, * k, * v;
    rwkv_att_rkv_v4(ctx, layer, x0, x_prev, r, k, v);

    if (sequence_length == 1) {
        struct ggml_tensor * wkv = rwkv_att_wkv_v4(ctx, layer.att_time_first, layer.att_time_decay, k, v, state.att_aa, state.att_bb, state.att_pp);

        // ow @ (r * xx)
        return ggml_mul_mat(ctx, layer.att_output, ggml_mul(ctx, r, wkv));
    } else {
        ggml_build_forward_expand(graph.cgraph, r);

        for (size_t t = 0; t < sequence_length; t++) {
            struct ggml_tensor * kt = ggml_view_1d(ctx, k, n_embed, n_embed * sizeof(float) * t);
            struct ggml_tensor * vt = ggml_view_1d(ctx, v, n_embed, n_embed * sizeof(float) * t);
            struct ggml_tensor * xt = ggml_view_1d(ctx, x_prev, n_embed, n_embed * sizeof(float) * t);
            struct ggml_tensor * wkv = rwkv_att_wkv_v4(ctx, layer.att_time_first, layer.att_time_decay, kt, vt, state.att_aa, state.att_bb, state.att_pp);
            xt = ggml_set_1d_inplace(ctx, xt, wkv, 0);
            ggml_build_forward_expand(graph.cgraph, xt);
        }

        return ggml_mul_mat(ctx, layer.att_output, ggml_mul(ctx, r, x_prev));
    }
}

static struct ggml_tensor * rwkv_att_v5(
    struct ggml_context * ctx,
    struct ggml_tensor * x,
    struct rwkv_layer layer,
    struct rwkv_layer_state & state,
    const int64_t head_count,
    const int64_t head_size,
    const uint32_t arch_version_minor
) {
    size_t n_embed = x->ne[0];
    size_t sequence_length = x->ne[1];

    struct ggml_tensor * x_prev;
    rwkv_carry_x(ctx, layer.ln1_weight, layer.ln1_bias, x, x_prev, state.att_xx);

    struct ggml_tensor * xk = ggml_add(
        ctx,
        ggml_mul(ctx, x, layer.att_time_mix_k),
        ggml_sub(ctx, x_prev, ggml_mul(ctx, x_prev, layer.att_time_mix_k))
    );

    struct ggml_tensor * xv = ggml_add(
        ctx,
        ggml_mul(ctx, x, layer.att_time_mix_v),
        ggml_sub(ctx, x_prev, ggml_mul(ctx, x_prev, layer.att_time_mix_v))

    );

    struct ggml_tensor * xr = ggml_add(
        ctx,
        ggml_mul(ctx, x, layer.att_time_mix_r),
        ggml_sub(ctx, x_prev, ggml_mul(ctx, x_prev, layer.att_time_mix_r))
    );

    struct ggml_tensor * xg = NULL;

    if (arch_version_minor >= 2) {
        xg = ggml_add(
            ctx,
            ggml_mul(ctx, x, layer.att_time_mix_g),
            ggml_sub(ctx, x_prev, ggml_mul(ctx, x_prev, layer.att_time_mix_g))
        );
    }

    state.att_xx = ggml_view_1d(ctx, x, n_embed, n_embed * (sequence_length - 1) * sizeof(float));
    struct ggml_tensor * r = ggml_reshape_4d(ctx, ggml_mul_mat(ctx, layer.att_receptance, xr), 1,         head_size, head_count, sequence_length);
    struct ggml_tensor * k = ggml_reshape_4d(ctx, ggml_mul_mat(ctx, layer.att_key,        xk), head_size, 1,         head_count, sequence_length);
    struct ggml_tensor * v = ggml_reshape_4d(ctx, ggml_mul_mat(ctx, layer.att_value,      xv), 1,         head_size, head_count, sequence_length);
    struct ggml_tensor * g = NULL;

    if (arch_version_minor >= 2) {
        g = ggml_silu(
            ctx,
            ggml_mul_mat(ctx, layer.att_gate, xg)
        );
    }

    struct ggml_tensor * time_first;
    struct ggml_tensor * time_decay;

    if (arch_version_minor >= 2) {
        time_first = layer.att_time_faaaa;
        time_decay = layer.att_time_decay;
    } else {
        struct ggml_tensor * dummy = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, 1, head_size, head_count);

        time_first = ggml_repeat(ctx, layer.att_time_first, dummy);
        time_decay = ggml_repeat(ctx, layer.att_time_decay, dummy);
    }

    // To be able to use ggml's wkv6 gpu impls.
    {
        struct ggml_tensor * dummy = ggml_new_tensor_4d(ctx, GGML_TYPE_F32, 1, head_size, head_count, sequence_length);
        time_decay = ggml_repeat(ctx, time_decay, dummy);
    }

    struct ggml_tensor * wkv_out = ggml_rwkv_wkv6(ctx, k, v, r, time_first, time_decay, state.att_heads);
    x = ggml_view_1d(ctx, wkv_out, n_embed * sequence_length, 0);

    state.att_heads = ggml_view_1d(ctx, wkv_out, n_embed * head_size, n_embed * sequence_length * sizeof(float));

    // group norm with head_count groups
    x = ggml_reshape_3d(ctx, x, n_embed / head_count, head_count, sequence_length);
    x = ggml_norm(ctx, x, 1e-5f);
    // Convert back to a regular vector.
    x = ggml_reshape_2d(ctx, x, n_embed, sequence_length);
    x = ggml_add(ctx, ggml_mul(ctx, x, layer.att_ln_x_weight), layer.att_ln_x_bias);

    if (arch_version_minor >= 2) {
        x = ggml_mul(ctx, x, g);
    }

    return ggml_mul_mat(ctx, layer.att_output, x);
}

static struct ggml_tensor * rwkv_att_v6(
    struct ggml_context * ctx,
    struct ggml_tensor * x,
    struct rwkv_layer layer,
    struct rwkv_layer_state & state,
    const int64_t head_count,
    const int64_t head_size
) {
    size_t n_embed = x->ne[0];
    size_t sequence_length = x->ne[1];

    struct ggml_tensor * x_prev;
    rwkv_carry_x(ctx, layer.ln1_weight, layer.ln1_bias, x, x_prev, state.att_xx);

    // sx = x - state.att_xx
    // xxx = x + sx * x_maa
    x_prev = ggml_sub(ctx, x_prev, x);
    struct ggml_tensor * xxx = ggml_add(ctx, ggml_mul(ctx, x_prev, layer.att_time_maa_x), x);

    // xxx = tanh(xxx @ tm_w1).view(5, 1, -1)
    xxx = ggml_reshape_4d(
        ctx,
        ggml_tanh(
            ctx,
            ggml_mul_mat(ctx, layer.att_time_maa_w1, xxx)
        ),
        layer.att_time_maa_w1->ne[1] / 5, 1, 5, sequence_length
    );

    xxx = ggml_cont(ctx, ggml_permute(ctx, xxx, 0, 1, 3, 2));

    // xxx = torch.bmm(xxx, tm_w2)
    xxx = ggml_mul_mat(
        ctx,
        ggml_reshape_4d(
            ctx,
            layer.att_time_maa_w2,
            layer.att_time_maa_w2->ne[0], layer.att_time_maa_w2->ne[1], 1, 5
        ),
        xxx
    );

    struct ggml_tensor *mw = ggml_view_2d(ctx, xxx, n_embed, sequence_length, xxx->nb[1], 0);
    struct ggml_tensor *mk = ggml_view_2d(ctx, xxx, n_embed, sequence_length, xxx->nb[1], n_embed * sequence_length * sizeof(float));
    struct ggml_tensor *mv = ggml_view_2d(ctx, xxx, n_embed, sequence_length, xxx->nb[1], n_embed * sequence_length * 2 * sizeof(float));
    struct ggml_tensor *mr = ggml_view_2d(ctx, xxx, n_embed, sequence_length, xxx->nb[1], n_embed * sequence_length * 3 * sizeof(float));
    struct ggml_tensor *mg = ggml_view_2d(ctx, xxx, n_embed, sequence_length, xxx->nb[1], n_embed * sequence_length * 4 * sizeof(float));

    struct ggml_tensor * xw = ggml_add(ctx, ggml_mul(ctx, ggml_add(ctx, mw, layer.att_time_maa_w), x_prev), x);
    struct ggml_tensor * xk = ggml_add(ctx, ggml_mul(ctx, ggml_add(ctx, mk, layer.att_time_maa_k), x_prev), x);
    struct ggml_tensor * xv = ggml_add(ctx, ggml_mul(ctx, ggml_add(ctx, mv, layer.att_time_maa_v), x_prev), x);
    struct ggml_tensor * xr = ggml_add(ctx, ggml_mul(ctx, ggml_add(ctx, mr, layer.att_time_maa_r), x_prev), x);
    struct ggml_tensor * xg = ggml_add(ctx, ggml_mul(ctx, ggml_add(ctx, mg, layer.att_time_maa_g), x_prev), x);

    state.att_xx = ggml_view_1d(ctx, x, n_embed, n_embed * (sequence_length - 1) * sizeof(float));
    struct ggml_tensor * r = ggml_reshape_4d(ctx, ggml_mul_mat(ctx, layer.att_receptance, xr), 1,         head_size, head_count, sequence_length);
    struct ggml_tensor * k = ggml_reshape_4d(ctx, ggml_mul_mat(ctx, layer.att_key,        xk), head_size, 1,         head_count, sequence_length);
    struct ggml_tensor * v = ggml_reshape_4d(ctx, ggml_mul_mat(ctx, layer.att_value,      xv), 1,         head_size, head_count, sequence_length);
    struct ggml_tensor * g = ggml_silu(
        ctx,
        ggml_mul_mat(ctx, layer.att_gate, xg)
    );

    struct ggml_tensor * w = ggml_mul_mat(
        ctx,
        layer.att_time_decay_w2,
        ggml_tanh(
            ctx,
            ggml_mul_mat(ctx, layer.att_time_decay_w1, xw)
        )
    );
    w = ggml_add(ctx, w, ggml_reshape_1d(ctx, layer.att_time_decay, n_embed));

    w = ggml_exp(ctx, ggml_neg(ctx, ggml_exp(ctx, w)));
    w = ggml_reshape_4d(ctx, w, 1, head_size, head_count, sequence_length);

    struct ggml_tensor * wkv_out = ggml_rwkv_wkv6(ctx, k, v, r, layer.att_time_faaaa, w, state.att_heads);
    x = ggml_view_1d(ctx, wkv_out, n_embed * sequence_length, 0);

    state.att_heads = ggml_view_1d(ctx, wkv_out, n_embed * head_size, n_embed * sequence_length * sizeof(float));

    // group norm with head_count groups
    x = ggml_reshape_3d(ctx, x, head_size, head_count, sequence_length);
    x = ggml_norm(ctx, x, 64e-5f);
    // Convert back to a regular vector.
    x = ggml_reshape_2d(ctx, x, n_embed, sequence_length);
    x = ggml_add(ctx, ggml_mul(ctx, x, layer.att_ln_x_weight), layer.att_ln_x_bias);

    x = ggml_mul(ctx, x, g);

    return ggml_mul_mat(ctx, layer.att_output, x);
}

static struct ggml_tensor * rwkv_att_v7(
    struct ggml_context * ctx,
    struct ggml_tensor * x,
    struct ggml_tensor * &v_first,
    struct rwkv_layer layer,
    struct rwkv_layer_state & state,
    const int64_t head_count,
    const int64_t head_size
) {
    size_t n_embed = x->ne[0];
    size_t sequence_length = x->ne[1];

    struct ggml_tensor * x_prev;
    rwkv_carry_x(ctx, layer.ln1_weight, layer.ln1_bias, x, x_prev, state.att_xx);

    // sx = x - x_prev
    struct ggml_tensor * sx = ggml_sub(ctx, x_prev, x);
    struct ggml_tensor * dummy = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, n_embed, sequence_length, 6);
    sx = ggml_repeat(ctx, sx, dummy);
    struct ggml_tensor * xxx = ggml_add(ctx, ggml_mul(ctx, sx, layer.att_x_rwkvag), x);

    struct ggml_tensor *xr = ggml_view_2d(ctx, xxx, n_embed, sequence_length, xxx->nb[1], 0);
    struct ggml_tensor *xw = ggml_view_2d(ctx, xxx, n_embed, sequence_length, xxx->nb[1], n_embed * sequence_length * sizeof(float));
    struct ggml_tensor *xk = ggml_view_2d(ctx, xxx, n_embed, sequence_length, xxx->nb[1], n_embed * sequence_length * 2 * sizeof(float));
    struct ggml_tensor *xv = ggml_view_2d(ctx, xxx, n_embed, sequence_length, xxx->nb[1], n_embed * sequence_length * 3 * sizeof(float));
    struct ggml_tensor *xa = ggml_view_2d(ctx, xxx, n_embed, sequence_length, xxx->nb[1], n_embed * sequence_length * 4 * sizeof(float));
    struct ggml_tensor *xg = ggml_view_2d(ctx, xxx, n_embed, sequence_length, xxx->nb[1], n_embed * sequence_length * 5 * sizeof(float));

    struct ggml_tensor * r = ggml_reshape_3d(ctx, ggml_mul_mat(ctx, layer.att_receptance, xr), head_size, head_count, sequence_length);
    struct ggml_tensor * g = ggml_mul_mat(ctx, layer.att_g2, ggml_sigmoid(ctx, ggml_mul_mat(ctx, layer.att_g1, xg)));
    struct ggml_tensor * a = ggml_sigmoid(ctx,
        ggml_add(
            ctx,
            ggml_mul_mat(ctx, layer.att_a2, ggml_mul_mat(ctx, layer.att_a1, xa)),
            layer.att_a0
        )
    );

    struct ggml_tensor * w = ggml_add(
        ctx,
        ggml_mul_mat(ctx, layer.att_w2, ggml_tanh(ctx, ggml_mul_mat(ctx, layer.att_w1, xw))),
        layer.att_w0
    );
    w = ggml_exp(ctx, ggml_scale(ctx, ggml_sigmoid(ctx, w), -0.606531));

    struct ggml_tensor * k = ggml_mul_mat(ctx, layer.att_key, xk);
    struct ggml_tensor * kk = ggml_reshape_3d(ctx, ggml_mul(ctx, k, layer.att_k_k), head_size, head_count, sequence_length);
    kk = rwkv_l2norm(ctx, kk);

    struct ggml_tensor * ka = ggml_mul(ctx, k, layer.att_k_a);
    k = ggml_add(ctx, k, ggml_sub(ctx, ggml_mul(ctx, a, ka), ka));
    
    struct ggml_tensor * v = ggml_mul_mat(ctx, layer.att_value, xv);
    if (v_first == NULL) {
        v_first = v;
    } else {
        v = ggml_add(ctx, v, ggml_mul(ctx,
                ggml_sub(ctx, v_first, v),
                ggml_sigmoid(ctx, 
                    ggml_add(ctx,
                        ggml_mul_mat(ctx, layer.att_v2, ggml_mul_mat(ctx, layer.att_v1, xv)),
                        layer.att_v0
                    )
                )
            )
        );
    }

    w = ggml_reshape_3d(ctx, w, head_size, head_count, sequence_length);
    k = ggml_reshape_3d(ctx, k, head_size, head_count, sequence_length);
    v = ggml_reshape_3d(ctx, v, head_size, head_count, sequence_length);
    a = ggml_reshape_3d(ctx, a, head_size, head_count, sequence_length);

    struct ggml_tensor * wkv_out = rwkv_wkv_v7(ctx, state.att_heads, r, w, k, v, ggml_neg(ctx, kk), ggml_mul(ctx, kk, a));
    x = ggml_view_1d(ctx, wkv_out, n_embed * sequence_length, 0);

    state.att_heads = ggml_view_1d(ctx, wkv_out, n_embed * head_size, n_embed * sequence_length * sizeof(float));

    // group norm with head_count groups
    x = ggml_reshape_3d(ctx, x, head_size, head_count, sequence_length);
    x = ggml_norm(ctx, x, 64e-5f);
    // Convert back to a regular vector.
    x = ggml_reshape_2d(ctx, x, n_embed, sequence_length);
    x = ggml_add(ctx, ggml_mul(ctx, x, layer.att_ln_x_weight), layer.att_ln_x_bias);

    x = ggml_add(ctx, x, 
        ggml_reshape_2d(ctx,
            ggml_mul(ctx, v, ggml_sum_rows(ctx, ggml_mul(ctx, ggml_mul(ctx, k, r), layer.att_r_k))),
            n_embed, sequence_length
        )
    );

    x = ggml_mul(ctx, x, g);

    return ggml_mul_mat(ctx, layer.att_output, x);
}

static struct ggml_tensor * rwkv_ffn_v4_v5(struct ggml_context * ctx, struct ggml_tensor * x, struct rwkv_layer layer, struct rwkv_layer_state & state) {
    struct ggml_tensor * x_prev;
    rwkv_carry_x(ctx, layer.ln2_weight, layer.ln2_bias, x, x_prev, state.ffn_xx);

    // xk = x * time_mix_k + state[5 * i + 1] * (1 - time_mix_k)
    // xk = x * time_mix_k + state[5 * i + 0] * (1 - time_mix_k)
    struct ggml_tensor * xk = ggml_add(
        ctx,
        ggml_mul(ctx, x, layer.ffn_time_mix_k),
        ggml_sub(ctx, x_prev, ggml_mul(ctx, x_prev, layer.ffn_time_mix_k))
    );

    // xr = x * time_mix_r + state[5 * i + 0] * (1 - time_mix_r)
    struct ggml_tensor * xr = ggml_add(
        ctx,
        ggml_mul(ctx, x, layer.ffn_time_mix_r),
        ggml_sub(ctx, x_prev, ggml_mul(ctx, x_prev, layer.ffn_time_mix_r))
    );

    // r = torch.sigmoid(rw @ xr)
    struct ggml_tensor * r = ggml_sigmoid(ctx, ggml_mul_mat(ctx, layer.ffn_receptance, xr));

    // k = torch.square(torch.relu(kw @ xk))
    struct ggml_tensor * k = ggml_sqr(ctx, ggml_relu(ctx, ggml_mul_mat(ctx, layer.ffn_key, xk)));

    // r * (vw @ k)
    return ggml_mul(ctx, r, ggml_mul_mat(ctx, layer.ffn_value, k));
}

static struct ggml_tensor * rwkv_ffn_v6(struct ggml_context * ctx, struct ggml_tensor * x, struct rwkv_layer layer, struct rwkv_layer_state & state) {
    struct ggml_tensor * x_prev;
    rwkv_carry_x(ctx, layer.ln2_weight, layer.ln2_bias, x, x_prev, state.ffn_xx);
    x_prev = ggml_sub(ctx, x_prev, x);

    // xk = x + sx * time_maa_k
    // xr = x + sx * time_maa_r
    struct ggml_tensor * xk = ggml_add(ctx, ggml_mul(ctx, x_prev, layer.ffn_time_maa_k), x);
    struct ggml_tensor * xr = ggml_add(ctx, ggml_mul(ctx, x_prev, layer.ffn_time_maa_r), x);

    // r = torch.sigmoid(rw @ xr)
    struct ggml_tensor * r = ggml_sigmoid(ctx, ggml_mul_mat(ctx, layer.ffn_receptance, xr));

    // k = torch.square(torch.relu(kw @ xk))
    struct ggml_tensor * k = ggml_sqr(ctx, ggml_relu(ctx, ggml_mul_mat(ctx, layer.ffn_key, xk)));

    // r * (vw @ k)
    return ggml_mul(ctx, r, ggml_mul_mat(ctx, layer.ffn_value, k));
}

static struct ggml_tensor * rwkv_ffn_v7(struct ggml_context * ctx, struct ggml_tensor * x, struct rwkv_layer layer, struct rwkv_layer_state & state) {
    struct ggml_tensor * x_prev;
    rwkv_carry_x(ctx, layer.ln2_weight, layer.ln2_bias, x, x_prev, state.ffn_xx);
    x_prev = ggml_sub(ctx, x_prev, x);

    struct ggml_tensor * xk = ggml_add(ctx, ggml_mul(ctx, x_prev, layer.ffn_x_k), x);

    struct ggml_tensor * k = ggml_sqr(ctx, ggml_relu(ctx, ggml_mul_mat(ctx, layer.ffn_key, xk)));

    return ggml_mul_mat(ctx, layer.ffn_value, k);
}

static void rwkv_create_input_and_output_views(
    struct ggml_context * ctx,
    struct rwkv_layer_state * inputs,
    struct rwkv_layer_state * outputs,
    struct ggml_tensor * input,
    struct ggml_tensor * output,
    const size_t n_layer,
    const size_t n_embed,
    const uint32_t arch_version_major,
    const int64_t head_count,
    const int64_t head_size
) {
    size_t sz_float = sizeof(float);

    for (size_t i = 0; i < n_layer; i++) {
        struct rwkv_layer_state & input_state = inputs[i];
        struct rwkv_layer_state & output_state = outputs[i];

        if (arch_version_major >= 5) {
            size_t vectors_per_layer = 2 + head_size;

            size_t att_heads_size = head_size * head_size * head_count;

            input_state.ffn_xx    = ggml_view_1d(ctx, input, n_embed,          n_embed * (i * vectors_per_layer + 0) * sz_float);
            input_state.att_xx    = ggml_view_1d(ctx, input, n_embed,          n_embed * (i * vectors_per_layer + 1) * sz_float);
            input_state.att_heads = ggml_view_1d(ctx, input, att_heads_size,   n_embed * (i * vectors_per_layer + 2) * sz_float);
            ggml_set_name(input_state.ffn_xx, ("ffn_xx.in." + std::to_string(i)).c_str());
            ggml_set_name(input_state.att_xx, ("att_xx.in." + std::to_string(i)).c_str());
            ggml_set_name(input_state.att_heads, ("att_heads.in." + std::to_string(i)).c_str());

            output_state.ffn_xx    = ggml_view_1d(ctx, output, n_embed,        n_embed * (i * vectors_per_layer + 0) * sz_float);
            output_state.att_xx    = ggml_view_1d(ctx, output, n_embed,        n_embed * (i * vectors_per_layer + 1) * sz_float);
            output_state.att_heads = ggml_view_1d(ctx, output, att_heads_size, n_embed * (i * vectors_per_layer + 2) * sz_float);
            ggml_set_name(output_state.ffn_xx, ("ffn_xx.out." + std::to_string(i)).c_str());
            ggml_set_name(output_state.att_xx, ("att_xx.out." + std::to_string(i)).c_str());
            ggml_set_name(output_state.att_heads, ("att_heads.out." + std::to_string(i)).c_str());
        } else {
            input_state.ffn_xx = ggml_view_1d(ctx, input, n_embed, n_embed * (i * 5 + 0) * sz_float);
            input_state.att_xx = ggml_view_1d(ctx, input, n_embed, n_embed * (i * 5 + 1) * sz_float);
            input_state.att_aa = ggml_view_1d(ctx, input, n_embed, n_embed * (i * 5 + 2) * sz_float);
            input_state.att_bb = ggml_view_1d(ctx, input, n_embed, n_embed * (i * 5 + 3) * sz_float);
            input_state.att_pp = ggml_view_1d(ctx, input, n_embed, n_embed * (i * 5 + 4) * sz_float);
            ggml_set_name(input_state.ffn_xx, ("ffn_xx.in." + std::to_string(i)).c_str());
            ggml_set_name(input_state.att_xx, ("att_xx.in." + std::to_string(i)).c_str());
            ggml_set_name(input_state.att_aa, ("att_aa.in." + std::to_string(i)).c_str());
            ggml_set_name(input_state.att_bb, ("att_bb.in." + std::to_string(i)).c_str());
            ggml_set_name(input_state.att_pp, ("att_pp.in." + std::to_string(i)).c_str());

            output_state.ffn_xx = ggml_view_1d(ctx, output, n_embed, n_embed * (i * 5 + 0) * sz_float);
            output_state.att_xx = ggml_view_1d(ctx, output, n_embed, n_embed * (i * 5 + 1) * sz_float);
            output_state.att_aa = ggml_view_1d(ctx, output, n_embed, n_embed * (i * 5 + 2) * sz_float);
            output_state.att_bb = ggml_view_1d(ctx, output, n_embed, n_embed * (i * 5 + 3) * sz_float);
            output_state.att_pp = ggml_view_1d(ctx, output, n_embed, n_embed * (i * 5 + 4) * sz_float);
            ggml_set_name(output_state.ffn_xx, ("ffn_xx.out." + std::to_string(i)).c_str());
            ggml_set_name(output_state.att_xx, ("att_xx.out." + std::to_string(i)).c_str());
            ggml_set_name(output_state.att_aa, ("att_aa.out." + std::to_string(i)).c_str());
            ggml_set_name(output_state.att_bb, ("att_bb.out." + std::to_string(i)).c_str());
            ggml_set_name(output_state.att_pp, ("att_pp.out." + std::to_string(i)).c_str());
        }

    }
}

// Serial graph (token-by-token eval)

// Creates and sets the input and output ggml tensors, builds the computation graph.
static bool rwkv_build_serial_graph(struct rwkv_model & model, struct rwkv_computation_graph & graph) {
    if (!graph.cgraph) {
        graph.cgraph = ggml_new_graph_custom(graph.ggml_ctx, RWKV_MAX_NODES, false);
    }

    struct rwkv_file_header & header = model.header;
    const size_t n_vocab = header.n_vocab;
    const size_t n_embed = header.n_embed;
    const size_t n_layer = header.n_layer;

    struct ggml_context * ctx = graph.ggml_ctx;

    // Creates a 1-element tensor.
    graph.tokens = ggml_new_tensor_1d(ctx, GGML_TYPE_I32, 1);

    size_t vectors_per_layer = model.arch_version_major >= 5 ?
        2 + model.head_size :
        5;

    struct ggml_tensor * input = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embed * vectors_per_layer * n_layer);
    struct ggml_tensor * output = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embed * vectors_per_layer * n_layer);

    // We collect parts of input state here. Each part is (n_embed) vector.
    std::unique_ptr<struct rwkv_layer_state[]> inputs(new(std::nothrow) struct rwkv_layer_state[n_layer]);
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_ALLOC, inputs.get(), "Failed to allocate input state parts");

    // We collect parts of output state here. Each part is (n_embed) vector.
    std::unique_ptr<struct rwkv_layer_state[]> outputs(new(std::nothrow) struct rwkv_layer_state[n_layer]);
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_ALLOC, outputs.get(), "Failed to allocate output state parts");

    rwkv_create_input_and_output_views(ctx, inputs.get(), outputs.get(), input, output, n_layer, n_embed, model.arch_version_major, model.head_count, model.head_size);

    graph.logits = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_vocab);

    ggml_set_input(input);
    ggml_set_output(output);
    ggml_set_name(input, "state.in");
    ggml_set_name(output, "state.out");
    ggml_set_input(graph.tokens);

    // For v7.
    struct ggml_tensor * v_first = NULL;

    // x = self.w.emb.weight[token]
    struct ggml_tensor * x = ggml_get_rows(ctx, model.emb, graph.tokens);

    // x = self.layer_norm(x, self.w.blocks[0].ln0)
    x = rwkv_layer_norm(ctx, x, model.ln0_weight, model.ln0_bias);

    for (size_t i = 0; i < n_layer; i++) {
        struct rwkv_layer & layer = model.layers[i];

        struct rwkv_layer_state state = inputs[i];

        switch (model.arch_version_major) {
            case 7:
                x = ggml_add(ctx, x, rwkv_att_v7(ctx, x, v_first, layer, state, model.head_count, model.head_size));
                x = ggml_add(ctx, x, rwkv_ffn_v7(ctx, x, layer, state));
                break;
            case 6:
                x = ggml_add(ctx, x, rwkv_att_v6(ctx, x, layer, state, model.head_count, model.head_size));
                x = ggml_add(ctx, x, rwkv_ffn_v6(ctx, x, layer, state));
                break;
            case 5:
                x = ggml_add(ctx, x, rwkv_att_v5(ctx, x, layer, state, model.head_count, model.head_size, model.arch_version_minor));
                x = ggml_add(ctx, x, rwkv_ffn_v4_v5(ctx, x, layer, state));
                break;
            case 4:
                x = ggml_add(ctx, x, rwkv_att_v4(ctx, x, layer, state, graph));
                x = ggml_add(ctx, x, rwkv_ffn_v4_v5(ctx, x, layer, state));
                break;
            default:
                RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_UNSUPPORTED, false, "Unsupported model architecture version");
                break;
        }

        struct rwkv_layer_state & output_state = outputs[i];

        ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, state.ffn_xx, output_state.ffn_xx));
        ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, state.att_xx, output_state.att_xx));

        if (model.arch_version_major >= 5) {
            ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, state.att_heads, output_state.att_heads));
        } else {
            ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, state.att_aa, output_state.att_aa));
            ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, state.att_bb, output_state.att_bb));
            ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, state.att_pp, output_state.att_pp));
        }
    }

    graph.pre_logits_nodes = graph.cgraph->n_nodes;
    graph.pre_logits_leafs = graph.cgraph->n_leafs;

    // x = self.layer_norm(x[-1,:], self.w.ln_out)
    x = rwkv_layer_norm(ctx, x, model.ln_out_weight, model.ln_out_bias);

    // x = (self.w.head.weight @ x).float()
    ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, ggml_mul_mat(ctx, model.head, x), graph.logits));

    graph.post_logits_nodes = graph.cgraph->n_nodes;
    graph.post_logits_leafs = graph.cgraph->n_leafs;

    graph.input_state = input;
    graph.input_layers = std::move(inputs);

    graph.output_state = output;
    graph.output_layers = std::move(outputs);

    return true;
}

// Copy-pasted from llama.cpp.
static const size_t tensor_alignment = 32;

// Prepares the computation graph for inference, measuring and allocating all input and output tensors.
static bool rwkv_measure_and_build_serial_context(struct rwkv_model & model, struct rwkv_computation_graph & graph) {
    if (graph.ggml_ctx) {
        ggml_free(graph.ggml_ctx);

        graph.ggml_ctx = NULL;
        graph.cgraph = NULL;
    }

    graph.ggml_ctx = rwkv_init_ggml_context(rwkv_ggml_overhead(), true);

    RWKV_ENSURE_OR_FALSE(rwkv_build_serial_graph(model, graph));

    return true;
}

// Sequential graph

// Creates and sets the input and output ggml tensors, builds the computation graph.
static bool rwkv_build_sequential_graph(struct rwkv_model & model, struct rwkv_computation_graph & graph, const size_t sequence_length) {
    if (!graph.cgraph) {
        graph.cgraph = ggml_new_graph_custom(graph.ggml_ctx, RWKV_MAX_NODES, false);
    }

    struct rwkv_file_header & header = model.header;
    const size_t n_vocab = header.n_vocab;
    const size_t n_embed = header.n_embed;
    const size_t n_layer = header.n_layer;

    struct ggml_context * ctx = graph.ggml_ctx;

    graph.tokens = ggml_new_tensor_1d(ctx, GGML_TYPE_I32, sequence_length);

    size_t vectors_per_layer = model.arch_version_major >= 5 ?
        2 + model.head_size :
        5;

    struct ggml_tensor * input = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embed * vectors_per_layer * n_layer);
    struct ggml_tensor * output = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embed * vectors_per_layer * n_layer);

    // We collect parts of input state here. Each part is (n_embed) vector.
    std::unique_ptr<struct rwkv_layer_state[]> inputs(new(std::nothrow) struct rwkv_layer_state[n_layer]);
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_ALLOC, inputs.get(), "Failed to allocate input state parts");

    // We collect parts of output state here. Each part is (n_embed) vector.
    std::unique_ptr<struct rwkv_layer_state[]> outputs(new(std::nothrow) struct rwkv_layer_state[n_layer]);
    RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_ALLOC, outputs.get(), "Failed to allocate output state parts");

    rwkv_create_input_and_output_views(ctx, inputs.get(), outputs.get(), input, output, n_layer, n_embed, model.arch_version_major, model.head_count, model.head_size);

    graph.logits = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_vocab);

    ggml_set_input(input);
    ggml_set_output(output);
    ggml_set_name(input, "state.in");
    ggml_set_name(output, "state.out");
    ggml_set_input(graph.tokens);

    // For v7.
    struct ggml_tensor * v_first = NULL;

    // x = self.w.emb.weight[token]
    struct ggml_tensor * x = ggml_get_rows(ctx, model.emb, graph.tokens);

    // x = self.layer_norm(x, self.w.blocks[0].ln0)
    x = rwkv_layer_norm(ctx, x, ggml_repeat(ctx, model.ln0_weight, x), ggml_repeat(ctx, model.ln0_bias, x));

    for (size_t i = 0; i < model.header.n_layer; i++) {
        struct rwkv_layer & layer = model.layers[i];

        struct rwkv_layer_state state = inputs[i];

        switch (model.arch_version_major) {
            case 7:
                x = ggml_add(ctx, x, rwkv_att_v7(ctx, x, v_first, layer, state, model.head_count, model.head_size));
                break;
            case 6:
                x = ggml_add(ctx, x, rwkv_att_v6(ctx, x, layer, state, model.head_count, model.head_size));
                break;
            case 5:
                x = ggml_add(ctx, x, rwkv_att_v5(ctx, x, layer, state, model.head_count, model.head_size, model.arch_version_minor));
                break;
            case 4:
                x = ggml_add(ctx, x, rwkv_att_v4(ctx, x, layer, state, graph));
                break;
            default:
                RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_UNSUPPORTED, false, "Unsupported model architecture version");
                break;
        }

        // TODO Can we skip ffn for all but the last token, the same way we skip unembedding?
        switch (model.arch_version_major) {
            case 7:
                x = ggml_add(ctx, x, rwkv_ffn_v7(ctx, x, layer, state));
                break;
            case 6:
                x = ggml_add(ctx, x, rwkv_ffn_v6(ctx, x, layer, state));
                break;
            case 5:
            case 4:
                x = ggml_add(ctx, x, rwkv_ffn_v4_v5(ctx, x, layer, state));
                break;
            default:
                RWKV_ASSERT_FALSE_MSG(RWKV_ERROR_UNSUPPORTED, false, "Unsupported model architecture version");
                break;
        }

        struct rwkv_layer_state & output_state = outputs[i];

        output_state.att_xx = ggml_set_1d_inplace(ctx, output_state.att_xx, state.att_xx, 0);
        ggml_build_forward_expand(graph.cgraph, output_state.att_xx);
        ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, state.ffn_xx, output_state.ffn_xx));

        if (model.arch_version_major >= 5) {
            ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, state.att_heads, output_state.att_heads));
        } else {
            ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, state.att_aa, output_state.att_aa));
            ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, state.att_bb, output_state.att_bb));
            ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, state.att_pp, output_state.att_pp));
        }
    }

    graph.pre_logits_nodes = graph.cgraph->n_nodes;
    graph.pre_logits_leafs = graph.cgraph->n_leafs;

    // x = self.layer_norm(x[-1,:], self.w.ln_out)
    x = rwkv_layer_norm(ctx, ggml_view_1d(ctx, x, n_embed, n_embed * sizeof(float) * (sequence_length - 1)), model.ln_out_weight, model.ln_out_bias);

    // x = (self.w.head.weight @ x).float()
    ggml_build_forward_expand(graph.cgraph, ggml_cpy(ctx, ggml_mul_mat(ctx, model.head, x), graph.logits));

    graph.post_logits_nodes = graph.cgraph->n_nodes;
    graph.post_logits_leafs = graph.cgraph->n_leafs;

    graph.input_state = input;
    graph.input_layers = std::move(inputs);

    graph.output_state = output;
    graph.output_layers = std::move(outputs);

    return true;
}

// Prepares the computation graph for inference, measuring and allocating all input and output tensors.
static bool rwkv_measure_and_build_sequential_context(struct rwkv_model & model, struct rwkv_computation_graph & graph, const size_t sequence_length) {
    if (graph.ggml_ctx) {
        ggml_free(graph.ggml_ctx);

        graph.ggml_ctx = NULL;
        graph.cgraph = NULL;
    }

    graph.ggml_ctx = rwkv_init_ggml_context(rwkv_ggml_overhead(), true);

    RWKV_ENSURE_OR_FALSE(rwkv_build_sequential_graph(model, graph, sequence_length));

    return true;
}
