// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ffmpeg_base.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace AVFormat {

class AVIOFormatNameLength : public HostFunction<AVIOFormatNameLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AVIOFormatId,
                       uint32_t FormatType);
};

class AVInputFormatName : public HostFunction<AVInputFormatName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t NamePtr,
                       uint32_t NameLen);
};

class AVOutputFormatName : public HostFunction<AVOutputFormatName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t NamePtr,
                       uint32_t NameLen);
};

class AVIOFormatLongNameLength : public HostFunction<AVIOFormatLongNameLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AVIOFormatId,
                       uint32_t FormatType);
};

class AVInputFormatLongName : public HostFunction<AVInputFormatLongName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t LongNamePtr,
                       uint32_t LongNameLen);
};

class AVOutputFormatLongName : public HostFunction<AVOutputFormatLongName> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t LongNamePtr,
                       uint32_t LongNameLen);
};

class AVIOFormatExtensionsLength
    : public HostFunction<AVIOFormatExtensionsLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AVIOFormatId,
                       uint32_t FormatType);
};

class AVInputFormatExtensions : public HostFunction<AVInputFormatExtensions> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t Extensions,
                       uint32_t ExtensionsLen);
};

class AVOutputFormatExtensions : public HostFunction<AVOutputFormatExtensions> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t Extensions,
                       uint32_t ExtensionsLen);
};

class AVIOFormatMimeTypeLength : public HostFunction<AVIOFormatMimeTypeLength> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &, uint32_t AVIOFormatId,
                       uint32_t FormatType);
};

class AVInputFormatMimeType : public HostFunction<AVInputFormatMimeType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t MimeTypePtr,
                       uint32_t MimeTypeLen);
};

class AVOutputFormatMimeType : public HostFunction<AVOutputFormatMimeType> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId, uint32_t MimeTypePtr,
                       uint32_t MimeTypeLen);
};

class AVOutputFormatFlags : public HostFunction<AVOutputFormatFlags> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputFormatId);
};

class AVInputOutputFormatFree : public HostFunction<AVInputOutputFormatFree> {
public:
  using HostFunction::HostFunction;
  Expect<int32_t> body(const Runtime::CallingFrame &Frame,
                       uint32_t AVInputOutputId);
};

} // namespace AVFormat
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
