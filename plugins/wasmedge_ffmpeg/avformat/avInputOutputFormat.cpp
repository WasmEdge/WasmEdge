
#include "avInputOutputFormat.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

Expect<int32_t> AVIOFormatNameLength::body(const Runtime::CallingFrame &,
                                           uint32_t AVIOFormatId,
                                           uint32_t FormatType) {

  const char *Name;

  if (FormatType == 0) {
    FFMPEG_PTR_FETCH(AvInputFormat, AVIOFormatId, AVInputFormat);
    Name = AvInputFormat->name;
  } else {
    FFMPEG_PTR_FETCH(AvOutputFormat, AVIOFormatId, AVOutputFormat);
    Name = AvOutputFormat->name;
  }

  if (Name == NULL)
    return 0;
  return strlen(Name);
}

Expect<int32_t> AVInputFormatName::body(const Runtime::CallingFrame &Frame,
                                        uint32_t AVInputFormatId,
                                        uint32_t NamePtr, uint32_t NameLen) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameBuf, MemInst, char, NamePtr, NameLen, "");
  FFMPEG_PTR_FETCH(AvInputFormat, AVInputFormatId, AVInputFormat);

  const char *name = AvInputFormat->name;
  memmove(NameBuf.data(), name, NameLen);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVOutputFormatName::body(const Runtime::CallingFrame &Frame,
                                         uint32_t AVOutputFormatId,
                                         uint32_t NamePtr, uint32_t NameLen) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameBuf, MemInst, char, NamePtr, NameLen, "");
  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);

  const char *name = AvOutputFormat->name;
  memmove(NameBuf.data(), name, NameLen);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVIOFormatLongNameLength::body(const Runtime::CallingFrame &,
                                               uint32_t AVIOFormatId,
                                               uint32_t FormatType) {

  const char *LongName;

  if (FormatType == 0) {
    FFMPEG_PTR_FETCH(AvInputFormat, AVIOFormatId, AVInputFormat);
    LongName = AvInputFormat->long_name;
  } else {
    FFMPEG_PTR_FETCH(AvOutputFormat, AVIOFormatId, AVOutputFormat);
    LongName = AvOutputFormat->long_name;
  }

  if (LongName == NULL)
    return 0;
  return strlen(LongName);
}

Expect<int32_t> AVInputFormatLongName::body(const Runtime::CallingFrame &Frame,
                                            uint32_t AVInputFormatId,
                                            uint32_t LongNamePtr,
                                            uint32_t LongNameLen) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(LongNameBuf, MemInst, char, LongNamePtr, LongNameLen, "");
  FFMPEG_PTR_FETCH(AvInputFormat, AVInputFormatId, AVInputFormat);

  const char *LongName = AvInputFormat->long_name;
  memmove(LongNameBuf.data(), LongName, LongNameLen);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVOutputFormatLongName::body(const Runtime::CallingFrame &Frame,
                                             uint32_t AVOutputFormatId,
                                             uint32_t LongNamePtr,
                                             uint32_t LongNameLen) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(LongNameBuf, MemInst, char, LongNamePtr, LongNameLen, "");
  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);

  const char *LongName = AvOutputFormat->long_name;
  memmove(LongNameBuf.data(), LongName, LongNameLen);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVIOFormatExtensionsLength::body(const Runtime::CallingFrame &,
                                                 uint32_t AVIOFormatId,
                                                 uint32_t FormatType) {

  const char *Extensions;

  if (FormatType == 0) {
    FFMPEG_PTR_FETCH(AvInputFormat, AVIOFormatId, AVInputFormat);
    Extensions = AvInputFormat->extensions;
  } else {
    FFMPEG_PTR_FETCH(AvOutputFormat, AVIOFormatId, AVOutputFormat);
    Extensions = AvOutputFormat->extensions;
  }

  if (Extensions == NULL)
    return 0;
  return strlen(Extensions);
}

Expect<int32_t>
AVInputFormatExtensions::body(const Runtime::CallingFrame &Frame,
                              uint32_t AVInputFormatId, uint32_t ExtensionsPtr,
                              uint32_t ExtensionsLen) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(ExtensionsBuf, MemInst, char, ExtensionsPtr, ExtensionsLen,
                 "");
  FFMPEG_PTR_FETCH(AvInputFormat, AVInputFormatId, AVInputFormat);

  const char *Extensions = AvInputFormat->extensions;
  memmove(ExtensionsBuf.data(), Extensions, ExtensionsLen);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t>
AVOutputFormatExtensions::body(const Runtime::CallingFrame &Frame,
                               uint32_t AVOutputFormatId,
                               uint32_t ExtensionsPtr, uint32_t ExtensionsLen) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(ExtensionsBuf, MemInst, char, ExtensionsPtr, ExtensionsLen,
                 "");
  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);

  const char *Extensions = AvOutputFormat->extensions;
  memmove(ExtensionsBuf.data(), Extensions, ExtensionsLen);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVIOFormatMimeTypeLength::body(const Runtime::CallingFrame &,
                                               uint32_t AVIOFormatId,
                                               uint32_t FormatType) {
  const char *MimeType;

  if (FormatType == 0) {
    FFMPEG_PTR_FETCH(AvInputFormat, AVIOFormatId, AVInputFormat);
    MimeType = AvInputFormat->mime_type;
  } else {
    FFMPEG_PTR_FETCH(AvOutputFormat, AVIOFormatId, AVOutputFormat);
    MimeType = AvOutputFormat->mime_type;
  }

  if (MimeType == NULL)
    return 0;
  return strlen(MimeType);
}

Expect<int32_t> AVInputFormatMimeType::body(const Runtime::CallingFrame &Frame,
                                            uint32_t AVInputFormatId,
                                            uint32_t MimeTypePtr,
                                            uint32_t MimeTypeLen) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(MimeTypeBuf, MemInst, char, MimeTypePtr, MimeTypeLen, "");
  FFMPEG_PTR_FETCH(AvInputFormat, AVInputFormatId, AVInputFormat);

  const char *MimeType = AvInputFormat->mime_type;
  memmove(MimeTypeBuf.data(), MimeType, MimeTypeLen);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVOutputFormatMimeType::body(const Runtime::CallingFrame &Frame,
                                             uint32_t AVOutputFormatId,
                                             uint32_t MimeTypePtr,
                                             uint32_t MimeTypeLen) {

  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(MimeTypeBuf, MemInst, char, MimeTypePtr, MimeTypeLen, "");
  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);

  const char *MimeType = AvOutputFormat->mime_type;
  memmove(MimeTypeBuf.data(), MimeType, MimeTypeLen);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVOutputFormatFlags::body(const Runtime::CallingFrame &,
                                          uint32_t AVOutputFormatId) {

  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);
  return AvOutputFormat->flags;
}

Expect<int32_t> AVInputOutputFormatFree::body(const Runtime::CallingFrame &,
                                              uint32_t AVInputOutputId) {
  FFMPEG_PTR_DELETE(AVInputOutputId);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<uint32_t> AVGuessCodec::body(const Runtime::CallingFrame &,
                                    uint32_t AVOutputFormatId) {

  //  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, const AVOutputFormat);
  //  enum AVCodecID const AvCodecId = av_guess_codec(AvOutputFormat, );
  //  return FFmpegUtils::CodecID::fromAVCodecID(AvCodecId);
  return 1;
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge