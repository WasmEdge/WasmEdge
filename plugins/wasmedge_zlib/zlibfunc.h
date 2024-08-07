// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "zlibbase.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeZlibDeflateInit : public WasmEdgeZlib<WasmEdgeZlibDeflateInit> {
public:
  WasmEdgeZlibDeflateInit(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Level);
};

class WasmEdgeZlibDeflate : public WasmEdgeZlib<WasmEdgeZlibDeflate> {
public:
  WasmEdgeZlibDeflate(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Flush);
};

class WasmEdgeZlibDeflateEnd : public WasmEdgeZlib<WasmEdgeZlibDeflateEnd> {
public:
  WasmEdgeZlibDeflateEnd(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibInflateInit : public WasmEdgeZlib<WasmEdgeZlibInflateInit> {
public:
  WasmEdgeZlibInflateInit(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibInflate : public WasmEdgeZlib<WasmEdgeZlibInflate> {
public:
  WasmEdgeZlibInflate(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Flush);
};

class WasmEdgeZlibInflateEnd : public WasmEdgeZlib<WasmEdgeZlibInflateEnd> {
public:
  WasmEdgeZlibInflateEnd(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibDeflateInit2 : public WasmEdgeZlib<WasmEdgeZlibDeflateInit2> {
public:
  WasmEdgeZlibDeflateInit2(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Level, int32_t Method, int32_t WindowBits,
                       int32_t MemLevel, int32_t Strategy);
};

class WasmEdgeZlibDeflateSetDictionary
    : public WasmEdgeZlib<WasmEdgeZlibDeflateSetDictionary> {
public:
  WasmEdgeZlibDeflateSetDictionary(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       uint32_t DictionaryPtr, uint32_t DictLength);
};

class WasmEdgeZlibDeflateGetDictionary
    : public WasmEdgeZlib<WasmEdgeZlibDeflateGetDictionary> {
public:
  WasmEdgeZlibDeflateGetDictionary(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       uint32_t DictionaryPtr, uint32_t DictLengthPtr);
};

class WasmEdgeZlibDeflateCopy : public WasmEdgeZlib<WasmEdgeZlibDeflateCopy> {
public:
  WasmEdgeZlibDeflateCopy(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestPtr,
                       uint32_t SourcePtr);
};

class WasmEdgeZlibDeflateReset : public WasmEdgeZlib<WasmEdgeZlibDeflateReset> {
public:
  WasmEdgeZlibDeflateReset(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibDeflateParams
    : public WasmEdgeZlib<WasmEdgeZlibDeflateParams> {
public:
  WasmEdgeZlibDeflateParams(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Level, int32_t Strategy);
};

class WasmEdgeZlibDeflateTune : public WasmEdgeZlib<WasmEdgeZlibDeflateTune> {
public:
  WasmEdgeZlibDeflateTune(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t GoodLength, int32_t MaxLazy, int32_t NiceLength,
                       int32_t MaxChain);
};

// https://github.com/emscripten-core/emscripten/issues/17009
// Using 32bit, because on wasm-side it will be 32bit long
class WasmEdgeZlibDeflateBound : public WasmEdgeZlib<WasmEdgeZlibDeflateBound> {
public:
  WasmEdgeZlibDeflateBound(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       uint32_t SourceLen);
};

class WasmEdgeZlibDeflatePending
    : public WasmEdgeZlib<WasmEdgeZlibDeflatePending> {
public:
  WasmEdgeZlibDeflatePending(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       uint32_t PendingPtr, uint32_t BitsPtr);
};

class WasmEdgeZlibDeflatePrime : public WasmEdgeZlib<WasmEdgeZlibDeflatePrime> {
public:
  WasmEdgeZlibDeflatePrime(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Bits, int32_t Value);
};

class WasmEdgeZlibDeflateSetHeader
    : public WasmEdgeZlib<WasmEdgeZlibDeflateSetHeader> {
public:
  WasmEdgeZlibDeflateSetHeader(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       uint32_t HeadPtr);
};

class WasmEdgeZlibInflateInit2 : public WasmEdgeZlib<WasmEdgeZlibInflateInit2> {
public:
  WasmEdgeZlibInflateInit2(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t WindowBits);
};

class WasmEdgeZlibInflateSetDictionary
    : public WasmEdgeZlib<WasmEdgeZlibInflateSetDictionary> {
public:
  WasmEdgeZlibInflateSetDictionary(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       uint32_t DictionaryPtr, uint32_t DictLength);
};

class WasmEdgeZlibInflateGetDictionary
    : public WasmEdgeZlib<WasmEdgeZlibInflateGetDictionary> {
public:
  WasmEdgeZlibInflateGetDictionary(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       uint32_t DictionaryPtr, uint32_t DictLengthPtr);
};

class WasmEdgeZlibInflateSync : public WasmEdgeZlib<WasmEdgeZlibInflateSync> {
public:
  WasmEdgeZlibInflateSync(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibInflateCopy : public WasmEdgeZlib<WasmEdgeZlibInflateCopy> {
public:
  WasmEdgeZlibInflateCopy(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestPtr,
                       uint32_t SourcePtr);
};

class WasmEdgeZlibInflateReset : public WasmEdgeZlib<WasmEdgeZlibInflateReset> {
public:
  WasmEdgeZlibInflateReset(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibInflateReset2
    : public WasmEdgeZlib<WasmEdgeZlibInflateReset2> {
public:
  WasmEdgeZlibInflateReset2(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t WindowBits);
};

class WasmEdgeZlibInflatePrime : public WasmEdgeZlib<WasmEdgeZlibInflatePrime> {
public:
  WasmEdgeZlibInflatePrime(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Bits, int32_t Value);
};

class WasmEdgeZlibInflateMark : public WasmEdgeZlib<WasmEdgeZlibInflateMark> {
public:
  WasmEdgeZlibInflateMark(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibInflateGetHeader
    : public WasmEdgeZlib<WasmEdgeZlibInflateGetHeader> {
public:
  WasmEdgeZlibInflateGetHeader(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       uint32_t HeadPtr);
};

class WasmEdgeZlibInflateBackInit
    : public WasmEdgeZlib<WasmEdgeZlibInflateBackInit> {
public:
  WasmEdgeZlibInflateBackInit(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t WindowBits, uint32_t WindowPtr);
};

class WasmEdgeZlibInflateBackEnd
    : public WasmEdgeZlib<WasmEdgeZlibInflateBackEnd> {
public:
  WasmEdgeZlibInflateBackEnd(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibZlibCompilerFlags
    : public WasmEdgeZlib<WasmEdgeZlibZlibCompilerFlags> {
public:
  WasmEdgeZlibZlibCompilerFlags(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame);
};

class WasmEdgeZlibCompress : public WasmEdgeZlib<WasmEdgeZlibCompress> {
public:
  WasmEdgeZlibCompress(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestPtr,
                       uint32_t DestLenPtr, uint32_t SourcePtr,
                       uint32_t SourceLen);
};

class WasmEdgeZlibCompress2 : public WasmEdgeZlib<WasmEdgeZlibCompress2> {
public:
  WasmEdgeZlibCompress2(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestPtr,
                       uint32_t DestLenPtr, uint32_t SourcePtr,
                       uint32_t SourceLen, int32_t Level);
};

class WasmEdgeZlibCompressBound
    : public WasmEdgeZlib<WasmEdgeZlibCompressBound> {
public:
  WasmEdgeZlibCompressBound(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t SourceLen);
};

class WasmEdgeZlibUncompress : public WasmEdgeZlib<WasmEdgeZlibUncompress> {
public:
  WasmEdgeZlibUncompress(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestPtr,
                       uint32_t DestLenPtr, uint32_t SourcePtr,
                       uint32_t SourceLen);
};

class WasmEdgeZlibUncompress2 : public WasmEdgeZlib<WasmEdgeZlibUncompress2> {
public:
  WasmEdgeZlibUncompress2(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t DestPtr,
                       uint32_t DestLenPtr, uint32_t SourcePtr,
                       uint32_t SourceLenPtr);
};

class WasmEdgeZlibGZOpen : public WasmEdgeZlib<WasmEdgeZlibGZOpen> {
public:
  WasmEdgeZlibGZOpen(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t PathPtr,
                        uint32_t ModePtr);
};

class WasmEdgeZlibGZDOpen : public WasmEdgeZlib<WasmEdgeZlibGZDOpen> {
public:
  WasmEdgeZlibGZDOpen(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t FD,
                        uint32_t ModePtr);
};

class WasmEdgeZlibGZBuffer : public WasmEdgeZlib<WasmEdgeZlibGZBuffer> {
public:
  WasmEdgeZlibGZBuffer(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile,
                       uint32_t Size);
};

class WasmEdgeZlibGZSetParams : public WasmEdgeZlib<WasmEdgeZlibGZSetParams> {
public:
  WasmEdgeZlibGZSetParams(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile,
                       int32_t Level, int32_t Strategy);
};

class WasmEdgeZlibGZRead : public WasmEdgeZlib<WasmEdgeZlibGZRead> {
public:
  WasmEdgeZlibGZRead(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile,
                       uint32_t BufPtr, uint32_t Len);
};

class WasmEdgeZlibGZFread : public WasmEdgeZlib<WasmEdgeZlibGZFread> {
public:
  WasmEdgeZlibGZFread(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t BufPtr,
                       uint32_t Size, uint32_t NItems, uint32_t GZFile);
};

class WasmEdgeZlibGZWrite : public WasmEdgeZlib<WasmEdgeZlibGZWrite> {
public:
  WasmEdgeZlibGZWrite(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile,
                       uint32_t BufPtr, uint32_t Len);
};

class WasmEdgeZlibGZFwrite : public WasmEdgeZlib<WasmEdgeZlibGZFwrite> {
public:
  WasmEdgeZlibGZFwrite(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t BufPtr,
                       uint32_t Size, uint32_t NItems, uint32_t GZFile);
};

class WasmEdgeZlibGZPuts : public WasmEdgeZlib<WasmEdgeZlibGZPuts> {
public:
  WasmEdgeZlibGZPuts(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile,
                       uint32_t StringPtr);
};

class WasmEdgeZlibGZPutc : public WasmEdgeZlib<WasmEdgeZlibGZPutc> {
public:
  WasmEdgeZlibGZPutc(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile,
                       int32_t C);
};

class WasmEdgeZlibGZGetc : public WasmEdgeZlib<WasmEdgeZlibGZGetc> {
public:
  WasmEdgeZlibGZGetc(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile);
};

class WasmEdgeZlibGZUngetc : public WasmEdgeZlib<WasmEdgeZlibGZUngetc> {
public:
  WasmEdgeZlibGZUngetc(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t C,
                       uint32_t GZFile);
};

class WasmEdgeZlibGZFlush : public WasmEdgeZlib<WasmEdgeZlibGZFlush> {
public:
  WasmEdgeZlibGZFlush(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile,
                       int32_t Flush);
};

// z_off_t --> long
class WasmEdgeZlibGZSeek : public WasmEdgeZlib<WasmEdgeZlibGZSeek> {
public:
  WasmEdgeZlibGZSeek(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile,
                       int32_t Offset, int32_t Whence);
};

class WasmEdgeZlibGZRewind : public WasmEdgeZlib<WasmEdgeZlibGZRewind> {
public:
  WasmEdgeZlibGZRewind(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile);
};

class WasmEdgeZlibGZTell : public WasmEdgeZlib<WasmEdgeZlibGZTell> {
public:
  WasmEdgeZlibGZTell(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile);
};

class WasmEdgeZlibGZOffset : public WasmEdgeZlib<WasmEdgeZlibGZOffset> {
public:
  WasmEdgeZlibGZOffset(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile);
};

class WasmEdgeZlibGZEof : public WasmEdgeZlib<WasmEdgeZlibGZEof> {
public:
  WasmEdgeZlibGZEof(WasmEdgeZlibEnvironment &HostEnv) : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile);
};

class WasmEdgeZlibGZDirect : public WasmEdgeZlib<WasmEdgeZlibGZDirect> {
public:
  WasmEdgeZlibGZDirect(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile);
};

class WasmEdgeZlibGZClose : public WasmEdgeZlib<WasmEdgeZlibGZClose> {
public:
  WasmEdgeZlibGZClose(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile);
};

class WasmEdgeZlibGZClose_r : public WasmEdgeZlib<WasmEdgeZlibGZClose_r> {
public:
  WasmEdgeZlibGZClose_r(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile);
};

class WasmEdgeZlibGZClose_w : public WasmEdgeZlib<WasmEdgeZlibGZClose_w> {
public:
  WasmEdgeZlibGZClose_w(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile);
};

class WasmEdgeZlibGZClearerr : public WasmEdgeZlib<WasmEdgeZlibGZClearerr> {
public:
  WasmEdgeZlibGZClearerr(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t GZFile);
};

class WasmEdgeZlibAdler32 : public WasmEdgeZlib<WasmEdgeZlibAdler32> {
public:
  WasmEdgeZlibAdler32(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t Adler,
                       uint32_t BufPtr, uint32_t Len);
};

class WasmEdgeZlibAdler32_z : public WasmEdgeZlib<WasmEdgeZlibAdler32_z> {
public:
  WasmEdgeZlibAdler32_z(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t Adler,
                       uint32_t BufPtr, uint32_t Len);
};

// z_off_t --> long
class WasmEdgeZlibAdler32Combine
    : public WasmEdgeZlib<WasmEdgeZlibAdler32Combine> {
public:
  WasmEdgeZlibAdler32Combine(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t Adler1,
                       uint32_t Adler2, int32_t Len2);
};

class WasmEdgeZlibCRC32 : public WasmEdgeZlib<WasmEdgeZlibCRC32> {
public:
  WasmEdgeZlibCRC32(WasmEdgeZlibEnvironment &HostEnv) : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t CRC,
                       uint32_t BufPtr, uint32_t Len);
};

class WasmEdgeZlibCRC32_z : public WasmEdgeZlib<WasmEdgeZlibCRC32_z> {
public:
  WasmEdgeZlibCRC32_z(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t CRC,
                       uint32_t BufPtr, uint32_t Len);
};

// z_off_t --> long
class WasmEdgeZlibCRC32Combine : public WasmEdgeZlib<WasmEdgeZlibCRC32Combine> {
public:
  WasmEdgeZlibCRC32Combine(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t CRC1,
                       uint32_t CRC2, int32_t Len2);
};

class WasmEdgeZlibDeflateInit_ : public WasmEdgeZlib<WasmEdgeZlibDeflateInit_> {
public:
  WasmEdgeZlibDeflateInit_(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Level, uint32_t VersionPtr, int32_t StreamSize);
};

class WasmEdgeZlibInflateInit_ : public WasmEdgeZlib<WasmEdgeZlibInflateInit_> {
public:
  WasmEdgeZlibInflateInit_(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       uint32_t VersionPtr, int32_t StreamSize);
};

class WasmEdgeZlibDeflateInit2_
    : public WasmEdgeZlib<WasmEdgeZlibDeflateInit2_> {
public:
  WasmEdgeZlibDeflateInit2_(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Level, int32_t Method, int32_t WindowBits,
                       int32_t MemLevel, int32_t Strategy, uint32_t VersionPtr,
                       int32_t StreamSize);
};

class WasmEdgeZlibInflateInit2_
    : public WasmEdgeZlib<WasmEdgeZlibInflateInit2_> {
public:
  WasmEdgeZlibInflateInit2_(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t WindowBits, uint32_t VersionPtr,
                       int32_t StreamSize);
};

class WasmEdgeZlibInflateBackInit_
    : public WasmEdgeZlib<WasmEdgeZlibInflateBackInit_> {
public:
  WasmEdgeZlibInflateBackInit_(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t WindowBits, uint32_t WindowPtr,
                       uint32_t VersionPtr, int32_t StreamSize);
};

class WasmEdgeZlibGZGetc_ : public WasmEdgeZlib<WasmEdgeZlibGZGetc_> {
public:
  WasmEdgeZlibGZGetc_(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t GZFile);
};

class WasmEdgeZlibInflateSyncPoint
    : public WasmEdgeZlib<WasmEdgeZlibInflateSyncPoint> {
public:
  WasmEdgeZlibInflateSyncPoint(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibInflateUndermine
    : public WasmEdgeZlib<WasmEdgeZlibInflateUndermine> {
public:
  WasmEdgeZlibInflateUndermine(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Subvert);
};

class WasmEdgeZlibInflateValidate
    : public WasmEdgeZlib<WasmEdgeZlibInflateValidate> {
public:
  WasmEdgeZlibInflateValidate(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Check);
};

class WasmEdgeZlibInflateCodesUsed
    : public WasmEdgeZlib<WasmEdgeZlibInflateCodesUsed> {
public:
  WasmEdgeZlibInflateCodesUsed(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibInflateResetKeep
    : public WasmEdgeZlib<WasmEdgeZlibInflateResetKeep> {
public:
  WasmEdgeZlibInflateResetKeep(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibDeflateResetKeep
    : public WasmEdgeZlib<WasmEdgeZlibDeflateResetKeep> {
public:
  WasmEdgeZlibDeflateResetKeep(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

} // namespace Host
} // namespace WasmEdge
