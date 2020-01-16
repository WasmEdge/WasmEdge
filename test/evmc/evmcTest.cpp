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

#include "evmc/evmc.h"
#include "evmc/loader.h"
#include "support/hexstr.h"
#include "gtest/gtest.h"

#include "erc20.h"
#include "example_host.h"

#include <cstring>
#include <iostream>

namespace {

evmc_context *context = example_host_create_context(evmc_tx_context{});

TEST(EVMCTest, Run__1_deploy) {
  enum evmc_loader_error_code err;
  struct evmc_instance *vm =
      evmc_load_and_create("../../tools/ssvm-evmc/libssvm-evmc.so", &err);
  EXPECT_EQ(err, EVMC_LOADER_SUCCESS);

  evmc_address sender = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x7f, 0xff, 0xff, 0xff};
  evmc_address destination = {};
  int64_t gas = 999999;
  evmc_uint256be value = {};
  evmc_bytes32 create2_salt = {};

  evmc_message msg{EVMC_CALL,
                   0,
                   0,
                   gas,
                   destination,
                   sender,
                   erc20_deploy_wasm.data(),
                   erc20_deploy_wasm.size(),
                   value,
                   create2_salt};
  evmc_result result =
      vm->execute(vm, context, EVMC_MAX_REVISION, &msg,
                  erc20_deploy_wasm.data(), erc20_deploy_wasm.size());

  EXPECT_EQ(result.status_code, EVMC_SUCCESS);
  EXPECT_EQ(result.output_size, erc20_wasm.size());
  EXPECT_EQ(0,
            memcmp(result.output_data, erc20_wasm.data(), result.output_size));

  if (result.release)
    result.release(&result);
}

evmc_address string_to_address(std::string SenderStr) {
  evmc_address address;
  std::vector<uint8_t> Sender;
  SSVM::Support::convertHexStrToBytes(SenderStr, Sender);
  for (int i = 0; i < 20; i++) {
    address.bytes[i] = Sender[i];
  }
  return address;
}

evmc_result evmc_vm_execute(evmc_instance *vm, std::string SenderStr,
                            std::string CallDataStr) {
  std::vector<uint8_t> CallData;
  SSVM::Support::convertHexStrToBytes(CallDataStr, CallData);
  evmc_address sender = string_to_address(SenderStr);
  evmc_address destination = {};
  int64_t gas = 999999;
  evmc_uint256be value = {};
  evmc_bytes32 create2_salt = {};

  evmc_message msg{EVMC_CALL,
                   0,
                   0,
                   gas,
                   destination,
                   sender,
                   CallData.data(),
                   CallData.size(),
                   value,
                   create2_salt};
  evmc_result result = vm->execute(vm, context, EVMC_MAX_REVISION, &msg,
                                   erc20_wasm.data(), erc20_wasm.size());
  return result;
}

TEST(EVMCTest, Run__2_check_balance_of_0x7FFFFFFF) {
  enum evmc_loader_error_code err;
  struct evmc_instance *vm =
      evmc_load_and_create("../../tools/ssvm-evmc/libssvm-evmc.so", &err);
  EXPECT_EQ(err, EVMC_LOADER_SUCCESS);
  std::string SenderStr = "000000000000000000000000000000007fffffff";
  std::string CallDataStr =
      "70a08231"
      "000000000000000000000000000000000000000000000000000000007fffffff";
  evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

  std::array<uint8_t, 32> expected_result = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe8}};
  EXPECT_EQ(result.status_code, EVMC_SUCCESS);
  EXPECT_EQ(result.output_size, expected_result.size());
  EXPECT_EQ(0, memcmp(result.output_data, expected_result.data(),
                      result.output_size));

  if (result.release)
    result.release(&result);
}

TEST(EVMCTest, Run__3_check_total_supply) {
  enum evmc_loader_error_code err;
  struct evmc_instance *vm =
      evmc_load_and_create("../../tools/ssvm-evmc/libssvm-evmc.so", &err);
  EXPECT_EQ(err, EVMC_LOADER_SUCCESS);
  std::string SenderStr = "000000000000000000000000000000007fffffff";
  std::string CallDataStr = "18160ddd";
  evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

  std::array<uint8_t, 32> expected_result = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe8}};
  EXPECT_EQ(result.status_code, EVMC_SUCCESS);
  EXPECT_EQ(result.output_size, expected_result.size());
  EXPECT_EQ(0, memcmp(result.output_data, expected_result.data(),
                      result.output_size));

  if (result.release)
    result.release(&result);
}

TEST(EVMCTest, Run__4_transfer_20_from_0x7FFFFFFF_to_0x01) {
  enum evmc_loader_error_code err;
  struct evmc_instance *vm =
      evmc_load_and_create("../../tools/ssvm-evmc/libssvm-evmc.so", &err);
  EXPECT_EQ(err, EVMC_LOADER_SUCCESS);
  std::string SenderStr = "000000000000000000000000000000007fffffff";
  std::string CallDataStr =
      "a9059cbb"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000014";
  evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

  std::array<uint8_t, 32> expected_result = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}};
  EXPECT_EQ(result.status_code, EVMC_SUCCESS);
  EXPECT_EQ(result.output_size, expected_result.size());
  EXPECT_EQ(0, memcmp(result.output_data, expected_result.data(),
                      result.output_size));

  if (result.release)
    result.release(&result);
}

TEST(EVMCTest, Run__5_check_balance_of_0x01) {
  enum evmc_loader_error_code err;
  struct evmc_instance *vm =
      evmc_load_and_create("../../tools/ssvm-evmc/libssvm-evmc.so", &err);
  EXPECT_EQ(err, EVMC_LOADER_SUCCESS);
  std::string SenderStr = "000000000000000000000000000000007fffffff";
  std::string CallDataStr =
      "70a08231"
      "0000000000000000000000000000000000000000000000000000000000000001";
  evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

  std::array<uint8_t, 32> expected_result = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14}};
  EXPECT_EQ(result.status_code, EVMC_SUCCESS);
  EXPECT_EQ(result.output_size, expected_result.size());
  EXPECT_EQ(0, memcmp(result.output_data, expected_result.data(),
                      result.output_size));

  if (result.release)
    result.release(&result);
}

TEST(EVMCTest, Run__6_approve_10_from_0x7FFFFFFF_for_0x01_to_spend) {
  enum evmc_loader_error_code err;
  struct evmc_instance *vm =
      evmc_load_and_create("../../tools/ssvm-evmc/libssvm-evmc.so", &err);
  EXPECT_EQ(err, EVMC_LOADER_SUCCESS);
  std::string SenderStr = "000000000000000000000000000000007fffffff";
  std::string CallDataStr =
      "095ea7b3"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "000000000000000000000000000000000000000000000000000000000000000a";
  evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

  std::array<uint8_t, 32> expected_result = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}};
  EXPECT_EQ(result.status_code, EVMC_SUCCESS);
  EXPECT_EQ(result.output_size, expected_result.size());
  EXPECT_EQ(0, memcmp(result.output_data, expected_result.data(),
                      result.output_size));

  if (result.release)
    result.release(&result);
}

TEST(EVMCTest, Run__7_check_allowance_from_0x7FFFFFFF_by_0x01) {
  enum evmc_loader_error_code err;
  struct evmc_instance *vm =
      evmc_load_and_create("../../tools/ssvm-evmc/libssvm-evmc.so", &err);
  EXPECT_EQ(err, EVMC_LOADER_SUCCESS);
  std::string SenderStr = "000000000000000000000000000000007fffffff";
  std::string CallDataStr =
      "dd62ed3e"
      "000000000000000000000000000000000000000000000000000000007fffffff"
      "0000000000000000000000000000000000000000000000000000000000000001";
  evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

  std::array<uint8_t, 32> expected_result = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a}};
  EXPECT_EQ(result.status_code, EVMC_SUCCESS);
  EXPECT_EQ(result.output_size, expected_result.size());
  EXPECT_EQ(0, memcmp(result.output_data, expected_result.data(),
                      result.output_size));

  if (result.release)
    result.release(&result);
}

TEST(EVMCTest, Run__8_transfer_3_from_0x7FFFFFFF_by_0x01_to_0x02) {
  enum evmc_loader_error_code err;
  struct evmc_instance *vm =
      evmc_load_and_create("../../tools/ssvm-evmc/libssvm-evmc.so", &err);
  EXPECT_EQ(err, EVMC_LOADER_SUCCESS);
  std::string SenderStr = "0000000000000000000000000000000000000001";
  std::string CallDataStr =
      "23b872dd"
      "000000000000000000000000000000000000000000000000000000007fffffff"
      "0000000000000000000000000000000000000000000000000000000000000002"
      "0000000000000000000000000000000000000000000000000000000000000003";
  evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

  std::array<uint8_t, 32> expected_result = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}};
  EXPECT_EQ(result.status_code, EVMC_SUCCESS);
  EXPECT_EQ(result.output_size, expected_result.size());
  EXPECT_EQ(0, memcmp(result.output_data, expected_result.data(),
                      result.output_size));

  if (result.release)
    result.release(&result);
}

TEST(EVMCTest, Run__9_check_balance_of_0x7FFFFFFF) {
  enum evmc_loader_error_code err;
  struct evmc_instance *vm =
      evmc_load_and_create("../../tools/ssvm-evmc/libssvm-evmc.so", &err);
  EXPECT_EQ(err, EVMC_LOADER_SUCCESS);
  std::string SenderStr = "0000000000000000000000000000000000000001";
  std::string CallDataStr =
      "70a08231"
      "000000000000000000000000000000000000000000000000000000007fffffff";
  evmc_result result = evmc_vm_execute(vm, SenderStr, CallDataStr);

  std::array<uint8_t, 32> expected_result = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xd1}};
  EXPECT_EQ(result.status_code, EVMC_SUCCESS);
  EXPECT_EQ(result.output_size, expected_result.size());
  EXPECT_EQ(0, memcmp(result.output_data, expected_result.data(),
                      result.output_size));

  if (result.release)
    result.release(&result);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
