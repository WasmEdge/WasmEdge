// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"

#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {
/// TODO: Better doc
/// An authentication tag is always returned as handle. wasi-crypto bindings
/// SHOULD verify them with the symmetric_tag_verify() function instead of
/// exporting them and doing the verification themselves.
///
/// Authentication tags are assumed to be very small. For this reason, copying
/// the actual tag to the guest environment requires a single function call,
/// which also immediately invalides the handle. Unlike array_output handles, no
/// streaming interface is necessary. Implementation can directly map handles to
/// the raw representation of a tag.
/// A guest application can obtain the size of a tag in bytes using the
/// symmetric_tag_len() function on an existing tag. For most algorithms, this
/// is a constant.
///
/// Finally, guest applications can obtain the byte representation of a tag
/// using symmetric_tag_pull():
/// If this function succeeds, the tag handle is automatically closed.
///
/// The output buffer is expected to have a size that exactly matches the tag
/// length, as returned by symmetric_tag_len().
//
/// The host MUST return an overflow error code if the output buffer is too
/// small, and invalid_length if it is too large.
class Tag {
public:
  Tag(std::vector<uint8_t> &&Data) : Data(Data) {}

  const auto &data() const { return Data; }

  /// The expected tag is always supplied as a byte string. Implementations are
  /// not required to support any serialization format.
  ///
  /// @return  The function MUST return `__WASI_CRYPTO_ERRNO_INVALID_TAG` if the
  /// tags don't match.
  WasiCryptoExpect<void> verify(Span<const uint8_t> ExpectedRaw) const;

private:
  const std::vector<uint8_t> Data;
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
