// SPDX-License-Identifier: Apache-2.0
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

#include "support/hexstr.h"
#include "vm/configure.h"
#include "vm/environment.h"
#include "vm/result.h"
#include "vm/vm.h"
#include "gtest/gtest.h"

#include <iostream>

namespace {

/// Helper function to check return data string
bool isReturnExpected(const std::string &ExpStr,
                      const std::vector<uint8_t> &Data) {
  std::string DataStr = "";
  SSVM::Support::convertBytesToHexStr(Data, DataStr);
  return DataStr == ExpStr;
}

/// Helper function to load file to vector
void readBinary(const std::string &Path, std::vector<uint8_t> &Dst) {
  std::ifstream F(Path, std::ios::binary);
  F.unsetf(std::ios::skipws);
  std::streampos FSize;
  F.seekg(0, std::ios::end);
  FSize = F.tellg();
  F.seekg(0, std::ios::beg);
  Dst.clear();
  Dst.reserve(FSize);
  Dst.insert(Dst.begin(), std::istream_iterator<uint8_t>(F),
             std::istream_iterator<uint8_t>());
}

/// VMUnit class for google test global variable.
struct VMUnit {
  std::string Erc20Path = "ethereumTestData/erc20.wasm";
  std::string Erc20DeployPath = "ethereumTestData/erc20.deploy.wasm";
  std::vector<uint8_t> Erc20Bin;
  std::vector<uint8_t> Erc20DeployBin;
  SSVM::VM::Configure Conf;
  std::unique_ptr<SSVM::VM::VM> VM;
  SSVM::VM::EVMEnvironment *Env = nullptr;

  VMUnit() {
    Conf.addVMType(SSVM::VM::Configure::VMType::Ewasm);
    VM = std::make_unique<SSVM::VM::VM>(Conf);
    Env = VM->getEnvironment<SSVM::VM::EVMEnvironment>(
        SSVM::VM::Configure::VMType::Ewasm);
    VM->setCostLimit(UINT64_MAX);
    Env->getCallValue() = "00000000000000000000000000000000";
    readBinary(Erc20Path, Erc20Bin);
    readBinary(Erc20DeployPath, Erc20DeployBin);
  }

  void printResult() {
    std::cout << " Storage: {" << std::endl;
    for (auto &it : Env->getStorage()) {
      std::cout << "   " << it.first << " : " << it.second << std::endl;
    }
    std::cout << "          }" << std::endl;
    std::string DataStr;
    SSVM::Support::convertBytesToHexStr(Env->getReturnData(), DataStr, 64);
    std::cout << " Return Data: " << DataStr << std::endl;
  }
};

/// Testing setup class
class ERC20Test : public ::testing::Test {
protected:
  static void SetUpTestSuite() { EVM = new VMUnit; }

  static void TearDownTestSuite() {
    delete EVM;
    EVM = nullptr;
  }

  static VMUnit *EVM;
};

VMUnit *ERC20Test::EVM = nullptr;

TEST_F(ERC20Test, Run__1_deploy) {
  /// Set caller and data.
  EVM->Env->getCaller() = "7fffffff";
  EVM->Env->getCallData() = EVM->Erc20DeployBin;

  /// Execute.
  EVM->VM->setPath(EVM->Erc20DeployPath);
  EXPECT_EQ(EVM->VM->execute("main"), SSVM::VM::ErrCode::Success);

  /// Check return data.
  std::vector<unsigned char> &RetData = EVM->Env->getReturnData();
  std::string ExpStr = "";
  SSVM::Support::convertBytesToHexStr(EVM->Erc20Bin, ExpStr);
  EXPECT_TRUE(isReturnExpected(ExpStr, EVM->Env->getReturnData()));

  /// Check result.
  EVM->printResult();
}

TEST_F(ERC20Test, Run__2_check_balance_of_0x7FFFFFFF) {
  /// Set call data
  std::string CallDataStr = "70a08231000000000000000000000000000000000000000000"
                            "000000000000007fffffff";
  SSVM::Support::convertHexStrToBytes(CallDataStr, EVM->Env->getCallData());

  /// Execute.
  EVM->VM->setPath(EVM->Erc20Path);
  EXPECT_EQ(EVM->VM->execute("main"), SSVM::VM::ErrCode::Success);

  /// Check return data.
  std::string ExpStr =
      "00000000000000000000000000000000000000000000000000000000000003e8";
  EXPECT_TRUE(isReturnExpected(ExpStr, EVM->Env->getReturnData()));

  /// Check result.
  EVM->printResult();
}

TEST_F(ERC20Test, Run__3_check_total_supply) {
  /// Set call data
  std::string CallDataStr = "18160ddd";
  SSVM::Support::convertHexStrToBytes(CallDataStr, EVM->Env->getCallData());

  /// Execute.
  EVM->VM->setPath(EVM->Erc20Path);
  EXPECT_EQ(EVM->VM->execute("main"), SSVM::VM::ErrCode::Success);

  /// Check return data.
  std::string ExpStr =
      "00000000000000000000000000000000000000000000000000000000000003e8";
  EXPECT_TRUE(isReturnExpected(ExpStr, EVM->Env->getReturnData()));
  ;

  /// Check result.
  EVM->printResult();
}

TEST_F(ERC20Test, Run__4_transfer_20_from_0x7FFFFFFF_to_0x01) {
  /// Set caller and data.
  EVM->Env->getCaller() = "7fffffff";
  std::string CallDataStr =
      "a9059cbb0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000014";
  SSVM::Support::convertHexStrToBytes(CallDataStr, EVM->Env->getCallData());

  /// Execute.
  EVM->VM->setPath(EVM->Erc20Path);
  EXPECT_EQ(EVM->VM->execute("main"), SSVM::VM::ErrCode::Success);

  /// Check return data.
  std::string ExpStr =
      "0000000000000000000000000000000000000000000000000000000000000001";
  EXPECT_TRUE(isReturnExpected(ExpStr, EVM->Env->getReturnData()));

  /// Check result.
  EVM->printResult();
}

TEST_F(ERC20Test, Run__5_check_balance_of_0x01) {
  /// Set call data
  std::string CallDataStr = "70a08231000000000000000000000000000000000000000000"
                            "0000000000000000000001";
  SSVM::Support::convertHexStrToBytes(CallDataStr, EVM->Env->getCallData());

  /// Execute.
  EVM->VM->setPath(EVM->Erc20Path);
  EXPECT_EQ(EVM->VM->execute("main"), SSVM::VM::ErrCode::Success);

  /// Check return data.
  std::string ExpStr =
      "0000000000000000000000000000000000000000000000000000000000000014";
  EXPECT_TRUE(isReturnExpected(ExpStr, EVM->Env->getReturnData()));

  /// Check result.
  EVM->printResult();
}

TEST_F(ERC20Test, Run__6_approve_10_from_0x7FFFFFFF_for_0x01_to_spend) {
  /// Set caller and data.
  EVM->Env->getCaller() = "7fffffff";
  std::string CallDataStr =
      "095ea7b30000000000000000000000000000000000000000000000000000000000000001"
      "000000000000000000000000000000000000000000000000000000000000000a";
  SSVM::Support::convertHexStrToBytes(CallDataStr, EVM->Env->getCallData());

  /// Execute.
  EVM->VM->setPath(EVM->Erc20Path);
  EXPECT_EQ(EVM->VM->execute("main"), SSVM::VM::ErrCode::Success);

  /// Check return data.
  std::string ExpStr =
      "0000000000000000000000000000000000000000000000000000000000000001";
  EXPECT_TRUE(isReturnExpected(ExpStr, EVM->Env->getReturnData()));

  /// Check result.
  EVM->printResult();
}

TEST_F(ERC20Test, Run__7_check_allowance_from_0x7FFFFFFF_by_0x01) {
  /// Set call data
  std::string CallDataStr =
      "dd62ed3e000000000000000000000000000000000000000000000000000000007fffffff"
      "0000000000000000000000000000000000000000000000000000000000000001";
  SSVM::Support::convertHexStrToBytes(CallDataStr, EVM->Env->getCallData());

  /// Execute.
  EVM->VM->setPath(EVM->Erc20Path);
  EXPECT_EQ(EVM->VM->execute("main"), SSVM::VM::ErrCode::Success);

  /// Check return data.
  std::string ExpStr =
      "000000000000000000000000000000000000000000000000000000000000000a";
  EXPECT_TRUE(isReturnExpected(ExpStr, EVM->Env->getReturnData()));

  /// Check result.
  EVM->printResult();
}

TEST_F(ERC20Test, Run__8_transfer_3_from_0x7FFFFFFF_by_0x01_to_0x02) {
  /// Set caller and data.
  EVM->Env->getCaller() = "01";
  std::string CallDataStr =
      "23b872dd000000000000000000000000000000000000000000000000000000007fffffff"
      "000000000000000000000000000000000000000000000000000000000000000200000000"
      "00000000000000000000000000000000000000000000000000000003";
  SSVM::Support::convertHexStrToBytes(CallDataStr, EVM->Env->getCallData());

  /// Execute.
  EVM->VM->setPath(EVM->Erc20Path);
  EXPECT_EQ(EVM->VM->execute("main"), SSVM::VM::ErrCode::Success);

  /// Check return data.
  std::string ExpStr =
      "0000000000000000000000000000000000000000000000000000000000000001";
  EXPECT_TRUE(isReturnExpected(ExpStr, EVM->Env->getReturnData()));

  /// Check result.
  EVM->printResult();
}

TEST_F(ERC20Test, Run__9_check_balance_of_0x7FFFFFFF) {
  /// Set call data
  std::string CallDataStr = "70a08231000000000000000000000000000000000000000000"
                            "000000000000007fffffff";
  SSVM::Support::convertHexStrToBytes(CallDataStr, EVM->Env->getCallData());

  /// Execute.
  EVM->VM->setPath(EVM->Erc20Path);
  EXPECT_EQ(EVM->VM->execute("main"), SSVM::VM::ErrCode::Success);

  /// Check return data.
  std::string ExpStr =
      "00000000000000000000000000000000000000000000000000000000000003d1";
  EXPECT_TRUE(isReturnExpected(ExpStr, EVM->Env->getReturnData()));

  /// Check result.
  EVM->printResult();
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
