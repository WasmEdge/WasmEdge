#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <random>
#include <type_traits>
#include <vector>

// ----------------------------------------------------------
// TODO: LOOK FOR A .WIT REPLACEMENT

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define PRESERVE EMSCRIPTEN_KEEPALIVE
#define IMPORT __attribute__((import_module("wasmedge_zlib"))) extern "C"
#else
#include <zlib.h>
#define PRESERVE
#define IMPORT extern "C"
#endif

#ifdef __EMSCRIPTEN__

#define ZLIB_VERSION "1.2.11"
#define Z_OK 0
#define Z_STREAM_END 1
#define Z_STREAM_ERROR (-2)
#define Z_FINISH 4
#define Z_NULL NULL

typedef void *(*alloc_func)(void *opaque, unsigned int items,
                            unsigned int size);
typedef void (*free_func)(void *opaque, void *address);

struct internal_state;

typedef struct z_stream_s {
  unsigned char *next_in; /* next input byte */
  uint32_t avail_in;      /* number of bytes available at next_in */
  unsigned long total_in; /* total number of input bytes read so far */

  unsigned char *next_out; /* next output byte will go here */
  uint32_t avail_out;      /* remaining free space at next_out */
  unsigned long total_out; /* total number of bytes output so far */

  char *msg;                    /* last error message, NULL if no error */
  struct internal_state *state; /* not visible by applications */

  alloc_func zalloc; /* used to allocate the internal state */
  free_func zfree;   /* used to free the internal state */
  void *opaque;      /* private data object passed to zalloc and zfree */

  int data_type;       /* best guess about the data type: binary or text
                          for deflate, or the decoding state for inflate */
  unsigned long adler; /* Adler-32 or CRC-32 value of the uncompressed data */
  unsigned long reserved; /* reserved for future use */
} z_stream;

IMPORT int deflateInit_(z_stream *strm, int level, const char *version,
                        int stream_size);
IMPORT int inflateInit_(z_stream *strm, const char *version, int stream_size);

#define deflateInit(strm, level)                                               \
  deflateInit_((strm), (level), ZLIB_VERSION, (int)sizeof(z_stream))
#define inflateInit(strm)                                                      \
  inflateInit_((strm), ZLIB_VERSION, (int)sizeof(z_stream))

IMPORT int deflate(z_stream *strm, int flush);
IMPORT int inflate(z_stream *strm, int flush);

IMPORT int deflateEnd(z_stream *strm);
IMPORT int inflateEnd(z_stream *strm);

#endif

// ----------------------------------------------------------

static constexpr size_t DataSize = 1 * 1024 * 1024;

constexpr auto RandChar = []() -> char {
  constexpr char Charset[] = "0123456789"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "abcdefghijklmnopqrstuvwxyz";
  constexpr size_t MaxIndex = (sizeof(Charset) - 1);
  return Charset[rand() % MaxIndex];
};

void *CustomMalloc(void *Opaque, unsigned int Items, unsigned int Size) {

  auto Add = malloc(Items * Size);
  std::cout << "zalloc : " << Add << " = " << Items * Size << std::endl;
  return Add;
}

void CustomFree(void *Opaque, void *Address) {
  std::cout << "zfree : " << Address << std::endl;

  free(Address);
}

int InitDeflateZStream(z_stream &Strm, int Level) {
  Strm.zalloc = CustomMalloc;
  Strm.zfree = CustomFree;
  Strm.opaque = Z_NULL;

  int Ret = deflateInit(&Strm, Level);

  if (Ret != Z_OK) {
    std::cerr << "'deflateInit' failed!" << std::endl;
    std::terminate();
  }

  return Ret;
}

int InitInflateZStream(z_stream &Strm) {
  Strm.zalloc = CustomMalloc;
  Strm.zfree = CustomFree;
  Strm.opaque = Z_NULL;

  int Ret = inflateInit(&Strm);

  if (Ret != Z_OK) {
    std::cerr << "'inflateInit' failed!" << std::endl;
    std::terminate();
  }

  return Ret;
}

template <typename T>
std::vector<unsigned char> Deflate(const std::vector<T> &Source,
                                   int Level = -1) {

  int Ret;
  z_stream Strm;
  Ret = InitDeflateZStream(Strm, Level);
  const std::size_t SrcSize = Source.size() * sizeof(T);
  std::size_t OutBufferSize = SrcSize / 3 + 16;
  std::vector<unsigned char> OutBuffer(OutBufferSize, {});

  Strm.avail_in = SrcSize;
  Strm.next_in = reinterpret_cast<unsigned char *>(
      const_cast<std::remove_const_t<T> *>(Source.data()));
  Strm.avail_out = OutBuffer.size();
  Strm.next_out = OutBuffer.data();

  do {

    if (Strm.avail_out == 0) {
      const std::size_t ExtensionSize = SrcSize / 3 + 16;
      Strm.avail_out = ExtensionSize;
      OutBuffer.resize(OutBufferSize + ExtensionSize, {});
      Strm.next_out = std::next(OutBuffer.data(), OutBufferSize);
      OutBufferSize += ExtensionSize;
    }

    Ret = deflate(&Strm, Z_FINISH);

    if (Ret == Z_STREAM_ERROR) {
      std::cerr << "Zlib Stream Error!" << std::endl;
      std::terminate();
    }
  } while (Ret != Z_STREAM_END);

  deflateEnd(&Strm);
  OutBuffer.resize(OutBufferSize - Strm.avail_out);

  return OutBuffer;
}

template <typename T>
std::vector<T> Inflate(const std::vector<unsigned char> &Source) {

  int Ret;
  z_stream Strm;
  Ret = InitInflateZStream(Strm);
  const std::size_t SrcSize = Source.size();
  std::size_t OutBufferSize = SrcSize / 3 + 16;
  std::vector<unsigned char> OutBuffer(OutBufferSize, {});

  Strm.avail_in = SrcSize;
  Strm.next_in = const_cast<unsigned char *>(Source.data());
  Strm.avail_out = OutBuffer.size();
  Strm.next_out = OutBuffer.data();

  do {

    if (Strm.avail_out == 0) {
      const std::size_t ExtensionSize = SrcSize / 3 + 16;
      Strm.avail_out = ExtensionSize;
      OutBuffer.resize(OutBufferSize + ExtensionSize, {});
      Strm.next_out = std::next(OutBuffer.data(), OutBufferSize);
      OutBufferSize += ExtensionSize;
    }

    Ret = inflate(&Strm, Z_FINISH);

    if (Ret == Z_STREAM_ERROR) {
      std::cerr << "Zlib Stream Error!" << std::endl;
      std::terminate();
    }
  } while (Ret != Z_STREAM_END);

  inflateEnd(&Strm);
  OutBufferSize -= Strm.avail_out;

  std::vector<T> RetBuffer(reinterpret_cast<T *>(OutBuffer.data()),
                           std::next(reinterpret_cast<T *>(OutBuffer.data()),
                                     (OutBufferSize / sizeof(T))));

  return RetBuffer;
}

int test() {
  std::vector<char> Data(DataSize, {});
  std::generate_n(std::begin(Data), DataSize, RandChar);

  std::cout << "Compressing Buffer of size : " << DataSize << "B" << std::endl;
  const auto CompressedBuffer = Deflate(Data, 6);

  std::cout << "Decompressing Buffer of size : " << CompressedBuffer.size()
            << "B" << std::endl;
  const auto DecompressedBuffer = Inflate<char>(CompressedBuffer);

  auto CompareResult = Data == DecompressedBuffer;
  std::cout << (CompareResult ? "Success" : "Fail") << std::endl;

  return CompareResult;
}

int main() {
  test();
  return 0;
}
