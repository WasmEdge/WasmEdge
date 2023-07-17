#pragma once

#include "ast/instruction.h"
#include "runtime/instance/module.h"
#include "runtime/instance/function.h"

#include <map>
#include <iostream>

namespace WasmEdge {
namespace Executor {

class Migrator {
public:
  // TODO: コンストラクタにする
  void preDumpIter(const Runtime::Instance::ModuleInstance* ModInstA) {
    ModInst = ModInstA;

    for (uint32_t I = 0; I < ModInst->getFuncNum(); ++I) {
      Runtime::Instance::FunctionInstance* FuncInst = ModInst->getFunc(I).value();
      AST::InstrView Instr = FuncInst->getInstrs();
      AST::InstrView::iterator PC = Instr.begin();
      AST::InstrView::iterator PCEnd = Instr.end();
      
      uint32_t Offset = 0;
      while (PC != PCEnd) {
        IterMigrator[PC] = {I, Offset};
        Offset++;
        PC++;
      }
    }
    return;
  }
  
  void dumpIter(AST::InstrView::iterator Iter) {
    struct IterData Data = IterMigrator[Iter];
    std::ofstream iterStream;
    iterStream.open("iter.img", std::ios::trunc);

    iterStream << Data.FuncIdx << std::endl;
    iterStream << Data.Offset;
      
    iterStream.close();
  }

  AST::InstrView::iterator restoreIter() {
    std::ifstream iterStream;
    iterStream.open("iter.img");
    
    std::string iterString;
    // FuncIdx
    getline(iterStream, iterString);
    uint32_t FuncIdx = static_cast<uint32_t>(std::stoul(iterString));
    // Offset
    getline(iterStream, iterString);
    uint32_t Offset = static_cast<uint32_t>(std::stoul(iterString));

    iterStream.close();
    
    // FuncIdxとOffsetからitertorを復元
    Runtime::Instance::FunctionInstance* FuncInst = ModInst->getFunc(FuncIdx).value();
    AST::InstrView::iterator Iter = FuncInst->getInstrs().begin();
    for (uint32_t I = 0; I < Offset; ++I) {
      Iter++;
    }

    return Iter;
  }
  
  void dumpMemInst() {
    ModInst->dumpMemInst();
  }

  void dumpGlobInst() {
    ModInst->dumpGlobInst();
  }


private:
  struct IterData {
    uint32_t FuncIdx;
    uint32_t Offset;
  };
  std::map<AST::InstrView::iterator, IterData> IterMigrator;
  const Runtime::Instance::ModuleInstance* ModInst;
};

} // namespace Runtime
} // namespace WasmEdge