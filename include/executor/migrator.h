#pragma once

#include "ast/instruction.h"
#include "runtime/instance/module.h"
#include "runtime/instance/function.h"
#include "runtime/stackmgr.h"
#include "runtime/storemgr.h"
#include "executor/executor.h"

#include <map>
#include <iostream>
#include <vector>
#include <string>
#include <cassert>

namespace WasmEdge {
  
namespace Runtime {
  class StackManager;
}

namespace Executor {

class Migrator {
public:
  struct IterData {
    uint32_t FuncIdx;
    uint32_t Offset;
  };
  /// TODO: ModuleInstanceがnullだったときの名前。重複しないようにする
  const std::string NULL_MOD_NAME = "null";
  using IterMigratorType = std::map<AST::InstrView::iterator, IterData>;

  /// ================
  /// Interface
  /// ================
  // const Runtime::Instance::FunctionInstance& restoreFuncInstance(const Runtime::Instance::FunctionInstance& Func) {
  //   Runtime::Instance::FunctionInstance* newFunc = const_cast<Runtime::Instance::FunctionInstance*>(&Func);

  //   const Runtime::Instance::ModuleInstance* restoredModInst = restoreModInst(Func.getModule());
  //   newFunc->setModule(restoredModInst);

  //   const Runtime::Instance::FunctionInstance* constFunc = const_cast<const Runtime::Instance::FunctionInstance*>(newFunc);
  //   return constFunc;
  // } 

  /// Find module by name.
  const Runtime::Instance::ModuleInstance *findModule(std::string_view Name) const {
    auto Iter = NamedMod.find(Name);
    if (likely(Iter != NamedMod.cend())) {
      return Iter->second;
    }
    return nullptr;
  }

  /// ================
  /// Tools
  /// ================
  void setIterMigrator(const Runtime::Instance::ModuleInstance* ModInst) {
    if (IterMigrators.count((std::string)ModInst->getModuleName())) {
      return;
    }

    IterMigratorType IterMigrator;

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

    IterMigrators[(std::string)ModInst->getModuleName()] = IterMigrator;
  }
  
  IterMigratorType getIterMigrator(const Runtime::Instance::ModuleInstance* ModInst) {
    if (IterMigrators.count((std::string)ModInst->getModuleName())) {
      return IterMigrators[(std::string)ModInst->getModuleName()];
    }

    std::string_view ModName = ModInst->getModuleName();
    setIterMigrator(ModInst);
    return IterMigrators[(std::string)ModName];
  }

  IterMigratorType getIterMigratorByName(std::string ModName) {
    if (IterMigrators.count(ModName)) {
      return IterMigrators[ModName];
    } else {
      IterMigratorType I;
      return I;
    }
  }
  
  /// ================
  /// Dump functions
  /// ================
  /// TODO: 関数名を中身にあったものにrenameする
  void preDumpIter(const Runtime::Instance::ModuleInstance* ModInst) {
    setIterMigrator(ModInst);
    BaseModName = ModInst->getModuleName();
  }
  
  void dumpIter(AST::InstrView::iterator Iter, std::string fname_header = "") {
    IterMigratorType IterMigrator = getIterMigratorByName(BaseModName);
    assert(IterMigrator);

    struct IterData Data = IterMigrator[Iter];
    std::ofstream iterStream;
    iterStream.open(fname_header + "iter.img", std::ios::trunc);

    iterStream << Data.FuncIdx << std::endl;
    iterStream << Data.Offset;
      
    iterStream.close();
  }

  void dumpStackMgrFrame(Runtime::StackManager& StackMgr, std::string fname_header = "") {
    std::vector<Runtime::StackManager::Frame> FrameStack = StackMgr.getFrameStack();
    std::ofstream FrameStream;
    FrameStream.open(fname_header + "stackmgr_frame.img", std::ios::trunc);

    std::map<std::string_view, bool> seenModInst;
    for (size_t I = 0; I < FrameStack.size(); ++I) {
      Runtime::StackManager::Frame f = FrameStack[I];

      // ModuleInstance
      const Runtime::Instance::ModuleInstance* ModInst = f.Module;
      std::string_view ModName = NULL_MOD_NAME;

      // ModInstがnullの場合は、ModNameだけ出力して、continue
      if (ModInst == nullptr) {
        FrameStream << NULL_MOD_NAME << std::endl;
        FrameStream << std::endl; 
        continue; 
      }

      ModName = ModInst->getModuleName();
      FrameStream << ModName << std::endl;

      // まだそのModInstを保存してなければ、dumpする
      if(!seenModInst[ModName]) {
        ModInst->dumpMemInst(std::string(ModName));
        ModInst->dumpGlobInst(std::string(ModName));
        seenModInst[ModName] = true;
      }
      
      // Iterator
      IterMigratorType IterMigrator = getIterMigrator(ModInst);
      struct IterData Data = IterMigrator[const_cast<AST::InstrView::iterator>(f.From)];
      FrameStream << Data.FuncIdx << std::endl;
      FrameStream << Data.Offset << std::endl;

      // Locals, VPos, Arity
      FrameStream << f.Locals << std::endl;
      FrameStream << f.VPos << std::endl;
      FrameStream << f.Arity << std::endl;
      FrameStream << std::endl; 
    }  
    
    FrameStream.close();
  }
  
  void dumpStackMgrValue(Runtime::StackManager& StackMgr, std::string fname_header = "") {
    std::ofstream ValueStream;
    ValueStream.open(fname_header + "stackmgr_value.img", std::ios::trunc);

    using Value = ValVariant;
    std::vector<Value> ValueStack = StackMgr.getValueStack();
    for (size_t I = 0; I < ValueStack.size(); ++I) {
      Value v = ValueStack[I];
      ValueStream << v.get<uint128_t>() << std::endl;
    }
    
    ValueStream.close();
  }
  
  /// ================
  /// Restore functions
  /// ================
  AST::InstrView::iterator _restoreIter(const Runtime::Instance::ModuleInstance* ModInst, uint32_t FuncIdx, uint32_t Offset) {
    assert(ModInst != nullptr);
    
    auto Res = ModInst->getFunc(FuncIdx);
    Runtime::Instance::FunctionInstance* FuncInst = Res.value();
    assert(FuncInst != nullptr);

    AST::InstrView::iterator Iter = FuncInst->getInstrs().begin();
    assert(Iter != nullptr);

    for (uint32_t I = 0; I < Offset; ++I) {
      Iter++;
    }
    return Iter;
  }

  AST::InstrView::iterator restoreIter(const Runtime::Instance::ModuleInstance* ModInst) {
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

    std::cout << FuncIdx << " " << Offset << std::endl;
    
    // FuncIdxとOffsetからitertorを復元
    auto Iter = _restoreIter(ModInst, FuncIdx, Offset);
    std::cout << "Success to restore iter" << std::endl;
    return Iter;
  }
  
  std::vector<Runtime::StackManager::Frame> restoreStackMgrFrame() {
    std::ifstream FrameStream;
    FrameStream.open("stackmgr_frame.img");
    Runtime::StackManager StackMgr;

    std::vector<Runtime::StackManager::Frame> FrameStack;
    FrameStack.reserve(16U);
    std::string FrameString;
    /// TODO: ループ条件見直す
    while(getline(FrameStream, FrameString)) {
      // ModuleInstance
      // getline(FrameStream, FrameString);
      std::string ModName = FrameString;
      const Runtime::Instance::ModuleInstance* ModInst;
      std::cout << "restore frame: 1" << std::endl;

      // ModInstがnullの場合
      if (ModName == NULL_MOD_NAME) {
        AST::InstrView::iterator From;
        uint32_t Locals, VPos, Arity;

        Runtime::StackManager::Frame f(ModInst, From, Locals, VPos, Arity);
        FrameStack.push_back(f);

        // 次の行がなければ終了
        if(!getline(FrameStream, FrameString)) {
          break;
        }
        continue;
      }

      // ModInstがnullじゃない場合
      ModInst = findModule(ModName);
      std::cout << "restored ModName is " << ModName << std::endl;
      if (ModInst == nullptr) {
        std::cout << "ModInst is nullptr" << std::endl;
        assert(-1);
      }
      std::cout << "restore frame: 2" << std::endl;

      /// TODO: 同じModuleの復元をしないよう、キャッシュを作る
      if (1) {
        std::cout << ModInst->getMemoryNum() << std::endl;
        ModInst->restoreMemInst(std::string(ModName));
        ModInst->restoreGlobInst(std::string(ModName));
      }
      std::cout << "restore frame: 3" << std::endl;

      // Iterator
      getline(FrameStream, FrameString);
      uint32_t FuncIdx = static_cast<uint32_t>(std::stoul(FrameString));
      getline(FrameStream, FrameString);
      uint32_t Offset = static_cast<uint32_t>(std::stoul(FrameString));
      AST::InstrView::iterator From = _restoreIter(ModInst, FuncIdx, Offset);
      std::cout << "restore frame: 4" << std::endl;

      // Locals, VPos, Arity
      getline(FrameStream, FrameString);
      uint32_t Locals = static_cast<uint32_t>(std::stoul(FrameString));
      getline(FrameStream, FrameString);
      uint32_t VPos = static_cast<uint32_t>(std::stoul(FrameString));
      getline(FrameStream, FrameString);
      uint32_t Arity = static_cast<uint32_t>(std::stoul(FrameString));
      std::cout << "restore frame: 5" << std::endl;

      Runtime::StackManager::Frame f(ModInst, From, Locals, VPos, Arity);
      FrameStack.push_back(f);

      // 空の行を読み捨て
      getline(FrameStream, FrameString);
    }

    FrameStream.close();
    return FrameStack;
  }
  
  std::vector<Runtime::StackManager::Value> restoreStackMgrValue() {	  // Runtime::StackManager restoreStackMgr() {
    std::ifstream ValueStream;	  // }
    ValueStream.open("stackmgr_value.img");	
    Runtime::StackManager StackMgr;	

    std::vector<Runtime::StackManager::Value> ValueStack;	
    ValueStack.reserve(2048U);
    std::string ValueString;	
    /// TODO: ループ条件見直す	
    while(1) {	
      getline(ValueStream, ValueString);	
      // ValueStringが空の場合はエラー	
      assert(ValueString.size() > 0);	

      Runtime::StackManager::Value v = static_cast<uint128_t>(std::stoul(ValueString));	
      ValueStack.push_back(v);	

      if(!getline(ValueStream, ValueString)) {	
        break;	
      }	
    }	

    ValueStream.close();	
    return ValueStack;    	
  }

  Runtime::StackManager restoreStackMgr() {
    std::vector<Runtime::StackManager::Frame> fs = restoreStackMgrFrame();
    std::cout << "Success to restore stack frame" << std::endl;
    std::vector<Runtime::StackManager::Value> vs = restoreStackMgrValue();
    std::cout << "Success to restore stack value" << std::endl;

    Runtime::StackManager StackMgr;
    StackMgr.setFrameStack(fs);
    StackMgr.setValueStack(vs);
    std::cout << "Success to restore stack manager" << std::endl;

    return StackMgr;
  }

  bool DumpFlag; 
  bool RestoreFlag;
private:
  friend class Executor;

  std::map<std::string, IterMigratorType> IterMigrators;
  std::string BaseModName;
  /// \name Module name mapping.
  std::map<std::string, const Runtime::Instance::ModuleInstance *, std::less<>> NamedMod;
};

} // namespace Runtime
} // namespace WasmEdge
