// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "mlx/base.h"

#include <mlx/array.h>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core {

mx::array gelu(mx::array X);

mx::array silu(mx::array X);
mx::array geluApprox(mx::array X);
} // namespace mlx::core
} // namespace WasmEdge::Host::WASINN::MLX
