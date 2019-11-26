//===-- ssvm/validator/validator.h - Loader flow control class definition
//-------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the validator class, which controls
/// flow of WASM loading.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include <deque>
#include <map>
#include <string>

namespace SSVM {
namespace Validator {

namespace {
using OpCode = AST::Instruction::OpCode;
} // namespace

enum class ErrCode : unsigned int { Success = 0, Invalid };

enum class ValType : unsigned int { Unknown, I32, I64, F32, F64 };

struct CtrlFrame {
  std::vector<ValType> label_types;
  std::vector<ValType> end_types;
  size_t height;
  bool unreachable;
};

class ValidatMachine {
  void runop(AST::Instruction *);
  void push_opd(ValType);
  ValType pop_opd();
  ValType pop_opd(ValType);
  void pop_opds(const std::vector<ValType> &);
  void push_opds(const std::vector<ValType> &);
  void push_ctrl(const std::vector<ValType> &, const std::vector<ValType> &);
  std::vector<ValType> pop_ctrl();
  void unreachable();

  ValType getlocal(unsigned int);
  void setlocal(unsigned int, ValType);
  ValType getglobal(unsigned int);
  void setglobal(unsigned int, ValType);
  ErrCode validateWarp(const AST::InstrVec *);

public:
  void addloacl(unsigned int, AST::ValType);
  void addglobal(unsigned int, AST::GlobalType);
  void addfunc(AST::FunctionType *);
  void reset(bool CleanGlobal = false);
  void init();
  ErrCode validate(const AST::InstrVec &);
  std::deque<ValType> result() { return ValStack; };

private:
  std::map<unsigned int, ValType> local;
  std::deque<ValType> ValStack;
  std::deque<CtrlFrame> CtrlStack;

  std::map<unsigned int, AST::GlobalType> global;
  std::vector<std::pair<std::vector<ValType>, std::vector<ValType>>> funcs;

  static const size_t NAT = -1;
};

/// Loader flow control class.
class Validator {
  /// Sec. valid types
  ErrCode validate(const AST::Limit *, unsigned int);
  ErrCode validate(AST::FunctionType *);
  ErrCode validate(AST::TableType *);
  ErrCode validate(AST::MemoryType *);
  ErrCode validate(AST::GlobalType *);

  /// Sec. Instructions types

  /// Sec. Instructions types
  ErrCode validate(AST::FunctionSection *, AST::CodeSection *,
                   AST::TypeSection *);
  ErrCode validate(AST::CodeSegment *, AST::FunctionType *);
  ErrCode validate(AST::MemorySection *);
  ErrCode validate(AST::GlobalSection *);
  ErrCode validate(AST::GlobalSegment *);
  ErrCode validate(AST::ElementSection *);
  ErrCode validate(AST::ElementSegment *);
  ErrCode validate(AST::StartSection *);
  ErrCode validate(AST::ExportSection *);
  ErrCode validate(AST::ExportDesc *);
  ErrCode validate(AST::ImportSection *, AST::TypeSection *);
  ErrCode validate(AST::ImportDesc *, AST::TypeSection *);

  /// Validator Stack Operation
public:
  Validator() = default;
  ~Validator() = default;

  /// Validate AST::Module.
  ErrCode validate(std::unique_ptr<AST::Module> &);
  void reset();

private:
  ValidatMachine vm;

  static const unsigned int LIMIT_TABLETYPE = 4294967295; // 2^32-1
  static const unsigned int LIMIT_MEMORYTYPE = 1U << 16;
};

} // namespace Validator
} // namespace SSVM