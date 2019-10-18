//===-- ssvm/test/loader/ethereumTest.cpp - Ethereum related wasm tests ---===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading WASM about Ethereum.
///
//===----------------------------------------------------------------------===//

#include "vm/configure.h"
#include "vm/environment.h"
#include "vm/result.h"
#include "vm/vm.h"
#include "gtest/gtest.h"

#include <iostream>

namespace {

const std::string Erc20Path("ethereumTestData/erc20.wasm");
SSVM::VM::Configure Conf(SSVM::VM::Configure::VMType::EWasm);
SSVM::VM::VM EVM(Conf);
SSVM::VM::EVMEnvironment *Env = nullptr;

TEST(ERC20Test, Run__mint) {
  EXPECT_EQ(EVM.getEnvironment(Env), SSVM::VM::ErrCode::Success);
  Env->clear();
  std::string &Caller = Env->getCaller();
  Caller = "1234567890123456789012345678901234567890";
  std::string &CallValue = Env->getCallValue();
  CallValue = "ffffffffffffffffffffffffffffffff";
  std::vector<unsigned char> &CallData = Env->getCallData();
  CallData = {78, 110, 194, 71, 0,  0,   0,   0,   0,  0,  0,   0,   0,  0,
              0,  0,   18,  52, 86, 120, 144, 18,  52, 86, 120, 144, 18, 52,
              86, 120, 144, 18, 52, 86,  120, 144, 0,  0,  0,   0,   0,  0,
              0,  0,   0,   0,  0,  0,   0,   0,   0,  0,  0,   0,   0,  0,
              0,  0,   0,   0,  0,  0,   0,   0,   0,  0,  0,   100};
  EVM.setPath(Erc20Path);
  EXPECT_EQ(EVM.execute(), SSVM::VM::ErrCode::Success);

  std::map<std::string, std::string> &FinalStorage = Env->getStorage();
  std::cout << "    --- result storage: " << std::endl;
  for (auto it = FinalStorage.begin(); it != FinalStorage.end(); ++it) {
    std::cout << "         " << it->first << " " << it->second << std::endl;
  }

  std::vector<unsigned char> &FinalReturn = Env->getReturnData();
  std::cout << "    --- return data: " << std::endl << "         ";
  for (auto it = FinalReturn.begin(); it != FinalReturn.end(); ++it) {
    printf("%u ", *it);
  }
  std::cout << std::endl;
}

TEST(ERC20Test, Run__transfer) {
  Env->clear();
  EXPECT_EQ(EVM.getEnvironment(Env), SSVM::VM::ErrCode::Success);
  std::string &Caller = Env->getCaller();
  Caller = "1234567890123456789012345678901234567890";
  std::string &CallValue = Env->getCallValue();
  CallValue = "ffffffffffffffffffffffffffffffff";
  std::vector<unsigned char> &CallData = Env->getCallData();
  CallData = {169, 5, 156, 187, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
              0,   0, 0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
              0,   1, 0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
              0,   0, 0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10};
  std::map<std::string, std::string> &Storage = Env->getStorage();
  Storage["0"] = "64";
  Storage["f5b24dcea0e9381721a8c72784d30cfe64c11b4591226269f839d095b3e9cf10"] =
      "64";
  EVM.setPath(Erc20Path);
  EXPECT_EQ(EVM.execute(), SSVM::VM::ErrCode::Success);

  std::map<std::string, std::string> &FinalStorage = Env->getStorage();
  std::cout << "    --- result storage: " << std::endl;
  for (auto it = FinalStorage.begin(); it != FinalStorage.end(); ++it) {
    std::cout << "         " << it->first << " " << it->second << std::endl;
  }

  std::vector<unsigned char> &FinalReturn = Env->getReturnData();
  std::cout << "    --- return data: " << std::endl << "         ";
  for (auto it = FinalReturn.begin(); it != FinalReturn.end(); ++it) {
    printf("%u ", *it);
  }
  std::cout << std::endl;
}

TEST(ERC20Test, Run__balanceOf) {
  Env->clear();
  EXPECT_EQ(EVM.getEnvironment(Env), SSVM::VM::ErrCode::Success);
  std::string &Caller = Env->getCaller();
  Caller = "1234567890123456789012345678901234567890";
  std::string &CallValue = Env->getCallValue();
  CallValue = "ffffffffffffffffffffffffffffffff";
  std::vector<unsigned char> &CallData = Env->getCallData();
  CallData = {112, 160, 130, 49, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
              0,   0,   0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  std::map<std::string, std::string> &Storage = Env->getStorage();
  Storage["0"] = "64";
  Storage["f5b24dcea0e9381721a8c72784d30cfe64c11b4591226269f839d095b3e9cf10"] =
      "5a";
  Storage["3cbd4ff31ab6027f35d2d04a81e2957deb3a24998b9cea0327c6e16d0b547a1d"] =
      "a";
  EVM.setPath(Erc20Path);
  EXPECT_EQ(EVM.execute(), SSVM::VM::ErrCode::Success);

  std::map<std::string, std::string> &FinalStorage = Env->getStorage();
  std::cout << "    --- result storage: " << std::endl;
  for (auto it = FinalStorage.begin(); it != FinalStorage.end(); ++it) {
    std::cout << "         " << it->first << " " << it->second << std::endl;
  }

  std::vector<unsigned char> &FinalReturn = Env->getReturnData();
  std::cout << "    --- return data: " << std::endl << "         ";
  for (auto it = FinalReturn.begin(); it != FinalReturn.end(); ++it) {
    printf("%u ", *it);
  }
  std::cout << std::endl;
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}