// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
    FFMPEG_PTR_CHECK(AvInputFormat, 0);
    Name = AvInputFormat->name;
  } else {
    FFMPEG_PTR_FETCH(AvOutputFormat, AVIOFormatId, AVOutputFormat);
    FFMPEG_PTR_CHECK(AvOutputFormat, 0);
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
  FFMPEG_PTR_CHECK(AvInputFormat, static_cast<int32_t>(ErrNo::InternalError));

  const char *Name = AvInputFormat->name;
  copyCStringToBuffer(NameBuf.data(), NameLen, Name);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVOutputFormatName::body(const Runtime::CallingFrame &Frame,
                                         uint32_t AVOutputFormatId,
                                         uint32_t NamePtr, uint32_t NameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(NameBuf, MemInst, char, NamePtr, NameLen, "");
  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);
  FFMPEG_PTR_CHECK(AvOutputFormat, static_cast<int32_t>(ErrNo::InternalError));

  const char *Name = AvOutputFormat->name;
  copyCStringToBuffer(NameBuf.data(), NameLen, Name);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVIOFormatLongNameLength::body(const Runtime::CallingFrame &,
                                               uint32_t AVIOFormatId,
                                               uint32_t FormatType) {
  const char *LongName;

  if (FormatType == 0) {
    FFMPEG_PTR_FETCH(AvInputFormat, AVIOFormatId, AVInputFormat);
    FFMPEG_PTR_CHECK(AvInputFormat, 0);
    LongName = AvInputFormat->long_name;
  } else {
    FFMPEG_PTR_FETCH(AvOutputFormat, AVIOFormatId, AVOutputFormat);
    FFMPEG_PTR_CHECK(AvOutputFormat, 0);
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
  FFMPEG_PTR_CHECK(AvInputFormat, static_cast<int32_t>(ErrNo::InternalError));

  const char *LongName = AvInputFormat->long_name;
  copyCStringToBuffer(LongNameBuf.data(), LongNameLen, LongName);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVOutputFormatLongName::body(const Runtime::CallingFrame &Frame,
                                             uint32_t AVOutputFormatId,
                                             uint32_t LongNamePtr,
                                             uint32_t LongNameLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(LongNameBuf, MemInst, char, LongNamePtr, LongNameLen, "");
  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);
  FFMPEG_PTR_CHECK(AvOutputFormat, static_cast<int32_t>(ErrNo::InternalError));

  const char *LongName = AvOutputFormat->long_name;
  copyCStringToBuffer(LongNameBuf.data(), LongNameLen, LongName);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVIOFormatExtensionsLength::body(const Runtime::CallingFrame &,
                                                 uint32_t AVIOFormatId,
                                                 uint32_t FormatType) {
  const char *Extensions;

  if (FormatType == 0) {
    FFMPEG_PTR_FETCH(AvInputFormat, AVIOFormatId, AVInputFormat);
    FFMPEG_PTR_CHECK(AvInputFormat, 0);
    Extensions = AvInputFormat->extensions;
  } else {
    FFMPEG_PTR_FETCH(AvOutputFormat, AVIOFormatId, AVOutputFormat);
    FFMPEG_PTR_CHECK(AvOutputFormat, 0);
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
  FFMPEG_PTR_CHECK(AvInputFormat, static_cast<int32_t>(ErrNo::InternalError));

  const char *Extensions = AvInputFormat->extensions;
  copyCStringToBuffer(ExtensionsBuf.data(), ExtensionsLen, Extensions);
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
  FFMPEG_PTR_CHECK(AvOutputFormat, static_cast<int32_t>(ErrNo::InternalError));

  const char *Extensions = AvOutputFormat->extensions;
  copyCStringToBuffer(ExtensionsBuf.data(), ExtensionsLen, Extensions);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVIOFormatMimeTypeLength::body(const Runtime::CallingFrame &,
                                               uint32_t AVIOFormatId,
                                               uint32_t FormatType) {
  const char *MimeType;

  if (FormatType == 0) {
    FFMPEG_PTR_FETCH(AvInputFormat, AVIOFormatId, AVInputFormat);
    FFMPEG_PTR_CHECK(AvInputFormat, 0);
    MimeType = AvInputFormat->mime_type;
  } else {
    FFMPEG_PTR_FETCH(AvOutputFormat, AVIOFormatId, AVOutputFormat);
    FFMPEG_PTR_CHECK(AvOutputFormat, 0);
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
  FFMPEG_PTR_CHECK(AvInputFormat, static_cast<int32_t>(ErrNo::InternalError));

  const char *MimeType = AvInputFormat->mime_type;
  copyCStringToBuffer(MimeTypeBuf.data(), MimeTypeLen, MimeType);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVOutputFormatMimeType::body(const Runtime::CallingFrame &Frame,
                                             uint32_t AVOutputFormatId,
                                             uint32_t MimeTypePtr,
                                             uint32_t MimeTypeLen) {
  MEMINST_CHECK(MemInst, Frame, 0);
  MEM_SPAN_CHECK(MimeTypeBuf, MemInst, char, MimeTypePtr, MimeTypeLen, "");
  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);
  FFMPEG_PTR_CHECK(AvOutputFormat, static_cast<int32_t>(ErrNo::InternalError));

  const char *MimeType = AvOutputFormat->mime_type;
  copyCStringToBuffer(MimeTypeBuf.data(), MimeTypeLen, MimeType);
  return static_cast<int32_t>(ErrNo::Success);
}

Expect<int32_t> AVOutputFormatFlags::body(const Runtime::CallingFrame &,
                                          uint32_t AVOutputFormatId) {
  FFMPEG_PTR_FETCH(AvOutputFormat, AVOutputFormatId, AVOutputFormat);
  FFMPEG_PTR_CHECK(AvOutputFormat, 0);
  return AvOutputFormat->flags;
}

Expect<int32_t> AVInputOutputFormatFree::body(const Runtime::CallingFrame &,
                                              uint32_t AVInputOutputId) {
  // The id may hold either an AVInputFormat or an AVOutputFormat (both
  // library-owned statics; freeing only drops the id), so accept either type.
  // Id 0 and stale ids stay no-ops, matching the other free wrappers.
  FFMPEG_PTR_FETCH(AvInputFormat, AVInputOutputId, AVInputFormat);
  FFMPEG_PTR_FETCH(AvOutputFormat, AVInputOutputId, AVOutputFormat);
  if (AvInputFormat == nullptr && AvOutputFormat == nullptr) {
    if (Env.get()->fetchData(AVInputOutputId) != nullptr) {
      return static_cast<int32_t>(ErrNo::InternalError);
    }
    return static_cast<int32_t>(ErrNo::Success);
  }
  FFMPEG_PTR_DELETE(AVInputOutputId);
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
