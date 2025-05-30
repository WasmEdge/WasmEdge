#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

// For native use the zlib library
#ifndef __EMSCRIPTEN__
#include <zlib.h>
#endif

// Include the wit-bindgen generated header only for Emscripten(wasm) builds.
#ifdef __EMSCRIPTEN__
#include "bindings/zlib_component.h"
#endif

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ZLIB Function Implementations
// These are used by the test() function directly.
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
extern "C" {
#ifdef __EMSCRIPTEN__
// Wasm imports for Emscripten builds
__attribute__((import_module("wasmedge_zlib"))) int32_t compress(
    uint8_t *dest, size_t *destLen, const uint8_t *source, size_t sourceLen);

__attribute__((import_module("wasmedge_zlib"))) int32_t uncompress(
    uint8_t *dest, size_t *destLen, const uint8_t *source, size_t sourceLen);

// Define our internal compress/uncompress to call the Wasm imports
int32_t zlib_custom_compress(uint8_t *dest, size_t *destLen,
                             const uint8_t *source, size_t sourceLen) {
  return compress(dest, destLen, source, sourceLen);
}
int32_t zlib_custom_uncompress(uint8_t *dest, size_t *destLen,
                               const uint8_t *source, size_t sourceLen) {
  return uncompress(dest, destLen, source, sourceLen);
}

#else
// Native zlib wrapper functions
int32_t zlib_custom_compress(uint8_t *dest, size_t *destLen,
                             const uint8_t *source, size_t sourceLen) {
  uLongf z_destLen = *destLen;
  int ret = ::compress(dest, &z_destLen, source, (uLong)sourceLen);
  if (ret == Z_OK) {
    *destLen = (size_t)z_destLen;
    return 0;
  }
  return ret;
}

int32_t zlib_custom_uncompress(uint8_t *dest, size_t *destLen,
                               const uint8_t *source, size_t sourceLen) {
  uLongf z_destLen = *destLen;
  int ret = ::uncompress(dest, &z_destLen, source, (uLong)sourceLen);
  if (ret == Z_OK) {
    *destLen = (size_t)z_destLen;
    return 0;
  }
  return ret;
}
#endif
} // extern "C"

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// WIT-bindgen C ABI Layer Implementation
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef __EMSCRIPTEN__

static std::string format_error_helper(const char *prefix, int32_t code) {
  return std::string(prefix) + std::to_string(code);
}

extern "C" {

// canonical_abi_realloc and canonical_abi_free are required by wit-bindgen
// generated code.
__attribute__((export_name("canonical_abi_realloc"))) void *
canonical_abi_realloc(void *old_ptr, size_t old_size, size_t align,
                      size_t new_size) {
  (void)old_size;
  (void)align;
  if (new_size == 0) {
    if (old_ptr) {
      free(old_ptr);
    }
    return nullptr;
  }
  void *new_ptr = realloc(old_ptr, new_size);
  if (new_ptr == nullptr && new_size > 0) {
    fprintf(stderr,
            "FATAL: canonical_abi_realloc failed to allocate %zu bytes.\n",
            new_size);
  }
  return new_ptr;
}

__attribute__((export_name("canonical_abi_free"))) void
canonical_abi_free(void *ptr, size_t size, size_t align) {
  (void)size;
  (void)align;
  if (ptr) {
    free(ptr);
  }
}

// Helper function to convert std::vector<uint8_t> to zlib_component_list_u8_t
static void vec_to_list_u8(const std::vector<uint8_t> &vec,
                           zlib_component_list_u8_t *list) {
  if (vec.empty()) {
    list->ptr = nullptr;
    list->len = 0;
    return;
  }
  list->ptr =
      static_cast<uint8_t *>(canonical_abi_realloc(nullptr, 0, 1, vec.size()));
  if (list->ptr) {
    memcpy(list->ptr, vec.data(), vec.size());
    list->len = vec.size();
  } else {
    list->len = 0; // Allocation failed
    fprintf(stderr, "ABI Layer Error: Memory allocation failed for list_u8_t "
                    "conversion.\n");
  }
}

// Helper function to convert std::string to zlib_component_string_t
static void str_to_component_string(const std::string &str,
                                    zlib_component_string_t *comp_str) {
  if (str.empty()) {
    comp_str->ptr = nullptr;
    comp_str->len = 0;
    return;
  }
  comp_str->ptr = static_cast<uint8_t *>(
      canonical_abi_realloc(nullptr, 0, 1, str.length()));
  if (comp_str->ptr) {
    memcpy(comp_str->ptr, str.data(), str.length());
    comp_str->len = str.length();
  } else {
    comp_str->len = 0; // Allocation failed
    fprintf(
        stderr,
        "ABI Layer Error: Memory allocation failed for string conversion.\n");
  }
}

bool exports_example_zlib_compressor_deflate(zlib_component_list_u8_t *input,
                                             zlib_component_list_u8_t *ret_ok,
                                             zlib_component_string_t *ret_err) {
  std::vector<uint8_t> source_data;
  if (input->len > 0) {
    if (input->ptr == nullptr) {
      std::string err_msg =
          "Invalid input to deflate: non-zero length with null pointer.";
      str_to_component_string(err_msg, ret_err);
      ret_ok->ptr = nullptr;
      ret_ok->len = 0;
      return false;
    }
    source_data.assign(input->ptr, input->ptr + input->len);
  }

  if (source_data.empty()) {
    ret_ok->ptr = nullptr;
    ret_ok->len = 0;
    ret_err->ptr = nullptr;
    ret_err->len = 0;
    return true;
  }

  size_t dest_len_estimate = source_data.size() * 2;
  if (dest_len_estimate < 128)
    dest_len_estimate = 128;

  std::vector<uint8_t> compressed_data(dest_len_estimate);
  size_t actual_dest_len = dest_len_estimate;

  int32_t result_code =
      zlib_custom_compress(compressed_data.data(), &actual_dest_len,
                           source_data.data(), source_data.size());

  if (result_code == 0) { // Success
    compressed_data.resize(actual_dest_len);
    vec_to_list_u8(compressed_data, ret_ok);
    if (actual_dest_len > 0 && ret_ok->ptr == nullptr && ret_ok->len == 0) {
      str_to_component_string("Memory allocation failed for deflate Ok value.",
                              ret_err);
      return false;
    }
    ret_err->ptr = nullptr;
    ret_err->len = 0;
    return true;
  } else {
    str_to_component_string(
        format_error_helper("Compression failed with error code: ",
                            result_code),
        ret_err);
    ret_ok->ptr = nullptr;
    ret_ok->len = 0;
    return false;
  }
}

bool exports_example_zlib_compressor_inflate(zlib_component_list_u8_t *input,
                                             zlib_component_list_u8_t *ret_ok,
                                             zlib_component_string_t *ret_err) {
  std::vector<uint8_t> source_data;
  if (input->len > 0) {
    if (input->ptr == nullptr) {
      std::string err_msg =
          "Invalid input to inflate: non-zero length with null pointer.";
      str_to_component_string(err_msg, ret_err);
      ret_ok->ptr = nullptr;
      ret_ok->len = 0;
      return false;
    }
    source_data.assign(input->ptr, input->ptr + input->len);
  }

  if (source_data.empty()) {
    ret_ok->ptr = nullptr;
    ret_ok->len = 0;
    ret_err->ptr = nullptr;
    ret_err->len = 0;
    return true;
  }

  size_t dest_len_estimate = source_data.size() * 4;
  if (dest_len_estimate < 1024)
    dest_len_estimate = 1024;
  if (source_data.size() > (512 * 1024) &&
      dest_len_estimate < source_data.size() * 8) {
    dest_len_estimate = source_data.size() * 8;
  }

  std::vector<uint8_t> decompressed_data(dest_len_estimate);
  size_t actual_dest_len = dest_len_estimate;

  int32_t result_code =
      zlib_custom_uncompress(decompressed_data.data(), &actual_dest_len,
                             source_data.data(), source_data.size());

  if (result_code == -5 /* Z_BUF_ERROR */ &&
      actual_dest_len == dest_len_estimate) {
    size_t larger_dest_len_estimate = dest_len_estimate;
    if (source_data.size() * 10 > larger_dest_len_estimate) {
      larger_dest_len_estimate = source_data.size() * 10;
    } else {
      larger_dest_len_estimate *= 2;
    }
    const size_t MAX_DECOMPRESSION_BUFFER = 256 * 1024 * 1024;
    if (larger_dest_len_estimate > MAX_DECOMPRESSION_BUFFER) {
      larger_dest_len_estimate = MAX_DECOMPRESSION_BUFFER;
    }

    if (larger_dest_len_estimate > dest_len_estimate) {
      fprintf(stderr,
              "Decompression: Z_BUF_ERROR, retrying with larger buffer: %zu "
              "bytes (previous: %zu)\n",
              larger_dest_len_estimate, dest_len_estimate);
      decompressed_data.resize(larger_dest_len_estimate);
      actual_dest_len = larger_dest_len_estimate;
      result_code =
          zlib_custom_uncompress(decompressed_data.data(), &actual_dest_len,
                                 source_data.data(), source_data.size());
    }
  }

  if (result_code == 0) { // Success
    decompressed_data.resize(actual_dest_len);
    vec_to_list_u8(decompressed_data, ret_ok);
    if (actual_dest_len > 0 && ret_ok->ptr == nullptr && ret_ok->len == 0) {
      str_to_component_string("Memory allocation failed for inflate Ok value.",
                              ret_err);
      return false;
    }
    ret_err->ptr = nullptr;
    ret_err->len = 0;
    return true;
  } else {
    str_to_component_string(
        format_error_helper("Decompression failed with error code: ",
                            result_code),
        ret_err);
    ret_ok->ptr = nullptr;
    ret_ok->len = 0;
    return false;
  }
}

} // extern "C"
#endif // __EMSCRIPTEN__

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Test function and main
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int test() {
  const size_t DataSize = 1 * 1024 * 1024;
  std::vector<uint8_t> Data(DataSize);
  std::generate(Data.begin(), Data.end(), []() -> uint8_t {
    constexpr char Charset[] = "0123456789"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz";
    constexpr size_t MaxIndex = (sizeof(Charset) - 1);
    return static_cast<uint8_t>(Charset[rand() % MaxIndex]);
  });

  std::cout << "Compressing Buffer of size : " << DataSize << "B" << std::endl;

  // Test compression
  size_t compressedSize = DataSize * 2;
  std::vector<uint8_t> compressed(compressedSize);
  // Use zlib_custom_compress
  int32_t result = zlib_custom_compress(compressed.data(), &compressedSize,
                                        Data.data(), Data.size());
  if (result != 0) {
    std::cerr << "Compression failed with error code: " << result << std::endl;
    return 1;
  }
  compressed.resize(compressedSize);

  std::cout << "Decompressing Buffer of size : " << compressed.size() << "B"
            << std::endl;

  // Test decompression
  size_t decompressedSize = DataSize;
  std::vector<uint8_t> decompressed(decompressedSize);

  // Use zlib_custom_uncompress
  result = zlib_custom_uncompress(decompressed.data(), &decompressedSize,
                                  compressed.data(), compressed.size());
  if (result != 0) {
    std::cerr << "Decompression failed with error code: " << result
              << std::endl;
    return 1;
  }
  decompressed.resize(decompressedSize);

  bool const success = (Data == decompressed);
  std::cout << (success ? "Success" : "Fail") << std::endl;
  return success ? 0 : 1;
}

int main() { return test(); }