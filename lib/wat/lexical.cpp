// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wat/lexical.h"
#include "common/utf8.h"
#include "wat/wat_util.h"

#include <cctype>

using namespace std::string_view_literals;

namespace WasmEdge {
namespace WAT {

namespace {

// Classify a non-ASCII byte sequence at Src[Pos] and move Pos past it.
// For valid UTF-8, the result is WatIllegalCharacter. Such a sequence is legal
// Unicode, but WAT does not permit non-ASCII outside a string. For bytes that
// are not valid, the result is MalformedUTF8.
ErrCode::Value classifyNonASCII(std::string_view Src, size_t &Pos) {
  if (const size_t Len = utf8SequenceLength(Src, Pos); Len != 0) {
    Pos += Len;
    return ErrCode::Value::WatIllegalCharacter;
  }
  ++Pos;
  return ErrCode::Value::MalformedUTF8;
}

// Examine the id of an annotation in Text. The id starts at the position I,
// directly after the "(@". Reject an empty id, such as (@), (@ x), or (@""), a
// first character that is not an idchar, a control character in a quoted id,
// and a quoted id that does not decode to valid UTF-8. Give the position after
// the id.
Expect<size_t> checkAnnotationIdAt(std::string_view Text, size_t I) {
  if (I >= Text.size() ||
      isAnnotationIdEnd(static_cast<unsigned char>(Text[I]))) {
    return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
  }
  if (Text[I] != '"') {
    // An unquoted id is idchar+, and it ends at the first character that is
    // not an idchar, as in the external scanner and the reference
    // interpreter. A character that cannot start an id, such as '{', leaves
    // the annotation with no id. The rest of the annotation, such as the "{b"
    // of (@a{b), is the payload.
    if (!isIdChar(static_cast<unsigned char>(Text[I]))) {
      return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
    }
    ++I;
    while (I < Text.size() && isIdChar(static_cast<unsigned char>(Text[I]))) {
      ++I;
    }
    return I;
  }
  size_t J = I + 1;
  if (J < Text.size() && Text[J] == '"') {
    return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
  }
  while (J < Text.size() && Text[J] != '"') {
    if (isControlChar(static_cast<unsigned char>(Text[J]))) {
      return Unexpect(ErrCode::Value::WatEmptyAnnotationId);
    }
    // Skip the character after a backslash, so that an escaped quote does
    // not end the id.
    J += Text[J] == '\\' ? 2 : 1;
  }
  if (J >= Text.size()) {
    return Unexpect(ErrCode::Value::WatUnclosedString);
  }
  // An id is a name, so its decoded bytes must be valid UTF-8. A \HH escape
  // can give a byte of a multi-byte sequence.
  if (!parseString(Text.substr(I, J + 1 - I), /*ValidateUTF8=*/true)) {
    return Unexpect(ErrCode::Value::MalformedUTF8);
  }
  return J + 1;
}

// Examine the payload Text of an annotation. The payload of an unknown
// annotation has no meaning, but its tokens still follow the lexical rules.
// Each string must be a valid string literal. A quoted identifier $"..." and
// a nested annotation with a quoted id follow the rules of a quoted id, as in
// the reference lexer. Thus a string in the payload can hold any bytes, but a
// quoted identifier or a nested quoted id cannot be empty, and it must be
// valid UTF-8. A nested "(@" with no quoted id is data, also with no id. The
// scan is linear, so the depth of the nested annotations does not bound the
// native stack. The external scanner already found the end of each string and
// each comment.
Expect<void> checkAnnotationPayload(std::string_view Text) {
  for (size_t I = 0; I < Text.size(); ++I) {
    const char C = Text[I];
    const char Next = I + 1 < Text.size() ? Text[I + 1] : '\0';
    if (C == '(' && Next == ';') {
      I = blockCommentEnd(Text, I + 2) - 1;
    } else if (C == ';' && Next == ';') {
      I = lineCommentEnd(Text, I + 2);
    } else if (C == '(' && Next == '@' && I + 2 < Text.size() &&
               Text[I + 2] == '"') {
      EXPECTED_TRY(auto End, checkAnnotationIdAt(Text, I + 2));
      I = End - 1;
    } else if (C == '"') {
      size_t J = I + 1;
      while (J < Text.size() && Text[J] != '"') {
        J += Text[J] == '\\' ? 2 : 1;
      }
      // A '$' that starts a token makes the string a quoted identifier.
      const bool IsId =
          I > 0 && Text[I - 1] == '$' &&
          (I < 2 || !isIdChar(static_cast<unsigned char>(Text[I - 2])));
      if (IsId && J == I + 1) {
        return Unexpect(ErrCode::Value::WatEmptyIdentifier);
      }
      EXPECTED_TRY(parseString(Text.substr(I, J + 1 - I), IsId));
      I = J;
    }
  }
  return {};
}
} // namespace

// The scan skips the strings and the comments, so a "(@" in AnnotationStarts
// is never data of a string or a comment. A raw search of the source also
// finds those.
Expect<void> scanSourceCharacters(std::string_view Source,
                                  std::vector<size_t> *AnnotationStarts) {
  auto *Data = reinterpret_cast<const unsigned char *>(Source.data());
  size_t Size = Source.size();
  bool InString = false;
  for (size_t I = 0; I < Size; ++I) {
    auto C = Data[I];
    if (InString) {
      if (C >= 0x80) {
        // In a string, valid non-ASCII UTF-8 is permitted, but malformed
        // UTF-8 is not permitted.
        auto Err = classifyNonASCII(Source, I);
        if (Err == ErrCode::Value::MalformedUTF8) {
          return Unexpect(Err);
        }
        // classifyNonASCII moved I past the sequence. Adjust I for the ++I of
        // the loop.
        --I;
        continue;
      }
      if (C == '"') {
        // If the count of the backslashes before the quote is even, then the
        // quote is real and has no escape. A count of 0 is even.
        size_t Backslashes = 0;
        while (Backslashes < I && Source[I - 1 - Backslashes] == '\\') {
          ++Backslashes;
        }
        if (Backslashes % 2 == 0) {
          InString = false;
        }
      }
      continue;
    }
    // Skip a comment. A line comment starts with ;; and ends at the end of
    // the line. A block comment has the form (; ... ;), and it can nest. Any
    // character can be in a comment, but the text must be valid UTF-8.
    const bool LineComment = C == ';' && I + 1 < Size && Data[I + 1] == ';';
    if (LineComment || (C == '(' && I + 1 < Size && Data[I + 1] == ';')) {
      const size_t End = LineComment ? lineCommentEnd(Source, I + 2)
                                     : blockCommentEnd(Source, I + 2);
      if (!isValidUTF8(Source.substr(I + 2, End - I - 2))) {
        return Unexpect(ErrCode::Value::MalformedUTF8);
      }
      // Adjust I for the ++I of the outer loop. The loop then also skips the
      // newline at the end of a line comment, which is whitespace.
      I = LineComment ? End : End - 1;
      continue;
    }
    if (C == '"') {
      InString = true;
      continue;
    }
    if (AnnotationStarts && C == '(' && I + 1 < Size && Data[I + 1] == '@') {
      AnnotationStarts->push_back(I);
    }
    if (C >= 0x80) {
      return Unexpect(classifyNonASCII(Source, I));
    }
    if (isControlChar(C) && !isWatSpace(C)) {
      return Unexpect(ErrCode::Value::WatIllegalCharacter);
    }
  }
  return {};
}

// Examine the annotation Text: its id, then its payload with the nested
// annotations. The caller takes Text from an annotation node of the tree, so a
// (@ inside a comment, a string, or a name does not count.
Expect<void> checkAnnotation(std::string_view Text) {
  EXPECTED_TRY(auto End, checkAnnotationIdAt(Text, 2));
  return checkAnnotationPayload(Text.substr(End));
}

// Two tokens need whitespace between them. Tree-sitter can accept two adjacent
// tokens, for example "i32.const0" as "i32.const" and "0". The WAT spec does
// not permit this. The caller gives the texts of two consecutive leaf tokens
// with no gap between them.
Expect<void> checkAdjacentTokens(std::string_view PrevText,
                                 std::string_view CurrText) {
  // A '(' and a ')' can touch any token.
  if (PrevText == "("sv || PrevText == ")"sv || CurrText == "("sv ||
      CurrText == ")"sv) {
    return {};
  }
  // Two adjacent tokens that are not parentheses are malformed. This test
  // also rejects two adjacent strings such as "a""b". A SIMD lane needs an
  // integer. At such a position, a pattern that looks like a float, for
  // example "1" and ".5", gives an unexpected token. It does not give an
  // unknown operator.
  const auto IsDigit = [](char Ch) {
    return std::isdigit(static_cast<unsigned char>(Ch)) != 0;
  };
  if (!PrevText.empty() && !CurrText.empty() &&
      ((IsDigit(PrevText.back()) && CurrText.front() == '.') ||
       (PrevText.back() == '.' && IsDigit(CurrText.front())))) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  return Unexpect(ErrCode::Value::WatUnknownOperator);
}

} // namespace WAT
} // namespace WasmEdge
