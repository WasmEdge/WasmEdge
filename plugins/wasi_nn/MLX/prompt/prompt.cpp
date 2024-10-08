// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "prompt/prompt.h"

#include <string>

namespace WasmEdge::Host::WASINN::MLX {

std::string TinyLLaMAPrompt::prepare(std::string Prompt) {
  return SystemStart + TextEnd + Prompt + TextEnd + Assistant;
}

std::string LLaMA2Prompt::prepare(std::string Prompt) {
  return InstStart + SysStart + SysEnd + Prompt + TextEnd;
}

std::string LLaMA3Prompt::prepare(std::string Prompt) {
  return PropmtStart + StartHeader + "system" + EndHeader + TextEnd + Prompt +
         EndHeader + TextEnd + StartHeader + "user" + EndHeader + Prompt +
         TextEnd + StartHeader + "assistant" + EndHeader;
}

} // namespace WasmEdge::Host::WASINN::MLX
