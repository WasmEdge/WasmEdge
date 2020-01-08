/* EVMC: Ethereum Client-VM Connector API.
 * Copyright 2018-2019 The EVMC Authors.
 * Licensed under the Apache License, Version 2.0.
 */

/**
 * EVM Instruction Tables
 *
 * A collection of metrics for EVM1 instruction set.
 *
 * @defgroup instructions EVM Instructions
 * @{
 */
#pragma once

#include <evmc/evmc.h>
#include <evmc/utils.h>

#if __cplusplus
extern "C" {
#endif

/**
 * The list of EVM 1 opcodes from every EVM revision.
 */
enum evmc_opcode
{
    OP_STOP = 0x00,
    OP_ADD = 0x01,
    OP_MUL = 0x02,
    OP_SUB = 0x03,
    OP_DIV = 0x04,
    OP_SDIV = 0x05,
    OP_MOD = 0x06,
    OP_SMOD = 0x07,
    OP_ADDMOD = 0x08,
    OP_MULMOD = 0x09,
    OP_EXP = 0x0a,
    OP_SIGNEXTEND = 0x0b,

    OP_LT = 0x10,
    OP_GT = 0x11,
    OP_SLT = 0x12,
    OP_SGT = 0x13,
    OP_EQ = 0x14,
    OP_ISZERO = 0x15,
    OP_AND = 0x16,
    OP_OR = 0x17,
    OP_XOR = 0x18,
    OP_NOT = 0x19,
    OP_BYTE = 0x1a,
    OP_SHL = 0x1b,
    OP_SHR = 0x1c,
    OP_SAR = 0x1d,

    OP_SHA3 = 0x20,

    OP_ADDRESS = 0x30,
    OP_BALANCE = 0x31,
    OP_ORIGIN = 0x32,
    OP_CALLER = 0x33,
    OP_CALLVALUE = 0x34,
    OP_CALLDATALOAD = 0x35,
    OP_CALLDATASIZE = 0x36,
    OP_CALLDATACOPY = 0x37,
    OP_CODESIZE = 0x38,
    OP_CODECOPY = 0x39,
    OP_GASPRICE = 0x3a,
    OP_EXTCODESIZE = 0x3b,
    OP_EXTCODECOPY = 0x3c,
    OP_RETURNDATASIZE = 0x3d,
    OP_RETURNDATACOPY = 0x3e,
    OP_EXTCODEHASH = 0x3f,

    OP_BLOCKHASH = 0x40,
    OP_COINBASE = 0x41,
    OP_TIMESTAMP = 0x42,
    OP_NUMBER = 0x43,
    OP_DIFFICULTY = 0x44,
    OP_GASLIMIT = 0x45,

    OP_POP = 0x50,
    OP_MLOAD = 0x51,
    OP_MSTORE = 0x52,
    OP_MSTORE8 = 0x53,
    OP_SLOAD = 0x54,
    OP_SSTORE = 0x55,
    OP_JUMP = 0x56,
    OP_JUMPI = 0x57,
    OP_PC = 0x58,
    OP_MSIZE = 0x59,
    OP_GAS = 0x5a,
    OP_JUMPDEST = 0x5b,

    OP_PUSH1 = 0x60,
    OP_PUSH2 = 0x61,
    OP_PUSH3 = 0x62,
    OP_PUSH4 = 0x63,
    OP_PUSH5 = 0x64,
    OP_PUSH6 = 0x65,
    OP_PUSH7 = 0x66,
    OP_PUSH8 = 0x67,
    OP_PUSH9 = 0x68,
    OP_PUSH10 = 0x69,
    OP_PUSH11 = 0x6a,
    OP_PUSH12 = 0x6b,
    OP_PUSH13 = 0x6c,
    OP_PUSH14 = 0x6d,
    OP_PUSH15 = 0x6e,
    OP_PUSH16 = 0x6f,
    OP_PUSH17 = 0x70,
    OP_PUSH18 = 0x71,
    OP_PUSH19 = 0x72,
    OP_PUSH20 = 0x73,
    OP_PUSH21 = 0x74,
    OP_PUSH22 = 0x75,
    OP_PUSH23 = 0x76,
    OP_PUSH24 = 0x77,
    OP_PUSH25 = 0x78,
    OP_PUSH26 = 0x79,
    OP_PUSH27 = 0x7a,
    OP_PUSH28 = 0x7b,
    OP_PUSH29 = 0x7c,
    OP_PUSH30 = 0x7d,
    OP_PUSH31 = 0x7e,
    OP_PUSH32 = 0x7f,
    OP_DUP1 = 0x80,
    OP_DUP2 = 0x81,
    OP_DUP3 = 0x82,
    OP_DUP4 = 0x83,
    OP_DUP5 = 0x84,
    OP_DUP6 = 0x85,
    OP_DUP7 = 0x86,
    OP_DUP8 = 0x87,
    OP_DUP9 = 0x88,
    OP_DUP10 = 0x89,
    OP_DUP11 = 0x8a,
    OP_DUP12 = 0x8b,
    OP_DUP13 = 0x8c,
    OP_DUP14 = 0x8d,
    OP_DUP15 = 0x8e,
    OP_DUP16 = 0x8f,
    OP_SWAP1 = 0x90,
    OP_SWAP2 = 0x91,
    OP_SWAP3 = 0x92,
    OP_SWAP4 = 0x93,
    OP_SWAP5 = 0x94,
    OP_SWAP6 = 0x95,
    OP_SWAP7 = 0x96,
    OP_SWAP8 = 0x97,
    OP_SWAP9 = 0x98,
    OP_SWAP10 = 0x99,
    OP_SWAP11 = 0x9a,
    OP_SWAP12 = 0x9b,
    OP_SWAP13 = 0x9c,
    OP_SWAP14 = 0x9d,
    OP_SWAP15 = 0x9e,
    OP_SWAP16 = 0x9f,
    OP_LOG0 = 0xa0,
    OP_LOG1 = 0xa1,
    OP_LOG2 = 0xa2,
    OP_LOG3 = 0xa3,
    OP_LOG4 = 0xa4,

    OP_CREATE = 0xf0,
    OP_CALL = 0xf1,
    OP_CALLCODE = 0xf2,
    OP_RETURN = 0xf3,
    OP_DELEGATECALL = 0xf4,
    OP_CREATE2 = 0xf5,

    OP_STATICCALL = 0xfa,

    OP_REVERT = 0xfd,
    OP_INVALID = 0xfe,
    OP_SELFDESTRUCT = 0xff
};

/**
 * Metrics for an EVM 1 instruction.
 *
 * Small integer types are used here to make the tables of metrics cache friendly.
 */
struct evmc_instruction_metrics
{
    /** The instruction gas cost. Value -1 indicates an undefined instruction. */
    int16_t gas_cost;

    /** The number of items the instruction pops from the EVM stack before execution. */
    int8_t num_stack_arguments;

    /** The number of items the instruction pushes to the EVM stack after execution. */
    int8_t num_stack_returned_items;
};

/**
 * Get the table of the EVM 1 instructions metrics.
 *
 * @param revision  The EVM revision.
 * @return          The pointer to the array of 256 instruction metrics. Null pointer in case
 *                  an invalid EVM revision provided.
 */
EVMC_EXPORT const struct evmc_instruction_metrics* evmc_get_instruction_metrics_table(
    enum evmc_revision revision);

/**
 * Get the table of the EVM 1 instruction names.
 *
 * The entries for undefined instructions contain null pointers.
 *
 * @param revision  The EVM revision.
 * @return          The pointer to the array of 256 instruction names. Null pointer in case
 *                  an invalid EVM revision provided.
 */
EVMC_EXPORT const char* const* evmc_get_instruction_names_table(enum evmc_revision revision);

#if __cplusplus
}
#endif

/** @} */
