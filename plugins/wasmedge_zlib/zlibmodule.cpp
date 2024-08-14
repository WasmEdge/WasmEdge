// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "zlibmodule.h"
#include "zlibfunc.h"

namespace WasmEdge {
namespace Host {

/// Register your functions in module.
WasmEdgeZlibModule::WasmEdgeZlibModule() : ModuleInstance("wasmedge_zlib") {
  addHostFunc("deflateInit", std::make_unique<WasmEdgeZlibDeflateInit>(Env));
  addHostFunc("deflate", std::make_unique<WasmEdgeZlibDeflate>(Env));
  addHostFunc("deflateEnd", std::make_unique<WasmEdgeZlibDeflateEnd>(Env));
  addHostFunc("inflateInit", std::make_unique<WasmEdgeZlibInflateInit>(Env));
  addHostFunc("inflate", std::make_unique<WasmEdgeZlibInflate>(Env));
  addHostFunc("inflateEnd", std::make_unique<WasmEdgeZlibInflateEnd>(Env));
  addHostFunc("deflateInit2", std::make_unique<WasmEdgeZlibDeflateInit2>(Env));
  addHostFunc("deflateSetDictionary",
              std::make_unique<WasmEdgeZlibDeflateSetDictionary>(Env));
  addHostFunc("deflateGetDictionary",
              std::make_unique<WasmEdgeZlibDeflateGetDictionary>(Env));
  addHostFunc("deflateCopy", std::make_unique<WasmEdgeZlibDeflateCopy>(Env));
  addHostFunc("deflateReset", std::make_unique<WasmEdgeZlibDeflateReset>(Env));
  addHostFunc("deflateParams",
              std::make_unique<WasmEdgeZlibDeflateParams>(Env));
  addHostFunc("deflateTune", std::make_unique<WasmEdgeZlibDeflateTune>(Env));
  addHostFunc("deflateBound", std::make_unique<WasmEdgeZlibDeflateBound>(Env));
  addHostFunc("deflatePending",
              std::make_unique<WasmEdgeZlibDeflatePending>(Env));
  addHostFunc("deflatePrime", std::make_unique<WasmEdgeZlibDeflatePrime>(Env));
  addHostFunc("deflateSetHeader",
              std::make_unique<WasmEdgeZlibDeflateSetHeader>(Env));
  addHostFunc("inflateInit2", std::make_unique<WasmEdgeZlibInflateInit2>(Env));
  addHostFunc("inflateSetDictionary",
              std::make_unique<WasmEdgeZlibInflateSetDictionary>(Env));
  addHostFunc("inflateGetDictionary",
              std::make_unique<WasmEdgeZlibInflateGetDictionary>(Env));
  addHostFunc("inflateSync", std::make_unique<WasmEdgeZlibInflateSync>(Env));
  addHostFunc("inflateCopy", std::make_unique<WasmEdgeZlibInflateCopy>(Env));
  addHostFunc("inflateReset", std::make_unique<WasmEdgeZlibInflateReset>(Env));
  addHostFunc("inflateReset2",
              std::make_unique<WasmEdgeZlibInflateReset2>(Env));
  addHostFunc("inflatePrime", std::make_unique<WasmEdgeZlibInflatePrime>(Env));
  addHostFunc("inflateMark", std::make_unique<WasmEdgeZlibInflateMark>(Env));
  addHostFunc("inflateGetHeader",
              std::make_unique<WasmEdgeZlibInflateGetHeader>(Env));
  addHostFunc("inflateBackInit",
              std::make_unique<WasmEdgeZlibInflateBackInit>(Env));
  addHostFunc("inflateBackEnd",
              std::make_unique<WasmEdgeZlibInflateBackEnd>(Env));
  addHostFunc("zlibCompileFlags",
              std::make_unique<WasmEdgeZlibZlibCompilerFlags>(Env));
  addHostFunc("compress", std::make_unique<WasmEdgeZlibCompress>(Env));
  addHostFunc("compress2", std::make_unique<WasmEdgeZlibCompress2>(Env));
  addHostFunc("compressBound",
              std::make_unique<WasmEdgeZlibCompressBound>(Env));
  addHostFunc("uncompress", std::make_unique<WasmEdgeZlibUncompress>(Env));
  addHostFunc("uncompress2", std::make_unique<WasmEdgeZlibUncompress2>(Env));
  addHostFunc("gzopen", std::make_unique<WasmEdgeZlibGZOpen>(Env));
  addHostFunc("gzdopen", std::make_unique<WasmEdgeZlibGZDOpen>(Env));
  addHostFunc("gzbuffer", std::make_unique<WasmEdgeZlibGZBuffer>(Env));
  addHostFunc("gzsetparams", std::make_unique<WasmEdgeZlibGZSetParams>(Env));
  addHostFunc("gzread", std::make_unique<WasmEdgeZlibGZRead>(Env));
  addHostFunc("gzfread", std::make_unique<WasmEdgeZlibGZFread>(Env));
  addHostFunc("gzwrite", std::make_unique<WasmEdgeZlibGZWrite>(Env));
  addHostFunc("gzfwrite", std::make_unique<WasmEdgeZlibGZFwrite>(Env));
  addHostFunc("gzputs", std::make_unique<WasmEdgeZlibGZPuts>(Env));
  addHostFunc("gzputc", std::make_unique<WasmEdgeZlibGZPutc>(Env));
  addHostFunc("gzgetc", std::make_unique<WasmEdgeZlibGZGetc>(Env));
  addHostFunc("gzungetc", std::make_unique<WasmEdgeZlibGZUngetc>(Env));
  addHostFunc("gzflush", std::make_unique<WasmEdgeZlibGZFlush>(Env));
  addHostFunc("gzseek", std::make_unique<WasmEdgeZlibGZSeek>(Env));
  addHostFunc("gzrewind", std::make_unique<WasmEdgeZlibGZRewind>(Env));
  addHostFunc("gztell", std::make_unique<WasmEdgeZlibGZTell>(Env));
  addHostFunc("gzoffset", std::make_unique<WasmEdgeZlibGZOffset>(Env));
  addHostFunc("gzeof", std::make_unique<WasmEdgeZlibGZEof>(Env));
  addHostFunc("gzdirect", std::make_unique<WasmEdgeZlibGZDirect>(Env));
  addHostFunc("gzclose", std::make_unique<WasmEdgeZlibGZClose>(Env));
  addHostFunc("gzclose_r", std::make_unique<WasmEdgeZlibGZClose_r>(Env));
  addHostFunc("gzclose_w", std::make_unique<WasmEdgeZlibGZClose_w>(Env));
  addHostFunc("gzclearerr", std::make_unique<WasmEdgeZlibGZClearerr>(Env));
  addHostFunc("adler32", std::make_unique<WasmEdgeZlibAdler32>(Env));
  addHostFunc("adler32_z", std::make_unique<WasmEdgeZlibAdler32_z>(Env));
  addHostFunc("adler32_combine",
              std::make_unique<WasmEdgeZlibAdler32Combine>(Env));
  addHostFunc("crc32", std::make_unique<WasmEdgeZlibCRC32>(Env));
  addHostFunc("crc32_z", std::make_unique<WasmEdgeZlibCRC32_z>(Env));
  addHostFunc("crc32_combine", std::make_unique<WasmEdgeZlibCRC32Combine>(Env));
  addHostFunc("deflateInit_", std::make_unique<WasmEdgeZlibDeflateInit_>(Env));
  addHostFunc("inflateInit_", std::make_unique<WasmEdgeZlibInflateInit_>(Env));
  addHostFunc("deflateInit2_",
              std::make_unique<WasmEdgeZlibDeflateInit2_>(Env));
  addHostFunc("inflateInit2_",
              std::make_unique<WasmEdgeZlibInflateInit2_>(Env));
  addHostFunc("inflateBackInit_",
              std::make_unique<WasmEdgeZlibInflateBackInit_>(Env));
  addHostFunc("gzgetc_", std::make_unique<WasmEdgeZlibGZGetc_>(Env));
  addHostFunc("inflateSyncPoint",
              std::make_unique<WasmEdgeZlibInflateSyncPoint>(Env));
  addHostFunc("inflateUndermine",
              std::make_unique<WasmEdgeZlibInflateUndermine>(Env));
  addHostFunc("inflateValidate",
              std::make_unique<WasmEdgeZlibInflateValidate>(Env));
  addHostFunc("inflateCodesUsed",
              std::make_unique<WasmEdgeZlibInflateCodesUsed>(Env));
  addHostFunc("inflateResetKeep",
              std::make_unique<WasmEdgeZlibInflateResetKeep>(Env));
  addHostFunc("deflateResetKeep",
              std::make_unique<WasmEdgeZlibDeflateResetKeep>(Env));
}

} // namespace Host
} // namespace WasmEdge
