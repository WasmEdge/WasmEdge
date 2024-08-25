// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVFormatOpenInput : public HostFunction<AVFormatOpenInput> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxPtr, uint32_t UrlPtr,
                       uint32_t UrlSize, uint32_t AvInputFormatId,
                       uint32_t AvDictionaryId);
};

class AVFormatFindStreamInfo : public HostFunction<AVFormatFindStreamInfo> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       uint32_t AvDictionaryId);
};

class AVFormatCloseInput : public HostFunction<AVFormatCloseInput> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVReadPause : public HostFunction<AVReadPause> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId);
};

class AVReadPlay : public HostFunction<AVReadPlay> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId);
};

class AVFormatSeekFile : public HostFunction<AVFormatSeekFile> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       uint32_t StreamIdx, int64_t MinTs, int64_t Ts,
                       int64_t MaxTs, int32_t Flags);
};

class AVDumpFormat : public HostFunction<AVDumpFormat> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, int32_t Idx, uint32_t UrlPtr,
                       uint32_t UrlSize, int32_t IsOutput);
};

class AVFormatFreeContext : public HostFunction<AVFormatFreeContext> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxPtr);
};

class AVFindBestStream : public HostFunction<AVFindBestStream> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       int32_t MediaTypeId, int32_t WantedStream,
                       int32_t RelatedStream, uint32_t DecoderRetId,
                       int32_t Flags);
};

class AVReadFrame : public HostFunction<AVReadFrame> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       uint32_t PacketId);
};

class AVIOClose : public HostFunction<AVIOClose> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVFormatNetworkInit : public HostFunction<AVFormatNetworkInit> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFormatNetworkDeInit : public HostFunction<AVFormatNetworkDeInit> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFormatWriteHeader : public HostFunction<AVFormatWriteHeader> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t DictId);
};

class AVFormatWriteTrailer : public HostFunction<AVFormatWriteTrailer> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId);
};

class AVFormatAllocOutputContext2
    : public HostFunction<AVFormatAllocOutputContext2> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxPtr, uint32_t AVOutputFormatId,
                       uint32_t FormatNamePtr, uint32_t FormatLen,
                       uint32_t FileNamePtr, uint32_t FileNameLen);
};

class AVIOOpen : public HostFunction<AVIOOpen> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t FileNamePtr,
                       uint32_t FileNameLen, int32_t Flags);
};

class AVIOOpen2 : public HostFunction<AVIOOpen2> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxtId, uint32_t UrlPtr,
                       uint32_t UrlLen, int32_t Flags,
                       uint32_t AVIOInterruptCBId, uint32_t AVDictionaryId);
};

class AVFormatVersion : public HostFunction<AVFormatVersion> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class AVChapterMallocz : public HostFunction<AVChapterMallocz> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVChapterPtr);
};

class AVChapterDynarrayAdd : public HostFunction<AVChapterDynarrayAdd> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       int32_t NbChaptersPtr, uint32_t AvChapterId);
};

class AVFreeP : public HostFunction<AVFreeP> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvChapterId);
};

class AVInterleavedWriteFrame : public HostFunction<AVInterleavedWriteFrame> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       uint32_t AvPacketId);
};

class AVWriteFrame : public HostFunction<AVWriteFrame> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AvFormatCtxId,
                       uint32_t AvPacketId);
};

class AVFormatNewStream : public HostFunction<AVFormatNewStream> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVFormatCtxId, uint32_t AVCodecId);
};

class AVGuessCodec : public HostFunction<AVGuessCodec> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AVInputOutputId, uint32_t ShortNamePtr,
                        uint32_t ShortNameLen, uint32_t FileNamePtr,
                        uint32_t FileNameLen, uint32_t MimeTypePtr,
                        uint32_t MimeTypeLen, int32_t MediaTypeId);
};

class AVFormatConfigurationLength
    : public HostFunction<AVFormatConfigurationLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFormatConfiguration : public HostFunction<AVFormatConfiguration> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ConfigPtr,
                       uint32_t ConfigLen);
};

class AVFormatLicenseLength : public HostFunction<AVFormatLicenseLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class AVFormatLicense : public HostFunction<AVFormatLicense> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t LicensePtr,
                       uint32_t LicenseLen);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
