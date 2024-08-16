// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVChapterId : public HostFunction<AVChapterId> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx);
};

class AVChapterSetId : public HostFunction<AVChapterSetId> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx,
                       int64_t ChapterId);
};

class AVChapterTimebase : public HostFunction<AVChapterTimebase> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t NumPtr,
                       uint32_t DenPtr, uint32_t AvFormatCtxId,
                       uint32_t ChapterIdx);
};

class AVChapterSetTimebase : public HostFunction<AVChapterSetTimebase> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t Num,
                       int32_t Den, uint32_t AvFormatCtxId,
                       uint32_t ChapterIdx);
};

class AVChapterStart : public HostFunction<AVChapterStart> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx);
};

class AVChapterSetStart : public HostFunction<AVChapterSetStart> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx,
                       int64_t StartValue);
};

class AVChapterEnd : public HostFunction<AVChapterEnd> {
public:
  using HostFunction::HostFunction;
  Expect<int64_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx);
};

class AVChapterSetEnd : public HostFunction<AVChapterSetEnd> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx,
                       int64_t EndValue);
};

class AVChapterMetadata : public HostFunction<AVChapterMetadata> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx,
                       uint32_t DictPtr);
};

class AVChapterSetMetadata : public HostFunction<AVChapterSetMetadata> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AvFormatCtxId, uint32_t ChapterIdx,
                       uint32_t DictId);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
