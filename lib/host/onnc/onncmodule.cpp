// SPDX-License-Identifier: Apache-2.0
#include "host/onnc/onncmodule.h"
#include "host/onnc/onncfunc.h"

#include <memory>

namespace SSVM {
namespace Host {

ONNCModule::ONNCModule() : ImportObject("onnc_wasm") {
  addHostFunc("ONNC_RUNTIME_add_float",
              std::make_unique<ONNCRuntimeAddFloat>());
  addHostFunc("ONNC_RUNTIME_add_int8", std::make_unique<ONNCRuntimeAddInt8>());
  addHostFunc("ONNC_RUNTIME_averagepool_float",
              std::make_unique<ONNCRuntimeAveragepoolFloat>());
  addHostFunc("ONNC_RUNTIME_batchnormalization_float",
              std::make_unique<ONNCRuntimeBatchnormalizationFloat>());
  addHostFunc("ONNC_RUNTIME_batchnormalization_int8",
              std::make_unique<ONNCRuntimeBatchnormalizationInt8>());
  addHostFunc("ONNC_RUNTIME_concat_float",
              std::make_unique<ONNCRuntimeConcatFloat>());
  addHostFunc("ONNC_RUNTIME_conv_float",
              std::make_unique<ONNCRuntimeConvFloat>());
  addHostFunc("ONNC_RUNTIME_conv_int8",
              std::make_unique<ONNCRuntimeConvInt8>());
  addHostFunc("ONNC_RUNTIME_gemm_float",
              std::make_unique<ONNCRuntimeGemmFloat>());
  addHostFunc("ONNC_RUNTIME_globalaveragepool_float",
              std::make_unique<ONNCRuntimeGlobalaveragepoolFloat>());
  addHostFunc("ONNC_RUNTIME_lrn_float",
              std::make_unique<ONNCRuntimeLrnFloat>());
  addHostFunc("ONNC_RUNTIME_maxpool_float",
              std::make_unique<ONNCRuntimeMaxpoolFloat>());
  addHostFunc("ONNC_RUNTIME_maxpool_int8",
              std::make_unique<ONNCRuntimeMaxpoolInt8>());
  addHostFunc("ONNC_RUNTIME_mul_float",
              std::make_unique<ONNCRuntimeMulFloat>());
  addHostFunc("ONNC_RUNTIME_mul_int8", std::make_unique<ONNCRuntimeMulInt8>());
  addHostFunc("ONNC_RUNTIME_relu_float",
              std::make_unique<ONNCRuntimeReluFloat>());
  addHostFunc("ONNC_RUNTIME_relu_int8",
              std::make_unique<ONNCRuntimeReluInt8>());
  addHostFunc("ONNC_RUNTIME_reshape_float",
              std::make_unique<ONNCRuntimeReshapeFloat>());
  addHostFunc("ONNC_RUNTIME_softmax_float",
              std::make_unique<ONNCRuntimeSoftmaxFloat>());
  addHostFunc("ONNC_RUNTIME_sum_float",
              std::make_unique<ONNCRuntimeSumFloat>());
  addHostFunc("ONNC_RUNTIME_transpose_float",
              std::make_unique<ONNCRuntimeTransposeFloat>());
  addHostFunc("ONNC_RUNTIME_unsqueeze_float",
              std::make_unique<ONNCRuntimeUnsqueezeFloat>());
}

} // namespace Host
} // namespace SSVM