// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "zlibbase.h"

namespace WasmEdge {
namespace Host {

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

class WasmEdgeZlibDeflate : public WasmEdgeZlib<WasmEdgeZlibDeflate> {
public:
  WasmEdgeZlibDeflate(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Flush);
};

class WasmEdgeZlibInflate : public WasmEdgeZlib<WasmEdgeZlibInflate> {
public:
  WasmEdgeZlibInflate(WasmEdgeZlibEnvironment &HostEnv)
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

class WasmEdgeZlibInflateEnd : public WasmEdgeZlib<WasmEdgeZlibInflateEnd> {
public:
  WasmEdgeZlibInflateEnd(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
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

class WasmEdgeZlibInflateBack : public WasmEdgeZlib<WasmEdgeZlibInflateBack> {
public:
  WasmEdgeZlibInflateBack(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Bits, int32_t Value);
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

} // namespace Host
} // namespace WasmEdge
