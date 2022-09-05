# WebAssembly SIMD Example in C

[128-bit packed Single Instruction Multiple Data (SIMD)](https://webassembly.github.io/simd/core/syntax/instructions.html#simd-instructions) instructions provide simultaneous computations over packed data in just one instruction. It's commonly used to improve performance for multimedia applications. With the SIMD proposal, the modules can benefit from using these commonly used instructions in modern hardware to gain more speedup.

If you are interested in enabling the SIMD proposal will improve how much performance of the applications, please refer to our [wasm32-wasi benchmark](https://github.com/second-state/wasm32-wasi-benchmark/tree/simd) for more information.
In our benchmark, the Mandelbrot Set application can have **2.65x** speedup.

## C language Code - Mandelbrot Set

We modified the Mandelbrot Set example from our [wasm32-wasi benchmark project](https://github.com/second-state/wasm32-wasi-benchmark/blob/master/src/mandelbrot.c).

```c
#define LIMIT_SQUARED 4.0
#define MAXIMUM_ITERATIONS 50

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef double doublex2 __attribute__((vector_size(16)));
typedef uint64_t uint64x2_t __attribute__((vector_size(16)));

static inline void calcSum(doublex2 *r, doublex2 *i, doublex2 *sum,
                           const doublex2 init_r[4], const doublex2 init_i) {
  for (uint64_t x_Minor = 0; x_Minor < 4; x_Minor++) {
    doublex2 r2 = r[x_Minor] * r[x_Minor];
    doublex2 i2 = i[x_Minor] * i[x_Minor];
    doublex2 ri = r[x_Minor] * i[x_Minor];

    sum[x_Minor] = r2 + i2;

    r[x_Minor] = r2 - i2 + init_r[x_Minor];
    i[x_Minor] = ri + ri + init_i;
  }
}

static inline bool vec_gt(const doublex2 *v) {
  const doublex2 f = {LIMIT_SQUARED, LIMIT_SQUARED};
  const uint64x2_t r = (v[0] > f) & (v[1] > f) & (v[2] > f) & (v[3] > f);
  return r[0] && r[1];
}

static inline uint8_t clrPixels_gt(const doublex2 *v) {
  const doublex2 f = {LIMIT_SQUARED, LIMIT_SQUARED};
  const uint64x2_t r0 = v[0] <= f;
  const uint64x2_t r1 = v[1] <= f;
  const uint64x2_t r2 = v[2] <= f;
  const uint64x2_t r3 = v[3] <= f;
  return (r0[0] & 0x1) << 7 | (r0[1] & 0x1) << 6 | (r1[0] & 0x1) << 5 |
         (r1[1] & 0x1) << 4 | (r2[0] & 0x1) << 3 | (r2[1] & 0x1) << 2 |
         (r3[0] & 0x1) << 1 | (r3[1] & 0x1) << 0;
}

static inline uint8_t mand8(const doublex2 init_r[4], const doublex2 init_i) {
  doublex2 pixel_Group_r[4], pixel_Group_i[4];
  for (uint64_t x_Minor = 0; x_Minor < 4; x_Minor++) {
    pixel_Group_r[x_Minor] = init_r[x_Minor];
    pixel_Group_i[x_Minor] = init_i;
  }

  doublex2 sum[4];
  for (unsigned j = 0; j < 6; j++) {
    for (unsigned k = 0; k < 8; k++) {
      calcSum(pixel_Group_r, pixel_Group_i, sum, init_r, init_i);
    }
    if (vec_gt(sum)) {
      return 0x00;
    }
  }
  calcSum(pixel_Group_r, pixel_Group_i, sum, init_r, init_i);
  calcSum(pixel_Group_r, pixel_Group_i, sum, init_r, init_i);
  return clrPixels_gt(sum);
}

static inline uint64_t mand64(const doublex2 init_r[4], const doublex2 init_i) {
  uint64_t sixtyfour_Pixels = 0;
  for (uint64_t byte = 0; byte < 8; byte++) {
    const uint64_t eight_Pixels = mand8(init_r + 4 * byte, init_i);
    sixtyfour_Pixels =
        (sixtyfour_Pixels >> UINT64_C(8)) | (eight_Pixels << UINT64_C(56));
  }
  return sixtyfour_Pixels;
}

int main(int argc, char **argv) {
  const uint64_t image_Width_And_Height =
      (__builtin_expect(atoi(argv[1]), 15000) + 7) / 8 * 8;

  uint8_t *const pixels =
      malloc(image_Width_And_Height * image_Width_And_Height / 8);

  doublex2 initial_r[image_Width_And_Height / 2];
  double initial_i[image_Width_And_Height];
  for (uint64_t xy = 0; xy < image_Width_And_Height; xy++) {
    initial_r[xy / 2] =
        2.0 / image_Width_And_Height * (doublex2){xy, xy + 1} - 1.5;
    initial_i[xy] = 2.0 / image_Width_And_Height * xy - 1.0;
  }

  if (image_Width_And_Height % 64) {
    // process 8 pixels (one byte) at a time
    for (uint64_t y = 0; y < image_Width_And_Height; y++) {
      const doublex2 prefetched_Initial_i = {initial_i[y], initial_i[y]};
      const size_t rowStart = y * image_Width_And_Height / 8;
      for (uint64_t x_Major = 0; x_Major < image_Width_And_Height;
           x_Major += 8) {
        const doublex2 *prefetched_Initial_r = &initial_r[x_Major / 2];
        pixels[rowStart + x_Major / 8] =
            mand8(prefetched_Initial_r, prefetched_Initial_i);
      }
    }
  } else {
    // process 64 pixels (8 bytes) at a time
    for (uint64_t y = 0; y < image_Width_And_Height; y++) {
      const doublex2 prefetched_Initial_i = {initial_i[y], initial_i[y]};
      const size_t rowStart = y * image_Width_And_Height / 8;
      for (uint64_t x_Major = 0; x_Major < image_Width_And_Height;
           x_Major += 8) {
        const doublex2 *prefetched_Initial_r = &initial_r[x_Major / 2];
        const uint64_t sixtyfour_Pixels =
            mand64(prefetched_Initial_r, prefetched_Initial_i);
        memcpy(&pixels[rowStart + x_Major / 8], &sixtyfour_Pixels, 8);
      }
    }
  }

  fprintf(stdout, "P4\n%" PRIu64 " %" PRIu64 "\n", image_Width_And_Height,
          image_Width_And_Height);
  fwrite(pixels, image_Width_And_Height * image_Width_And_Height / 8, 1,
         stdout);

  free(pixels);

  return 0;
}
```

## Compile the C-SIMD application to Wasm-SIMD binary with emcc

### Install emcc

To compile it, you will need to install the latest emcc toolchain. Please refer to the [emcc official repository](https://github.com/emscripten-core/emsdk) for the detailed instructions.

```bash
git clone --depth 1 https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### Compile with emcc

```bash
emcc -g -Oz --llvm-lto 1 -s STANDALONE_WASM -s INITIAL_MEMORY=32MB -s MAXIMUM_MEMORY=4GB \
  -mmutable-globals \
  -mnontrapping-fptoint \
  -msign-ext \
  mandelbrot-simd.c -o mandelbrot-simd.wasm
```

## Run with wasmedge

### Interpreter mode

```bash
wasmedge mandelbrot-simd.wasm 15000
```

### Ahead-of-Time mode

```bash
# Compile wasm-simd with wasmedge aot compiler
$ wasmedgec mandelbrot-simd.wasm mandelbrot-simd-out.wasm
# Run the native binary with wasmedge
$ wasmedge mandelbrot-simd-out.wasm 15000
```
