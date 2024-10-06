// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/ast/section.h - Section class definitions ----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Section node classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/alias.h"
#include "ast/component/canonical.h"
#include "ast/component/import_export.h"
#include "ast/component/instance.h"
#include "ast/component/start.h"
#include "ast/component/type.h"
#include "ast/description.h"
#include "ast/segment.h"

#include <optional>
#include <vector>

namespace WasmEdge {
namespace AST {

/// Section's base class.
class Section {
public:
  /// Getter and setter of content size.
  uint64_t getContentSize() const noexcept { return ContentSize; }
  void setContentSize(uint64_t Size) noexcept { ContentSize = Size; }

  /// Getter and setter of start offset in source.
  uint64_t getStartOffset() const noexcept { return StartOffset; }
  void setStartOffset(uint64_t Off) noexcept { StartOffset = Off; }

protected:
  /// Content size of this section.
  uint64_t ContentSize = 0;

  /// Start offset in source of this section.
  uint64_t StartOffset = 0;
};

/// AST CustomSection node.
class CustomSection : public Section {
public:
  /// Getter and setter of name.
  std::string_view getName() const noexcept { return Name; }
  void setName(std::string_view N) { Name = N; }

  /// Getter of content vector.
  Span<const Byte> getContent() const noexcept { return Content; }
  std::vector<Byte> &getContent() noexcept { return Content; }

private:
  /// \name Data of CustomSection.
  /// @{
  std::string Name;
  std::vector<Byte> Content;
  /// @}
};

/// AST TypeSection node.
class TypeSection : public Section {
public:
  /// Getter of content vector.
  Span<const SubType> getContent() const noexcept { return Content; }
  std::vector<SubType> &getContent() noexcept { return Content; }

private:
  /// \name Data of TypeSection.
  /// @{
  std::vector<SubType> Content;
  /// @}
};

/// AST ImportSection node.
class ImportSection : public Section {
public:
  /// Getter of content vector.
  Span<const ImportDesc> getContent() const noexcept { return Content; }
  std::vector<ImportDesc> &getContent() noexcept { return Content; }

private:
  /// \name Data of ImportSection.
  /// @{
  std::vector<ImportDesc> Content;
  /// @}
};

/// AST FunctionSection node.
class FunctionSection : public Section {
public:
  /// Getter of content vector.
  Span<const uint32_t> getContent() const noexcept { return Content; }
  std::vector<uint32_t> &getContent() noexcept { return Content; }

private:
  /// \name Data of FunctionSection.
  /// @{
  std::vector<uint32_t> Content;
  /// @}
};

/// AST TableSection node.
class TableSection : public Section {
public:
  /// Getter of content vector.
  Span<const TableSegment> getContent() const noexcept { return Content; }
  std::vector<TableSegment> &getContent() noexcept { return Content; }

private:
  /// \name Data of TableSection.
  /// @{
  std::vector<TableSegment> Content;
  /// @}
};

/// AST MemorySection node.
class MemorySection : public Section {
public:
  /// Getter of content vector.
  Span<const MemoryType> getContent() const noexcept { return Content; }
  std::vector<MemoryType> &getContent() noexcept { return Content; }

private:
  /// \name Data of MemorySection.
  /// @{
  std::vector<MemoryType> Content;
  /// @}
};

/// AST GlobalSection node.
class GlobalSection : public Section {
public:
  /// Getter of content vector.
  Span<const GlobalSegment> getContent() const noexcept { return Content; }
  std::vector<GlobalSegment> &getContent() noexcept { return Content; }

private:
  /// \name Data of GlobalSection.
  /// @{
  std::vector<GlobalSegment> Content;
  /// @}
};

/// AST ExportSection node.
class ExportSection : public Section {
public:
  /// Getter of content vector.
  Span<const ExportDesc> getContent() const noexcept { return Content; }
  std::vector<ExportDesc> &getContent() noexcept { return Content; }

private:
  /// \name Data of ExportSection.
  /// @{
  std::vector<ExportDesc> Content;
  /// @}
};

/// AST StartSection node.
class StartSection : public Section {
public:
  /// Getter and setter of content.
  std::optional<uint32_t> getContent() const { return Content; }
  void setContent(uint32_t Val) noexcept { Content = Val; }

private:
  /// \name Data of StartSection.
  /// @{
  std::optional<uint32_t> Content = std::nullopt;
  /// @}
};

/// AST ElementSection node.
class ElementSection : public Section {
public:
  /// Getter of content vector.
  Span<const ElementSegment> getContent() const noexcept { return Content; }
  std::vector<ElementSegment> &getContent() noexcept { return Content; }

private:
  /// \name Data of ElementSection.
  /// @{
  std::vector<ElementSegment> Content;
  /// @}
};

/// AST CodeSection node.
class CodeSection : public Section {
public:
  /// Getter of content vector.
  Span<const CodeSegment> getContent() const noexcept { return Content; }
  std::vector<CodeSegment> &getContent() noexcept { return Content; }

private:
  /// \name Data of CodeSection.
  /// @{
  std::vector<CodeSegment> Content;
  /// @}
};

/// AST DataSection node.
class DataSection : public Section {
public:
  /// Getter of content vector.
  Span<const DataSegment> getContent() const noexcept { return Content; }
  std::vector<DataSegment> &getContent() noexcept { return Content; }

private:
  /// \name Data of DataSection.
  /// @{
  std::vector<DataSegment> Content;
  /// @}
};

/// AST DataCountSection node.
class DataCountSection : public Section {
public:
  /// Getter and setter of content.
  std::optional<uint32_t> getContent() const noexcept { return Content; }
  void setContent(uint32_t Val) noexcept { Content = Val; }

private:
  /// \name Data of DataCountSection.
  /// @{
  std::optional<uint32_t> Content = std::nullopt;
  /// @}
};

/// AST TagSection node.
class TagSection : public Section {
public:
  /// Getter of content vector.
  Span<const TagType> getContent() const noexcept { return Content; }
  std::vector<TagType> &getContent() noexcept { return Content; }

private:
  /// \name Data of TagSection.
  /// @{
  std::vector<TagType> Content;
  /// @}
};

class AOTSection {
public:
  /// Getter and setter of version.
  uint32_t getVersion() const noexcept { return Version; }
  void setVersion(uint32_t Ver) noexcept { Version = Ver; }

  /// Getter and setter of OS type.
  uint8_t getOSType() const noexcept { return OSType; }
  void setOSType(uint8_t Type) noexcept { OSType = Type; }

  /// Getter and setter of arch type.
  uint8_t getArchType() const noexcept { return ArchType; }
  void setArchType(uint8_t Type) noexcept { ArchType = Type; }

  /// Getter and setter of version address.
  uint64_t getVersionAddress() const noexcept { return VersionAddress; }
  void setVersionAddress(uint64_t Addr) noexcept { VersionAddress = Addr; }

  /// Getter and setter of intrinsics address.
  uint64_t getIntrinsicsAddress() const noexcept { return IntrinsicsAddress; }
  void setIntrinsicsAddress(uint64_t Addr) noexcept {
    IntrinsicsAddress = Addr;
  }

  /// Getter of type addresses.
  constexpr const auto &getTypesAddress() const noexcept {
    return TypesAddress;
  }
  constexpr auto &getTypesAddress() noexcept { return TypesAddress; }

  /// Getter of code addresses.
  constexpr const auto &getCodesAddress() const noexcept {
    return CodesAddress;
  }
  constexpr auto &getCodesAddress() noexcept { return CodesAddress; }

  /// Getter of sections.
  constexpr const auto &getSections() const noexcept { return Sections; }
  constexpr auto &getSections() noexcept { return Sections; }

private:
  /// \name Data of AOTSection.
  /// @{
  uint32_t Version;
  uint8_t OSType;
  uint8_t ArchType;
  uint64_t VersionAddress;
  uint64_t IntrinsicsAddress;
  std::vector<uintptr_t> TypesAddress;
  std::vector<uintptr_t> CodesAddress;
  std::vector<std::tuple<uint8_t, uint64_t, uint64_t, std::vector<Byte>>>
      Sections;
  std::vector<uint8_t> Bytecodes;
  /// @}
};

namespace Component {

class AliasSection : public Section {
public:
  /// Getter of content module.
  Span<const Alias> getContent() const noexcept { return Content; }
  std::vector<Alias> &getContent() noexcept { return Content; }

private:
  /// \name Data of AliasSection.
  /// @{
  std::vector<Alias> Content;
  /// @}
};

class CoreInstanceSection : public Section {
public:
  /// Getter of content module.
  Span<const CoreInstanceExpr> getContent() const noexcept { return Content; }
  std::vector<CoreInstanceExpr> &getContent() noexcept { return Content; }

private:
  /// \name Data of CoreInstanceSection.
  /// @{
  std::vector<CoreInstanceExpr> Content;
  /// @}
};

class InstanceSection : public Section {
public:
  /// Getter of content module.
  Span<const InstanceExpr> getContent() const noexcept { return Content; }
  std::vector<InstanceExpr> &getContent() noexcept { return Content; }

private:
  /// \name Data of InstanceSection.
  /// @{
  std::vector<InstanceExpr> Content;
  /// @}
};

class CoreTypeSection : public Section {
public:
  /// Getter of content module.
  Span<const CoreDefType> getContent() const noexcept { return Content; }
  std::vector<CoreDefType> &getContent() noexcept { return Content; }

private:
  /// \name Data of CoreTypeSection.
  /// @{
  std::vector<CoreDefType> Content;
  /// @}
};

class TypeSection : public Section {
public:
  /// Getter of content module.
  Span<const DefType> getContent() const noexcept { return Content; }
  std::vector<DefType> &getContent() noexcept { return Content; }

private:
  /// \name Data of TypeSection.
  /// @{
  std::vector<DefType> Content;
  /// @}
};

class CanonSection : public Section {
public:
  /// Getter of content module.
  Span<const Canon> getContent() const noexcept { return Content; }
  std::vector<Canon> &getContent() noexcept { return Content; }

private:
  /// \name Data of CanonicalSection.
  /// @{
  std::vector<Canon> Content;
  /// @}
};

class StartSection : public Section {
public:
  /// Getter of content module.
  const Start &getContent() const noexcept { return Content; }
  Start &getContent() noexcept { return Content; }

private:
  /// \name Data of StartSection.
  /// @{
  Start Content;
  /// @}
};

class ImportSection : public Section {
public:
  /// Getter of content module.
  Span<const Import> getContent() const noexcept { return Content; }
  std::vector<Import> &getContent() noexcept { return Content; }

private:
  /// \name Data of ImportSection.
  /// @{
  std::vector<Import> Content;
  /// @}
};

class ExportSection : public Section {
public:
  /// Getter of content module.
  Span<const Export> getContent() const noexcept { return Content; }
  std::vector<Export> &getContent() noexcept { return Content; }

private:
  /// \name Data of ExportSection.
  /// @{
  std::vector<Export> Content;
  /// @}
};

class Component;

class ComponentSection : public Section {
public:
  /// Getter of content.
  const Component &getContent() const noexcept { return *Content; }
  std::shared_ptr<Component> getContent() noexcept { return Content; }

private:
  std::shared_ptr<Component> Content;
};

} // namespace Component

} // namespace AST
} // namespace WasmEdge
