#pragma once

#include "filemgr.h"
#include "type.h"
#include <string>
#include <variant>

namespace AST {

class Desc : public Base {
public:
  enum class ExternalType : char {
    Function = 0x00,
    Table = 0x01,
    Memory = 0x02,
    Global = 0x03
  };

protected:
  ExternalType ExtType;
};

class ImportDesc : public Desc {
public:
  virtual bool loadBinary(FileMgr &Mgr);
  using ExtContentType =
      std::variant<std::unique_ptr<unsigned int>, std::unique_ptr<TableType>,
                   std::unique_ptr<MemoryType>, std::unique_ptr<GlobalType>>;

protected:
  Attr NodeAttr = Attr::Desc_Import;

private:
  std::string ModName;
  std::string ExtName;
  ExtContentType ExtContent;
};

class ExportDesc : public Desc {
public:
  virtual bool loadBinary(FileMgr &Mgr);

protected:
  Attr NodeAttr = Attr::Desc_Export;

private:
  std::string ExtName;
  unsigned int ExtIdx;
};

} // namespace AST
