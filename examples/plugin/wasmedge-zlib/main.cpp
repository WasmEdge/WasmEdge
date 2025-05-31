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

// NOTE: Some function signatures, structs etc need to be same as the ones in zlib_component.h
// example: `bool exports_example_zlib_compressor_deflate(...)`
// This is to ensure that the C ABI layer can call these functions correctly.
// This is also true for some extern "C" functions.

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
int32_t zlibCustomCompress(uint8_t *Dest, size_t *DestLen,
                             const uint8_t *Source, size_t SourceLen) {
  return compress(Dest, DestLen, Source, SourceLen);
}
int32_t zlibCustomUncompress(uint8_t *Dest, size_t *DestLen,
                               const uint8_t *Source, size_t SourceLen) {
  return uncompress(Dest, DestLen, Source, SourceLen);
}

#else
// Native zlib wrapper functions
int32_t zlibCustomCompress(uint8_t *Dest, size_t *DestLen,
                             const uint8_t *Source, size_t SourceLen) {
  uLongf ZDestLen = *DestLen;
  int Ret = ::compress(Dest, &ZDestLen, Source, (uLong)SourceLen);
  if (Ret == Z_OK) {
    *DestLen = (size_t)ZDestLen;
    return 0;
  }
  return Ret;
}

int32_t zlibCustomUncompress(uint8_t *Dest, size_t *DestLen,
                               const uint8_t *Source, size_t SourceLen) {
  uLongf ZDestLen = *DestLen;
  int Ret = ::uncompress(Dest, &ZDestLen, Source, (uLong)SourceLen);
  if (Ret == Z_OK) {
    *DestLen = (size_t)ZDestLen;
    return 0;
  }
  return Ret;
}
#endif
} // extern "C"

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// WIT-bindgen C ABI Layer Implementation
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef __EMSCRIPTEN__

static std::string formatErrorHelper(const char *Prefix, int32_t Code) {
  return std::string(Prefix) + std::to_string(Code);
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
static void vecToListU8(const std::vector<uint8_t> &Vec,
                        zlib_component_list_u8_t *List) {
  if (Vec.empty()) {
    List->ptr = nullptr;
    List->len = 0;
    return;
  }
  List->ptr =
      static_cast<uint8_t *>(canonical_abi_realloc(nullptr, 0, 1, Vec.size()));
  if (List->ptr) {
    memcpy(List->ptr, Vec.data(), Vec.size());
    List->len = Vec.size();
  } else {
    List->len = 0; // Allocation failed
    fprintf(stderr, "ABI Layer Error: Memory allocation failed for list_u8_t "
                    "conversion.\n");
  }
}

// Helper function to convert std::string to zlib_component_string_t
static void strToComponentString(const std::string &Str,
                                 zlib_component_string_t *CompStr) {
  if (Str.empty()) {
    CompStr->ptr = nullptr;
    CompStr->len = 0;
    return;
  }
  CompStr->ptr = static_cast<uint8_t *>(
      canonical_abi_realloc(nullptr, 0, 1, Str.length()));
  if (CompStr->ptr) {
    memcpy(CompStr->ptr, Str.data(), Str.length());
    CompStr->len = Str.length();
  } else {
    CompStr->len = 0; // Allocation failed
    fprintf(
        stderr,
        "ABI Layer Error: Memory allocation failed for string conversion.\n");
  }
}

bool exports_example_zlib_compressor_deflate(zlib_component_list_u8_t *input,
                                             zlib_component_list_u8_t *ret_ok,
                                             zlib_component_string_t *ret_err) {
  std::vector<uint8_t> SourceData;
  if (input->len > 0) {
    if (input->ptr == nullptr) {
      std::string ErrMsg =
          "Invalid input to deflate: non-zero length with null pointer.";
      strToComponentString(ErrMsg, ret_err);
      ret_ok->ptr = nullptr;
      ret_ok->len = 0;
      return false;
    }
    SourceData.assign(input->ptr, input->ptr + input->len);
  }

  if (SourceData.empty()) {
    ret_ok->ptr = nullptr;
    ret_ok->len = 0;
    ret_err->ptr = nullptr;
    ret_err->len = 0;
    return true;
  }

  size_t DestLenEstimate = SourceData.size() * 2;
  if (DestLenEstimate < 128)
    DestLenEstimate = 128;

  std::vector<uint8_t> CompressedData(DestLenEstimate);
  size_t ActualDestLen = DestLenEstimate;

  int32_t ResultCode =
      zlibCustomCompress(CompressedData.data(), &ActualDestLen,
                           SourceData.data(), SourceData.size());

  if (ResultCode == 0) { // Success
    CompressedData.resize(ActualDestLen);
    vecToListU8(CompressedData, ret_ok);
    if (ActualDestLen > 0 && ret_ok->ptr == nullptr && ret_ok->len == 0) {
      strToComponentString("Memory allocation failed for deflate Ok value.",
                           ret_err);
      return false;
    }
    ret_err->ptr = nullptr;
    ret_err->len = 0;
    return true;
  } else {
    strToComponentString(
        formatErrorHelper("Compression failed with error code: ",
                          ResultCode),
        ret_err);
    ret_ok->ptr = nullptr;
    ret_ok->len = 0;
    return false;
  }
}

bool exports_example_zlib_compressor_inflate(zlib_component_list_u8_t *input,
                                             zlib_component_list_u8_t *ret_ok,
                                             zlib_component_string_t *ret_err) {
  std::vector<uint8_t> SourceData;
  if (input->len > 0) {
    if (input->ptr == nullptr) {
      std::string ErrMsg =
          "Invalid input to inflate: non-zero length with null pointer.";
      strToComponentString(ErrMsg, ret_err);
      ret_ok->ptr = nullptr;
      ret_ok->len = 0;
      return false;
    }
    SourceData.assign(input->ptr, input->ptr + input->len);
  }

  if (SourceData.empty()) {
    ret_ok->ptr = nullptr;
    ret_ok->len = 0;
    ret_err->ptr = nullptr;
    ret_err->len = 0;
    return true;
  }

  size_t DestLenEstimate = SourceData.size() * 4;
  if (DestLenEstimate < 1024)
    DestLenEstimate = 1024;
  if (SourceData.size() > (512 * 1024) &&
      DestLenEstimate < SourceData.size() * 8) {
    DestLenEstimate = SourceData.size() * 8;
  }

  std::vector<uint8_t> DecompressedData(DestLenEstimate);
  size_t ActualDestLen = DestLenEstimate;

  int32_t ResultCode =
      zlibCustomUncompress(DecompressedData.data(), &ActualDestLen,
                             SourceData.data(), SourceData.size());

  if (ResultCode == -5 /* Z_BUF_ERROR */ &&
      ActualDestLen == DestLenEstimate) {
    size_t LargerDestLenEstimate = DestLenEstimate;
    if (SourceData.size() * 10 > LargerDestLenEstimate) {
      LargerDestLenEstimate = SourceData.size() * 10;
    } else {
      LargerDestLenEstimate *= 2;
    }
    const size_t MaxDecompressionBuffer = 256 * 1024 * 1024;
    if (LargerDestLenEstimate > MaxDecompressionBuffer) {
      LargerDestLenEstimate = MaxDecompressionBuffer;
    }

    if (LargerDestLenEstimate > DestLenEstimate) {
      fprintf(stderr,
              "Decompression: Z_BUF_ERROR, retrying with larger buffer: %zu "
              "bytes (previous: %zu)\n",
              LargerDestLenEstimate, DestLenEstimate);
      DecompressedData.resize(LargerDestLenEstimate);
      ActualDestLen = LargerDestLenEstimate;
      ResultCode =
          zlibCustomUncompress(DecompressedData.data(), &ActualDestLen,
                                 SourceData.data(), SourceData.size());
    }
  }

  if (ResultCode == 0) { // Success
    DecompressedData.resize(ActualDestLen);
    vecToListU8(DecompressedData, ret_ok);
    if (ActualDestLen > 0 && ret_ok->ptr == nullptr && ret_ok->len == 0) {
      strToComponentString("Memory allocation failed for inflate Ok value.",
                           ret_err);
      return false;
    }
    ret_err->ptr = nullptr;
    ret_err->len = 0;
    return true;
  } else {
    strToComponentString(
        formatErrorHelper("Decompression failed with error code: ",
                          ResultCode),
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
  size_t CompressedSize = DataSize * 2;
  std::vector<uint8_t> Compressed(CompressedSize);
  // Use zlibCustomCompress
  int32_t Result = zlibCustomCompress(Compressed.data(), &CompressedSize,
                                        Data.data(), Data.size());
  if (Result != 0) {
    std::cerr << "Compression failed with error code: " << Result << std::endl;
    return 1;
  }
  Compressed.resize(CompressedSize);

  std::cout << "Decompressing Buffer of size : " << Compressed.size() << "B"
            << std::endl;

  // Test decompression
  size_t DecompressedSize = DataSize;
  std::vector<uint8_t> Decompressed(DecompressedSize);

  // Use zlibCustomUncompress
  Result = zlibCustomUncompress(Decompressed.data(), &DecompressedSize,
                                  Compressed.data(), Compressed.size());
  if (Result != 0) {
    std::cerr << "Decompression failed with error code: " << Result
              << std::endl;
    return 1;
  }
  Decompressed.resize(DecompressedSize);

  bool const Success = (Data == Decompressed);
  std::cout << (Success ? "Success" : "Fail") << std::endl;
  return Success ? 0 : 1;
}

int main() { return test(); }
