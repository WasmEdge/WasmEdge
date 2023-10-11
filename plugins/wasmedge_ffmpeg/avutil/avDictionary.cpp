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
                "Failed when accessing the return Key memory");
  MEM_PTR_CHECK(ValueBuf, MemInst, char, ValuePtr,
                "Failed when accessing the return Value memory");
  MEM_PTR_CHECK(DictId, MemInst, uint32_t, DictPtr,
                "Failed to access Memory for AVDict")

  std::string Key;
  std::string Value;
  std::copy_n(KeyBuf, KeyLen, std::back_inserter(Key));
  std::copy_n(ValueBuf, ValueLen, std::back_inserter(Value));

  int res;
  if (*DictId) {
    FFMPEG_PTR_FETCH(AvDict, *DictId, AVDictionary *);
    res = av_dict_set(AvDict, Key.c_str(), Value.c_str(), Flags);
  } else {
    AVDictionary **AvDict = (AVDictionary **)malloc(sizeof(AVDictionary **));
    res = av_dict_set(AvDict, Key.c_str(), Value.c_str(), Flags);
    FFMPEG_PTR_STORE(AvDict, DictId);
  }

  return res;
}

Expect<int32_t> AVDictCopy::body(const Runtime::CallingFrame &Frame,
                                 uint32_t DestDictPtr, uint32_t SrcDictId,
                                 uint32_t Flags) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(DestDictId, MemInst, uint32_t, DestDictPtr,
                "Failed to access Memory for AVDict")

  FFMPEG_PTR_FETCH(SrcAvDict, SrcDictId, AVDictionary *);

  int res;
  if (*DestDictId) {
    FFMPEG_PTR_FETCH(DestAvDict, *DestDictId, AVDictionary *);
    res = av_dict_copy(DestAvDict, *SrcAvDict, Flags);
  } else {
    AVDictionary **DestAvDict =
        (AVDictionary **)malloc(sizeof(AVDictionary **));
    av_dict_copy(DestAvDict, *SrcAvDict, Flags);
    FFMPEG_PTR_STORE(DestAvDict, DestDictId);
  }

  return res;
}

Expect<int32_t> AVDictGet::body(const Runtime::CallingFrame &Frame,
                                uint32_t DictId, uint32_t KeyPtr,
                                uint32_t KeyLen, uint32_t PrevDictEntryIdx,
                                uint32_t Flags, uint32_t KeyLenPtr,
                                uint32_t ValueLenPtr) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(KeyStr, MemInst, char, KeyPtr,
                "Failed when accessing the return Key memory");
  MEM_PTR_CHECK(KeyLenId, MemInst, uint32_t, KeyLenPtr,
                "Failed when accessing the return KeyLen memory");
  MEM_PTR_CHECK(ValueLenId, MemInst, uint32_t, ValueLenPtr,
                "Failed when accessing the return ValueLen memory");

  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);

  std::string Key;
  std::copy_n(KeyStr, KeyLen, std::back_inserter(Key));

  AVDictionaryEntry *DictEntry = nullptr;
  uint32_t curr = 0;
  while (curr <= PrevDictEntryIdx) {
    DictEntry = av_dict_get(*AvDict, Key.c_str(), DictEntry, Flags);
    curr++;
  }

  if (DictEntry == nullptr)
    return -1;

  *KeyLenId = strlen(DictEntry->key);
  *ValueLenId = strlen(DictEntry->value);
  return curr;
}

Expect<int32_t> AVDictGetKeyValue::body(
    const Runtime::CallingFrame &Frame, uint32_t DictId, uint32_t KeyPtr,
    uint32_t KeyLen, uint32_t ValBufPtr, uint32_t ValBufLen, uint32_t KeyBufPtr,
    uint32_t KeyBufLen, uint32_t PrevDictEntryIdx, uint32_t Flags) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(KeyStr, MemInst, char, KeyPtr,
                "Failed when accessing the return Key memory");
  MEM_SPAN_CHECK(KeyBuf, MemInst, char, KeyBufPtr, KeyBufLen, "");
  MEM_SPAN_CHECK(ValBuf, MemInst, char, ValBufPtr, ValBufLen, "");

  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);

  std::string Key;
  std::copy_n(KeyStr, KeyLen, std::back_inserter(Key));

  AVDictionaryEntry *DictEntry = nullptr;
  uint32_t curr = 0;
  while (curr <= PrevDictEntryIdx) {
    DictEntry = av_dict_get(*AvDict, Key.c_str(), DictEntry, Flags);
    curr++;
  }
  if (DictEntry == nullptr)
    return -1;
  memmove(ValBuf.data(), DictEntry->value, strlen(DictEntry->value));
  memmove(KeyBuf.data(), DictEntry->key, strlen(DictEntry->key));
  return curr;
}

Expect<int32_t> AVDictFree::body(const Runtime::CallingFrame &,
                                 uint32_t DictId) {

  if (DictId == 0)
    return static_cast<int32_t>(ErrNo::Success);
  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);
  av_dict_free(AvDict);
  FFMPEG_PTR_DELETE(DictId);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
