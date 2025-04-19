// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once
#include "mlx/base.h"
#include <mlx/mlx.h>
namespace WasmEdge::Host::WASINN::MLX {
namespace vlm {

mx::array topPSampling(const mx::array &Logits, float TopP, float Temperature);

} // namespace vlm
} // namespace WasmEdge::Host::WASINN::MLX
