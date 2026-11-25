// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/wat/lexical.h - WAT lexical checks -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The lexical checks of the WAT text format that the parse tree does not
/// show. The tree-sitter scanner finds the token boundaries. These checks find
/// the characters, the annotations, and the adjacent tokens that the rules do
/// not permit. The WAT parser and the WAST parser share them.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"

#include <cstddef>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace WAT {

/// Scan the text Source for the characters that the lexical rules do not
/// permit. Outside a string and a comment, the text cannot hold the DEL
/// character, a non-ASCII character, or a control character other than
/// whitespace. A string and a comment can hold any character, but they must be
/// valid UTF-8. If AnnotationStarts is not null, the function appends to it the
/// position of each "(@" outside a string and a comment.
Expect<void> scanSourceCharacters(std::string_view Source,
                                  std::vector<size_t> *AnnotationStarts);

/// Examine the annotation Text, which starts with "(@" and is the text of an
/// annotation node of the parse tree. The id must not be empty, and a quoted id
/// must be valid UTF-8. The strings and the quoted ids of nested annotations in
/// the payload follow the same rules.
Expect<void> checkAnnotation(std::string_view Text);

/// Examine two consecutive leaf tokens PrevText and CurrText with no gap
/// between them. Only a parenthesis can touch another token. Two strings such
/// as "a""b" also need whitespace between them.
Expect<void> checkAdjacentTokens(std::string_view PrevText,
                                 std::string_view CurrText);

} // namespace WAT
} // namespace WasmEdge
