/* EVMC: Ethereum Client-VM Connector API.
 * Copyright 2016-2019 The EVMC Authors.
 * Licensed under the Apache License, Version 2.0.
 */

/// @file
/// Example implementation of an EVMC Host.

#include "example_host.h"

#include <evmc/evmc.hpp>

#include <iostream>
#include <map>

using namespace evmc::literals;

struct account {
  evmc::uint256be balance = {};
  size_t code_size = 0;
  evmc::bytes32 code_hash = {};
  std::map<evmc::bytes32, evmc::bytes32> storage;
};

class ExampleHost : public evmc::Host {
  std::map<evmc::address, account> accounts;
  evmc_tx_context tx_context{};

public:
  ExampleHost() = default;
  explicit ExampleHost(evmc_tx_context &_tx_context) noexcept
      : tx_context{_tx_context} {};

  bool account_exists(const evmc::address &addr) noexcept final {
    return accounts.find(addr) != accounts.end();
  }

  evmc::bytes32 get_storage(const evmc::address &addr,
                            const evmc::bytes32 &key) noexcept final {
    auto it = accounts.find(addr);
    if (it != accounts.end())
      return it->second.storage[key];
    return {};
  }

  evmc_storage_status set_storage(const evmc::address &addr,
                                  const evmc::bytes32 &key,
                                  const evmc::bytes32 &value) noexcept final {
    auto &account = accounts[addr];
    auto prev_value = account.storage[key];
    account.storage[key] = value;

    return (prev_value == value) ? EVMC_STORAGE_UNCHANGED
                                 : EVMC_STORAGE_MODIFIED;
  }

  evmc::uint256be get_balance(const evmc::address &addr) noexcept final {
    auto it = accounts.find(addr);
    if (it != accounts.end())
      return it->second.balance;
    return {};
  }

  size_t get_code_size(const evmc::address &addr) noexcept final {
    auto it = accounts.find(addr);
    if (it != accounts.end())
      return it->second.code_size;
    return 0;
  }

  evmc::bytes32 get_code_hash(const evmc::address &addr) noexcept final {
    auto it = accounts.find(addr);
    if (it != accounts.end())
      return it->second.code_hash;
    return {};
  }

  size_t copy_code(const evmc::address &addr, size_t code_offset,
                   uint8_t *buffer_data, size_t buffer_size) noexcept final {
    (void)addr;
    (void)code_offset;
    (void)buffer_data;
    (void)buffer_size;
    return 0;
  }

  void selfdestruct(const evmc::address &addr,
                    const evmc::address &beneficiary) noexcept final {
    (void)addr;
    (void)beneficiary;
  }

  evmc::result call(const evmc_message &msg) noexcept final {
    return {EVMC_REVERT, msg.gas, msg.input_data, msg.input_size};
  }

  evmc_tx_context get_tx_context() noexcept final { return tx_context; }

  evmc::bytes32 get_block_hash(int64_t number) noexcept final {
    const int64_t current_block_number = get_tx_context().block_number;

    return (number < current_block_number &&
            number >= current_block_number - 256)
               ? 0xb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5fb10c8a5f_bytes32
               : 0_bytes32;
  }

  void emit_log(const evmc::address &addr, const uint8_t *data,
                size_t data_size, const evmc::bytes32 topics[],
                size_t topics_count) noexcept final {
    (void)addr;
    (void)data;
    (void)data_size;
    (void)topics;
    (void)topics_count;
  }
};

extern "C" {

evmc_context *example_host_create_context(evmc_tx_context tx_context) {
  return new ExampleHost(tx_context);
}

void example_host_destroy_context(evmc_context *context) {
  delete static_cast<ExampleHost *>(context);
}
}
