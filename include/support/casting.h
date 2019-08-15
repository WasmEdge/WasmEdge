#pragma once

#include <type_traits>

namespace SSVM {
namespace Support {

template<typename X, typename Y>
inline bool isa(const Y ptr) {
    return dynamic_cast<const X*>(ptr) != nullptr;
}

template<typename T>
inline T bytesToInt(const std::vector<unsigned char> &Bytes) {
  if (std::is_same<T, int32_t>::value) {
    return (Bytes[3] << 3
            | Bytes[2] << 2
            | Bytes[1] << 1
            | Bytes[0]);
  } else if (std::is_same<T, int64_t>::value) {
    return (Bytes[7] << 7
            | Bytes[6] << 6
            | Bytes[5] << 5
            | Bytes[4] << 4
            | Bytes[3] << 3
            | Bytes[2] << 2
            | Bytes[1] << 1
            | Bytes[0]);
  } else {
    /// TODO: We do not handle the `t.loadN_sx` instructions.
    return 0;
  }
}

template<typename T>
inline std::vector<unsigned char> IntTobytes(T Int) {
  std::vector<unsigned char> Bytes;
  if (std::is_same<T, int32_t>::value) {
    Bytes.push_back(Int & 0xFF);
    Bytes.push_back(Int & 0xFF00);
    Bytes.push_back(Int & 0xFF0000);
    Bytes.push_back(Int & 0xFF000000);
  } else if (std::is_same<T, int64_t>::value) {
    Bytes.push_back(Int & 0xFF);
    Bytes.push_back(Int & 0xFF00);
    Bytes.push_back(Int & 0xFF0000);
    Bytes.push_back(Int & 0xFF000000);
    Bytes.push_back(Int & 0xFF00000000);
    Bytes.push_back(Int & 0xFF0000000000);
    Bytes.push_back(Int & 0xFF000000000000);
    Bytes.push_back(Int & 0xFF00000000000000);
  }

  return Bytes;
}

} // namespace Support
} // namespace SSVM
