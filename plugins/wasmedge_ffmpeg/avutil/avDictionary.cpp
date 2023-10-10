#include "avDictionary.h"

extern "C" {
#include "libavutil/dict.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<int32_t> AVDictNew::body(const Runtime::CallingFrame &Frame,
                                uint32_t DictPtr) {

  MEMINST_CHECK(MemInst, Frame, 0)
  MEM_PTR_CHECK(DictId, MemInst, uint32_t, DictPtr,
                "Failed to access Memory for AVDict")

  AVDictionary *AvDictionary = NULL;
  FFMPEG_PTR_STORE(&AvDictionary, DictId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVDictSet::body(const Runtime::CallingFrame &Frame,
                                uint32_t DictId, uint32_t KeyPtr,
                                uint32_t KeyLen, uint32_t ValuePtr,
                                uint32_t ValueLen, int32_t Flags) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(KeyBuf, MemInst, char, KeyPtr,
                "Failed when accessing the return Key memory");
  MEM_PTR_CHECK(ValueBuf, MemInst, char, ValuePtr,
                "Failed when accessing the return Value memory");

  std::string Key;
  std::string Value;
  std::copy_n(KeyBuf, KeyLen, std::back_inserter(Key));
  std::copy_n(ValueBuf, ValueLen, std::back_inserter(Value));
  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);

  av_dict_set(AvDict, Key.c_str(), Value.c_str(), Flags);
  return av_dict_set(AvDict, Key.c_str(), Value.c_str(), Flags);
}

Expect<int32_t> AVDictGet::body(const Runtime::CallingFrame &Frame,
                                uint32_t DictId, uint32_t KeyPtr,
                                uint32_t KeyLen, uint32_t PrevDictEntryIdx,
                                uint32_t Flags) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_PTR_CHECK(KeyBuf, MemInst, char, KeyPtr,
                "Failed when accessing the return Key memory");
  MEM_PTR_CHECK(DictEntryIdx, MemInst, uint32_t, PrevDictEntryIdx,
                "Failed when accessing the return PrevDictEntry memory");

  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);

  std::string Key;
  std::copy_n(KeyBuf, KeyLen, std::back_inserter(Key));

  AVDictionaryEntry *DictEntry = NULL;
  uint32_t curr = 0;
  while (curr++ <= *DictEntryIdx)
    DictEntry = av_dict_get(*AvDict, Key.c_str(), DictEntry, Flags);

  if (DictEntry == NULL)
    return -1;
  return curr;
}

// Expect<int32_t> AVDictGetValue::body(const Runtime::CallingFrame &Frame,
//                                      uint32_t DictId, uint32_t KeyPtr,
//                                      uint32_t KeyLen, uint32_t ValPtr,
//                                      uint32_t ValLen, uint32_t
//                                      PrevDictEntryIdx, uint32_t Flags) {
//
//   MEMINST_CHECK(MemInst, Frame, 0);
//   MEM_PTR_CHECK(KeyBuf, MemInst, char, KeyPtr,
//                 "Failed when accessing the return Key memory");
//   MEM_PTR_CHECK(ValId, MemInst, char, ValPtr,
//                 "Failed when accessing the return Value memory");
//
//   MEM_PTR_CHECK(DictEntryIdx, MemInst, uint32_t, PrevDictEntryIdx,
//                 "Failed when accessing the return PrevDictEntry memory");
//   FFMPEG_PTR_FETCH(AvDict, DictId, const AVDictionary);
//
//   std::string Key;
//   std::copy_n(KeyBuf, KeyLen, std::back_inserter(Key));
//
//   AVDictionaryEntry *DictEntry =
//       av_dict_get(AvDict, Key.c_str(), DictEntry, Flags);
// }

Expect<int32_t> AVDictCopy::body(const Runtime::CallingFrame &,
                                 uint32_t DestDictId, uint32_t SrcDictId,
                                 uint32_t Flags) {

  FFMPEG_PTR_FETCH(SrcAvDict, SrcDictId, AVDictionary *);
  FFMPEG_PTR_FETCH(DestAvDict, DestDictId, AVDictionary *);

  return av_dict_copy(DestAvDict, *SrcAvDict, Flags);
}

Expect<int32_t> AVDictFree::body(const Runtime::CallingFrame &,
                                 uint32_t DictId) {

  FFMPEG_PTR_FETCH(AvDict, DictId, AVDictionary *);
  av_dict_free(AvDict);
  FFMPEG_PTR_DELETE(DictId);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
