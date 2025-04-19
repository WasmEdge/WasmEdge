// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "model/vlm_sampling.h"
namespace WasmEdge::Host::WASINN::MLX {
namespace vlm {

mx::array topPSampling(const mx::array &Logits, float TopP, float Temperature) {

  mx::array WorkingLogits = Logits;

  if (WorkingLogits.dtype() == mx::bfloat16) {
    WorkingLogits = astype(WorkingLogits, mx::float32);
  }

  mx::array Probs = mx::softmax(WorkingLogits / Temperature, -1);
  mx::array SortedIndices = mx::argsort(Probs, -1);
  mx::array SqueezedIndices = mx::squeeze(SortedIndices, 0);
  mx::array SortedProbs = mx::take(Probs, SqueezedIndices, -1);
  mx::array CumulativeProbs = mx::cumsum(SortedProbs, -1);
  mx::array TopProbs = mx::where(CumulativeProbs > 1.0f - TopP, SortedProbs,
                                 mx::zeros_like(SortedProbs));
  mx::array SortedToken = mx::random::categorical(mx::log(TopProbs));
  mx::array Token = mx::take(SqueezedIndices, SortedToken);

  return Token;
}

} // namespace vlm
} // namespace WasmEdge::Host::WASINN::MLX
