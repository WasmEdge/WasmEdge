// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
//
// This is the external scanner of the WAT grammar. It reads the block comments
// (;...;), which can nest and which conflict with the '(' token of a sexpr. It
// also reads the annotations (@id ...).

#include "tree_sitter/parser.h"

enum TokenType {
  BLOCK_COMMENT,
  ANNOTATION,
};

void *tree_sitter_wat_external_scanner_create(void) { return NULL; }

void tree_sitter_wat_external_scanner_destroy(void *payload) { (void)payload; }

unsigned tree_sitter_wat_external_scanner_serialize(void *payload,
                                                    char *buffer) {
  (void)payload;
  (void)buffer;
  return 0;
}

void tree_sitter_wat_external_scanner_deserialize(void *payload,
                                                  const char *buffer,
                                                  unsigned length) {
  (void)payload;
  (void)buffer;
  (void)length;
}

static void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

static void skip_ws(TSLexer *lexer) { lexer->advance(lexer, true); }

static bool is_space(int32_t c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static bool parse_string(TSLexer *lexer) {
  advance(lexer); // Consume the first quote.
  while (!lexer->eof(lexer) && lexer->lookahead != '"') {
    // The scanner only finds the closing quote. The character after a
    // backslash cannot end the string, and the rest of an escape such as
    // \u{...} or \HH holds no quote. The converter decodes the escapes.
    if (lexer->lookahead == '\\') {
      advance(lexer); // Consume the backslash.
      if (lexer->eof(lexer)) {
        return false;
      }
    }
    advance(lexer);
  }
  if (lexer->eof(lexer)) {
    return false;
  }
  advance(lexer); // Consume the last quote.
  return true;
}

static void parse_line_comment(TSLexer *lexer) {
  advance(lexer); // ';'
  while (!lexer->eof(lexer) && lexer->lookahead != '\n' &&
         lexer->lookahead != '\r') {
    advance(lexer); // Consume the regular character.
  }
}

static bool parse_block_comment(TSLexer *lexer) {
  advance(lexer); // ';'
  int depth = 1;
  while (depth > 0 && !lexer->eof(lexer)) {
    if (lexer->lookahead == '(') {
      advance(lexer);
      if (lexer->lookahead == ';') {
        advance(lexer);
        depth++;
      }
    } else if (lexer->lookahead == ';') {
      advance(lexer);
      if (lexer->lookahead == ')') {
        advance(lexer);
        depth--;
      }
    } else {
      advance(lexer);
    }
  }
  return depth == 0;
}

static bool parse_annotation(TSLexer *lexer) {
  advance(lexer); // Consume the '@'.
  // The annotation id is a keyword or a quoted string.
  if (lexer->lookahead == '"') {
    if (!parse_string(lexer)) {
      return false;
    }
  } else {
    while (!lexer->eof(lexer) && !is_space(lexer->lookahead) &&
           lexer->lookahead != '(' && lexer->lookahead != ')') {
      advance(lexer);
    }
  }
  // Scan the balanced parentheses to the matching ')'. Read the block
  // comments, the line comments, and the strings, so that their content does
  // not change the count of the parentheses.
  int depth = 1;
  while (depth > 0 && !lexer->eof(lexer)) {
    if (lexer->lookahead == '(') {
      advance(lexer); // '('
      if (lexer->lookahead == ';') {
        if (!parse_block_comment(lexer)) {
          return false;
        }
      } else {
        depth++;
      }
    } else if (lexer->lookahead == ')') {
      advance(lexer); // ')'
      depth--;
    } else if (lexer->lookahead == ';') {
      advance(lexer); // ';'
      if (lexer->lookahead == ';') {
        parse_line_comment(lexer);
      }
    } else if (lexer->lookahead == '"') {
      if (!parse_string(lexer)) {
        return false;
      }
    } else {
      advance(lexer); // Consume the regular character.
    }
  }
  return depth == 0;
}

bool tree_sitter_wat_external_scanner_scan(void *payload, TSLexer *lexer,
                                           const bool *valid_symbols) {
  (void)payload;

  // Skip the whitespace before you look for the '(' at the start.
  while (is_space(lexer->lookahead)) {
    skip_ws(lexer);
  }

  if (lexer->lookahead != '(') {
    return false;
  }

  lexer->mark_end(lexer);
  advance(lexer);

  // A '(;' starts a block comment.
  if (lexer->lookahead == ';' && valid_symbols[BLOCK_COMMENT]) {
    if (!parse_block_comment(lexer)) {
      return false;
    }
    lexer->mark_end(lexer);
    lexer->result_symbol = BLOCK_COMMENT;
    return true;
  }

  // A '(@' starts an annotation. Scan the balanced parentheses to the last
  // ')'.
  if (lexer->lookahead == '@' && valid_symbols[ANNOTATION]) {
    if (!parse_annotation(lexer)) {
      return false;
    }
    lexer->mark_end(lexer);
    lexer->result_symbol = ANNOTATION;
    return true;
  }

  return false;
}
