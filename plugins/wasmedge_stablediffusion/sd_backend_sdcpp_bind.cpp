// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_backend_sdcpp.h"

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

const LibraryApi &realLibraryApi() noexcept {
  static const LibraryApi Api = [] {
    LibraryApi Real{};
    Real.GetLastError = ::sd_get_last_error;
    Real.SetLogCallback = ::sd_set_log_callback;
    Real.SetProgressCallback = ::sd_set_progress_callback;
    Real.TensorCreate = ::sd_tensor_create;
    Real.TensorFree = ::sd_tensor_free;
    Real.TensorType = ::sd_tensor_type;
    Real.TensorRank = ::sd_tensor_rank;
    Real.TensorDim = ::sd_tensor_dim;
    Real.TensorByteSize = ::sd_tensor_byte_size;
    Real.TensorData = ::sd_tensor_data;
    Real.CtxOptCreate = ::sd_ctx_opt_create;
    Real.ImgGenOptCreate = ::sd_img_gen_opt_create;
    Real.UpscalerOptCreate = ::sd_upscaler_opt_create;
    Real.UpscaleOptCreate = ::sd_upscale_opt_create;
    Real.OptFree = ::sd_opt_free;
    Real.OptSetBool = ::sd_opt_set_bool;
    Real.OptSetInt = ::sd_opt_set_int;
    Real.OptSetFloat = ::sd_opt_set_float;
    Real.OptSetString = ::sd_opt_set_string;
    Real.OptSetTensor = ::sd_opt_set_tensor;
    Real.OptSetLoras = ::sd_opt_set_loras;
    Real.OptSetEmbeddings = ::sd_opt_set_embeddings;
    Real.CtxCreate = ::sd_ctx_create;
    Real.CtxFree = ::sd_ctx_free;
    Real.CtxSetFileAccessCallback = ::sd_ctx_set_file_access_callback;
    Real.CtxSupportsImageGeneration = ::sd_ctx_supports_image_generation;
    Real.GenerateImage = ::sd_generate_image;
    Real.ResultGetImageCount = ::sd_result_get_image_count;
    Real.ResultGetImage = ::sd_result_get_image;
    Real.ResultFree = ::sd_result_free;
    Real.UpscalerCreate = ::sd_upscaler_create;
    Real.UpscalerFree = ::sd_upscaler_free;
    Real.UpscalerGetScale = ::sd_upscaler_get_scale;
    Real.Upscale = ::sd_upscale;
    return Real;
  }();
  return Api;
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
