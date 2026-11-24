// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "safetensors_fixture.h"
#include "sd_abi.h"
#include "sd_image.h"

#include "ast/type.h"
#include "common/defines.h"
#include "common/filesystem.h"
#include "common/span.h"
#include "common/types.h"
#include "host/wasi/wasimodule.h"
#include "plugin/plugin.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/function.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <ios>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

// The caller module of a host function: one memory of 16 pages, a WASI module
// if the test gives it preopens, and the frame that shows both to the host
// function. put() places bytes at a bump pointer that starts at 1024, so the
// first kilobyte stays zero. It never frees them.
class Guest {
public:
  Guest() {
    Caller.addHostMemory("memory",
                         std::make_unique<Runtime::Instance::MemoryInstance>(
                             AST::MemoryType(kPages)));
    Memory = Caller.findMemoryExports("memory");
  }
  explicit Guest(std::vector<std::string> Dirs) : Guest() {
    withPreopens(std::move(Dirs));
  }
  Guest(const Guest &) = delete;
  Guest &operator=(const Guest &) = delete;

  // Dirs has the format of the --dir option: "guest:host" or
  // "guest:host:readonly".
  Guest &withPreopens(std::vector<std::string> Dirs) {
    Wasi.emplace();
    Wasi->init(Dirs, "test", {}, {});
    Caller.setWASIModule(&*Wasi);
    Preopens = std::move(Dirs);
    return *this;
  }
  const std::vector<std::string> &preopens() const noexcept { return Preopens; }

  uint32_t put(std::string_view Bytes) {
    return put(Span<const uint8_t>(
        reinterpret_cast<const uint8_t *>(Bytes.data()), Bytes.size()));
  }
  uint32_t put(Span<const uint8_t> Bytes) {
    if (Bytes.size() > memorySize() - Next) {
      ADD_FAILURE() << "the guest memory is full";
      return 0;
    }
    const uint32_t Ptr = Next;
    std::copy(Bytes.begin(), Bytes.end(), Memory->getDataPtr() + Ptr);
    Next += static_cast<uint32_t>(Bytes.size());
    return Ptr;
  }
  // A table of {ptr, len} pairs, each two little-endian u32.
  uint32_t blobs(std::vector<std::pair<uint32_t, uint32_t>> Table) {
    std::vector<uint8_t> Bytes;
    for (const auto &[Ptr, Len] : Table) {
      for (const uint32_t Value : {Ptr, Len}) {
        for (uint32_t Shift = 0; Shift < 32; Shift += 8) {
          Bytes.push_back(static_cast<uint8_t>(Value >> Shift));
        }
      }
    }
    return put(Bytes);
  }

  uint32_t byte(uint32_t Ptr) const {
    if (Ptr >= memorySize()) {
      ADD_FAILURE() << "byte " << Ptr << " is outside the guest memory";
      return 0;
    }
    return Memory->getDataPtr()[Ptr];
  }
  // The little-endian u32 at Ptr.
  uint32_t u32(uint32_t Ptr) const {
    if (Ptr > memorySize() - 4) {
      ADD_FAILURE() << "u32 " << Ptr << " is outside the guest memory";
      return 0;
    }
    uint32_t Value = 0;
    for (uint32_t I = 0; I < 4; ++I) {
      Value |= static_cast<uint32_t>(Memory->getDataPtr()[Ptr + I]) << (8 * I);
    }
    return Value;
  }
  uint32_t memorySize() const {
    return static_cast<uint32_t>(Memory->getSize());
  }

  // Calls export Name of Mod with Args from this guest and returns its u32
  // result. A trap fails the test.
  uint32_t call(Runtime::Instance::ModuleInstance &Mod, std::string_view Name,
                std::initializer_list<ValVariant> Args) {
    return run(Frame, Mod, Name, Args);
  }
  // Same as call(), but from a module with no memory and no WASI module.
  uint32_t callWithoutMemory(Runtime::Instance::ModuleInstance &Mod,
                             std::string_view Name,
                             std::initializer_list<ValVariant> Args) {
    return run(BareFrame, Mod, Name, Args);
  }

private:
  class CallerModule : public Runtime::Instance::ModuleInstance {
  public:
    using ModuleInstance::ModuleInstance;
    using ModuleInstance::setWASIModule;
  };
  static constexpr uint64_t kPages = 16;

  static uint32_t run(const Runtime::CallingFrame &From,
                      Runtime::Instance::ModuleInstance &Mod,
                      std::string_view Name,
                      std::initializer_list<ValVariant> Args) {
    auto *Fn = Mod.findFuncExports(Name);
    if (Fn == nullptr) {
      ADD_FAILURE() << "no export " << Name;
      return UINT32_MAX;
    }
    std::array<ValVariant, 1> Ret = {UINT32_C(0)};
    if (!Fn->getHostFunc().run(
            From, Span<const ValVariant>(Args.begin(), Args.size()), Ret)) {
      ADD_FAILURE() << Name << " trapped";
      return UINT32_MAX;
    }
    return Ret[0].get<uint32_t>();
  }

  std::optional<WasiModule> Wasi;
  std::vector<std::string> Preopens;
  CallerModule Caller{""};
  Runtime::Instance::MemoryInstance *Memory = nullptr;
  Runtime::CallingFrame Frame{nullptr, &Caller};
  Runtime::Instance::ModuleInstance Bare{""};
  Runtime::CallingFrame BareFrame{nullptr, &Bare};
  uint32_t Next = 1024;
};

// A guest with the preopens of Other and its own memory, for calls from another
// thread.
inline Guest guestLike(const Guest &Other) { return Guest(Other.preopens()); }

// Returns a module instance of the built plug-in. The first call loads the
// plug-in from the build tree. Returns null if the load fails.
inline std::unique_ptr<Runtime::Instance::ModuleInstance> loadPlugin() {
  WasmEdge::Plugin::Plugin::load(WasmEdge::u8path(
      "../../../plugins/wasmedge_stablediffusion/" WASMEDGE_LIB_PREFIX
      "wasmedgePluginWasmEdgeStableDiffusion" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_stablediffusion"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_stablediffusion"sv)) {
      return Module->create();
    }
  }
  return {};
}

inline std::string repeat(std::string_view Text, size_t Count) {
  std::string Out;
  Out.reserve(Text.size() * Count);
  for (size_t I = 0; I < Count; ++I) {
    Out += Text;
  }
  return Out;
}

// Returns Text as a JSON string: quoted, with quotes, backslashes, and control
// characters escaped.
inline std::string jsonString(std::string_view Text) {
  constexpr std::string_view kHex = "0123456789abcdef"sv;
  std::string Out = "\"";
  for (const char C : Text) {
    const auto Byte = static_cast<unsigned char>(C);
    if (C == '"' || C == '\\') {
      Out += '\\';
      Out += C;
    } else if (Byte < 0x20) {
      Out += "\\u00";
      Out += kHex[Byte >> 4];
      Out += kHex[Byte & 0xF];
    } else {
      Out += C;
    }
  }
  return Out + "\"";
}

// The bytes of a W x H RGB PNG.
inline std::vector<uint8_t> png(uint32_t W, uint32_t H) {
  const std::vector<uint8_t> Pixels(static_cast<size_t>(W) * H * 3, 0x40);
  auto Image = OwnedImage::copyOf(Pixels, W, H, 3);
  if (!Image) {
    ADD_FAILURE() << "no image of " << W << "x" << H;
    return {};
  }
  auto Encoded = encodePng(*Image);
  if (!Encoded) {
    ADD_FAILURE() << "no PNG of " << W << "x" << H;
    return {};
  }
  return std::vector<uint8_t>(Encoded->Data.get(),
                              Encoded->Data.get() + Encoded->Size);
}

// While it lives, redirects descriptor 1 to a temporary file. At the end, it
// copies the file to the original descriptor 1, so a failure report in the
// meantime is not lost.
class CapturedStdout {
public:
  CapturedStdout() {
    std::fflush(stdout);
    File = std::tmpfile();
    Saved = ::dup(STDOUT_FILENO);
    if (File == nullptr || Saved < 0 ||
        ::dup2(::fileno(File), STDOUT_FILENO) < 0) {
      ADD_FAILURE() << "stdout cannot be captured";
    }
  }
  CapturedStdout(const CapturedStdout &) = delete;
  CapturedStdout &operator=(const CapturedStdout &) = delete;
  ~CapturedStdout() {
    std::fflush(stdout);
    if (Saved >= 0) {
      ::dup2(Saved, STDOUT_FILENO);
      ::close(Saved);
    }
    const auto Text = written();
    if (File != nullptr) {
      std::fclose(File);
    }
    std::fwrite(Text.data(), 1, Text.size(), stdout);
    std::fflush(stdout);
  }

  // Counts the captured lines that are not empty and are not default-logger
  // lines whose message starts with Prefix. A carriage return ends a line, as a
  // line feed does. A log line must match from its first byte, so any text
  // printed before it makes the line count.
  size_t linesWithout(std::string_view Prefix) const {
    std::fflush(stdout);
    const auto Text = written();
    size_t Count = 0;
    for (size_t Begin = 0; Begin <= Text.size();) {
      const auto End = std::min(Text.find_first_of("\r\n", Begin), Text.size());
      const std::string_view Line(Text.data() + Begin, End - Begin);
      if (!Line.empty() && !isLogLine(Line, Prefix)) {
        ++Count;
      }
      Begin = End + 1;
    }
    return Count;
  }
  // Returns true when the captured output holds Text.
  bool holds(std::string_view Text) const {
    std::fflush(stdout);
    return written().find(Text) != std::string::npos;
  }

private:
  // Returns true when Line starts like a default-logger line,
  // "[YYYY-MM-DD HH:MM:SS.mmm] [level] ", followed by Prefix. The level can
  // have color codes when the logger writes to a terminal. The regex reads only
  // the first 64 bytes, which is more than the pattern needs: library lines can
  // be kilobytes long.
  static bool isLogLine(std::string_view Line, std::string_view Prefix) {
    static const std::regex Shape(
        R"re(\[\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d{3}\] )re"
        R"re(\[(?:\x1b\[[0-9;]*m)*(?:trace|debug|info|warning|error|critical))re"
        R"re((?:\x1b\[[0-9;]*m)*\] )re");
    const auto Head = Line.substr(0, 64);
    std::cmatch Match;
    return std::regex_search(Head.data(), Head.data() + Head.size(), Match,
                             Shape, std::regex_constants::match_continuous) &&
           Line.substr(static_cast<size_t>(Match.length(0)))
                   .substr(0, Prefix.size()) == Prefix;
  }

  std::string written() const {
    std::string Text;
    std::array<char, 4096> Chunk{};
    off_t At = 0;
    ssize_t Got = 0;
    while (File != nullptr && (Got = ::pread(::fileno(File), Chunk.data(),
                                             Chunk.size(), At)) > 0) {
      Text.append(Chunk.data(), static_cast<size_t>(Got));
      At += Got;
    }
    return Text;
  }

  std::FILE *File = nullptr;
  int Saved = -1;
};

// Tests of the built plug-in through its six exports, with the real library and
// the fixture model on the CPU. Each test has its own module instance, so its
// sessions are released at the end. Its guest preopens the fixture directory as
// /models and Aux (the directory for test files) as /aux, both read-only.
class SDContractTest : public ::testing::Test {
public:
  static void SetUpTestSuite() {
    Aux = std::filesystem::temp_directory_path() /
          ("wasmedge-sd-contract-" + std::to_string(::getpid()));
    std::error_code Error;
    std::filesystem::remove_all(Aux, Error);
    std::filesystem::create_directories(Aux);
  }
  static void TearDownTestSuite() {
    std::error_code Error;
    std::filesystem::remove_all(Aux, Error);
  }

protected:
  // A tensor for writeSafetensors. Every element holds Fill.
  struct TensorSpec {
    std::string Name;
    std::string Dtype; // F32, F16, or I64
    std::vector<uint64_t> Shape;
    float Fill;
  };
  static constexpr std::string_view kLoraBase =
      "lora_unet_down_blocks_0_attentions_0_transformer_blocks_0_attn1_to_q"sv;

  void SetUp() override {
    Mod = loadPlugin();
    ASSERT_TRUE(Mod);
    G.withPreopens({"/models:" + modelDir().string() + ":readonly",
                    "/aux:" + Aux.string() + ":readonly"});
  }

  // The fixture directory beside the test binary.
  static std::filesystem::path modelDir() {
    return std::filesystem::absolute("stableDiffusion");
  }

  // Calls session_open with Config. On success, the id goes to *Id.
  uint32_t openCode(std::string_view Config, uint32_t *Id = nullptr) {
    const auto IdPtr = G.put(std::string(4, '\xff'));
    const auto Code =
        G.call(*Mod, "session_open",
               {G.put(Config), static_cast<uint32_t>(Config.size()), IdPtr});
    if (Id != nullptr && Code == kSuccess) {
      *Id = G.u32(IdPtr);
    }
    return Code;
  }
  // Opens a session on the model, with ExtraConfigJson added to the
  // configuration. Returns UINT32_MAX, which is never an id, if the open fails.
  uint32_t session(std::string_view ExtraConfigJson = "") {
    const auto Config =
        R"({"model_path":"/models/sd-v1-5-fp16.safetensors","device":"cpu",)"
        R"("n_threads":4)"s +
        std::string(ExtraConfigJson) + "}";
    uint32_t Id = UINT32_MAX;
    EXPECT_EQ(openCode(Config, &Id), kSuccess) << Config;
    return Id;
  }

  // Calls generate from From on session Id, with Blobs as the blob table. The
  // guest address of the count goes to *CountAt.
  uint32_t generateWith(Guest &From, uint32_t Id, std::string_view RequestJson,
                        const std::vector<std::vector<uint8_t>> &Blobs = {},
                        uint32_t *CountAt = nullptr) {
    std::vector<std::pair<uint32_t, uint32_t>> Table;
    for (const auto &Blob : Blobs) {
      Table.emplace_back(From.put(Blob), static_cast<uint32_t>(Blob.size()));
    }
    const auto TablePtr = Table.empty() ? UINT32_C(0) : From.blobs(Table);
    const auto Text = From.put(RequestJson);
    const auto Count = From.put(std::string(4, '\0'));
    if (CountAt != nullptr) {
      *CountAt = Count;
    }
    return From.call(*Mod, "generate",
                     {Id, Text, static_cast<uint32_t>(RequestJson.size()),
                      TablePtr, static_cast<uint32_t>(Blobs.size()), Count});
  }
  uint32_t generate(uint32_t Id, std::string_view RequestJson,
                    const std::vector<std::vector<uint8_t>> &Blobs = {}) {
    return generateWith(G, Id, RequestJson, Blobs, &CountSlot);
  }
  uint32_t resultCount() const { return G.u32(CountSlot); }

  // Returns the bytes of result Index of session Id in Format, from
  // result_read. If the read fails, reports a failure and returns empty.
  std::vector<uint8_t> readResult(uint32_t Id, uint32_t Index,
                                  uint32_t Format) {
    const auto Info = G.put(std::string(16, '\0'));
    if (const auto Code =
            G.call(*Mod, "result_info", {Id, Index, Format, Info});
        Code != kSuccess) {
      ADD_FAILURE() << "result_info of result " << Index << " failed with "
                    << Code;
      return {};
    }
    const auto Size = G.u32(Info + 12);
    const auto Buf = G.put(std::string(Size, '\0'));
    const auto Written = G.put(std::string(4, '\0'));
    if (Buf == 0) {
      return {};
    }
    if (const auto Code = G.call(*Mod, "result_read",
                                 {Id, Index, Format, Buf, Size, Written});
        Code != kSuccess) {
      ADD_FAILURE() << "result_read of result " << Index << " failed with "
                    << Code;
      return {};
    }
    std::vector<uint8_t> Bytes(G.u32(Written));
    for (size_t I = 0; I < Bytes.size(); ++I) {
      Bytes[I] = static_cast<uint8_t>(G.byte(Buf + static_cast<uint32_t>(I)));
    }
    return Bytes;
  }
  // Returns the 64-bit FNV-1a hash of the RGB8 bytes of result Index of session
  // Id. An all-zero result is a failure: no image generated here is black, and
  // bytes that were never written would always hash the same.
  uint64_t hash(uint32_t Id, uint32_t Index) {
    const auto Bytes =
        readResult(Id, Index, static_cast<uint32_t>(ResultFormat::Rgb8));
    if (std::all_of(Bytes.begin(), Bytes.end(),
                    [](uint8_t Byte) { return Byte == 0; })) {
      ADD_FAILURE() << "result " << Index << " of session " << Id
                    << " has no byte that is not zero";
    }
    uint64_t Hash = UINT64_C(0xcbf29ce484222325);
    for (const uint8_t Byte : Bytes) {
      Hash = (Hash ^ Byte) * UINT64_C(0x100000001b3);
    }
    return Hash;
  }
  uint32_t close(uint32_t Id) { return G.call(*Mod, "session_close", {Id}); }

  // A request for one 64x64 image, two steps, seed 7, with the keys of Extra
  // added.
  static std::string small(std::string_view Extra = "",
                           std::string_view Prompt = "a cat") {
    return "{\"prompt\":" + jsonString(Prompt) +
           R"(,"width":64,"height":64,"steps":2,"seed":7)" +
           std::string(Extra) + "}";
  }

  static void writeBytes(std::string_view Name, std::string_view Bytes) {
    std::ofstream Out(Aux / std::string(Name),
                      std::ios::binary | std::ios::trunc);
    Out.write(Bytes.data(), static_cast<std::streamsize>(Bytes.size()));
    Out.close();
    if (!Out) {
      ADD_FAILURE() << "cannot write " << Name;
    }
  }
  // Writes a file of Size bytes that starts with a safetensors header with the
  // text Header. The rest is a hole.
  static void writeSparse(std::string_view Name, uint64_t Size,
                          std::string_view Header) {
    Fixture::writeHeaderFile(Aux / std::string(Name), Header.size(), Header,
                             Size - 8 - Header.size());
  }
  // Writes the first Bytes bytes of the model.
  static void writeTruncatedCopyOfTheModel(std::string_view Name,
                                           uint64_t Bytes) {
    std::ifstream In(modelDir() / "sd-v1-5-fp16.safetensors", std::ios::binary);
    std::string Head(Bytes, '\0');
    In.read(Head.data(), static_cast<std::streamsize>(Bytes));
    if (static_cast<uint64_t>(In.gcount()) != Bytes) {
      ADD_FAILURE() << "the model has fewer than " << Bytes << " bytes";
    }
    writeBytes(Name, Head);
  }
  // Writes a safetensors file with Tensors. Their data follow in the same
  // order.
  static void writeSafetensors(std::string_view Name,
                               const std::vector<TensorSpec> &Tensors) {
    std::string Header = "{";
    std::string Data;
    for (const auto &Tensor : Tensors) {
      const auto Element = elementBytes(Tensor.Dtype, Tensor.Fill);
      uint64_t Elements = 1;
      std::string Shape;
      for (const auto Dim : Tensor.Shape) {
        Elements *= Dim;
        Shape += (Shape.empty() ? "" : ",") + std::to_string(Dim);
      }
      const auto Begin = Data.size();
      for (uint64_t I = 0; I < Elements; ++I) {
        Data += Element;
      }
      Header += (Header.size() > 1 ? "," : "") + jsonString(Tensor.Name) +
                R"(:{"dtype":)" + jsonString(Tensor.Dtype) + R"(,"shape":[)" +
                Shape + R"(],"data_offsets":[)" + std::to_string(Begin) + "," +
                std::to_string(Data.size()) + "]}";
    }
    Header += "}";
    Header.append((8 - Header.size() % 8) % 8, ' ');
    std::string Length;
    for (size_t I = 0; I < 8; ++I) {
      Length += static_cast<char>((Header.size() >> (8 * I)) & 0xFFU);
    }
    writeBytes(Name, Length + Header + Data);
  }

  std::unique_ptr<Runtime::Instance::ModuleInstance> Mod;
  Guest G;
  inline static std::filesystem::path Aux;

private:
  static constexpr uint32_t kSuccess = static_cast<uint32_t>(ErrNo::Success);

  // Returns one element with the value Fill in Dtype, little-endian. For F16,
  // Fill must be zero or a normal F16 number, and the fraction is truncated.
  static std::string elementBytes(std::string_view Dtype, float Fill) {
    uint32_t Bits = 0;
    std::memcpy(&Bits, &Fill, sizeof(Bits));
    uint64_t Value = 0;
    size_t Size = 0;
    if (Dtype == "F32"sv) {
      Value = Bits;
      Size = 4;
    } else if (Dtype == "F16"sv) {
      const auto Exponent = static_cast<int32_t>((Bits >> 23) & 0xFFU) - 112;
      const bool Zero = (Bits & 0x7FFFFFFFU) == 0;
      if (!Zero && (Exponent <= 0 || Exponent >= 31)) {
        ADD_FAILURE() << Fill << " is not a normal F16 number";
      }
      Value = ((Bits >> 16) & 0x8000U) |
              (Zero ? 0U
                    : (static_cast<uint32_t>(Exponent) << 10) |
                          ((Bits >> 13) & 0x3FFU));
      Size = 2;
    } else if (Dtype == "I64"sv) {
      Value = static_cast<uint64_t>(static_cast<int64_t>(Fill));
      Size = 8;
    } else {
      ADD_FAILURE() << "no element size for the dtype " << Dtype;
    }
    std::string Bytes;
    for (size_t I = 0; I < Size; ++I) {
      Bytes += static_cast<char>((Value >> (8 * I)) & 0xFFU);
    }
    return Bytes;
  }

  uint32_t CountSlot = 0;
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
