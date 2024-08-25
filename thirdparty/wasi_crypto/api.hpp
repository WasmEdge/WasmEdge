// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

/**
 * THIS FILE IS AUTO-GENERATED from the following files:
 *   proposal_kx.witx, proposal_asymmetric_common.witx, proposal_common.witx, proposal_signatures.witx, proposal_symmetric.witx, proposal_external_secrets.witx
 *
 * @file
 * This file describes the [WASI] interface, consisting of functions, types,
 * and defined values (macros).
 *
 * The interface described here is greatly inspired by [CloudABI]'s clean,
 * thoughtfully-designed, capability-oriented, POSIX-style API.
 *
 * [CloudABI]: https://github.com/NuxiNL/cloudlibc
 * [WASI]: https://github.com/WebAssembly/WASI/
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

using const_uint8_t_ptr = uint32_t;
using uint8_t_ptr = uint32_t;

#define DEFINE_ENUM_OPERATORS(type)                                            \
  inline constexpr type operator~(type a) noexcept {                           \
    return static_cast<type>(~static_cast<std::underlying_type_t<type>>(a));   \
  }                                                                            \
  inline constexpr type operator|(type a, type b) noexcept {                   \
    return static_cast<type>(static_cast<std::underlying_type_t<type>>(a) |    \
                             static_cast<std::underlying_type_t<type>>(b));    \
  }                                                                            \
  inline constexpr type &operator|=(type &a, type b) noexcept {                \
    a = a | b;                                                                 \
    return a;                                                                  \
  }                                                                            \
  inline constexpr type operator&(type a, type b) noexcept {                   \
    return static_cast<type>(static_cast<std::underlying_type_t<type>>(a) &    \
                             static_cast<std::underlying_type_t<type>>(b));    \
  }                                                                            \
  inline constexpr type &operator&=(type &a, type b) noexcept {                \
    a = a & b;                                                                 \
    return a;                                                                  \
  }

static_assert(alignof(int8_t) == 1, "non-wasi data layout");
static_assert(alignof(uint8_t) == 1, "non-wasi data layout");
static_assert(alignof(int16_t) == 2, "non-wasi data layout");
static_assert(alignof(uint16_t) == 2, "non-wasi data layout");
static_assert(alignof(int32_t) == 4, "non-wasi data layout");
static_assert(alignof(uint32_t) == 4, "non-wasi data layout");
static_assert(alignof(int64_t) == 8, "non-wasi data layout");
static_assert(alignof(uint64_t) == 8, "non-wasi data layout");
static_assert(alignof(const_uint8_t_ptr) == 4, "non-wasi data layout");
static_assert(alignof(uint8_t_ptr) == 4, "non-wasi data layout");

/**
 * Error codes.
 */
enum __wasi_crypto_errno_e_t : uint16_t {
  /**
   * Operation succeeded.
   */
  __WASI_CRYPTO_ERRNO_SUCCESS = 0,

  /**
   * An error occurred when trying to during a conversion from a host type to a guest type.
   * 
   * Only an internal bug can throw this error.
   */
  __WASI_CRYPTO_ERRNO_GUEST_ERROR = 1,

  /**
   * The requested operation is valid, but not implemented by the host.
   */
  __WASI_CRYPTO_ERRNO_NOT_IMPLEMENTED = 2,

  /**
   * The requested feature is not supported by the chosen algorithm.
   */
  __WASI_CRYPTO_ERRNO_UNSUPPORTED_FEATURE = 3,

  /**
   * The requested operation is valid, but was administratively prohibited.
   */
  __WASI_CRYPTO_ERRNO_PROHIBITED_OPERATION = 4,

  /**
   * Unsupported encoding for an import or export operation.
   */
  __WASI_CRYPTO_ERRNO_UNSUPPORTED_ENCODING = 5,

  /**
   * The requested algorithm is not supported by the host.
   */
  __WASI_CRYPTO_ERRNO_UNSUPPORTED_ALGORITHM = 6,

  /**
   * The requested option is not supported by the currently selected algorithm.
   */
  __WASI_CRYPTO_ERRNO_UNSUPPORTED_OPTION = 7,

  /**
   * An invalid or incompatible key was supplied.
   * 
   * The key may not be valid, or was generated for a different algorithm or parameters set.
   */
  __WASI_CRYPTO_ERRNO_INVALID_KEY = 8,

  /**
   * The currently selected algorithm doesn't support the requested output length.
   * 
   * This error is thrown by non-extensible hash functions, when requesting an output size larger than they produce out of a single block.
   */
  __WASI_CRYPTO_ERRNO_INVALID_LENGTH = 9,

  /**
   * A signature or authentication tag verification failed.
   */
  __WASI_CRYPTO_ERRNO_VERIFICATION_FAILED = 10,

  /**
   * A secure random numbers generator is not available.
   * 
   * The requested operation requires random numbers, but the host cannot securely generate them at the moment.
   */
  __WASI_CRYPTO_ERRNO_RNG_ERROR = 11,

  /**
   * An error was returned by the underlying cryptography library.
   * 
   * The host may be running out of memory, parameters may be incompatible with the chosen implementation of an algorithm or another unexpected error may have happened.
   * 
   * Ideally, the specification should provide enough details and guidance to make this error impossible to ever be thrown.
   * 
   * Realistically, the WASI crypto module cannot possibly cover all possible error types implementations can return, especially since some of these may be language-specific.
   * This error can thus be thrown when other error types are not suitable, and when the original error comes from the cryptographic primitives themselves and not from the WASI module.
   */
  __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE = 12,

  /**
   * The supplied signature is invalid, or incompatible with the chosen algorithm.
   */
  __WASI_CRYPTO_ERRNO_INVALID_SIGNATURE = 13,

  /**
   * An attempt was made to close a handle that was already closed.
   */
  __WASI_CRYPTO_ERRNO_CLOSED = 14,

  /**
   * A function was called with an unassigned handle, a closed handle, or handle of an unexpected type.
   */
  __WASI_CRYPTO_ERRNO_INVALID_HANDLE = 15,

  /**
   * The host needs to copy data to a guest-allocated buffer, but that buffer is too small.
   */
  __WASI_CRYPTO_ERRNO_OVERFLOW = 16,

  /**
   * An internal error occurred.
   * 
   * This error is reserved to internal consistency checks, and must only be sent if the internal state of the host remains safe after an inconsistency was detected.
   */
  __WASI_CRYPTO_ERRNO_INTERNAL_ERROR = 17,

  /**
   * Too many handles are currently open, and a new one cannot be created.
   * 
   * Implementations are free to represent handles as they want, and to enforce limits to limit resources usage.
   */
  __WASI_CRYPTO_ERRNO_TOO_MANY_HANDLES = 18,

  /**
   * A key was provided, but the chosen algorithm doesn't support keys.
   * 
   * This is returned by symmetric operations.
   * 
   * Many hash functions, in particular, do not support keys without being used in particular constructions.
   * Blindly ignoring a key provided by mistake while trying to open a context for such as function could cause serious security vulnerabilities.
   * 
   * These functions must refuse to create the context and return this error instead.
   */
  __WASI_CRYPTO_ERRNO_KEY_NOT_SUPPORTED = 19,

  /**
   * A key is required for the chosen algorithm, but none was given.
   */
  __WASI_CRYPTO_ERRNO_KEY_REQUIRED = 20,

  /**
   * The provided authentication tag is invalid or incompatible with the current algorithm.
   * 
   * This error is returned by decryption functions and tag verification functions.
   * 
   * Unlike `verification_failed`, this error code is returned when the tag cannot possibly verify for any input.
   */
  __WASI_CRYPTO_ERRNO_INVALID_TAG = 21,

  /**
   * The requested operation is incompatible with the current scheme.
   * 
   * For example, the `symmetric_state_encrypt()` function cannot complete if the selected construction is a key derivation function.
   * This error code will be returned instead.
   */
  __WASI_CRYPTO_ERRNO_INVALID_OPERATION = 22,

  /**
   * A nonce is required.
   * 
   * Most encryption schemes require a nonce.
   * 
   * In the absence of a nonce, the WASI cryptography module can automatically generate one, if that can be done safely. The nonce can be retrieved later with the `symmetric_state_option_get()` function using the `nonce` parameter.
   * If automatically generating a nonce cannot be done safely, the module never falls back to an insecure option and requests an explicit nonce by throwing that error.
   */
  __WASI_CRYPTO_ERRNO_NONCE_REQUIRED = 23,

  /**
   * The provided nonce doesn't have a correct size for the given cipher.
   */
  __WASI_CRYPTO_ERRNO_INVALID_NONCE = 24,

  /**
   * The named option was not set.
   * 
   * The caller tried to read the value of an option that was not set.
   * This error is used to make the distinction between an empty option, and an option that was not set and left to its default value.
   */
  __WASI_CRYPTO_ERRNO_OPTION_NOT_SET = 25,

  /**
   * A key or key pair matching the requested identifier cannot be found using the supplied information.
   * 
   * This error is returned by a secrets manager via the `keypair_from_id()` function.
   */
  __WASI_CRYPTO_ERRNO_NOT_FOUND = 26,

  /**
   * The algorithm requires parameters that haven't been set.
   * 
   * Non-generic options are required and must be given by building an `options` set and giving that object to functions instantiating that algorithm.
   */
  __WASI_CRYPTO_ERRNO_PARAMETERS_MISSING = 27,

  /**
   * A requested computation is not done yet, and additional calls to the function are required.
   * 
   * Some functions, such as functions generating key pairs and password stretching functions, can take a long time to complete.
   * 
   * In order to avoid a host call to be blocked for too long, these functions can return prematurely, requiring additional calls with the same parameters until they complete.
   */
  __WASI_CRYPTO_ERRNO_IN_PROGRESS = 28,

  /**
   * Multiple keys have been provided, but they do not share the same type.
   * 
   * This error is returned when trying to build a key pair from a public key and a secret key that were created for different and incompatible algorithms.
   */
  __WASI_CRYPTO_ERRNO_INCOMPATIBLE_KEYS = 29,

  /**
   * A managed key or secret expired and cannot be used any more.
   */
  __WASI_CRYPTO_ERRNO_EXPIRED = 30,

};
static_assert(sizeof(__wasi_crypto_errno_e_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_crypto_errno_e_t) == 2, "witx calculated align");

/**
 * Encoding to use for importing or exporting a key pair.
 */
enum __wasi_keypair_encoding_e_t : uint16_t {
  /**
   * Raw bytes.
   */
  __WASI_KEYPAIR_ENCODING_RAW = 0,

  /**
   * PCSK8/DER encoding.
   */
  __WASI_KEYPAIR_ENCODING_PKCS8 = 1,

  /**
   * PEM encoding.
   */
  __WASI_KEYPAIR_ENCODING_PEM = 2,

  /**
   * Implementation-defined encoding.
   */
  __WASI_KEYPAIR_ENCODING_LOCAL = 3,

};
static_assert(sizeof(__wasi_keypair_encoding_e_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_keypair_encoding_e_t) == 2, "witx calculated align");

/**
 * Encoding to use for importing or exporting a public key.
 */
enum __wasi_publickey_encoding_e_t : uint16_t {
  /**
   * Raw bytes.
   */
  __WASI_PUBLICKEY_ENCODING_RAW = 0,

  /**
   * PKCS8/DER encoding.
   */
  __WASI_PUBLICKEY_ENCODING_PKCS8 = 1,

  /**
   * PEM encoding.
   */
  __WASI_PUBLICKEY_ENCODING_PEM = 2,

  /**
   * SEC-1 encoding.
   */
  __WASI_PUBLICKEY_ENCODING_SEC = 3,

  /**
   * Implementation-defined encoding.
   */
  __WASI_PUBLICKEY_ENCODING_LOCAL = 4,

};
static_assert(sizeof(__wasi_publickey_encoding_e_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_publickey_encoding_e_t) == 2, "witx calculated align");

/**
 * Encoding to use for importing or exporting a secret key.
 */
enum __wasi_secretkey_encoding_e_t : uint16_t {
  /**
   * Raw bytes.
   */
  __WASI_SECRETKEY_ENCODING_RAW = 0,

  /**
   * PKCS8/DER encoding.
   */
  __WASI_SECRETKEY_ENCODING_PKCS8 = 1,

  /**
   * PEM encoding.
   */
  __WASI_SECRETKEY_ENCODING_PEM = 2,

  /**
   * SEC-1 encoding.
   */
  __WASI_SECRETKEY_ENCODING_SEC = 3,

  /**
   * Implementation-defined encoding.
   */
  __WASI_SECRETKEY_ENCODING_LOCAL = 4,

};
static_assert(sizeof(__wasi_secretkey_encoding_e_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_secretkey_encoding_e_t) == 2, "witx calculated align");

/**
 * Encoding to use for importing or exporting a signature.
 */
enum __wasi_signature_encoding_e_t : uint16_t {
  /**
   * Raw bytes.
   */
  __WASI_SIGNATURE_ENCODING_RAW = 0,

  /**
   * DER encoding.
   */
  __WASI_SIGNATURE_ENCODING_DER = 1,

};
static_assert(sizeof(__wasi_signature_encoding_e_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_signature_encoding_e_t) == 2, "witx calculated align");

/**
 * An algorithm category.
 */
enum __wasi_algorithm_type_e_t : uint16_t {
  __WASI_ALGORITHM_TYPE_SIGNATURES = 0,

  __WASI_ALGORITHM_TYPE_SYMMETRIC = 1,

  __WASI_ALGORITHM_TYPE_KEY_EXCHANGE = 2,

};
static_assert(sizeof(__wasi_algorithm_type_e_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_algorithm_type_e_t) == 2, "witx calculated align");

/**
 * Version of a managed key.
 * 
 * A version can be an arbitrary `u64` integer, with the exception of some reserved values.
 */
using __wasi_version_t = uint64_t;

static_assert(sizeof(__wasi_version_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_version_t) == 8, "witx calculated align");

/**
 * Size of a value.
 */
using __wasi_size_t = uint32_t;

static_assert(sizeof(__wasi_size_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_size_t) == 4, "witx calculated align");

/**
 * A UNIX timestamp, in seconds since 01/01/1970.
 */
using __wasi_timestamp_t = uint64_t;

static_assert(sizeof(__wasi_timestamp_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_timestamp_t) == 8, "witx calculated align");

/**
 * Handle for functions returning output whose size may be large or not known in advance.
 * 
 * An `array_output` object contains a host-allocated byte array.
 * 
 * A guest can get the size of that array after a function returns in order to then allocate a buffer of the correct size.
 * In addition, the content of such an object can be consumed by a guest in a streaming fashion.
 * 
 * An `array_output` handle is automatically closed after its full content has been consumed.
 */
using __wasi_array_output_t = int32_t;

static_assert(sizeof(__wasi_array_output_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_array_output_t) == 4, "witx calculated align");

/**
 * A set of options.
 * 
 * This type is used to set non-default parameters.
 * 
 * The exact set of allowed options depends on the algorithm being used.
 */
using __wasi_options_t = int32_t;

static_assert(sizeof(__wasi_options_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_options_t) == 4, "witx calculated align");

/**
 * A handle to the optional secrets management facilities offered by a host.
 * 
 * This is used to generate, retrieve and invalidate managed keys.
 */
using __wasi_secrets_manager_t = int32_t;

static_assert(sizeof(__wasi_secrets_manager_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_secrets_manager_t) == 4, "witx calculated align");

/**
 * A key pair.
 */
using __wasi_keypair_t = int32_t;

static_assert(sizeof(__wasi_keypair_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_keypair_t) == 4, "witx calculated align");

/**
 * A state to absorb data to be signed.
 * 
 * After a signature has been computed or verified, the state remains valid for further operations.
 * 
 * A subsequent signature would sign all the data accumulated since the creation of the state object.
 */
using __wasi_signature_state_t = int32_t;

static_assert(sizeof(__wasi_signature_state_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_signature_state_t) == 4, "witx calculated align");

/**
 * A signature.
 */
using __wasi_signature_t = int32_t;

static_assert(sizeof(__wasi_signature_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_signature_t) == 4, "witx calculated align");

/**
 * A public key, for key exchange and signature verification.
 */
using __wasi_publickey_t = int32_t;

static_assert(sizeof(__wasi_publickey_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_publickey_t) == 4, "witx calculated align");

/**
 * A secret key, for key exchange mechanisms.
 */
using __wasi_secretkey_t = int32_t;

static_assert(sizeof(__wasi_secretkey_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_secretkey_t) == 4, "witx calculated align");

/**
 * A state to absorb signed data to be verified.
 */
using __wasi_signature_verification_state_t = int32_t;

static_assert(sizeof(__wasi_signature_verification_state_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_signature_verification_state_t) == 4, "witx calculated align");

/**
 * A state to perform symmetric operations.
 * 
 * The state is not reset nor invalidated after an option has been performed.
 * Incremental updates and sessions are thus supported.
 */
using __wasi_symmetric_state_t = int32_t;

static_assert(sizeof(__wasi_symmetric_state_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_symmetric_state_t) == 4, "witx calculated align");

/**
 * A symmetric key.
 * 
 * The key can be imported from raw bytes, or can be a reference to a managed key.
 * 
 * If it was imported, the host will wipe it from memory as soon as the handle is closed.
 */
using __wasi_symmetric_key_t = int32_t;

static_assert(sizeof(__wasi_symmetric_key_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_symmetric_key_t) == 4, "witx calculated align");

/**
 * An authentication tag.
 * 
 * This is an object returned by functions computing authentication tags.
 * 
 * A tag can be compared against another tag (directly supplied as raw bytes) in constant time with the `symmetric_tag_verify()` function.
 * 
 * This object type can't be directly created from raw bytes. They are only returned by functions computing MACs.
 * 
 * The host is responsible for securely wiping them from memory on close.
 */
using __wasi_symmetric_tag_t = int32_t;

static_assert(sizeof(__wasi_symmetric_tag_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_symmetric_tag_t) == 4, "witx calculated align");

/**
 * Options index, only required by the Interface Types translation layer.
 */
enum __wasi_opt_options_u_e_t : uint8_t {
  __WASI_OPT_OPTIONS_U_SOME = 0,

  __WASI_OPT_OPTIONS_U_NONE = 1,

};
static_assert(sizeof(__wasi_opt_options_u_e_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_opt_options_u_e_t) == 1, "witx calculated align");

/**
 * An optional options set.
 * 
 * This union simulates an `Option<Options>` type to make the `options` parameter of some functions optional.
 */
union __wasi_opt_options_u_t {
  __wasi_options_t some;
};
struct __wasi_opt_options_t {
  __wasi_opt_options_u_e_t tag;
  __wasi_opt_options_u_t u;
};

static_assert(sizeof(__wasi_opt_options_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_opt_options_t) == 4, "witx calculated align");
static_assert(offsetof(__wasi_opt_options_t, u) == 4, "witx calculated union offset");

/**
 * Symmetric key index, only required by the Interface Types translation layer.
 */
enum __wasi_opt_symmetric_key_u_e_t : uint8_t {
  __WASI_OPT_SYMMETRIC_KEY_U_SOME = 0,

  __WASI_OPT_SYMMETRIC_KEY_U_NONE = 1,

};
static_assert(sizeof(__wasi_opt_symmetric_key_u_e_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_opt_symmetric_key_u_e_t) == 1, "witx calculated align");

/**
 * An optional symmetric key.
 * 
 * This union simulates an `Option<SymmetricKey>` type to make the `symmetric_key` parameter of some functions optional.
 */
union __wasi_opt_symmetric_key_u_t {
  __wasi_symmetric_key_t some;
};
struct __wasi_opt_symmetric_key_t {
  __wasi_opt_symmetric_key_u_e_t tag;
  __wasi_opt_symmetric_key_u_t u;
};

static_assert(sizeof(__wasi_opt_symmetric_key_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_opt_symmetric_key_t) == 4, "witx calculated align");
static_assert(offsetof(__wasi_opt_symmetric_key_t, u) == 4, "witx calculated union offset");

using __wasi_u64_t = uint64_t;

static_assert(sizeof(__wasi_u64_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_u64_t) == 8, "witx calculated align");

/**
 * `$kx_keypair` is just an alias for `$keypair`
 * 
 * However, bindings may want to define a specialized type `kx_keypair` as a super class of `keypair`.
 */
using __wasi_kx_keypair_t = __wasi_keypair_t;

static_assert(sizeof(__wasi_kx_keypair_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_kx_keypair_t) == 4, "witx calculated align");

/**
 * `$kx_publickey` is just an alias for `$publickey`
 * 
 * However, bindings may want to define a specialized type `kx_publickey` as a super class of `publickey`, with additional methods such as `dh`.
 */
using __wasi_kx_publickey_t = __wasi_publickey_t;

static_assert(sizeof(__wasi_kx_publickey_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_kx_publickey_t) == 4, "witx calculated align");

/**
 * `$kx_secretkey` is just an alias for `$secretkey`
 * 
 * However, bindings may want to define a specialized type `kx_secretkey` as a super class of `secretkeykey`, with additional methods such as `dh`.
 */
using __wasi_kx_secretkey_t = __wasi_secretkey_t;

static_assert(sizeof(__wasi_kx_secretkey_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_kx_secretkey_t) == 4, "witx calculated align");

/**
 * `$signature_keypair` is just an alias for `$keypair`
 * 
 * However, bindings may want to define a specialized type `signature_keypair` as a super class of `keypair`, with additional methods such as `sign`.
 */
using __wasi_signature_keypair_t = __wasi_keypair_t;

static_assert(sizeof(__wasi_signature_keypair_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_signature_keypair_t) == 4, "witx calculated align");

/**
 * `$signature_publickey` is just an alias for `$publickey`
 * 
 * However, bindings may want to define a specialized type `signature_publickey` as a super class of `publickey`, with additional methods such as `verify`.
 */
using __wasi_signature_publickey_t = __wasi_publickey_t;

static_assert(sizeof(__wasi_signature_publickey_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_signature_publickey_t) == 4, "witx calculated align");

/**
 * `$signature_secretkey` is just an alias for `$secretkey`
 * 
 * However, bindings may want to define a specialized type `signature_secretkey` as a super class of `secretkey`.
 */
using __wasi_signature_secretkey_t = __wasi_secretkey_t;

static_assert(sizeof(__wasi_signature_secretkey_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_signature_secretkey_t) == 4, "witx calculated align");
