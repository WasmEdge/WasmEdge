#include "validator/validator.h"

#include "ast/module.h"
#include "vm/common.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cassert>

using std::cout;
using std::endl;
using std::printf;
using std::puts;

namespace SSVM {
namespace Validator {

void ValidatMachine::reset(bool CleanGlobal)
{
  ValStack.clear();
  CtrlStack.clear();
  local.clear();

  if (CleanGlobal)
    global.clear();
}

static ValType Ast2ValType(AST::ValType v)
{
  ValType res;
  switch(v)
  {
    case AST::ValType::I32: res = ValType::I32; break;
    case AST::ValType::I64: res = ValType::I64; break;
    case AST::ValType::F32: res = ValType::F32; break;
    case AST::ValType::F64: res = ValType::F64; break;
    default:
      //TODO throw expect
      throw;
  }
  return res;
}

void ValidatMachine::addloacl(unsigned int idx, AST::ValType v)
{
  local[idx] = Ast2ValType(v);
}

ValType ValidatMachine::getlocal(unsigned int idx)
{
  if (local.count(idx) == 0)
    throw;
  return local[idx];
}

void ValidatMachine::setlocal(unsigned int idx, ValType v)
{
  if (local.count(idx) == 0)
    throw;
  local[idx] = v;
}

void ValidatMachine::addglobal(unsigned int idx, AST::GlobalType v)
{
  // Check type
  (void) Ast2ValType(v.getValueType());
  global[idx] = v;
}

ValType ValidatMachine::getglobal(unsigned int idx)
{
  if (global.count(idx) == 0)
    throw;
  return Ast2ValType(global[idx].getValueType());
}

void ValidatMachine::setglobal(unsigned int idx, ValType v)
{
  if (global.count(idx) == 0)
    throw;
  if (global[idx].getValueMutation() != AST::ValMut::Var)
    throw;
  if (Ast2ValType(global[idx].getValueType()) != v)
    throw;
}

void ValidatMachine::push_opd(ValType v)
{
  ValStack.emplace_front(v);
}

ValType ValidatMachine::pop_opd()
{
  //if (opds.size() = ctrls[0].height && ctrls[0].unreachable) return Unknown
  //error_if(opds.size() = ctrls[0].height)

  auto res = ValStack.front();
  ValStack.pop_front();
  return res;
}

ValType ValidatMachine::pop_opd(ValType expect)
{
  auto res = pop_opd();
  if( res == ValType::Unknown ) return expect;
  if( expect == ValType::Unknown ) return res;
  if( res != expect ) {
    cout << "Stack Val Not Match.." << endl;
    throw;
  }
  return res;
}

ErrCode ValidatMachine::validate(AST::InstrVec &insts)
{
  cout << insts.size() << " insts.." << endl;
  try {
    for(auto &op:insts)
      runop(op.get());
  } catch (...) {
    return ErrCode::Invalid;
  }
  return ErrCode::Success;
}

void ValidatMachine::runop(AST::Instruction *instr)
{
  auto stack_trans = [&](const std::vector<ValType> &take, const std::vector<ValType> &put)->void{
    auto takecp = take;
    std::reverse(takecp.begin(), takecp.end());
    for(auto val:takecp)
      pop_opd(val);
    for(auto val:put)
      push_opd(val);
  };

  auto opcode = instr->getOpCode();
  AST::VariableInstruction* VarInstr = dynamic_cast<AST::VariableInstruction *>(instr);
  switch(opcode)
  {
    /// 0x00
    case OpCode::Unreachable:
      //TODO:
    case OpCode::Nop:
      break;
    
    /// 0x10

    case OpCode::Drop:
      stack_trans({ValType::Unknown},{});
      break;
    case OpCode::Select:
    {
      pop_opd(ValType::I32);
      auto t1 = pop_opd();
      auto t2 = pop_opd(t1);
      push_opd(t2);
      break;
    }

    /// 0x20
    case OpCode::Local__get:
      stack_trans({},{getlocal(VarInstr->getIndex())});
      break;
    case OpCode::Local__set:
    {
      auto t = pop_opd();
      setlocal(VarInstr->getIndex(), t);
      break;
    }
    case OpCode::Local__tee:
    {
      auto t = pop_opd();
      setlocal(VarInstr->getIndex(), t);
      push_opd(t);
      break;
    }
    case OpCode::Global__get:
      stack_trans({},{getglobal(VarInstr->getIndex())});
    case OpCode::Global__set:
    {
      auto t = pop_opd();
      setglobal(VarInstr->getIndex(), t);
    }
      
    
    /// 0x30
    ///TODO: check store: https://webassembly.github.io/spec/core/appendix/properties.html
    case OpCode::I32__store:
      stack_trans({ValType::I32, ValType::I32},{});
      break;
    case OpCode::I64__store:
      stack_trans({ValType::I32, ValType::I64},{});
      break;
    case OpCode::F32__store:
      stack_trans({ValType::I32, ValType::F32},{});
      break;
    case OpCode::F64__store:
      stack_trans({ValType::I32, ValType::F64},{});
      break;
    case OpCode::I32__store8:
    case OpCode::I32__store16:
      stack_trans({ValType::I32, ValType::I32},{});
      break;
    case OpCode::I64__store8:
    case OpCode::I64__store16:
    case OpCode::I64__store32:
      stack_trans({ValType::I32, ValType::I64},{});
      break;
    case OpCode::Memory__size:
      ///TODO: check memory[0]
      stack_trans({},{ValType::I32});
      break;
    /// 0x40
      case OpCode::Memory__grow:
      ///TODO: check memory[0]
      stack_trans({ValType::I32},{ValType::I32});
      break;
    case OpCode::I32__const:
      stack_trans({},{ValType::I32});
      break;
    case OpCode::I64__const:
      stack_trans({},{ValType::I64});
      break;
    case OpCode::F32__const:
      stack_trans({},{ValType::F32});
      break;
    case OpCode::F64__const:
      stack_trans({},{ValType::F64});
      break;

    /// 0x50

    /// 0x60

    case OpCode::I32__add:
    case OpCode::I32__sub:
    case OpCode::I32__mul:
    case OpCode::I32__div_s:
    case OpCode::I32__div_u:
    case OpCode::I32__rem_s:
      stack_trans({ValType::I32, ValType::I32},{ValType::I32});
      break;
    /// 0x70
    case OpCode::I32__rem_u:
    case OpCode::I32__and:
    case OpCode::I32__or:
    case OpCode::I32__xor:
    case OpCode::I32__shl:
    case OpCode::I32__shr_s:
    case OpCode::I32__shr_u:
    case OpCode::I32__rotl:
    case OpCode::I32__rotr:
      stack_trans({ValType::I32, ValType::I32},{ValType::I32});
      break;
    case OpCode::I64__clz:
    case OpCode::I64__ctz:
    case OpCode::I64__popcnt:
      stack_trans({ValType::I64},{ValType::I64});
      break;
    case OpCode::I64__add:
    case OpCode::I64__sub:
    case OpCode::I64__mul:
    case OpCode::I64__div_s:
      stack_trans({ValType::I64, ValType::I64},{ValType::I64});
      break;
    /// 0x80
    case OpCode::I64__div_u:
    case OpCode::I64__rem_s:
    case OpCode::I64__rem_u:
    case OpCode::I64__and:
    case OpCode::I64__or:
    case OpCode::I64__xor:
    case OpCode::I64__shl:
    case OpCode::I64__shr_s:
    case OpCode::I64__shr_u:
    case OpCode::I64__rotl:
    case OpCode::I64__rotr:
      stack_trans({ValType::I64, ValType::I64},{ValType::I64});
      break;
    case OpCode::F32__abs:
    case OpCode::F32__neg:
    case OpCode::F32__ceil:
    case OpCode::F32__floor:
    case OpCode::F32__trunc:
      stack_trans({ValType::F32},{ValType::F32});
      break;
    /// 0x90
    case OpCode::F32__nearest:
    case OpCode::F32__sqrt:
      stack_trans({ValType::F32},{ValType::F32});
      break;
    case OpCode::F32__add:
    case OpCode::F32__sub:
    case OpCode::F32__mul:
    case OpCode::F32__div:
    case OpCode::F32__min:
    case OpCode::F32__max:
    case OpCode::F32__copysign:
      stack_trans({ValType::F32, ValType::F32},{ValType::F32});
      break;
    case OpCode::F64__abs:
    case OpCode::F64__neg:
    case OpCode::F64__ceil:
    case OpCode::F64__floor:
    case OpCode::F64__trunc:
    case OpCode::F64__nearest:
    case OpCode::F64__sqrt:
      stack_trans({ValType::F64},{ValType::F64});
      break;
    /// 0xA0
    case OpCode::F64__add:
    case OpCode::F64__sub:
    case OpCode::F64__mul:
    case OpCode::F64__div:
    case OpCode::F64__min:
    case OpCode::F64__max:
    case OpCode::F64__copysign:
      stack_trans({ValType::F64, ValType::F64},{ValType::F64});
      break;
    case OpCode::I32__wrap_i64:
      stack_trans({ValType::I64},{ValType::I32});
      break;
    case OpCode::I32__trunc_f32_s:
    case OpCode::I32__trunc_f32_u:
      stack_trans({ValType::F32},{ValType::I32});
      break;
    case OpCode::I32__trunc_f64_s:
    case OpCode::I32__trunc_f64_u:
      stack_trans({ValType::F64},{ValType::I32});
      break;
    case OpCode::I64__extend_i32_s:
    case OpCode::I64__extend_i32_u:
      stack_trans({ValType::I32},{ValType::I64});
      break;
    case OpCode::I64__trunc_f32_s:
    case OpCode::I64__trunc_f32_u:
      stack_trans({ValType::F32},{ValType::I64});
      break;
    /// 0XB0
    case OpCode::I64__trunc_f64_s:
    case OpCode::I64__trunc_f64_u:
      stack_trans({ValType::F64},{ValType::I64});
    case OpCode::F32__convert_i32_s:
    case OpCode::F32__convert_i32_u:
      stack_trans({ValType::I32},{ValType::F32});
      break;
    case OpCode::F32__convert_i64_s:
    case OpCode::F32__convert_i64_u:
      stack_trans({ValType::I64},{ValType::F32});
      break;
    case OpCode::F32__demote_f64:
      stack_trans({ValType::F64},{ValType::F32});
      break;
    case OpCode::F64__convert_i32_s:
    case OpCode::F64__convert_i32_u:
      stack_trans({ValType::I32},{ValType::F64});
      break;
    case OpCode::F64__convert_i64_s:
    case OpCode::F64__convert_i64_u:
      stack_trans({ValType::I64},{ValType::F64});
      break;
    case OpCode::F64__promote_f32:
      stack_trans({ValType::F32},{ValType::F64});
      break;
    case OpCode::I32__reinterpret_f32:
      stack_trans({ValType::F32},{ValType::I32});
      break;
    case OpCode::I64__reinterpret_f64:
      stack_trans({ValType::F64},{ValType::I64});
      break;
    case OpCode::F32__reinterpret_i32:
      stack_trans({ValType::I32},{ValType::F32});
      break;
    case OpCode::F64__reinterpret_i64:
      stack_trans({ValType::I64},{ValType::F64});
      break;
    
    default:
      cout<<"unimp opcode:0x"<<std::hex<<(uint)opcode<<std::dec<<endl;
      throw;

  }
}

ErrCode Validator::validate(const AST::Limit *limit, unsigned int K)
{
  bool cond1 = limit->getMin() <= K;
  bool cond2 = true;

  if( limit->hasMax() )
  {
    cond2 = limit->getMax() <= K && limit->getMin() <= limit->getMax();
  }

  if( cond1 && cond2 )
    return ErrCode::Success;
  return ErrCode::Invalid;
}

ErrCode Validator::validate(AST::FunctionType *Func)
{
  /// The restriction to at most one result may be removed in future versions of WebAssembly.
  if (Func->getReturnTypes().size() > 1)
    return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::TableType *Tab)
{
  return validate(Tab->getLimit(), LIMIT_TABLETYPE);
}

ErrCode Validator::validate(AST::MemoryType *Mem)
{
  return validate(Mem->getLimit(), LIMIT_MEMORYTYPE);
}

ErrCode Validator::validate(AST::GlobalType *)
{
  /// The global type is valid any way
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::FunctionSection *FuncSec, AST::CodeSection *CodeSec, AST::TypeSection *TypeSec)
{
  if (!FuncSec) return ErrCode::Success;

  if (FuncSec->getContent().size() != CodeSec->getContent().size())
    return ErrCode::Invalid;

  size_t TotoalFunctions = FuncSec->getContent().size();

  for (size_t id = 0 ; id < TotoalFunctions ; ++id )
  {
    auto tid = FuncSec->getContent().at(id);
    
    if( tid >= TypeSec->getContent().size() )
      return ErrCode::Invalid;
    
    validate(CodeSec->getContent().at(id).get(), TypeSec->getContent().at(tid).get());
  }
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::CodeSegment *CodeSeg, AST::FunctionType *Func)
{
  vm.reset();
  int idx = 0;

  for (auto val:Func->getParamTypes())
  {
    vm.addloacl(idx++, val);
  }

  for (auto val:CodeSeg->getLocals())
  {
    for (unsigned int cnt=0 ; cnt<val.first ; ++cnt)
      vm.addloacl(idx++, val.second);
  }
  cout << "locals= " << idx << endl;
  return vm.validate(CodeSeg->getInstrs());
}

ErrCode Validator::validate(AST::MemorySection *MemSec)
{
  if (!MemSec) return ErrCode::Invalid; /// Why?

  for (auto &mem:MemSec->getContent())
    if (validate(mem.get()) != ErrCode::Success)
      return ErrCode::Invalid;

  return ErrCode::Success;
}

ErrCode Validator::validate(AST::GlobalSection *GloSec)
{
  if (!GloSec) return ErrCode::Success;

  int idx = 0;
  for (auto &val:GloSec->getContent())
    if (validate(val.get()) != ErrCode::Success)
    {
      return ErrCode::Invalid;
    }
    else
    {
      vm.addglobal(idx++, *val.get()->getGlobalType());
    }
    
  cout << "globals=" << GloSec->getContent().size() << endl;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::GlobalSegment *)
{
  // TODO: Check GloSeg->getInstrs(); is a const expr
  return ErrCode::Success;
}


ErrCode Validator::validate(AST::ElementSegment *)
{cout<<"unimp ElementSegment";
  assert( false && "unimp ElementSegment" );
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ElementSection *EleSec)
{
  if (!EleSec) return ErrCode::Success;

  for( auto &element:EleSec->getContent())
    if (validate(element.get()) != ErrCode::Success)
      return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::StartSection *StartSec)
{
  if (!StartSec) return ErrCode::Success;
cout<<"unimp StartSection";
  assert( false && "unimp StartSection" );
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ExportSection *ExportSec)
{
  if (!ExportSec) return ErrCode::Success;

  for( auto &exportedc:ExportSec->getContent())
    if (validate(exportedc.get()) != ErrCode::Success)
      return ErrCode::Invalid;
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ExportDesc *ExportDesc)
{
  ///TODO: Ast not provide where the data is.
  ///      It need to enumerate func/table/mem/global to check the name are defined.
  return ErrCode::Success;
}

ErrCode Validator::validate(AST::ImportSection *ImportSec)
{
    if (!ImportSec) return ErrCode::Success;
cout<<"unimp ImportSection";
  assert( false && "unimp ImportSection" );
  return ErrCode::Success;
}

ErrCode Validator::validate(std::unique_ptr<AST::Module> &Mod)
{
  reset();

  // https://webassembly.github.io/spec/core/valid/modules.html
  if (validate((*Mod).getMemorySection()) != ErrCode::Success)
    return ErrCode::Invalid;
  
  if (validate((*Mod).getGlobalSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getElementSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getStartSection()) != ErrCode::Success)
    return ErrCode::Invalid;
  
  if (validate((*Mod).getExportSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getImportSection()) != ErrCode::Success)
    return ErrCode::Invalid;

  if (validate((*Mod).getFunctionSection(), (*Mod).getCodeSection(), (*Mod).getTypeSection()) != ErrCode::Success)
    return ErrCode::Invalid;
  
  cout<<"Validator OK"<<endl;
  cout<<std::flush;
  return ErrCode::Success;
}

void Validator::reset()
{
  vm.reset(true);
}

} // namespace Loader
} // namespace SSVM