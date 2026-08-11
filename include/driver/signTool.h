// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/driver/signTool.h - Wasm module signing tool -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Entry point for `wasmedge sign`.  Only available when
/// WASMEDGE_BUILD_SIGNATURE_TOOLS is ON.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace WasmEdge {
namespace Driver {

struct DriverToolOptions;

/// Run the `wasmedge sign` subcommand.
/// Reads the Wasm file specified in Opt.SoName, loads the private keypair from
/// Opt.KeyFile, computes the rolling-hash digest over every non-signature
/// section, produces a `signature` custom section prepended to the output, and
/// writes the signed module to Opt.OutputFile (or <input>.signed.wasm by
/// default).
///
/// Returns EXIT_SUCCESS or EXIT_FAILURE.
int SignTool(struct DriverToolOptions &Opt) noexcept;

} // namespace Driver
} // namespace WasmEdge
