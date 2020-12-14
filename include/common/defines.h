// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/defines.h - General defines ---------------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains macro defines that tweak ssvm internal.
///
//===----------------------------------------------------------------------===//
#pragma once
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
#define SSVM_OS_WINDOWS 1
#endif

#if defined(linux) || defined(__linux) || defined(__linux__) ||                \
    defined(__gnu_linux__)
#define SSVM_OS_LINUX 1
#endif

#if defined(macintosh) || defined(Macintosh) ||                                \
    (defined(__APPLE__) && defined(__MACH__))
#define SSVM_OS_MACOS 1
#endif
