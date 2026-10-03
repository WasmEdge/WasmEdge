// SPDX-License-Identifier: CC0-1.0
///
// simd - A C++17 implementation of a subset of the C++26 std::simd library
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
///
// Defining CXX26_SIMD_FORCE_GENERIC selects the std::array backend for every
// vec and mask. Defining CXX26_SIMD_FORCE_M128I selects the __m128i backend,
// which MSVC uses, for every 16-byte vec and mask on x86-64. Both change the
// definition of these templates, so they must be defined for the whole
// program, never for a single translation unit.
///

#pragma once

#include "experimental/simd/backend_array.hpp"
#include "experimental/simd/backend_m128i.hpp"
#include "experimental/simd/backend_vector_ext.hpp"
#include "experimental/simd/detail.hpp"
#include "experimental/simd/functions.hpp"
#include "experimental/simd/vec.hpp"
