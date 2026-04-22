#include "validator/component_context.h"

namespace WasmEdge {
namespace Validator {

using Sort = AST::Component::Sort;

uint32_t
ComponentContext::Context::getSortIndexSize(Sort::SortType ST) const noexcept {
  switch (ST) {
  case Sort::SortType::Func:
    return FuncCount;
  case Sort::SortType::Value:
    return ValueCount;
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

uint32_t ComponentContext::Context::getCoreSortIndexSize(
    Sort::CoreSortType ST) const noexcept {
  switch (ST) {
  case Sort::CoreSortType::Func:
    return CoreFuncCount;
  case Sort::CoreSortType::Table:
    return static_cast<uint32_t>(CoreTables.size());
  case Sort::CoreSortType::Memory:
    return static_cast<uint32_t>(CoreMemories.size());
  case Sort::CoreSortType::Global:
    return static_cast<uint32_t>(CoreGlobals.size());
  case Sort::CoreSortType::Tag:
    return CoreTagCount;
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

uint32_t ComponentContext::incSortIndexSize(Sort::SortType ST) noexcept {
  switch (ST) {
  case Sort::SortType::Func:
    return addFunc();
  case Sort::SortType::Value:
    return addValue();
  case Sort::SortType::Type:
    return addType();
  case Sort::SortType::Component:
    return addComponent();
  case Sort::SortType::Instance:
    return addInstance();
  default:
    return 0;
  }
}

uint32_t
ComponentContext::incCoreSortIndexSize(Sort::CoreSortType ST) noexcept {
  switch (ST) {
  case Sort::CoreSortType::Func:
    return addCoreFunc();
  case Sort::CoreSortType::Table:
    return addCoreTable();
  case Sort::CoreSortType::Memory:
    return addCoreMemory();
  case Sort::CoreSortType::Global:
    return addCoreGlobal();
  case Sort::CoreSortType::Tag:
    return addCoreTag();
  case Sort::CoreSortType::Type:
    return addCoreType();
  case Sort::CoreSortType::Module:
    return addCoreModule();
  case Sort::CoreSortType::Instance:
    return addCoreInstance();
  default:
    return 0;
  }
}

bool ComponentContext::Context::AddImportedName(
    const ComponentName &Name) noexcept {
  switch (Name.getKind()) {
  case ComponentNameKind::Constructor:
  case ComponentNameKind::Method:
  case ComponentNameKind::Static:
  case ComponentNameKind::InterfaceType:
  case ComponentNameKind::Label:
  case ComponentNameKind::LockedDep:
  case ComponentNameKind::UnlockedDep:
  case ComponentNameKind::Url:
  case ComponentNameKind::Integrity:
    break;
  default:
    return false;
  }

  auto toLowerString = [](std::string_view SV) {
    std::string Result = std::string(SV);
    std::transform(
        Result.begin(), Result.end(), Result.begin(),
        [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
    return Result;
  };

  // Handle the Constructor case separately.
  if (Name.getKind() == ComponentNameKind::Constructor) {
    std::string LowerCase = toLowerString(Name.getOriginalName());
    std::string Label = std::string(Name.getNoTagName());
    // check conflict with existing constructors
    if (ImportedNames.count(LowerCase)) {
      return false;
    }

    if (ImportedNames.count(toLowerString(Label))) {
      if (!ImportedNames.count(Label)) {
        return false;
      }
      // By rule, a constructor [constructor]X and X are strongly-unique.
      // if X and its lower-case x form both exist, it meaning x is coming
      // from X.
    }
    ImportedNames.insert(LowerCase);
    ImportedNames.insert(std::string(Name.getOriginalName()));
    return true;
  }

  // For case 2, L and L.L is not strongly-unique together.
  std::string Normal = std::string(Name.getNoTagName());
  std::string UniForm = toLowerString(Normal);
  std::string LdL =
      std::string(Name.getNoTagName()) + "." + std::string(Name.getNoTagName());

  if (ImportedNames.count(LdL)) {
    return false;
  }

  if (Normal.find('.') != std::string::npos) {
    std::string Left, Right;
    size_t Pos = Normal.find('.');
    Left = Normal.substr(0, Pos);
    Right = Normal.substr(Pos + 1);
    if (Left == Right) {
      // conflict with l.l and [*]l
      if (ImportedNames.count(toLowerString(Left))) {
        return false;
      }
    }
  }

  // case 3, check existing names
  if (ImportedNames.count(UniForm)) {
    return false;
  }

  // Special case, check conflict with constructor names
  std::string ConstrName = "[constructor]" + UniForm;
  if (ImportedNames.count(ConstrName)) {
    if (!ImportedNames.count("[constructor]" + Normal)) {
      return false;
    }
    // By rule, a constructor [constructor]X and X are strongly-unique.
    // if [constructor]X and its lower-case [constructor]x form both exist,
    // it meaning [constructor]x is coming from [constructor]X.
  }
  ImportedNames.insert(Normal);
  ImportedNames.insert(UniForm);
  return true;
}
} // namespace Validator
} // namespace WasmEdge
