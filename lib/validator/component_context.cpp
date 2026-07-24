// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

#include "validator/component_context.h"

#include <algorithm>
#include <cctype>

namespace WasmEdge {
namespace Validator {

using Sort = AST::Component::Sort;

uint32_t
ComponentContext::Scope::getSortSize(Sort::SortType ST) const noexcept {
  switch (ST) {
  case Sort::SortType::Func:
    return static_cast<uint32_t>(Funcs.size());
  case Sort::SortType::Value:
    return static_cast<uint32_t>(Values.size());
  case Sort::SortType::Type:
    return static_cast<uint32_t>(Types.size());
  case Sort::SortType::Component:
    return static_cast<uint32_t>(Components.size());
  case Sort::SortType::Instance:
    return static_cast<uint32_t>(Instances.size());
  default:
    return 0;
  }
}

uint32_t
ComponentContext::Scope::getCoreSortSize(Sort::CoreSortType ST) const noexcept {
  switch (ST) {
  case Sort::CoreSortType::Func:
    return static_cast<uint32_t>(CoreFuncs.size());
  case Sort::CoreSortType::Table:
    return static_cast<uint32_t>(CoreTables.size());
  case Sort::CoreSortType::Memory:
    return static_cast<uint32_t>(CoreMemories.size());
  case Sort::CoreSortType::Global:
    return static_cast<uint32_t>(CoreGlobals.size());
  case Sort::CoreSortType::Tag:
    return static_cast<uint32_t>(CoreTags.size());
  case Sort::CoreSortType::Type:
    return static_cast<uint32_t>(CoreTypes.size());
  case Sort::CoreSortType::Module:
    return static_cast<uint32_t>(CoreModules.size());
  case Sort::CoreSortType::Instance:
    return static_cast<uint32_t>(CoreInstances.size());
  default:
    return 0;
  }
}

namespace {
std::string toLowerCopy(std::string_view SV) noexcept {
  std::string R(SV);
  std::transform(R.begin(), R.end(), R.begin(), [](unsigned char C) {
    return static_cast<char>(std::tolower(C));
  });
  return R;
}
} // namespace

ComponentContext::NameRecord
ComponentContext::makeNameRecord(const ComponentName &Name) noexcept {
  NameRecord R;
  R.Original = std::string(Name.getOriginalName());
  switch (Name.getKind()) {
  case ComponentNameKind::Constructor:
    R.HasAnnotation = true;
    R.IsConstructor = true;
    R.StrippedExact = std::string(Name.getNoTagName());
    break;
  case ComponentNameKind::Method:
  case ComponentNameKind::Static: {
    R.HasAnnotation = true;
    R.StrippedExact = std::string(Name.getNoTagName());
    auto Dot = R.StrippedExact.find('.');
    if (Dot != std::string::npos) {
      R.DottedFirst = R.StrippedExact.substr(0, Dot);
      R.IsDottedSame = (R.DottedFirst == R.StrippedExact.substr(Dot + 1));
    }
    break;
  }
  case ComponentNameKind::Label:
    R.IsPlainLabel = true;
    R.StrippedExact = std::string(Name.getOriginalName());
    break;
  default:
    R.StrippedExact = std::string(Name.getOriginalName());
    break;
  }
  R.IsPlainish = R.IsPlainLabel || R.HasAnnotation;
  // Case-folding models the acronym rule, which only applies to labels and
  // interface names; dep/url/integrity names compare exactly.
  switch (Name.getKind()) {
  case ComponentNameKind::LockedDep:
  case ComponentNameKind::UnlockedDep:
  case ComponentNameKind::Url:
  case ComponentNameKind::Integrity:
    R.Stripped = R.StrippedExact;
    break;
  default:
    R.Stripped = toLowerCopy(R.StrippedExact);
    break;
  }
  return R;
}

ComponentContext::NameClash
ComponentContext::addUniqueName(std::vector<NameRecord> &Names,
                                const NameRecord &N) noexcept {
  for (const auto &E : Names) {
    if (E.Original == N.Original) {
      return NameClash::Duplicate;
    }
    if (E.Stripped == N.Stripped) {
      // `l` and `[constructor]l` (for the *same* label) are the one
      // strongly-unique annotated pair.
      const bool CtorException = ((E.IsConstructor && N.IsPlainLabel) ||
                                  (N.IsConstructor && E.IsPlainLabel)) &&
                                 E.StrippedExact == N.StrippedExact;
      if (!CtorException) {
        return NameClash::Conflict;
      }
      continue;
    }
    // `l` clashes with `[method]l.l` / `[static]l.l` for the same label.
    if ((N.IsPlainLabel && E.IsDottedSame &&
         E.DottedFirst == N.StrippedExact) ||
        (E.IsPlainLabel && N.IsDottedSame &&
         N.DottedFirst == E.StrippedExact)) {
      return NameClash::Conflict;
    }
  }
  Names.push_back(N);
  return NameClash::None;
}

} // namespace Validator
} // namespace WasmEdge
