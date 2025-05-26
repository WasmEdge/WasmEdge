#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

// Import declarations for wasmedge_zlib plugin
extern "C" {
    // Compression functions
#ifdef __EMSCRIPTEN__
    __attribute__((import_module("wasmedge_zlib")))
#endif
    int32_t compress(uint8_t* dest, size_t* destLen, const uint8_t* source, size_t sourceLen);

#ifdef __EMSCRIPTEN__
    __attribute__((import_module("wasmedge_zlib")))
#endif
    int32_t uncompress(uint8_t* dest, size_t* destLen, const uint8_t* source, size_t sourceLen);
}

// WIT interface implementation functions
extern "C" {

// Compression functions as defined in zlib.wit
#ifdef __EMSCRIPTEN__
    __attribute__((export_name("deflate")))
#endif
int32_t deflate(uint8_t* dest, size_t* destLen, const uint8_t* source, size_t sourceLen, int32_t level) {
    return compress(dest, destLen, source, sourceLen);
}

#ifdef __EMSCRIPTEN__
    __attribute__((export_name("inflate")))
#endif
int32_t inflate(uint8_t* dest, size_t* destLen, const uint8_t* source, size_t sourceLen) {
    return uncompress(dest, destLen, source, sourceLen);
}
}

// Test function
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
    size_t compressedSize = DataSize * 2; // Conservative estimate
    std::vector<uint8_t> compressed(compressedSize);
    int32_t result = deflate(compressed.data(), &compressedSize, Data.data(), Data.size(), -1);
    if (result != 0) {
        std::cerr << "Compression failed" << std::endl;
        return 1;
    }
    compressed.resize(compressedSize);
    
    std::cout << "Decompressing Buffer of size : " << compressed.size() << "B" << std::endl;
    
    // Test decompression
    size_t decompressedSize = DataSize;
    std::vector<uint8_t> decompressed(decompressedSize);
    result = inflate(decompressed.data(), &decompressedSize, compressed.data(), compressed.size());
    if (result != 0) {
        std::cerr << "Decompression failed" << std::endl;
        return 1;
    }
    decompressed.resize(decompressedSize);
    
    // Verify results
    bool success = (Data == decompressed);
    std::cout << (success ? "Success" : "Fail") << std::endl;
    
    return success ? 0 : 1;
}

int main() {
  return test();
 }
