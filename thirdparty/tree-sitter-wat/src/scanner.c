// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC
//
// External scanner for WAT grammar.
// Handles block comments (;...;) which are nestable and conflict with
// the '(' token in the sexpr rule. Also handles annotations (@id ...).

#include "tree_sitter/parser.h"
#include <ctype.h>

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

static bool parse_string(TSLexer *lexer) {
  advance(lexer); // opening quote
  while (!lexer->eof(lexer) && lexer->lookahead != '"') {
    if (lexer->lookahead == '\\') {
      advance(lexer); // backslash
      if (lexer->eof(lexer)) {
        return false;
      }
      if (lexer->lookahead == 'u') {
        advance(lexer); // 'u'
        if (lexer->eof(lexer)) {
          return false;
        }
        if (lexer->lookahead == '{') {
          advance(lexer); // '{'
          while (!lexer->eof(lexer) && isxdigit(lexer->lookahead)) {
            advance(lexer); // hex digits
          }
          if (lexer->eof(lexer)) {
            return false;
          }
          if (lexer->lookahead == '}') {
            advance(lexer); // '}'
          }
        }
      } else if (isxdigit(lexer->lookahead)) {
        advance(lexer); // first hex digit
        if (lexer->eof(lexer)) {
          return false;
        }
        if (isxdigit(lexer->lookahead)) {
          advance(lexer); // second hex digit
        }
      } else {
        advance(lexer); // other escape char
      }
    } else {
      advance(lexer); // regular char
    }
  }
  if (lexer->eof(lexer)) {
    return false;
  }
  advance(lexer); // closing quote
  return true;
}

static void parse_line_comment(TSLexer *lexer) {
  advance(lexer); // ';'
  while (!lexer->eof(lexer) && lexer->lookahead != '\n') {
    advance(lexer); // regular char
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
  advance(lexer); // '@'
  // Consume annotation id (keyword or quoted string after @)
  if (lexer->lookahead == '"') {
    if (!parse_string(lexer)) {
      return false;
    }
  } else {
    while (!lexer->eof(lexer) && lexer->lookahead != ' ' &&
           lexer->lookahead != '\t' && lexer->lookahead != '\n' &&
           lexer->lookahead != '\r' && lexer->lookahead != '(' &&
           lexer->lookahead != ')') {
      advance(lexer);
    }
  }
  // Scan balanced content until matching ')'.
  // Handle block comments (;...;), line comments ;;, and strings "...".
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
      advance(lexer); // regular char
    }
  }
  return depth == 0;
}

bool tree_sitter_wat_external_scanner_scan(void *payload, TSLexer *lexer,
                                           const bool *valid_symbols) {
  (void)payload;

  // Skip whitespace before checking for '(' opener.
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t' ||
         lexer->lookahead == '\n' || lexer->lookahead == '\r') {
    skip_ws(lexer);
  }

  if (lexer->lookahead != '(') {
    return false;
  }

  lexer->mark_end(lexer);
  advance(lexer);

  // '(;' — block comment opener
  if (lexer->lookahead == ';' && valid_symbols[BLOCK_COMMENT]) {
    if (!parse_block_comment(lexer)) {
      return false;
    }
    lexer->mark_end(lexer);
    lexer->result_symbol = BLOCK_COMMENT;
    return true;
  }

  // '(@' — annotation opener: scan balanced parens to closing ')'
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
