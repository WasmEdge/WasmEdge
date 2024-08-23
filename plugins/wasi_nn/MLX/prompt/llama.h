#pragma once

#include <cstring>
#include <iostream>

class BasePrompt {
public:
  std::string SystemStart;
  std::string User;
  std::string Assistant;
  std::string TextEnd;
  virtual std::string prepare(std::string Prompt) {
    return SystemStart + TextEnd + Prompt + TextEnd + Assistant;
  };
};

class TinyLLaMAPrompt : public BasePrompt {
public:
  std::string SystemStart = "<|system|>";
  std::string User = "<|user|>";
  std::string Assistant = "<|assistant|>";
  std::string TextEnd = "</s>";
  std::string prepare(std::string Prompt);
};