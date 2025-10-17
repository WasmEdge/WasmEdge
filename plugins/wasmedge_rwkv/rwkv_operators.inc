#include "rwkv_operators_wkv_v7.inc"

#define SUPPRESS_UNUSED_WARNINGS_IN_CUSTOM_OP() { (void) ith; (void) nth; (void) userdata; }

static void rwkv_max_impl(
    struct ggml_tensor * dest,
    const struct ggml_tensor * src0,
    const struct ggml_tensor * src1,
    int ith,
    int nth,
    void * userdata
) {
    GGML_ASSERT(dest->type == GGML_TYPE_F32);
    GGML_ASSERT(src0->type == GGML_TYPE_F32);
    GGML_ASSERT(src1->type == GGML_TYPE_F32);
    GGML_ASSERT(ggml_is_contiguous(dest));
    GGML_ASSERT(ggml_is_contiguous(src0));
    GGML_ASSERT(ggml_is_contiguous(src1));
    GGML_ASSERT(ggml_are_same_shape(src0, dest));
    GGML_ASSERT(ggml_are_same_shape(src1, dest));
    // Verify that the shape is 2D.
    GGML_ASSERT(dest->ne[2] == 1);
    GGML_ASSERT(dest->ne[3] == 1);

    int64_t element_count = src0->ne[0] * src0->ne[1];
    int64_t start = ith * element_count / nth;
    int64_t end = (ith + 1) * element_count / nth;
    float * src0_data = (float *) src0->data;
    float * src1_data = (float *) src1->data;
    float * dest_data = (float *) dest->data;

    for (int64_t i = start; i < end; i++) {
        dest_data[i] = fmaxf(src0_data[i], src1_data[i]);
    }

    SUPPRESS_UNUSED_WARNINGS_IN_CUSTOM_OP();
}

// TODO: Upstream to ggml
static void rwkv_l2norm_impl(
    struct ggml_tensor * dst,
    const struct ggml_tensor * src0,
    int ith,
    int nth,
    void * userdata
) {
    GGML_ASSERT(dst->type == GGML_TYPE_F32);
    GGML_ASSERT(src0->type == GGML_TYPE_F32);
    GGML_ASSERT(ggml_is_contiguous(dst));
    GGML_ASSERT(ggml_is_contiguous(src0));
    GGML_ASSERT(ggml_are_same_shape(src0, dst));

    GGML_TENSOR_UNARY_OP_LOCALS

    float eps = 1e-12f;

    // TODO: optimize
    for (int64_t i03 = 0; i03 < ne03; i03++) {
        for (int64_t i02 = 0; i02 < ne02; i02++) {
            for (int64_t i01 = ith; i01 < ne01; i01 += nth) {
                const float * x = (float *) ((char *) src0->data + i01*nb01 + i02*nb02 + i03*nb03);

                float sum = 0.0;
                for (int64_t i00 = 0; i00 < ne00; i00++) {
                    float v = x[i00];
                    sum += v*v;
                }

                float * y = (float *) ((char *) dst->data + i01*nb01 + i02*nb02 + i03*nb03);

                const float scale = 1.0f/fmaxf(sqrtf(sum), eps);

                // ggml_vec_scale_f32(ne00, y, scale);
                for (int64_t i00 = 0; i00 < ne00; i00++) {
                    y[i00] = x[i00] * scale;
                }
            }
        }
    }

    SUPPRESS_UNUSED_WARNINGS_IN_CUSTOM_OP();
}

// Element-wise max(x, y)
struct ggml_tensor * rwkv_max(struct ggml_context * ctx, struct ggml_tensor * x, struct ggml_tensor * y) {
    return ggml_map_custom2(ctx, x, y, rwkv_max_impl, 1, NULL);
}

struct ggml_tensor * rwkv_l2norm(struct ggml_context * ctx, struct ggml_tensor * x) {
    return ggml_map_custom1(ctx, x, rwkv_l2norm_impl, 1, NULL);
}

struct ggml_tensor * rwkv_layer_norm(struct ggml_context * ctx, struct ggml_tensor * x, struct ggml_tensor * weight, struct ggml_tensor * bias) {
    // LayerNorm in RWKV is `x = (x - mean(x)) / sqrt(variance(x) + 1e-5) * weight + bias`
    // Looks like ggml_norm does the first part, we only need to apply weight & bias.
    return ggml_add(ctx, ggml_mul(ctx, ggml_norm(ctx, x, 1e-5F), weight), bias);
}
