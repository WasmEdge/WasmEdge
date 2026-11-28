// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_backend.h"

#include "stable-diffusion.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

// One member for each library function that the adapter uses, typed from the
// library header.
struct LibraryApi {
  decltype(&::sd_get_last_error) GetLastError;
  decltype(&::sd_set_log_callback) SetLogCallback;
  decltype(&::sd_set_progress_callback) SetProgressCallback;
  decltype(&::sd_tensor_create) TensorCreate;
  decltype(&::sd_tensor_free) TensorFree;
  decltype(&::sd_tensor_type) TensorType;
  decltype(&::sd_tensor_rank) TensorRank;
  decltype(&::sd_tensor_dim) TensorDim;
  decltype(&::sd_tensor_byte_size) TensorByteSize;
  decltype(&::sd_tensor_data) TensorData;
  decltype(&::sd_ctx_opt_create) CtxOptCreate;
  decltype(&::sd_img_gen_opt_create) ImgGenOptCreate;
  decltype(&::sd_upscaler_opt_create) UpscalerOptCreate;
  decltype(&::sd_upscale_opt_create) UpscaleOptCreate;
  decltype(&::sd_opt_free) OptFree;
  decltype(&::sd_opt_set_bool) OptSetBool;
  decltype(&::sd_opt_set_int) OptSetInt;
  decltype(&::sd_opt_set_float) OptSetFloat;
  decltype(&::sd_opt_set_string) OptSetString;
  decltype(&::sd_opt_set_tensor) OptSetTensor;
  decltype(&::sd_opt_set_loras) OptSetLoras;
  decltype(&::sd_opt_set_embeddings) OptSetEmbeddings;
  decltype(&::sd_ctx_create) CtxCreate;
  decltype(&::sd_ctx_free) CtxFree;
  decltype(&::sd_ctx_set_file_access_callback) CtxSetFileAccessCallback;
  decltype(&::sd_ctx_supports_image_generation) CtxSupportsImageGeneration;
  decltype(&::sd_generate_image) GenerateImage;
  decltype(&::sd_result_get_image_count) ResultGetImageCount;
  decltype(&::sd_result_get_image) ResultGetImage;
  decltype(&::sd_result_free) ResultFree;
  decltype(&::sd_upscaler_create) UpscalerCreate;
  decltype(&::sd_upscaler_free) UpscalerFree;
  decltype(&::sd_upscaler_get_scale) UpscalerGetScale;
  decltype(&::sd_upscale) Upscale;
};

// Creates the adapter for stable-diffusion.cpp. Api must outlive the backend
// and all its contexts. Returns null only if the allocation fails.
std::shared_ptr<Backend> makeSdcppBackend(const LibraryApi &Api) noexcept;

// Returns a LibraryApi with every member set to the real function. Defined in
// sd_backend_sdcpp_bind.cpp, which only the plug-in compiles.
const LibraryApi &realLibraryApi() noexcept;

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
