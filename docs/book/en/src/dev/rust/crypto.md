# Crypto relative binding

While optimizing compilers could allow efficient implementation of cryptographic features in WebAssembly, there are several occasions as below where a host implementation is more desirable. [WASI-crypto](https://github.com/WebAssembly/wasi-crypto/blob/main/docs/HighLevelGoals.md) aims to fill those gaps by defining a standard interface as a set of APIs.
Current not support android.  

## Getting started with wasi-crypto

1. Build wasi-crypto with `-DWASMEDGE_BUILD_WASI_CRYPTO=ON`.
2. Using [wasi-crypto binding](https://github.com/WebAssembly/wasi-crypto/tree/main/implementations/bindings/rust) in your cargo.toml

```toml
[dependencies]
wasi-crypto-guest = { git ="https://github.com/sonder-joker/wasi-crypto", version = "0.1.0" }
```

## High level operation

### Hash Function

| Identifier              | Algorithm                                                                           |
| ----------------------- | ----------------------------------------------------------------------------------- |
| `SHA-256`               | SHA-256 hash function                                                               |
| `SHA-512`               | SHA-512 hash function                                                               |
| `SHA-512/256`           | SHA-512/256 hash function with a specific IV                                        |

```rust
// hash "test" by SHA-256
let hash : Vec<u8> = Hash::hash("SHA-256", b"test", 32, None)?;
assert_eq!(hash.len(), 32);
```

### Message Authentications function

| Identifier              | Algorithm                                                                           |
| ----------------------- | ----------------------------------------------------------------------------------- |
| `HMAC/SHA-256`          | RFC2104 MAC using the SHA-256 hash function                                         |
| `HMAC/SHA-512`          | RFC2104 MAC using the SHA-512 hash function                                         |

```rust
// generate key
let key = AuthKey::generate("HMAC/SHA-512")?;
// generate tag
let tag = Auth::auth("test", &key)?;
// verify
Auth::auth_verify("test", &key, tag)?;
```

### Key Driven function

| Identifier              | Algorithm                                                                           |
| ----------------------- | ----------------------------------------------------------------------------------- |
| `HKDF-EXTRACT/SHA-256`  | RFC5869 `EXTRACT` function using the SHA-256 hash function                          |
| `HKDF-EXTRACT/SHA-512`  | RFC5869 `EXTRACT` function using the SHA-512 hash function                          |
| `HKDF-EXPAND/SHA-256`   | RFC5869 `EXPAND` function using the SHA-256 hash function                           |
| `HKDF-EXPAND/SHA-512`   | RFC5869 `EXPAND` function using the SHA-512 hash function                           |

Example:

```rust
let key = HkdfKey::generate("HKDF-EXTRACT/SHA-512")?;
let prk = Hkdf::new("HKDF-EXPAND/SHA-512", &key, Some(b"salt"))?;
let derived_key = prk.expand("info", 100)?;
assert_eq!(derived_key.len(), 100);
```

### Signatures Operation

| Identifier              | Algorithm                                                                           |
| ----------------------- | ----------------------------------------------------------------------------------- |
| `ECDSA_P256_SHA256`     | ECDSA over the NIST p256 curve with the SHA-256 hash function                       |
| `ECDSA_K256_SHA256`     | ECDSA over the secp256k1 curve with the SHA-256 hash function                       |
| `Ed25519`               | Edwards Curve signatures over Edwards25519 (pure EdDSA) as specified in RFC8032     |
| `RSA_PKCS1_2048_SHA256` | RSA signatures with a 2048 bit modulus, PKCS1 padding and the SHA-256 hash function |
| `RSA_PKCS1_2048_SHA384` | RSA signatures with a 2048 bit modulus, PKCS1 padding and the SHA-384 hash function |
| `RSA_PKCS1_2048_SHA512` | RSA signatures with a 2048 bit modulus, PKCS1 padding and the SHA-512 hash function |
| `RSA_PKCS1_3072_SHA384` | RSA signatures with a 3072 bit modulus, PKCS1 padding and the SHA-384 hash function |
| `RSA_PKCS1_3072_SHA512` | RSA signatures with a 3072 bit modulus, PKCS1 padding and the SHA-512 hash function |
| `RSA_PKCS1_4096_SHA512` | RSA signatures with a 4096 bit modulus, PKCS1 padding and the SHA-512 hash function |
| `RSA_PSS_2048_SHA256`   | RSA signatures with a 2048 bit modulus, PSS padding and the SHA-256 hash function   |
| `RSA_PSS_2048_SHA384`   | RSA signatures with a 2048 bit modulus, PSS padding and the SHA-384 hash function   |
| `RSA_PSS_2048_SHA512`   | RSA signatures with a 2048 bit modulus, PSS padding and the SHA-512 hash function   |
| `RSA_PSS_3072_SHA384`   | RSA signatures with a 2048 bit modulus, PSS padding and the SHA-384 hash function   |
| `RSA_PSS_3072_SHA512`   | RSA signatures with a 3072 bit modulus, PSS padding and the SHA-512 hash function   |
| `RSA_PSS_4096_SHA512`   | RSA signatures with a 4096 bit modulus, PSS padding and the SHA-512 hash function   |

Example:

```rust
let pk = SignaturePublicKey::from_raw("Ed25519", &[0; 32])?;

let kp = SignatureKeyPair::generate("Ed25519")?;
let signature = kp.sign("hello")?;

kp.publickey()?.signature_verify("hello", &signature)?;
```
