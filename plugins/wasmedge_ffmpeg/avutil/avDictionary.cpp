// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avDictionary.h"

extern "C" {
#include "libavutil/dict.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<int32_t> AVDictSet::body(const Runtime::CallingFrame &Frame,
                                uint32_t DictPtr, uint32_t KeyPtr,
                                uint32_t KeyLen, uint32_t ValuePtr,
                                uint32_t ValueLen, int32_t Flags) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(KeyBuf, MemInst, char, KeyPtr,
                "Failed when accessing the return Key memory"sv);
  MEM_PTR_CHECK(ValueBuf, MemInst, char, ValuePtr,
                "Failed when accessing the return Value memory"sv);
  MEM_PTR_CHECK(DictId, MemInst, uint32_t, DictPtr,
                "Failed to access Memory for AVDict"sv)

  std::string Key;
  std::string Value;
  std::copy_n(KeyBuf, KeyLen, std::back_inserter(Key));
  std::copy_n(ValueBuf, ValueLen, std::back_inserter(Value));

  int Res = 0;

  // Using Maybe::uninit(); in Rust. If Uninitialized, zero is
  // passed. Else the Ptr contains a Number.
  if (*DictId) {
    FFMPEG_PTR_FETCH(AvDict, *DictId, AVDictionary *);
    Res = av_dict_set(AvDict, Key.c_str(), Value.c_str(), Flags);
  } else {
    AVDictionary **AvDict =
        static_cast<AVDictionary **>(av_mallocz(sizeof(AVDictionary *)));
    Res = av_dict_set(AvDict, Key.c_str(), Value.c_str(), Flags);
    FFMPEG_PTR_STORE(AvDict, DictId);
  }

  return Res;
}

Expect<int32_t> AVDictCopy::body(const Runtime::CallingFrame &Frame,
                                 uint32_t DestDictPtr, uint32_t SrcDictId,
                                 uint32_t Flags) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(DestDictId, MemInst, uint32_t, DestDictPtr,
                "Failed to access Memory for AVDict"sv)

  FFMPEG_PTR_FETCH(SrcAvDict, SrcDictId, AVDictionary *);

  int Res = 0;

  if (SrcAvDict == nullptr) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  if (*DestDictId) {
    FFMPEG_PTR_FETCH(DestAvDict, *DestDictId, AVDictionary *);
    Res = av_dict_copy(DestAvDict, *SrcAvDict, Flags);
  } else {
    AVDictionary **DestAvDict =
        static_cast<AVDictionary **>(av_mallocz(sizeof(AVDictionary *)));
    av_dict_copy(DestAvDict, *SrcAvDict, Flags);
    FFMPEG_PTR_STORE(DestAvDict, DestDictId);
  }

  return Res;
}

Expect<int32_t> AVDictGet::body(const Runtime::CallingFrame &Frame,
                                uint32_t DictId, uint32_t KeyPtr,
                                uint32_t KeyLen, uint32_t PrevDictEntryIdx,
                                uint32_t Flags, uint32_t KeyLenPtr,
                                uint32_t ValueLenPtr) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(KeyStr, MemInst, char, KeyPtr,
                "Failed when accessing the return Key memory"sv);
  MEM_PTR_CHECK(KeyLenId, MemInst, uint32_t, KeyLenPtr,
                "Failed when accessing the return KeyLen memory"sv);
  MEM_PTR_CHECK(ValueLenId, MemInst, uint32_t, ValueLenPtr,
                "Failed when accessing the return ValueLen memory"sv);

  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);

  // If Dict Not created return (i.e. 0 is passed as AVDictId)
  if (AvDict == nullptr) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  std::string Key;
  std::copy_n(KeyStr, KeyLen, std::back_inserter(Key));

  AVDictionaryEntry *DictEntry = nullptr;
  uint32_t Curr = 0;
  while (Curr <= PrevDictEntryIdx) {
    DictEntry = av_dict_get(*AvDict, Key.c_str(), DictEntry, Flags);
    Curr++;
  }

  if (DictEntry == nullptr) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  *KeyLenId = strlen(DictEntry->key);
  *ValueLenId = strlen(DictEntry->value);
  return Curr;
}

Expect<int32_t> AVDictGetKeyValue::body(
    const Runtime::CallingFrame &Frame, uint32_t DictId, uint32_t KeyPtr,
    uint32_t KeyLen, uint32_t ValBufPtr, uint32_t ValBufLen, uint32_t KeyBufPtr,
    uint32_t KeyBufLen, uint32_t PrevDictEntryIdx, uint32_t Flags) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(KeyStr, MemInst, char, KeyPtr,
                "Failed when accessing the return Key memory"sv);
  MEM_SPAN_CHECK(KeyBuf, MemInst, char, KeyBufPtr, KeyBufLen, "");
  MEM_SPAN_CHECK(ValBuf, MemInst, char, ValBufPtr, ValBufLen, "");

  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);

  // If Dict Not created return (i.e. 0 is passed as AVDictId)
  if (AvDict == nullptr) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }

  std::string Key;
  std::copy_n(KeyStr, KeyLen, std::back_inserter(Key));

  AVDictionaryEntry *DictEntry = nullptr;
  uint32_t Curr = 0;
  while (Curr <= PrevDictEntryIdx) {
    DictEntry = av_dict_get(*AvDict, Key.c_str(), DictEntry, Flags);
    Curr++;
  }
  if (DictEntry == nullptr) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  std::copy_n(DictEntry->value, strlen(DictEntry->value), ValBuf.data());
  std::copy_n(DictEntry->key, strlen(DictEntry->key), KeyBuf.data());
  return Curr;
}

Expect<int32_t> AVDictFree::body(const Runtime::CallingFrame &,
                                 uint32_t DictId) {
  if (DictId == 0) {
    return static_cast<int32_t>(ErrNo::Success);
  }
  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);
  av_dict_free(AvDict);
  FFMPEG_PTR_DELETE(DictId);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
