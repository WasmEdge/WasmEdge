#include "avDictionary.h"

extern "C"{
#include "libavutil/dict.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVUtil {

Expect<int32_t> AVDictNew::body(const Runtime::CallingFrame &Frame, uint32_t DictPtr){

  MEMINST_CHECK(MemInst, Frame, 0)
  MEM_PTR_CHECK(DictId, MemInst, uint32_t, DictPtr,
                "Failed to access Memory for AVDict")

  AVDictionary* AvDictionary = NULL;
  FFMPEG_PTR_STORE(&AvDictionary,DictId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVDictSet::body(const Runtime::CallingFrame &Frame, uint32_t DictId,uint32_t KeyPtr,uint32_t KeyLen,uint32_t ValuePtr,uint32_t ValueLen, int32_t Flags){

  MEMINST_CHECK(MemInst,Frame,0);
  MEM_PTR_CHECK(KeyId, MemInst, uint32_t , KeyPtr,
                "Failed when accessing the return Key memory");
  MEM_PTR_CHECK(ValueId, MemInst, uint32_t , ValuePtr,
                "Failed when accessing the return Value memory");

  std::string Key;
  std::string Value;
  std::copy_n(KeyId, KeyLen, std::back_inserter(Key));
  std::copy_n(ValueId, ValueLen, std::back_inserter(Value));
  FFMPEG_PTR_FETCH(AvDict,DictId,AVDictionary);
  // I changed AVFormat_func.cpp file. CHeck that once. Trying to store AVDictionary* instead of AVDictionary**

  return av_dict_set(&AvDict,Key.c_str(),Value.c_str(),Flags);
}
//
//Expect<int32_t> AVDictGet::body(const Runtime::CallingFrame &Frame,uint32_t DictId,uint32_t KeyPtr,uint32_t KeyLen,uint32_t PrevDictEntryId,uint32_t Flags){
//
//  MEMINST_CHECK(MemInst,Frame,0);
//  MEM_PTR_CHECK(KeyId, MemInst, uint32_t , KeyPtr,
//                "Failed when accessing the return Key memory");
//  MEM_PTR_CHECK(CurrDictEntryId, MemInst, uint32_t , CurrDictEntryPtr,
//                "Failed when accessing the return CurrDictEntry memory");
//
//  FFMPEG_PTR_FETCH(AvDict,DictId,const AVDictionary);
//  FFMPEG_PTR_FETCH(PrevAvDictEntry,PrevDictEntryId,const AVDictionaryEntry);
//
//  std::string Key;
//  std::copy_n(KeyId, KeyLen, std::back_inserter(Key));
//  // Change this.
//  return 1;
//}
//
Expect<int32_t> AVDictCopy::body(const Runtime::CallingFrame &, uint32_t DestDictId,uint32_t SrcDictId,uint32_t Flags){

  FFMPEG_PTR_FETCH(SrcAvDict,SrcDictId,AVDictionary);
  FFMPEG_PTR_FETCH(DestAvDict,DestDictId,AVDictionary);

  return av_dict_copy(&DestAvDict,SrcAvDict,Flags);
}

Expect<int32_t> AVDictFree::body(const Runtime::CallingFrame &,uint32_t DictId){

  FFMPEG_PTR_FETCH(AvDict,DictId,AVDictionary);
  av_dict_free(&AvDict);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVUtil
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
