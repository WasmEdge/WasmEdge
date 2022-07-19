# Crypto for WASI

While optimizing compilers could allow efficient implementation of cryptographic features in WebAssembly, there are several occasions as below where a host implementation is more desirable. [WASI-crypto](https://github.com/WebAssembly/wasi-crypto/blob/main/docs/HighLevelGoals.md) aims to fill those gaps by defining a standard interface as a set of APIs.
Current not support android.

## Prerequisites

Currently, WasmEdge used OpenSSL 1.1 for the WASI-Crypto implementation. For this demo, you need to install [OpenSSL 1.1](https://www.openssl.org/source/).

### OpenSSL Development Package Installation

For installing OpenSSL 1.1 development package on Ubuntu20.04, we recommend the following commands:

```bash
sudo apt update
sudo apt install -y libssl-dev
```

For legacy systems or if you want to build OpenSSL 1.1 from source, you can refer to the following commands:

```bash
# Download and extract the OpenSSL source to the current directory.
curl -s -L -O --remote-name-all https://www.openssl.org/source/openssl-1.1.1n.tar.gz
echo "40dceb51a4f6a5275bde0e6bf20ef4b91bfc32ed57c0552e2e8e15463372b17a openssl-1.1.1n.tar.gz" | sha256sum -c
tar -xf openssl-1.1.1n.tar.gz
cd ./openssl-1.1.1n
# OpenSSL configure need newer perl.
curl -s -L -O --remote-name-all https://www.cpan.org/src/5.0/perl-5.34.0.tar.gz
tar -xf perl-5.34.0.tar.gz
cd perl-5.34.0
mkdir localperl
./Configure -des -Dprefix=$(pwd)/localperl/
make -j
make install
export PATH="$(pwd)/localperl/bin/:$PATH"
cd ..
# Configure by previous perl.
mkdir openssl
./perl-5.34.0/localperl/bin/perl ./config --prefix=$(pwd)/openssl --openssldir=$(pwd)/openssl
make -j
make test
make install
cd ..
# The OpenSSL installation directory is at `$(pwd)/openssl-1.1.1n/openssl`.
# Then you can use the `-DOPENSSL_ROOT_DIR=` option of cmake to assign the directory.
```

### WasmEdge Building and Installation

To enable the WasmEdge WASI-Crypto, we need to [building the WasmEdge from source](../../extend/build.md) with the cmake option `-DWASMEDGE_PLUGIN_WASI_CRYPTO=ON` to enable the WASI-Crypto:

```bash
git clone https://github.com/WasmEdge/WasmEdge.git && cd WasmEdge
# If use docker
docker pull wasmedge/wasmedge
docker run -it --rm \
    -v <path/to/your/wasmedge/source/folder>:/root/wasmedge \
    wasmedge/wasmedge:latest
cd /root/wasmedge
# If you don't use docker, you need to run only the following commands in the cloned repository root
apt update
apt install -y libssl-dev
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_PLUGIN_WASI_CRYPTO=On .. && make -j
# For the WASI-Crypto plugin, you should install this project.
cmake --install .
```

### Rust Crate

For importing WASI-Crypto in rust, you should use the [wasi-crypto binding](https://github.com/WebAssembly/wasi-crypto/tree/main/implementations/bindings/rust) in your cargo.toml

```toml
[dependencies]
wasi-crypto-guest = { git ="https://github.com/sonder-joker/wasi-crypto", version = "0.1.0" }
```

## High Level Operations

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
