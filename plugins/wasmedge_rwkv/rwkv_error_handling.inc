thread_local enum rwkv_error_flags global_last_error = RWKV_ERROR_NONE;
thread_local bool global_print_errors = true;

inline static enum rwkv_error_flags operator|(enum rwkv_error_flags a, enum rwkv_error_flags b) {
    return static_cast<enum rwkv_error_flags>(static_cast<int>(a) | static_cast<int>(b));
}

inline static enum rwkv_error_flags operator|=(enum rwkv_error_flags & a, enum rwkv_error_flags b) {
    return a = a | b;
}

// Prints a message to stderr if error printing is enabled globally.
#define RWKV_MSG(...) do { if (global_print_errors) fprintf(stderr, __VA_ARGS__); } while (0)

// Prints a message to stderr if error printing is enabled in the context.
#define RWKV_CTX_MSG(ctx, ...) do { if (ctx->print_errors) fprintf(stderr, __VA_ARGS__); } while (0)

// If the condition x is false, adds ERR_VAL to the last error, and returns RET_VAL.
#define RWKV_ASSERT(ERR_VAL, RET_VAL, x) do { \
    if (!(x)) { \
        global_last_error |= ERR_VAL; \
        RWKV_MSG("\n%s:%d: %s\n", __FILE__, __LINE__, #x); \
        RWKV_MAYBE_BREAK; \
        return RET_VAL; \
    } } while (0)

// If the condition x is false, adds ERR_VAL to the last error, prints a message to stderr, and returns RET_VAL.
#define RWKV_ASSERT_MSG(ERR_VAL, RET_VAL, x, ...) do { \
    if (!(x)) { \
        global_last_error |= ERR_VAL; \
        RWKV_MSG(__VA_ARGS__); \
        RWKV_MSG("\n%s:%d: %s\n", __FILE__, __LINE__, #x); \
        RWKV_MAYBE_BREAK; \
        return RET_VAL; \
    } } while (0)

// If the condition x is false, adds ERR_VAL to the ctx's last error, prints a message to stderr, and returns RET_VAL.
#define RWKV_CTX_ASSERT_MSG(ctx, ERR_VAL, RET_VAL, x, ...) do { \
    if (!(x)) { \
        ((struct rwkv_context *) ctx)->last_error |= ERR_VAL; \
        RWKV_CTX_MSG(ctx, __VA_ARGS__); \
        RWKV_CTX_MSG(ctx, "\n%s:%d: %s\n", __FILE__, __LINE__, #x); \
        RWKV_MAYBE_BREAK; \
        return RET_VAL; \
    } } while (0)

// If the condition x is false, adds ERR_VAL to the ctx's last error, and returns RET_VAL.
#define RWKV_CTX_ASSERT(ctx, ERR_VAL, RET_VAL, x) do { \
    if (!(x)) { \
        ((struct rwkv_context *) ctx)->last_error |= ERR_VAL; \
        RWKV_CTX_MSG(ctx, "\n%s:%d: %s\n", __FILE__, __LINE__, #x); \
        RWKV_MAYBE_BREAK; \
        return RET_VAL; \
    } } while (0)

// If the condition x is false, returns RET_VAL.
#define RWKV_ENSURE(RET_VAL, x) do { \
    if (!(x)) { \
        RWKV_MSG("\n%s:%d: %s\n", __FILE__, __LINE__, #x); \
        RWKV_MAYBE_BREAK; \
        return RET_VAL; \
    } } while (0)

// If the condition x is false, prints a message to stderr, and returns RET_VAL.
#define RWKV_ENSURE_MSG(RET_VAL, x, ...) do { \
    if (!(x)) { \
        RWKV_MSG(__VA_ARGS__); \
        RWKV_MSG("\n%s:%d: %s\n", __FILE__, __LINE__, #x); \
        RWKV_MAYBE_BREAK; \
        return RET_VAL; \
    } } while (0)

// If the condition x is false, prints a message to stderr, and returns RET_VAL.
#define RWKV_CTX_ENSURE_MSG(ctx, RET_VAL, x, ...) do { \
    if (!(x)) { \
        ((struct rwkv_context *) ctx)->last_error |= ERR_VAL; \
        RWKV_CTX_MSG(ctx, __VA_ARGS__); \
        RWKV_CTX_MSG(ctx, "\n%s:%d: %s\n", __FILE__, __LINE__, #x); \
        RWKV_MAYBE_BREAK; \
        return RET_VAL; \
    } } while (0)

#define RWKV_ASSERT_FALSE_MSG(ERR_VAL, x, ...) RWKV_ASSERT_MSG(ERR_VAL, false, x, __VA_ARGS__)
#define RWKV_ASSERT_NULL_MSG(ERR_VAL, x, ...) RWKV_ASSERT_MSG(ERR_VAL, NULL, x, __VA_ARGS__)

#define RWKV_CTX_ASSERT_FALSE_MSG(ctx, ERR_VAL, x, ...) RWKV_CTX_ASSERT_MSG(ctx, ERR_VAL, false, x, __VA_ARGS__)

#define RWKV_ASSERT_FALSE(ERR_VAL, x) RWKV_ASSERT(ERR_VAL, false, x)
#define RWKV_ASSERT_NULL(ERR_VAL, x) RWKV_ASSERT(ERR_VAL, NULL, x)

#define RWKV_CTX_ASSERT_FALSE(ctx, ERR_VAL, x) RWKV_CTX_ASSERT(ctx, ERR_VAL, false, x)

#define RWKV_ENSURE_OR_FALSE(x) RWKV_ENSURE(false, x)
#define RWKV_ENSURE_OR_NULL(x) RWKV_ENSURE(NULL, x)
#define RWKV_ENSURE_OR_FALSE_MSG(x, ...) RWKV_ENSURE_MSG(false, x, __VA_ARGS__)
