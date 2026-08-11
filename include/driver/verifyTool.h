// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/driver/verifyTool.h - Wasm module verification tool ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Entry point for `wasmedge verify`.  Only available when
/// WASMEDGE_BUILD_SIGNATURE_TOOLS is ON.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace WasmEdge {
namespace Driver {

struct DriverToolOptions;

/// Run the `wasmedge verify` subcommand.
/// Reads the Wasm file specified in Opt.SoName, locates the leading `signature`
/// custom section, and verifies every recorded hash against the rolling SHA-256
/// computed over the remaining module bytes.
///
/// When Opt.KeyFile is non-empty, only signatures whose embedded public key
/// matches the supplied key are considered.  An empty KeyFile accepts any key.
///
/// Returns EXIT_SUCCESS or EXIT_FAILURE.
int VerifyTool(struct DriverToolOptions &Opt) noexcept;

} // namespace Driver
} // namespace WasmEdge
