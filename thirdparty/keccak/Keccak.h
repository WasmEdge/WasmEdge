// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

// State structure
class Keccak {
public:
  Keccak(unsigned int len);
  ~Keccak();
  std::vector<unsigned char> digest();
  void addPadding();
  void reset();
  void keccakf();
  void addData(uint8_t input);
  void addData(const uint8_t *input, unsigned int off, unsigned int len);
  void processBuffer();

private:
  uint64_t *A;
  unsigned int blockLen;
  uint8_t *buffer;
  unsigned int bufferLen;
  unsigned int length;
};