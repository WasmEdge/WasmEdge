#include "prompt.h"

std::string TinyLLaMAPrompt::prepare(std::string Prompt) {
  return SystemStart + TextEnd + Prompt + TextEnd + Assistant;
}