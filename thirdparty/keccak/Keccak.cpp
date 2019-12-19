// SPDX-License-Identifier: BSD-3-Clause
#include "Keccak.h"
#include "Endian.h"
#include "Rotation.h"

// Constants of the Keccak algorithm.

const uint64_t RC[] = {
    0x0000000000000001L, 0x0000000000008082L, 0x800000000000808aL,
    0x8000000080008000L, 0x000000000000808bL, 0x0000000080000001L,
    0x8000000080008081L, 0x8000000000008009L, 0x000000000000008aL,
    0x0000000000000088L, 0x0000000080008009L, 0x000000008000000aL,
    0x000000008000808bL, 0x800000000000008bL, 0x8000000000008089L,
    0x8000000000008003L, 0x8000000000008002L, 0x8000000000000080L,
    0x000000000000800aL, 0x800000008000000aL, 0x8000000080008081L,
    0x8000000000008080L, 0x0000000080000001L, 0x8000000080008008L};

const int R[] = {0,  1,  62, 28, 27, 36, 44, 6,  55, 20, 3,  10, 43,
                 25, 39, 41, 45, 15, 21, 8,  18, 2,  61, 56, 14};

static inline int index(int x);
static inline int index(int x, int y);

// Function to create the state structure for keccak application, of size length
// (where length is the number of bits in the hash)
Keccak::Keccak(unsigned int length_) {
  A = new uint64_t[25];
  memset(A, 0, 25 * sizeof(uint64_t));
  blockLen = 200 - 2 * (length_ / 8);
  buffer = new uint8_t[blockLen];
  memset(buffer, 0, blockLen * sizeof(uint8_t));
  bufferLen = 0;
  length = length_;
}

Keccak::~Keccak() {
  delete[] A;
  delete[] buffer;
}

void Keccak::reset() {
  for (unsigned int i = 0; i < 25; i++) {
    A[i] = 0L;
  }
  bufferLen = 0;
}

// keccakUpdate - Functions to pack input data into a block

// One byte input at a time - process buffer if it's empty
void Keccak::addData(uint8_t input) {
  buffer[bufferLen] = input;
  if (++(bufferLen) == blockLen) {
    processBuffer();
  }
}

// Process a larger buffer with varying amounts of data in it
void Keccak::addData(const uint8_t *input, unsigned int off, unsigned int len) {
  while (len > 0) {
    int cpLen = 0;
    if ((blockLen - bufferLen) > len) {
      cpLen = len;
    } else {
      cpLen = blockLen - bufferLen;
    }

    for (unsigned int i = 0; i != cpLen; i++) {
      buffer[bufferLen + i] = input[off + i];
    }
    bufferLen += cpLen;
    off += cpLen;
    len -= cpLen;
    if (bufferLen == blockLen) {
      processBuffer();
    }
  }
}

template <typename T1, typename T2, typename T3>
std::vector<unsigned char> digest_generic(uint64_t *A, unsigned int hashLength,
                                          T1 paddingFunc, T2 processBufferFunc,
                                          T3 resetFunc) {
  unsigned int lengthInBytes = hashLength / 8;
  unsigned int lengthInQuads = lengthInBytes / 8;
  bool rollOverData = false;
  if (lengthInBytes % 8 != 0) {
    rollOverData = true;
  }

  paddingFunc();
  processBufferFunc();
  std::vector<unsigned char> tmp;
  tmp.reserve(lengthInBytes);
  for (unsigned int i = 0; i < lengthInQuads; i++) {
    uint64_t b = A[i];
    for (unsigned int j = 0; j != 8; j++) {
      tmp.push_back((unsigned char)((b >> (8 * j)) & 0xFF));
    }
  }
  if (rollOverData) {
    uint64_t b = A[lengthInQuads];
    for (unsigned int i = 0; i != lengthInBytes % 8; i++) {
      tmp.push_back((unsigned char)((b >> (8 * i)) & 0xFF));
    }
  }

  resetFunc();
  return tmp;
}

// keccakDigest - called once all data has been few to the keccakUpdate
// functions Pads the structure (in case the input is not a multiple of the
// block length) returns the hash result in a char vector
std::vector<unsigned char> Keccak::digest() {
  return digest_generic(
      A, length, [this]() { addPadding(); }, [this]() { processBuffer(); },
      [this]() { reset(); });
}

void Keccak::addPadding() {
  if (bufferLen + 1 == blockLen) {
    buffer[bufferLen] = (uint8_t)0x81;
  } else {
    buffer[bufferLen] = (uint8_t)0x01;
    for (unsigned int i = bufferLen + 1; i < blockLen - 1; i++) {
      buffer[i] = 0;
    }
    buffer[blockLen - 1] = (uint8_t)0x80;
  }
}

void Keccak::processBuffer() {
  for (unsigned int i = 0; i < blockLen / 8; i++) {
    A[i] ^= LittleToNative(((uint64_t *)buffer)[i]);
  }
  keccakf();
  bufferLen = 0;
}

static inline int index(int x) { return x < 0 ? index(x + 5) : x % 5; }

static inline int index(int x, int y) { return index(x) + 5 * index(y); }

struct keccakfState {
  uint64_t B[25];
  uint64_t C[5];
  uint64_t D[5];
};

// Hash function proper.
void Keccak::keccakf() {
  uint64_t *A_ = A;
  keccakfState kState;
  for (int n = 0; n < 24; n++) {
    int x = 0;
    kState.C[x] = A_[index(x, 0)] ^ A_[index(x, 1)] ^ A_[index(x, 2)] ^
                  A_[index(x, 3)] ^ A_[index(x, 4)];
    x = 1;
    kState.C[x] = A_[index(x, 0)] ^ A_[index(x, 1)] ^ A_[index(x, 2)] ^
                  A_[index(x, 3)] ^ A_[index(x, 4)];
    x = 2;
    kState.C[x] = A_[index(x, 0)] ^ A_[index(x, 1)] ^ A_[index(x, 2)] ^
                  A_[index(x, 3)] ^ A_[index(x, 4)];
    x = 3;
    kState.C[x] = A_[index(x, 0)] ^ A_[index(x, 1)] ^ A_[index(x, 2)] ^
                  A_[index(x, 3)] ^ A_[index(x, 4)];
    x = 4;
    kState.C[x] = A_[index(x, 0)] ^ A_[index(x, 1)] ^ A_[index(x, 2)] ^
                  A_[index(x, 3)] ^ A_[index(x, 4)];

    int i;
    x = 0;
    int y = 0;
    kState.D[x] =
        kState.C[index(x - 1)] ^ rotateLeft(kState.C[index(x + 1)], 1);
    y = 0;
    A_[index(x, y)] ^= kState.D[x];
    y = 1;
    A_[index(x, y)] ^= kState.D[x];
    y = 2;
    A_[index(x, y)] ^= kState.D[x];
    y = 3;
    A_[index(x, y)] ^= kState.D[x];
    y = 4;
    A_[index(x, y)] ^= kState.D[x];
    x = 1;
    kState.D[x] =
        kState.C[index(x - 1)] ^ rotateLeft(kState.C[index(x + 1)], 1);
    y = 0;
    A_[index(x, y)] ^= kState.D[x];
    y = 1;
    A_[index(x, y)] ^= kState.D[x];
    y = 2;
    A_[index(x, y)] ^= kState.D[x];
    y = 3;
    A_[index(x, y)] ^= kState.D[x];
    y = 4;
    A_[index(x, y)] ^= kState.D[x];
    x = 2;
    kState.D[x] =
        kState.C[index(x - 1)] ^ rotateLeft(kState.C[index(x + 1)], 1);
    y = 0;
    A_[index(x, y)] ^= kState.D[x];
    y = 1;
    A_[index(x, y)] ^= kState.D[x];
    y = 2;
    A_[index(x, y)] ^= kState.D[x];
    y = 3;
    A_[index(x, y)] ^= kState.D[x];
    y = 4;
    A_[index(x, y)] ^= kState.D[x];
    x = 3;
    kState.D[x] =
        kState.C[index(x - 1)] ^ rotateLeft(kState.C[index(x + 1)], 1);
    y = 0;
    A_[index(x, y)] ^= kState.D[x];
    y = 1;
    A_[index(x, y)] ^= kState.D[x];
    y = 2;
    A_[index(x, y)] ^= kState.D[x];
    y = 3;
    A_[index(x, y)] ^= kState.D[x];
    y = 4;
    A_[index(x, y)] ^= kState.D[x];
    x = 4;
    kState.D[x] =
        kState.C[index(x - 1)] ^ rotateLeft(kState.C[index(x + 1)], 1);
    y = 0;
    A_[index(x, y)] ^= kState.D[x];
    y = 1;
    A_[index(x, y)] ^= kState.D[x];
    y = 2;
    A_[index(x, y)] ^= kState.D[x];
    y = 3;
    A_[index(x, y)] ^= kState.D[x];
    y = 4;
    A_[index(x, y)] ^= kState.D[x];

    x = 0;
    y = 0;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 1;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 2;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 3;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 4;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    x = 1;
    y = 0;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 1;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 2;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 3;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 4;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    x = 2;
    y = 0;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 1;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 2;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 3;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 4;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    x = 3;
    y = 0;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 1;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 2;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 3;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 4;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    x = 4;
    y = 0;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 1;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 2;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 3;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);
    y = 4;
    i = index(x, y);
    kState.B[index(y, x * 2 + 3 * y)] = rotateLeft(A_[i], R[i]);

    x = 0;
    y = 0;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 1;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 2;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 3;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 4;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    x = 1;
    y = 0;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 1;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 2;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 3;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 4;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    x = 2;
    y = 0;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 1;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 2;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 3;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 4;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    x = 3;
    y = 0;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 1;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 2;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 3;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 4;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    x = 4;
    y = 0;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 1;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 2;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 3;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);
    y = 4;
    i = index(x, y);
    A_[i] =
        kState.B[i] ^ (~kState.B[index(x + 1, y)] & kState.B[index(x + 2, y)]);

    A_[0] ^= RC[n];
  }
}
