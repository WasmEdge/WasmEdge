// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
//
// This is a minimal tree-sitter grammar for the WebAssembly Text Format (WAT).
// It gives only the S-expression structure and the raw token types. The
// converter holds all the semantics.

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
    // This is the top level. It holds zero S-expressions or more, and it can
    // also hold bare tokens.
    root: $ => repeat($._child),

    // This is a group in parentheses. It is the only structural form.
    sexpr: $ => seq('(', repeat($._child), ')'),

    // This is a child of a sexpr or of the root. It is another sexpr or a leaf
    // token.
    _child: $ => choice($.sexpr, $._leaf_token),

    // These are the leaf token types. The first type has the highest
    // precedence.
    _leaf_token: $ => choice(
      prec(1, $.keyword),
      prec(1, $.u),
      prec(1, $.s),
      prec(1, $.f),
      prec(1, $.string),
      prec(1, $.id),
      prec(0, $.reserved),
    ),

    // The form is $name or $"quoted name".
    id: _$ => token(seq('$', choice(
      repeat(ID_CHAR),
      seq('"', repeat(STRING_ELEM), '"'),
    ))),

    // The form is "string literal".
    string: _$ => token(seq('"', repeat(STRING_ELEM), '"')),

    // This is an unsigned integer in hex (0x...) or in decimal. It can hold
    // '_' separators.
    u: _$ => token(/(?:0x[0-9A-Fa-f]+(?:_[0-9A-Fa-f]+)*|[0-9]+(?:_[0-9]+)*)/),
    // This is a signed integer. A '+' or '-' comes before an unsigned integer.
    s: _$ => token(/[+-](?:0x[0-9A-Fa-f]+(?:_[0-9A-Fa-f]+)*|[0-9]+(?:_[0-9]+)*)/),
    // This is a float. A '+' or '-' can come first. The value is hex or
    // decimal. The inf form and the nan form are also floats.
    f: _$ => token(/[+-]?(?:0x[0-9A-Fa-f]+(?:_[0-9A-Fa-f]+)*(?:\.(?:[0-9A-Fa-f]+(?:_[0-9A-Fa-f]+)*)?)?(?:[pP][+-]?[0-9]+(?:_[0-9]+)*)?|[0-9]+(?:_[0-9]+)*(?:\.(?:[0-9]+(?:_[0-9]+)*)?)?(?:[eE][+-]?[0-9]+(?:_[0-9]+)*)?|inf|nan(?::0x[0-9A-Fa-f]+(?:_[0-9A-Fa-f]+)*)?)/),

    // These are the WAT keywords, such as an instruction, a type, or a section
    // name. The forms offset=N, align=N, inf, nan, and nan:0xN also match as
    // keywords. The converter classifies them.
    keyword: _$ => token(seq(/[a-z]/, repeat(ID_CHAR))),

    // This rule matches the other tokens, and it has the lowest precedence.
    // Some examples are 0$x, +inf, and -nan:0x1. The rule does not accept ';',
    // so a ';;' line comment and a '(;' block comment stay outside it.
    reserved: _$ => token(/[^\s();]+/),

    // This is a line comment. It starts with ;; and ends at the end of the
    // line, which is a CR, an LF, or a CRLF.
    _line_comment: _$ => token(prec(2, seq(';;', /[^\n\r]*/))),

    // This is a block comment (;...;). The external scanner reads it, because
    // it can nest. The scanner also prevents an ambiguity with the '(' of the
    // sexpr rule.
  },
});
