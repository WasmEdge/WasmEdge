// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include <cstring>
#include <iostream>
#include <string>

namespace WasmEdge::Host::WASINN::MLX {

class BasePrompt {
public:
  std::string TextEnd;

  virtual std::string prepare(std::string Prompt) { return Prompt + TextEnd; };
};

class TinyLLaMAPrompt : public BasePrompt {
public:
  std::string SystemStart;
  std::string User;
  std::string Assistant;

  TinyLLaMAPrompt() {
    SystemStart = "<|system|>";
    Assistant = "<|assistant|>";
    User = "<|user|>";
    TextEnd = "</s>";
  }

  std::string prepare(std::string Prompt) override;
};

class LLaMA2Prompt : public BasePrompt {
public:
  std::string SysStart;
  std::string SysEnd;
  std::string InstStart;

  LLaMA2Prompt() {
    SysStart = "<<SYS>>";
    SysEnd = "<</SYS>>";
    InstStart = "[INST]";
    TextEnd = "[/INST]";
  }

  std::string prepare(std::string Prompt) override;
};

class LLaMA3Prompt : public BasePrompt {
public:
  std::string PropmtStart;
  std::string StartHeader;
  std::string EndHeader;

  LLaMA3Prompt() {
    PropmtStart = "<|begin_of_text|>";
    StartHeader = "<|start_header_id|>";
    EndHeader = "<|end_header_id|>";
    TextEnd = "<|eot_id|>";
  }

  std::string prepare(std::string Prompt) override;
};

} // namespace WasmEdge::Host::WASINN::MLX
