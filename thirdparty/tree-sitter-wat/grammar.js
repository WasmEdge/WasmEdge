// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC
//
// Minimal tree-sitter grammar for WebAssembly Text Format (WAT).
// Only provides S-expression structure and raw token types.
// All semantic interpretation is handled by the converter.

/// <reference types="tree-sitter-cli/dsl" />

const ID_CHAR = /[0-9A-Za-z!#$%&'*+\-.\/:<=>?@\\^_`|~]/;

const STRING_ELEM = choice(
  /[^"\\]+/,
  /\\[tnr"'\\]/,
  /\\[0-9A-Fa-f]{2}/,
  /\\u\{[0-9A-Fa-f]+\}/,
);

module.exports = grammar({
  name: 'wat',

  word: $ => $.keyword,

  extras: $ => [/\s/, $._line_comment, $._block_comment, $.annotation],

  externals: $ => [$._block_comment, $.annotation],

  rules: {
    // Top-level: zero or more S-expressions or bare tokens.
    root: $ => repeat($._child),

    // Parenthesized group — the only structural construct.
    sexpr: $ => seq('(', repeat($._child), ')'),

    // A child of sexpr or root: another sexpr or a leaf token.
    _child: $ => choice($.sexpr, $._leaf_token),

    // Leaf token types in precedence order (highest first).
    _leaf_token: $ => choice(
      prec(1, $.keyword),
      prec(1, $.u),
      prec(1, $.s),
      prec(1, $.f),
      prec(1, $.string),
      prec(1, $.id),
      prec(0, $.reserved),
    ),

    // $name or $"quoted name"
    id: _$ => token(seq('$', choice(
      repeat(ID_CHAR),
      seq('"', repeat(STRING_ELEM), '"'),
    ))),

    // "string literal"
    string: _$ => token(seq('"', repeat(STRING_ELEM), '"')),

    // Integer literals
    // Unsigned integer literals: hex (0x...) or decimal, with optional _ separators.
    u: _$ => token(/(?:0x[0-9A-Fa-f]+(?:_[0-9A-Fa-f]+)*|[0-9]+(?:_[0-9]+)*)/),
    // Signed integer literals: + or - prefix, then unsigned literal.
    s: _$ => token(/[+-](?:0x[0-9A-Fa-f]+(?:_[0-9A-Fa-f]+)*|[0-9]+(?:_[0-9]+)*)/),
    // Float literals: optional + or - prefix, then hex or decimal with optional _ separators,
    f: _$ => token(/[+-]?(?:0x[0-9A-Fa-f]+(?:_[0-9A-Fa-f]+)*(?:\.(?:[0-9A-Fa-f]+(?:_[0-9A-Fa-f]+)*)?)?(?:[pP][+-]?[0-9]+(?:_[0-9]+)*)?|[0-9]+(?:_[0-9]+)*(?:\.(?:[0-9]+(?:_[0-9]+)*)?)?(?:[eE][+-]?[0-9]+(?:_[0-9]+)*)?|inf|nan(?::0x[0-9A-Fa-f]+(?:_[0-9A-Fa-f]+)*)?)/),

    // WAT keywords: instructions, types, section names, etc.
    // offset=N and align=N match as single keywords (converter splits on =).
    // inf, nan, nan:0xN match as keywords (converter classifies by context).
    keyword: _$ => token(seq(/[a-z]/, repeat(ID_CHAR))),

    // Catch-all for anything not matching above (lowest precedence).
    // Excludes ';' so that ';;' line comments and '(;' block comments
    // are not swallowed by the reserved rule.
    // Examples: 0$x, +inf, -nan:0x1
    reserved: _$ => token(/[^\s();]+/),

    // Line comment: ;; to end of line (CR, LF, or CRLF)
    _line_comment: _$ => token(prec(2, seq(';;', /[^\n\r]*/))),

    // Block comment (;...;) handled by external scanner (nestable, avoids
    // ambiguity with '(' in sexpr rule).
  },
});
