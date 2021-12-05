// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/asymmetric_common/keypair.h"
#include "host/wasi_crypto/asymmetric_common/publickey.h"
#include "host/wasi_crypto/asymmetric_common/secretkey.h"
#include "host/wasi_crypto/common/array_output.h"
#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/error.h"
#include "host/wasi_crypto/handles.h"
#include "host/wasi_crypto/lock.h"
#include "host/wasi_crypto/signature/alg.h"
#include "host/wasi_crypto/signature/signature.h"
#include "host/wasi_crypto/symmetric/state.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class WasiCryptoContext {
public:
  ///-------------------------------------------common---------------------------------------

  /// Return the length of an `array_output` object.
  ///
  /// This allows a guest to allocate a buffer of the correct size in order to
  /// copy the output of a function returning this object type.
  WasiCryptoExpect<__wasi_size_t>
  arrayOutputLen(__wasi_array_output_t ArrayOutputHandle);

  /// Copy the content of an `array_output` object into an application-allocated
  /// buffer.
  ///
  /// Multiple calls to that function can be made in order to consume the data
  /// in a streaming fashion, if necessary.
  ///
  /// The function returns the number of bytes that were actually copied. `0`
  /// means that the end of the stream has been reached. The total size always
  /// matches the output of `array_output_len()`.
  ///
  /// The handle is automatically closed after all the data has been consumed.
  ///
  /// Example usage:
  ///
  /// ```rust
  /// let len = array_output_len(output_handle)?;
  /// let mut out = vec![0u8; len];
  /// array_output_pull(output_handle, &mut out)?;
  ///
  WasiCryptoExpect<__wasi_size_t>
  arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                  Span<uint8_t> BufPtr);

  /// Create a new object to set non-default options.
  ///
  /// Example usage:
  ///
  /// ```rust
  /// let options_handle = options_open(AlgorithmType::Symmetric)?;
  /// options_set(options_handle, "context", context)?;
  /// options_set_u64(options_handle, "threads", 4)?;
  /// let state = symmetric_state_open("BLAKE3", None, Some(options_handle))?;
  /// options_close(options_handle)?;
  /// ```
  WasiCryptoExpect<__wasi_options_t>
  optionsOpen(__wasi_algorithm_type_e_t AlgorithmType);

  /// Destroy an options object.
  ///
  /// Objects are reference counted. It is safe to close an object immediately
  /// after the last function needing it is called.
  WasiCryptoExpect<void> optionsClose(__wasi_options_t Handle);

  /// Set or update an option.
  ///
  /// This is used to set algorithm-specific parameters.
  ///
  /// This function may return `unsupported_option` if an option that doesn't
  /// exist for any implemented algorithms is specified.
  WasiCryptoExpect<void> optionsSet(__wasi_options_t OptionsHandle,
                                    std::string_view Name,
                                    Span<uint8_t const> Value);

  /// Set or update an integer option.
  ///
  /// This is used to set algorithm-specific parameters.
  ///
  /// This function may return `unsupported_option` if an option that doesn't
  /// exist for any implemented algorithms is specified.
  WasiCryptoExpect<void> optionsSetU64(__wasi_options_t OptionsHandle,
                                       std::string_view Name, uint64_t Value);

  /// Set or update a guest-allocated memory that the host can use or return
  /// data into.
  ///
  /// This is for example used to set the scratch buffer required by memory-hard
  /// functions.
  ///
  /// This function may return `unsupported_option` if an option that doesn't
  /// exist for any implemented algorithms is specified.
  WasiCryptoExpect<void> optionsSetGuestBuffer(__wasi_options_t OptionsHandle,
                                               std::string_view Name,
                                               Span<uint8_t> Buf);

  WasiCryptoExpect<__wasi_secrets_manager_t>
  secretsMangerOpen(std::optional<__wasi_options_t> Options);

  WasiCryptoExpect<void>
  secretsMangerClose(__wasi_secrets_manager_t SecretsManger);

  WasiCryptoExpect<void>
  secretsManagerInvalidate(__wasi_secrets_manager_t SecretsManger,
                           Span<uint8_t const> KeyId, __wasi_version_t Version);

  ///-------------------------------------------symmetric---------------------------------------

  /// Generate a new symmetric key for a given algorithm.
  ///
  /// `options` can be `std::nullopt` to use the default parameters, or an
  /// algoritm-specific set of parameters to override.
  ///
  /// This function may return `unsupported_feature` if key generation is not
  /// supported by the host for the chosen algorithm, or `unsupported_algorithm`
  /// if the algorithm is not supported by the host.
  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyGenerate(SymmetricAlgorithm Alg,
                       std::optional<__wasi_options_t> OptionsHandle);

  /// Create a symmetric key from raw material.
  ///
  /// The algorithm is internally stored along with the key, and trying to use
  /// the key with an operation expecting a different algorithm will return
  /// `invalid_key`.
  ///
  /// The function may also return `unsupported_algorithm` if the algorithm is
  /// not supported by the host.
  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyImport(SymmetricAlgorithm Alg, Span<uint8_t const> Raw);

  /// Export a symmetric key as raw material.
  ///
  /// This is mainly useful to export a managed key.
  ///
  /// May return `prohibited_operation` if this operation is denied.
  WasiCryptoExpect<__wasi_array_output_t>
  symmetricKeyExport(__wasi_symmetric_key_t KeyHandle);

  /// Destroy a symmetric key.
  ///
  /// Objects are reference counted. It is safe to close an object immediately
  /// after the last function needing it is called.
  WasiCryptoExpect<void> symmetricKeyClose(__wasi_symmetric_key_t SymmetricKey);

  /// __(optional)__
  /// Generate a new managed symmetric key.
  ///
  /// The key is generated and stored by the secrets management facilities.
  ///
  /// It may be used through its identifier, but the host may not allow it to be
  /// exported.
  ///
  /// The function returns the `unsupported_feature` error code if secrets
  /// management facilities are not supported by the host, or
  /// `unsupported_algorithm` if a key cannot be created for the chosen
  /// algorithm.
  ///
  /// The function may also return `unsupported_algorithm` if the algorithm is
  /// not supported by the host.
  ///
  /// This is also an optional import, meaning that the function may not even
  /// exist.
  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyGenerateManaged(__wasi_secrets_manager_t SecretsManager,
                              SymmetricAlgorithm Alg,
                              std::optional<__wasi_options_t> Options);

  /// __(optional)__
  /// Store a symmetric key into the secrets manager.
  ///
  /// On success, the function stores the key identifier into
  /// `$symmetric_key_id`, into which up to `$symmetric_key_id_max_len` can be
  /// written.
  ///
  /// The function returns `overflow` if the supplied buffer is too small.
  WasiCryptoExpect<void>
  symmetricKeyStoreManaged(__wasi_secrets_manager_t SecretsManager,
                           __wasi_symmetric_key_t SymmetricKey,
                           uint8_t_ptr SymmetricKeyId,
                           __wasi_size_t SymmetricKeyIdMaxLen);

  /// __(optional)__
  /// Replace a managed symmetric key.
  ///
  /// This function crates a new version of a managed symmetric key, by
  /// replacing `$kp_old` with `$kp_new`.
  ///
  /// It does several things:
  ///
  /// - The key identifier for `$symmetric_key_new` is set to the one of
  /// `$symmetric_key_old`.
  /// - A new, unique version identifier is assigned to `$kp_new`. This version
  /// will be equivalent to using `$version_latest` until the key is replaced.
  /// - The `$symmetric_key_old` handle is closed.
  ///
  /// Both keys must share the same algorithm and have compatible parameters. If
  /// this is not the case, `incompatible_keys` is returned.
  ///
  /// The function may also return the `unsupported_feature` error code if
  /// secrets management facilities are not supported by the host, or if keys
  /// cannot be rotated.
  ///
  /// Finally, `prohibited_operation` can be returned if `$symmetric_key_new`
  /// wasn't created by the secrets manager, and the secrets manager prohibits
  /// imported keys.
  ///
  /// If the operation succeeded, the new version is returned.
  ///
  /// This is an optional import, meaning that the function may not even exist.
  WasiCryptoExpect<__wasi_version_t>
  symmetricKeyReplaceManaged(__wasi_secrets_manager_t SecretsManager,
                             __wasi_symmetric_key_t SymmetricKeyOld,
                             __wasi_symmetric_key_t SymmetricKeyNew);

  /// __(optional)__
  /// Return the key identifier and version of a managed symmetric key.
  ///
  /// If the key is not managed, `unsupported_feature` is returned instead.
  ///
  /// This is an optional import, meaning that the function may not even exist.
  WasiCryptoExpect<std::tuple<__wasi_size_t, __wasi_version_t>>
  symmetricKeyId(__wasi_symmetric_key_t SymmetricKey,
                 uint8_t_ptr SymmetricKeyId,
                 __wasi_size_t SymmetricKeyIdMaxLen);

  /// __(optional)__
  /// Return a managed symmetric key from a key identifier.
  ///
  /// `kp_version` can be set to `version_latest` to retrieve the most recent
  /// version of a symmetric key.
  ///
  /// If no key matching the provided information is found, `not_found` is
  /// returned instead.
  ///
  /// This is an optional import, meaning that the function may not even exist.
  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricKeyFromId(__wasi_secrets_manager_t SecretsManager,
                     Span<uint8_t> SymmetricKey,
                     __wasi_version_t SymmetricKeyVersion);

  /// Create a new state to aborb and produce data using symmetric operations.
  ///
  /// The state remains valid after every operation in order to support
  /// incremental updates.
  ///
  /// The function has two optional parameters: a key and an options set.
  ///
  /// It will fail with a `key_not_supported` error code if a key was provided
  /// but the chosen algorithm doesn't natively support keying.
  ///
  /// On the other hand, if a key is required, but was not provided, a
  /// `key_required` error will be thrown.
  ///
  /// Some algorithms may require additional parameters. They have to be
  /// supplied as an options set:
  ///
  /// If some parameters are mandatory but were not set, the
  /// `parameters_missing` error code will be returned.
  ///
  /// A notable exception is the `nonce` parameter, that is common to most AEAD
  /// constructions.
  ///
  /// If a nonce is required but was not supplied:
  ///
  /// - If it is safe to do so, the host will automatically generate a nonce.
  /// This is true for nonces that are large enough to be randomly generated, or
  /// if the host is able to maintain a global counter.
  /// - If not, the function will fail and return the dedicated `nonce_required`
  /// error code.
  ///
  /// A nonce that was automatically generated can be retrieved after the
  /// function returns with `symmetric_state_get(state_handle, "nonce")`.
  ///
  /// **Sample usage patterns:**
  ///
  /// - **Hashing**
  ///
  /// - **MAC**
  ///
  /// Verification:
  ///
  /// - **Tuple hashing**
  ///
  /// Unlike MACs and regular hash functions, inputs are domain separated
  /// instead of being concatenated.
  ///
  /// - **Key derivation using extract-and-expand**
  ///
  /// Extract:
  ///
  /// Expand:
  ///
  /// - **Key derivation using a XOF**
  ///
  /// - **Password hashing**
  ///
  /// - **AEAD encryption with an explicit nonce**
  ///
  /// - **AEAD encryption with automatic nonce generation**
  ///
  /// - **Session authenticated modes**
  ///
  WasiCryptoExpect<__wasi_symmetric_state_t>
  symmetricStateOpen(SymmetricAlgorithm Alg,
                     std::optional<__wasi_symmetric_key_t> KeyHandle,
                     std::optional<__wasi_options_t> OptionsHandle);

  /// Retrieve a parameter from the current state.
  ///
  /// In particular, `symmetric_state_options_get("nonce")` can be used to get a
  /// nonce that as automatically generated.
  ///
  /// The function may return `options_not_set` if an option was not set, which
  /// is different from an empty value.
  ///
  /// It may also return `unsupported_option` if the option doesn't exist for
  /// the chosen algorithm.
  WasiCryptoExpect<__wasi_size_t>
  symmetricStateOptionsGet(__wasi_symmetric_state_t Handle,
                           std::string_view Name, Span<uint8_t> Value);

  /// Retrieve an integer parameter from the current state.
  ///
  /// In particular, `symmetric_state_options_get("nonce")` can be used to get a
  /// nonce that as automatically generated.
  ///
  /// The function may return `options_not_set` if an option was not set.
  ///
  /// It may also return `unsupported_option` if the option doesn't exist for
  /// the chosen algorithm.
  WasiCryptoExpect<uint64_t>
  symmetricStateOptionsGetU64(__wasi_symmetric_state_t Handle,
                              std::string_view Name);

  /// Destroy a symmetric state.
  ///
  /// Objects are reference counted. It is safe to close an object immediately
  /// after the last function needing it is called.
  WasiCryptoExpect<void> symmetricStateClose(__wasi_symmetric_state_t Handle);

  /// Absorb data into the state.
  ///
  /// - **Hash functions:** adds data to be hashed.
  /// - **MAC functions:** adds data to be authenticated.
  /// - **Tuplehash-like constructions:** adds a new tuple to the state.
  /// - **Key derivation functions:** adds to the IKM or to the subkey
  /// information.
  /// - **AEAD constructions:** adds additional data to be authenticated.
  /// - **Stateful hash objects, permutation-based constructions:** absorbs.
  ///
  /// If the chosen algorithm doesn't accept input data, the `invalid_operation`
  /// error code is returned.
  ///
  /// If too much data has been fed for the algorithm, `overflow` may be thrown.
  WasiCryptoExpect<void> symmetricStateAbsorb(__wasi_symmetric_state_t Handle,
                                              Span<uint8_t const> Data);

  /// Squeeze bytes from the state.
  ///
  /// - **Hash functions:** this tries to output an `out_len` bytes digest from
  /// the absorbed data. The hash function output will be truncated if
  /// necessary. If the requested size is too large, the `invalid_len` error
  /// code is returned.
  /// - **Key derivation functions:** : outputs an arbitrary-long derived key.
  /// - **RNGs, DRBGs, stream ciphers:**: outputs arbitrary-long data.
  /// - **Stateful hash objects, permutation-based constructions:** squeeze.
  ///
  /// Other kinds of algorithms may return `invalid_operation` instead.
  ///
  /// For password-stretching functions, the function may return `in_progress`.
  /// In that case, the guest should retry with the same parameters until the
  /// function completes.
  WasiCryptoExpect<void> symmetricStateSqueeze(__wasi_symmetric_state_t Handle,
                                               Span<uint8_t> Out);

  /// Compute and return a tag for all the data injected into the state so far.
  ///
  /// - **MAC functions**: returns a tag authenticating the absorbed data.
  /// - **Tuplehash-like constructions:** returns a tag authenticating all the
  /// absorbed tuples.
  /// - **Password-hashing functions:** returns a standard string containing all
  /// the required parameters for password verification.
  ///
  /// Other kinds of algorithms may return `invalid_operation` instead.
  ///
  /// For password-stretching functions, the function may return `in_progress`.
  /// In that case, the guest should retry with the same parameters until the
  /// function completes.
  WasiCryptoExpect<__wasi_symmetric_tag_t>
  symmetricStateSqueezeTag(__wasi_symmetric_state_t Handle);

  /// Use the current state to produce a key for a target algorithm.
  ///
  /// For extract-then-expand constructions, this returns the PRK.
  /// For session-base authentication encryption, this returns a key that can be
  /// used to resume a session without storing a nonce.
  ///
  /// `invalid_operation` is returned for algorithms not supporting this
  /// operation.
  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricStateSqueezeKey(__wasi_symmetric_state_t StateHandle,
                           SymmetricAlgorithm Alg);

  /// Return the maximum length of an authentication tag for the current
  /// algorithm.
  ///
  /// This allows guests to compute the size required to store a ciphertext
  /// along with its authentication tag.
  ///
  /// The returned length may include the encryption mode's padding requirements
  /// in addition to the actual tag.
  ///
  /// For an encryption operation, the size of the output buffer should be
  /// `input_len + symmetric_state_max_tag_len()`.
  ///
  /// For a decryption operation, the size of the buffer that will store the
  /// decrypted data must be `ciphertext_len - symmetric_state_max_tag_len()`.
  WasiCryptoExpect<__wasi_size_t>
  symmetricStateMaxTagLen(__wasi_symmetric_state_t StateHandle);

  /// Encrypt data with an attached tag.
  ///
  /// - **Stream cipher:** adds the input to the stream cipher output. `out_len`
  /// and `data_len` can be equal, as no authentication tags will be added.
  /// - **AEAD:** encrypts `data` into `out`, including the authentication tag
  /// to the output. Additional data must have been previously absorbed using
  /// `symmetric_state_absorb()`. The `symmetric_state_max_tag_len()` function
  /// can be used to retrieve the overhead of adding the tag, as well as padding
  /// if necessary.
  /// - **SHOE, Xoodyak, Strobe:** encrypts data, squeezes a tag and appends it
  /// to the output.
  ///
  /// If `out` and `data` are the same address, encryption may happen in-place.
  ///
  /// The function returns the actual size of the ciphertext along with the tag.
  ///
  /// `invalid_operation` is returned for algorithms not supporting encryption.
  WasiCryptoExpect<__wasi_size_t>
  symmetricStateEncrypt(__wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
                        Span<uint8_t const> Data);

  /// Encrypt data, with a detached tag.
  ///
  /// - **Stream cipher:** returns `invalid_operation` since stream ciphers do
  /// not include authentication tags.
  /// - **AEAD:** encrypts `data` into `out` and returns the tag separately.
  /// Additional data must have been previously absorbed using
  /// `symmetric_state_absorb()`. The output and input buffers must be of the
  /// same length.
  /// - **SHOE, Xoodyak, Strobe:** encrypts data and squeezes a tag.
  ///
  /// If `out` and `data` are the same address, encryption may happen in-place.
  ///
  /// The function returns the tag.
  ///
  /// `invalid_operation` is returned for algorithms not supporting encryption.
  WasiCryptoExpect<__wasi_symmetric_key_t>
  symmetricStateEncryptDetached(__wasi_symmetric_state_t StateHandle,
                                Span<uint8_t> Out, Span<uint8_t const> Data);

  /// - **Stream cipher:** adds the input to the stream cipher output. `out_len`
  /// and `data_len` can be equal, as no authentication tags will be added.
  /// - **AEAD:** decrypts `data` into `out`. Additional data must have been
  /// previously absorbed using `symmetric_state_absorb()`.
  /// - **SHOE, Xoodyak, Strobe:** decrypts data, squeezes a tag and verify that
  /// it matches the one that was appended to the ciphertext.
  ///
  /// If `out` and `data` are the same address, decryption may happen in-place.
  ///
  /// `out_len` must be exactly `data_len` + `max_tag_len` bytes.
  ///
  /// The function returns the actual size of the decrypted message, which can
  /// be smaller than `out_len` for modes that requires padding.
  ///
  /// `invalid_tag` is returned if the tag didn't verify.
  ///
  /// `invalid_operation` is returned for algorithms not supporting encryption.
  WasiCryptoExpect<__wasi_size_t>
  symmetricStateDecrypt(__wasi_symmetric_state_t StateHandle, Span<uint8_t> Out,
                        Span<uint8_t const> Data);

  /// - **Stream cipher:** returns `invalid_operation` since stream ciphers do
  /// not include authentication tags.
  /// - **AEAD:** decrypts `data` into `out`. Additional data must have been
  /// previously absorbed using `symmetric_state_absorb()`.
  /// - **SHOE, Xoodyak, Strobe:** decrypts data, squeezes a tag and verify that
  /// it matches the expected one.
  ///
  /// `raw_tag` is the expected tag, as raw bytes.
  ///
  /// `out` and `data` be must have the same length.
  /// If they also share the same address, decryption may happen in-place.
  ///
  /// The function returns the actual size of the decrypted message.
  ///
  /// `invalid_tag` is returned if the tag verification failed.
  ///
  /// `invalid_operation` is returned for algorithms not supporting encryption.
  WasiCryptoExpect<__wasi_size_t>
  symmetricStateDecryptDetached(__wasi_symmetric_state_t StateHandle,
                                Span<uint8_t> Out, Span<uint8_t const> Data,
                                Span<uint8_t> RawTag);

  /// Make it impossible to recover the previous state.
  ///
  /// This operation is supported by some systems keeping a rolling state over
  /// an entire session, for forward security.
  ///
  /// `invalid_operation` is returned for algorithms not supporting ratcheting.
  WasiCryptoExpect<void>
  symmetricStateRatchet(__wasi_symmetric_state_t StateHandle);

  /// Return the length of an authentication tag.
  ///
  /// This function can be used by a guest to allocate the correct buffer size
  /// to copy a computed authentication tag.
  WasiCryptoExpect<__wasi_size_t>
  symmetricTagLen(__wasi_symmetric_tag_t TagHandle);

  /// Copy an authentication tag into a guest-allocated buffer.
  ///
  /// The handle automatically becomes invalid after this operation. Manually
  /// closing it is not required.
  ///
  /// The function returns `overflow` if the supplied buffer is too small to
  /// copy the tag.
  ///
  /// Otherwise, it returns the number of bytes that have been copied.
  WasiCryptoExpect<__wasi_size_t>
  symmetricTagPull(__wasi_symmetric_tag_t TagHandle, Span<uint8_t> Buf);

  /// Verify that a computed authentication tag matches the expected value, in
  /// constant-time.
  ///
  /// The expected tag must be provided as a raw byte string.
  ///
  /// The function returns `invalid_tag` if the tags don't match.
  ///
  WasiCryptoExpect<void> symmetricTagVerify(__wasi_symmetric_tag_t TagHandle,
                                            Span<uint8_t> RawTag);

  /// Explicitly destroy an unused authentication tag.
  ///
  /// This is usually not necessary, as `symmetric_tag_pull()` automatically
  /// closes a tag after it has been copied.
  ///
  /// Objects are reference counted. It is safe to close an object immediately
  /// after the last function needing it is called.
  WasiCryptoExpect<void> symmetricTagClose(__wasi_symmetric_tag_t TagHandle);

  ///-------------------------------------------asymmetric_common---------------------------------------

  WasiCryptoExpect<__wasi_keypair_t>
  keypairGenerate(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  std::optional<__wasi_options_t> OptOptionsHandle);

  WasiCryptoExpect<__wasi_keypair_t>
  keypairImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                Span<uint8_t> Encoded,
                __wasi_keypair_encoding_e_t Encoding);

  // opt
  WasiCryptoExpect<__wasi_keypair_t>
  keypairGenerateManaged(__wasi_secrets_manager_t SecretsManager,
                         __wasi_algorithm_type_e_t AlgType,
                         std::string_view AlgStr,
                         std::optional<__wasi_options_t> OptOptions);

  WasiCryptoExpect<void>
  keypairStoreManaged(__wasi_secrets_manager_t SecretsManager,
                      __wasi_keypair_t Keypair, uint8_t_ptr KpIdPtr,
                      __wasi_size_t KpIdLen);

  WasiCryptoExpect<__wasi_version_t>
  keypairReplaceManaged(__wasi_secrets_manager_t SecretsManager,
                        __wasi_keypair_t KpOld, __wasi_keypair_t KpNew);

  WasiCryptoExpect<std::tuple<__wasi_size_t, __wasi_version_t>>
  keypairId(__wasi_keypair_t Kp, uint8_t_ptr KpId, __wasi_size_t KpIdMaxLen);

  WasiCryptoExpect<__wasi_keypair_t>
  keypairFromId(__wasi_secrets_manager_t SecretsManager, const_uint8_t_ptr KpId,
                __wasi_size_t KpIdLen, __wasi_version_t KpIdVersion);

  WasiCryptoExpect<__wasi_keypair_t> keypairFromPkAndSk(__wasi_publickey_t PkHandle,
                                                        __wasi_secretkey_t SkHandle);
  WasiCryptoExpect<__wasi_array_output_t>
  keypairExport(__wasi_keypair_t KpHandle,
                __wasi_keypair_encoding_e_t KeypairEncoding);

  WasiCryptoExpect<__wasi_publickey_t>
  keypairPublickey(__wasi_keypair_t KpHandle);

  WasiCryptoExpect<__wasi_secretkey_t>
  keypairSecretkey(__wasi_keypair_t KpHandle);

  WasiCryptoExpect<void> keypairClose(__wasi_keypair_t KpHandle);

  WasiCryptoExpect<__wasi_publickey_t>
  publickeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  Span<uint8_t> Encoded,
                  __wasi_publickey_encoding_e_t Encoding);

  WasiCryptoExpect<__wasi_array_output_t>
  publickeyExport(__wasi_publickey_t PkHandle,
                  __wasi_publickey_encoding_e_t PkEncoding);

  WasiCryptoExpect<void> publickeyVerify(__wasi_publickey_t PkHandle);

  WasiCryptoExpect<__wasi_publickey_t>
  publickeyFromSecretkey(__wasi_secretkey_t SkHandle);

  WasiCryptoExpect<void> publickeyClose(__wasi_publickey_t PkHandle);

  WasiCryptoExpect<__wasi_secretkey_t>
  secretkeyImport(__wasi_algorithm_type_e_t AlgType, std::string_view AlgStr,
                  Span<uint8_t> Encoded,
                  __wasi_secretkey_encoding_e_t EncodingEnum);

  WasiCryptoExpect<__wasi_array_output_t>
  secretkeyExport(__wasi_secretkey_t SkHandle,
                  __wasi_secretkey_encoding_e_t SkEncoding);

  WasiCryptoExpect<void> secretkeyClose(__wasi_secretkey_t SkHandle);

  ///-------------------------------------------key_exchange---------------------------------------

  WasiCryptoExpect<__wasi_array_output_t> kxDh(__wasi_publickey_t PkHandle,
                                               __wasi_secretkey_t SkHandle);

  WasiCryptoExpect<std::tuple<__wasi_array_output_t, __wasi_array_output_t>>
  kxEncapsulate(__wasi_publickey_t PkHandle);

  WasiCryptoExpect<__wasi_array_output_t>
  kxDecapsulate(__wasi_secretkey_t SkHandle, Span<uint8_t const> EncapsulatedSecret);

  ///-------------------------------------------signature---------------------------------------

  WasiCryptoExpect<__wasi_array_output_t>
  signatureExport(__wasi_signature_t SigHandle,
                  __wasi_signature_encoding_e_t Encoding);

  WasiCryptoExpect<__wasi_signature_t>
  signatureImport(SignatureAlgorithm Alg, Span<uint8_t const> Encoded, __wasi_signature_encoding_e_t Encoding);

  WasiCryptoExpect<void> signatureClose(__wasi_signature_t SigHandle);

  WasiCryptoExpect<__wasi_signature_state_t>
  signatureStateOpen(__wasi_signature_keypair_t KpHandle);

  WasiCryptoExpect<void> signatureStateUpdate(__wasi_signature_state_t StateHandle,
                                              Span<uint8_t const> Input);

  WasiCryptoExpect<__wasi_signature_t>
  signatureStateSign(__wasi_signature_state_t StateHandle);

  WasiCryptoExpect<void> signatureStateClose(__wasi_signature_state_t StateHandle);

  WasiCryptoExpect<__wasi_signature_verification_state_t>
  signatureVerificationStateOpen(__wasi_signature_publickey_t PkHandle);

  WasiCryptoExpect<void>
  signatureVerificationStateUpdate(__wasi_signature_verification_state_t VerificationHandle,
                                   Span<uint8_t const> Input);

  WasiCryptoExpect<void>
  signatureVerificationStateVerify(__wasi_signature_verification_state_t VerificationHandle,
                                   __wasi_signature_t SigHandle);

  WasiCryptoExpect<void>
  signatureVerificationStateClose(__wasi_signature_verification_state_t VerificationHandle);

private:
  WasiCryptoExpect<uint8_t> allocateArrayOutput(std::vector<uint8_t> &&Data);

  WasiCryptoExpect<std::optional<SymmetricOptions>>
  readSymmetricOption(std::optional<__wasi_options_t> OptOptionsHandle);

  WasiCryptoExpect<std::optional<SymmetricKey>>
  readSymmetricKey(std::optional<__wasi_symmetric_key_t> KeyHandle);

  HandlesManger<__wasi_array_output_t, ArrayOutput> ArrayOutputManger{0x00};
  HandlesManger<__wasi_options_t, Options> OptionsManger{0x01};
  HandlesManger<__wasi_keypair_t, KeyPair> KeypairManger{0x02};
  HandlesManger<__wasi_keypair_t, PublicKey> PublickeyManger{0x03};
  HandlesManger<__wasi_keypair_t, SecretKey> SecretkeyManger{0x04};
  HandlesManger<__wasi_signature_state_t, SignatureState> SignatureStateManger{
      0x05};
  HandlesManger<__wasi_signature_t, Signature> SignatureManger{0x06};
  HandlesManger<__wasi_signature_verification_state_t,
                SignatureVerificationState>
      SignatureVerificationStateManger{0x07};
  HandlesManger<__wasi_symmetric_state_t, SymmetricState> SymmetricStateManger{
      0x08};
  HandlesManger<__wasi_symmetric_key_t, SymmetricKey> SymmetricKeyManger{0x09};
  HandlesManger<__wasi_symmetric_tag_t, SymmetricTag> SymmetricTagManger{0xa};
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
