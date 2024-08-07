// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "avInputOutputFormat.h"

extern "C" {
#include "libavformat/avformat.h"
}

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

  if (Name == nullptr) {
    return 0;
  }
  return strlen(Name);
}

Expect<int32_t> AVInputFormatName::body(const Runtime::CallingFrame &Frame,
                                        uint32_t AVInputFormatId,
                                        uint32_t NamePtr, uint32_t NameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameBuf, MemInst, char, NamePtr, NameLen, "");
  FFMPEG_PTR_FETCH(AvInputFormat, AVInputFormatId, AVInputFormat);

  const char *Name = AvInputFormat->name;
  std::copy_n(Name, NameLen, NameBuf.data());
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVOutputFormatName::body(const Runtime::CallingFrame &Frame,
                                         uint32_t AVOutputFormatId,
                                         uint32_t NamePtr, uint32_t NameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameBuf, MemInst, char, NamePtr, NameLen, "");
  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);

  const char *Name = AvOutputFormat->name;
  std::copy_n(Name, NameLen, NameBuf.data());
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

  if (LongName == nullptr) {
    return 0;
  }
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
  std::copy_n(LongName, LongNameLen, LongNameBuf.data());
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
  std::copy_n(LongName, LongNameLen, LongNameBuf.data());
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

  if (Extensions == nullptr) {
    return 0;
  }
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
  std::copy_n(Extensions, ExtensionsLen, ExtensionsBuf.data());
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
  std::copy_n(Extensions, ExtensionsLen, ExtensionsBuf.data());
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

  if (MimeType == nullptr) {
    return 0;
  }
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
  std::copy_n(MimeType, MimeTypeLen, MimeTypeBuf.data());
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
  std::copy_n(MimeType, MimeTypeLen, MimeTypeBuf.data());
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

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
