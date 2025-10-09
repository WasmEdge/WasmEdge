// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "GGML/core/ggml_core.h"
#include "simdjson.h"
#include "wasinntypes.h"

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
namespace {

// JSON parsing helper template
template <typename T>
T getJsonValue(const simdjson::dom::element &Doc, std::string_view Key) {
  if (Doc.at_key(Key).error() == simdjson::SUCCESS) {
    T Value{};
    auto Err = Doc[Key].get<T>().get(Value);
    if (Err) {
      std::string Msg = fmt::format("Unable to retrieve the {} option.", Key);
      spdlog::error("[WASI-NN] GGML backend: {}", Msg);
      throw ErrNo(ErrNo::InvalidArgument);
    }
    return Value;
  }
  throw ErrNo(ErrNo::NotFound);
}

template <typename T>
void parseJsonAuto(const simdjson::dom::element &Doc, std::string_view Key,
                   T &Var) {
  try {
    auto Result = getJsonValue<T>(Doc, Key);
    Var = Result;
  } catch (ErrNo E) {
    if (E != ErrNo::NotFound) {
      throw E;
    }
  }
}

template <typename FromType, typename ToType>
void parseJsonWithCastAuto(const simdjson::dom::element &Doc,
                           std::string_view Key, ToType &Var) {
  try {
    auto Result = getJsonValue<FromType>(Doc, Key);
    Var = static_cast<ToType>(Result);
  } catch (ErrNo E) {
    if (E != ErrNo::NotFound) {
      throw E;
    }
  }
}

template <typename T, typename Processor>
void parseJsonWithProcessorAuto(const simdjson::dom::element &Doc,
                                std::string_view Key, Processor Proc) {
  try {
    auto Result = getJsonValue<T>(Doc, Key);
    if (!Proc(Result)) {
      throw ErrNo{ErrNo::InvalidArgument};
    }
  } catch (ErrNo E) {
    if (E != ErrNo::NotFound) {
      throw E;
    }
  }
}

} // namespace
ErrNo parseMetadata(Graph &GraphRef, LocalConfig &ConfRef,
                    const std::string &Metadata, bool *IsModelUpdated = nullptr,
                    bool *IsContextUpdated = nullptr,
                    bool *IsSamplerUpdated = nullptr) noexcept;
#endif
} // namespace WasmEdge::Host::WASINN::GGML
