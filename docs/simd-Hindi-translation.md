१२८-बिट पैक्ड सिंगल इंस्ट्रक्शन मल्टीपल डाटा (एस.आई.एम.डी)

एस.आई.एम.डी निर्देश केवल एक दिशा में पैक किए गए डाटा पर एक साथ गड़ना प्रदान करते हैं। यह आमतौर पर मल्टीमीडिया अनुप्रयोगों के लिए प्रदर्शन के सुधार के लिए उपयोग किया जाता है। एस.आई.एम.डी प्रस्ताव के साथ, मॉड्यूल जो की आधुनिक हार्डवेयर में प्रयोग किए जाते हैं वो भी अपनी गति में वृध्दि प्राप्त कर सकते हैं।

यदि आप एस.आई.एम.डी प्रस्ताव को सक्षम करने में रुचि रखते हैं तो अनुप्रयोगों के प्रदर्शन में बदलाव की जानकारी के लिए हमारी डब्लू.ए. एस.एम.३२-डब्लू.ए.एस.आई बेंचमार्क देखें। हमारे बेंचमार्क में मैंडेलब्रॉट सेट अनुप्रयोग में २.६५ गुना की वृद्धि देखने को मिली।


वास्म एज एस.आई.एम.डी उदाहरण

C भाषा संहिता -मैंडेलब्रॉट सेट
हमने अपने डब्लू.ए. एस.एम.३२-डब्लू.ए.एस.आई बेंचमार्क प्रोजेक्ट से मैंडेलब्रोट सेट उदाहरण को संशोधित किया।

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


स –एस.आई. एम.डी अनुप्रयोग को संकलित करें वास्म-एस.आई.एम.डी बाइनरी  के साथ ई.एम.सी.सी का प्रयोग करके

इंस्टॉल ई.एम.सी.सी

इसको संकलित करने के लिए हमें ई.एम.सी.सी का सबसे नया टूलचेन इंस्टॉल करना होगा। इसके लिए कृपा करके ई.एम.सी.सी की औपचारिक रिपोजिट्री से विस्तृत निर्देश प्राप्त करें।

git clone --depth 1 https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

ई.एम.सी.सी से संकलित करें

emcc -g -Oz --llvm-lto 1 -s STANDALONE_WASM -s INITIAL_MEMORY=32MB -s MAXIMUM_MEMORY=4GB \
  -mmutable-globals \
  -mnontrapping-fptoint \
  -msign-ext \
  mandelbrot-simd.c -o mandelbrot-simd.wasm

वास्मएज का प्रयोग कर चलाएं

दुभाषिया मोड
$ wasmedge --enable-simd mandelbrot-simd.wasm 15000

समय से आगे मोड
# Compile wasm-simd with wasmedge aot compiler
$ wasmedgec mandelbrot-simd.wasm mandelbrot-simd-out.wasm
# Run the native binary with wasmedge
$ wasmedge mandelbrot-simd-out.wasm 15000
