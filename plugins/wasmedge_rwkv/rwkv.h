#ifndef RWKV_H
#define RWKV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(RWKV_SHARED)
#    if defined(_WIN32) && !defined(__MINGW32__)
#        if defined(RWKV_BUILD)
#            define RWKV_API __declspec(dllexport)
#        else
#            define RWKV_API __declspec(dllimport)
#        endif
#    else
#        define RWKV_API __attribute__ ((visibility ("default")))
#    endif
#else
#    define RWKV_API
#endif

// 'ggmf' in hex.
#define RWKV_FILE_MAGIC 0x67676d66

#define RWKV_FILE_VERSION_0 100
#define RWKV_FILE_VERSION_1 101
#define RWKV_FILE_VERSION_MIN RWKV_FILE_VERSION_0
#define RWKV_FILE_VERSION_MAX RWKV_FILE_VERSION_1
// Default file version is the latest version.
#define RWKV_FILE_VERSION RWKV_FILE_VERSION_MAX

#if defined(__cplusplus)
extern "C" {
#endif

    // Represents an error encountered during a function call.
    // These are flags, so an actual value might contain multiple errors.
    enum rwkv_error_flags {
        RWKV_ERROR_NONE = 0,

        RWKV_ERROR_ARGS = 1 << 8,
        RWKV_ERROR_FILE = 2 << 8,
        RWKV_ERROR_MODEL = 3 << 8,
        RWKV_ERROR_MODEL_PARAMS = 4 << 8,
        RWKV_ERROR_GRAPH = 5 << 8,
        RWKV_ERROR_CTX = 6 << 8,

        RWKV_ERROR_ALLOC = 1,
        RWKV_ERROR_FILE_OPEN = 2,
        RWKV_ERROR_FILE_STAT = 3,
        RWKV_ERROR_FILE_READ = 4,
        RWKV_ERROR_FILE_WRITE = 5,
        RWKV_ERROR_FILE_MAGIC = 6,
        RWKV_ERROR_FILE_VERSION = 7,
        RWKV_ERROR_DATA_TYPE = 8,
        RWKV_ERROR_UNSUPPORTED = 9,
        RWKV_ERROR_SHAPE = 10,
        RWKV_ERROR_DIMENSION = 11,
        RWKV_ERROR_KEY = 12,
        RWKV_ERROR_DATA = 13,
        RWKV_ERROR_PARAM_MISSING = 14
    };

    // RWKV context that can be used for inference.
    // All functions that operate on rwkv_context are thread-safe.
    // rwkv_context can be sent to different threads between calls to rwkv_eval.
    // There is no requirement for rwkv_context to be freed on the creating thread.
    struct rwkv_context;

    // Sets whether errors are automatically printed to stderr.
    // If this is set to false, you are responsible for calling rwkv_last_error manually if an operation fails.
    // - ctx: the context to suppress error messages for.
    //   If NULL, affects model load (rwkv_init_from_file) and quantization (rwkv_quantize_model_file) errors,
    //   as well as the default for new context.
    // - print_errors: whether error messages should be automatically printed.
    RWKV_API void rwkv_set_print_errors(struct rwkv_context * ctx, const bool print_errors);

    // Gets whether errors are automatically printed to stderr.
    // - ctx: the context to retrieve the setting for, or NULL for the global setting.
    RWKV_API bool rwkv_get_print_errors(const struct rwkv_context * ctx);

    // Retrieves and clears the error flags.
    // - ctx: the context the retrieve the error for, or NULL for the global error.
    RWKV_API enum rwkv_error_flags rwkv_get_last_error(struct rwkv_context * ctx);

    // Loads the model from a file and prepares it for inference.
    // Returns NULL on any error.
    // - model_file_path: path to model file in ggml format.
    // - n_threads: count of threads to use, must be positive.
    // - n_gpu_layer: count of layers need to load to gpu
    RWKV_API struct rwkv_context * rwkv_init_from_file(const char * model_file_path, const uint32_t n_threads, const uint32_t n_gpu_layers);

    // Creates a new context from an existing one.
    // This can allow you to run multiple rwkv_eval's in parallel, without having to load a single model multiple times.
    // Each rwkv_context can have one eval running at a time.
    // Every rwkv_context must be freed using rwkv_free.
    // - ctx: context to be cloned.
    // - n_threads: count of threads to use, must be positive.
    RWKV_API struct rwkv_context * rwkv_clone_context(struct rwkv_context * ctx, const uint32_t n_threads);

    // Evaluates the model for a single token.
    // You can pass NULL to logits_out whenever logits are not needed. This can improve speed by ~10 ms per iteration, because logits are not calculated.
    // Not thread-safe. For parallel inference, call rwkv_clone_context to create one rwkv_context for each thread.
    // Returns false on any error.
    // - token: next token index, in range 0 <= token < n_vocab.
    // - state_in: FP32 buffer of size rwkv_get_state_len(); or NULL, if this is a first pass.
    // - state_out: FP32 buffer of size rwkv_get_state_len(). This buffer will be written to if non-NULL.
    // - logits_out: FP32 buffer of size rwkv_get_logits_len(). This buffer will be written to if non-NULL.
    RWKV_API bool rwkv_eval(
        struct rwkv_context * ctx,
        const uint32_t token,
        const float * state_in,
        float * state_out,
        float * logits_out
    );

    // Evaluates the model for a sequence of tokens.
    // Uses a faster algorithm than `rwkv_eval` if you do not need the state and logits for every token. Best used with sequence lengths of 64 or so.
    // Has to build a computation graph on the first call for a given sequence, but will use this cached graph for subsequent calls of the same sequence length.
    //
    // NOTE ON GGML NODE LIMIT
    //
    // ggml has a hard-coded limit on max amount of nodes in a computation graph. The sequence graph is built in a way that quickly exceedes
    // this limit when using large models and/or large sequence lengths.
    // Fortunately, rwkv.cpp's fork of ggml has increased limit which was tested to work for sequence lengths up to 64 for 14B models.
    //
    // If you get `GGML_ASSERT: ...\ggml.c:16941: cgraph->n_nodes < GGML_MAX_NODES`, this means you've exceeded the limit.
    // To get rid of the assertion failure, reduce the model size and/or sequence length.
    //
    // TODO When Metal (MPS) support is implemented, check that large sequence lengths work
    //
    // You can pass NULL to logits_out whenever logits are not needed. This can improve speed by ~10 ms per iteration, because logits are not calculated.
    // Not thread-safe. For parallel inference, call `rwkv_clone_context` to create one rwkv_context for each thread.
    // Returns false on any error.
    // - tokens: pointer to an array of tokens. If NULL, the graph will be built and cached, but not executed: this can be useful for initialization.
    // - sequence_len: number of tokens to read from the array.
    // - state_in: FP32 buffer of size rwkv_get_state_len(), or NULL if this is a first pass.
    // - state_out: FP32 buffer of size rwkv_get_state_len(). This buffer will be written to if non-NULL.
    // - logits_out: FP32 buffer of size rwkv_get_logits_len(). This buffer will be written to if non-NULL.
    RWKV_API bool rwkv_eval_sequence(
        struct rwkv_context * ctx,
        const uint32_t * tokens,
        const size_t sequence_len,
        const float * state_in,
        float * state_out,
        float * logits_out
    );

    // Evaluates the model for a sequence of tokens using `rwkv_eval_sequence`, splitting a potentially long sequence into fixed-length chunks.
    // This function is useful for processing complete prompts and user input in chat & role-playing use-cases.
    // It is recommended to use this function instead of `rwkv_eval_sequence` to avoid mistakes and get maximum performance.
    //
    // Chunking allows processing sequences of thousands of tokens, while not reaching the ggml's node limit and not consuming too much memory.
    // A reasonable and recommended value of chunk size is 16. If you want maximum performance, try different chunk sizes in range [2..64]
    // and choose one that works the best in your use case.
    //
    // Not thread-safe. For parallel inference, call `rwkv_clone_context` to create one rwkv_context for each thread.
    // Returns false on any error.
    // - tokens: pointer to an array of tokens. If NULL, the graph will be built and cached, but not executed: this can be useful for initialization.
    // - sequence_len: number of tokens to read from the array.
    // - chunk_size: size of each chunk in tokens, must be positive.
    // - state_in: FP32 buffer of size rwkv_get_state_len(), or NULL if this is a first pass.
    // - state_out: FP32 buffer of size rwkv_get_state_len(). This buffer will be written to if non-NULL.
    // - logits_out: FP32 buffer of size rwkv_get_logits_len(). This buffer will be written to if non-NULL.
    RWKV_API bool rwkv_eval_sequence_in_chunks(
        struct rwkv_context * ctx,
        const uint32_t * tokens,
        const size_t sequence_len,
        const size_t chunk_size,
        const float * state_in,
        float * state_out,
        float * logits_out
    );

    // Returns the number of tokens in the given model's vocabulary.
    // Useful for telling 20B_tokenizer models (n_vocab = 50277) apart from World models (n_vocab = 65536).
    RWKV_API size_t rwkv_get_n_vocab(const struct rwkv_context * ctx);

    // Returns the number of elements in the given model's embedding.
    // Useful for reading individual fields of a model's hidden state.
    RWKV_API size_t rwkv_get_n_embed(const struct rwkv_context * ctx);

    // Returns the number of layers in the given model.
    // A layer is a pair of RWKV and FFN operations, stacked multiple times throughout the model.
    // Embedding matrix and model head (unembedding matrix) are NOT counted in `n_layer`.
    // Useful for always offloading the entire model to GPU.
    RWKV_API size_t rwkv_get_n_layer(const struct rwkv_context * ctx);

    // Returns the number of float elements in a complete state for the given model.
    // This is the number of elements you'll need to allocate for a call to rwkv_eval, rwkv_eval_sequence, or rwkv_init_state.
    RWKV_API size_t rwkv_get_state_len(const struct rwkv_context * ctx);

    // Returns the number of float elements in the logits output of a given model.
    // This is currently always identical to n_vocab.
    RWKV_API size_t rwkv_get_logits_len(const struct rwkv_context * ctx);

    // Initializes the given state so that passing it to rwkv_eval or rwkv_eval_sequence would be identical to passing NULL.
    // Useful in cases where tracking the first call to these functions may be annoying or expensive.
    // State must be initialized for behavior to be defined, passing a zeroed state to rwkv.cpp functions will result in NaNs.
    // - state: FP32 buffer of size rwkv_get_state_len() to initialize
    RWKV_API void rwkv_init_state(const struct rwkv_context * ctx, float * state);

    // Frees all allocated memory and the context.
    // Does not need to be called on the same thread that created the rwkv_context.
    RWKV_API void rwkv_free(struct rwkv_context * ctx);

    // Quantizes FP32 or FP16 model to one of quantized formats.
    // Returns false on any error. Error messages would be printed to stderr.
    // - model_file_path_in: path to model file in ggml format, must be either FP32 or FP16.
    // - model_file_path_out: quantized model will be written here.
    // - format_name: must be one of available format names below.
    // Available format names:
    // - Q4_0
    // - Q4_1
    // - Q5_0
    // - Q5_1
    // - Q8_0
    RWKV_API bool rwkv_quantize_model_file(const char * model_file_path_in, const char * model_file_path_out, const char * format_name);

    // Returns system information string.
    RWKV_API const char * rwkv_get_system_info_string(void);

#if defined(__cplusplus)
}
#endif

#endif
