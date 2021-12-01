// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/po/subcommand.h - SubCommand -----------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "po/helper.h"

#include <string_view>
#include <vector>

namespace WasmEdge {
namespace PO {

using namespace std::literals;
class SubCommand {
public:
  SubCommand() = default;
  template <typename... ArgsT>
  SubCommand(Description &&D, ArgsT &&...Args)
      : SubCommand(std::forward<ArgsT>(Args)...) {
    Desc = std::move(D.Value);
  }
  std::string_view description() const noexcept { return Desc; }
  void select() noexcept { Selected = true; }
  bool is_selected() const noexcept { return Selected; }

private:
  std::string_view Desc;
  bool Selected = false;
};

} // namespace PO
} // namespace WasmEdge
