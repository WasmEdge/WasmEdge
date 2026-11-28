// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_adapter_check.h"

#include "sd_log.h"

#include "common/spdlog.h"

#include "simdjson.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {
namespace {

// The library does not check the tensors of LoRA and embedding files. An
// unexpected shape fails a ggml assertion and ends the process. So this check
// accepts only the layouts that it has rules for, each tested against the
// library: plain LoRA (kohya and PEFT spelling) and embeddings with one or two
// dimensions. Other layouts (LoHa, LoKr, Tucker, full-difference) need a new
// rule first. The check reads names, types, and shapes only.

using JsonElement = simdjson::dom::element;
using JsonObject = simdjson::dom::object;
using JsonArray = simdjson::dom::array;
using JsonType = simdjson::dom::element_type;

constexpr std::string_view kMetadataKey = "__metadata__"sv;
constexpr std::string_view kAdapterConfigKey = "adapter_config"sv;
constexpr std::string_view kComfyQuantSuffix = ".comfy_quant"sv;
constexpr std::array<std::string_view, 2> kFp8Dtypes = {"F8_E4M3"sv,
                                                        "F8_E5M2"sv};
constexpr size_t kLengthBytes = 8;

enum class LoraPart : uint8_t { Down, Up, Alpha };
enum class Spelling : uint8_t { Kohya, Peft, Either };

constexpr std::array<std::string_view, 3> kPartNames = {"down"sv, "up"sv,
                                                        "alpha"sv};

struct LoraSuffix {
  std::string_view Text;
  LoraPart Part;
  Spelling Style;
};

constexpr std::array<LoraSuffix, 5> kLoraSuffixes{{
    {".lora_down.weight"sv, LoraPart::Down, Spelling::Kohya},
    {".lora_up.weight"sv, LoraPart::Up, Spelling::Kohya},
    {".lora_A.weight"sv, LoraPart::Down, Spelling::Peft},
    {".lora_B.weight"sv, LoraPart::Up, Spelling::Peft},
    {".alpha"sv, LoraPart::Alpha, Spelling::Either},
}};

struct Shape {
  std::array<uint64_t, 4> Dims{}; // the first four dimensions
  size_t DimCount = 0;
  uint64_t Elements = 1;
  bool Overflow = false; // the product of the dimensions exceeds 64 bits
  uint64_t Bytes = 0;    // Elements times the size of the dtype
};

struct LoraTensor {
  std::string_view Prefix;
  LoraPart Part;
  Spelling Style;
  uint64_t Rank; // r of a down or up tensor; 0 for an alpha
};

// Where a header came from, for the log line of a refusal.
struct Source {
  AdapterKind Kind;
  const std::filesystem::path *Path; // null: the header text was given alone
};

std::string_view kindName(AdapterKind Kind) noexcept {
  return Kind == AdapterKind::Lora ? "LoRA"sv : "embedding"sv;
}

bool refuse(const Source &From, std::string_view Reason) noexcept {
  if (From.Path == nullptr) {
    spdlog::error("[WasmEdge-StableDiffusion] {} header: {}"sv,
                  kindName(From.Kind), Reason);
  } else {
    spdlog::error("[WasmEdge-StableDiffusion] {} file {}: {}"sv,
                  kindName(From.Kind), From.Path->native(), Reason);
  }
  return false;
}

bool endsWith(std::string_view Text, std::string_view Suffix) noexcept {
  return Text.size() >= Suffix.size() &&
         Text.substr(Text.size() - Suffix.size()) == Suffix;
}

// Returns the first key that occurs twice in Object, if any. Keys is scratch
// space.
std::optional<std::string_view>
repeatedKey(const JsonObject &Object,
            std::vector<std::string_view> &Keys) noexcept {
  Keys.clear();
  for (const auto Member : Object) {
    Keys.push_back(Member.key);
  }
  std::sort(Keys.begin(), Keys.end());
  const auto Repeated = std::adjacent_find(Keys.begin(), Keys.end());
  if (Repeated == Keys.end()) {
    return std::nullopt;
  }
  return *Repeated;
}

std::optional<uint64_t> dtypeBytes(std::string_view Dtype) noexcept {
  if (Dtype == "F16"sv || Dtype == "BF16"sv) {
    return 2;
  }
  if (Dtype == "F32"sv) {
    return 4;
  }
  return std::nullopt;
}

// Reads an array of integers, each 1 or more. Returns false for anything else.
bool readShape(JsonElement Value, Shape &Out) noexcept {
  JsonArray Dims;
  if (Value.get(Dims) != simdjson::SUCCESS) {
    return false;
  }
  for (const auto Dim : Dims) {
    uint64_t Size = 0;
    if (Dim.type() == JsonType::INT64) {
      int64_t Signed = 0;
      if (Dim.get(Signed) != simdjson::SUCCESS || Signed < 1) {
        return false;
      }
      Size = static_cast<uint64_t>(Signed);
    } else if (Dim.type() != JsonType::UINT64 ||
               Dim.get(Size) != simdjson::SUCCESS) {
      return false;
    }
    if (Out.DimCount < Out.Dims.size()) {
      Out.Dims[Out.DimCount] = Size;
    }
    ++Out.DimCount;
    if (__builtin_mul_overflow(Out.Elements, Size, &Out.Elements)) {
      Out.Overflow = true;
    }
  }
  return true;
}

// Rules for every tensor of both file kinds.
bool readTensor(const Source &From, uint64_t FileSize, std::string_view Name,
                JsonElement Value, std::vector<std::string_view> &Keys,
                Shape &Out) noexcept {
  JsonObject Entry;
  if (Value.get(Entry) != simdjson::SUCCESS) {
    return refuse(
        From, fmt::format("tensor \"{}\" is not an object"sv, logText(Name)));
  }
  if (const auto Key = repeatedKey(Entry, Keys)) {
    return refuse(From, fmt::format("tensor \"{}\" has the key \"{}\" twice"sv,
                                    logText(Name), logText(*Key)));
  }
  std::optional<std::string_view> Dtype;
  std::optional<JsonElement> ShapeValue;
  for (const auto Field : Entry) {
    if (Field.key == "dtype"sv) {
      std::string_view Text;
      if (Field.value.get(Text) != simdjson::SUCCESS) {
        return refuse(
            From,
            fmt::format("tensor \"{}\" has a dtype that is not a string"sv,
                        logText(Name)));
      }
      Dtype = Text;
    } else if (Field.key == "shape"sv) {
      ShapeValue = Field.value;
    }
  }
  if (!Dtype) {
    return refuse(From,
                  fmt::format("tensor \"{}\" has no dtype"sv, logText(Name)));
  }
  const auto Bytes = dtypeBytes(*Dtype);
  if (!Bytes) {
    return refuse(
        From, fmt::format(
                  "tensor \"{}\" has the dtype \"{}\", not F16, BF16, or F32"sv,
                  logText(Name), logText(*Dtype)));
  }
  if (!ShapeValue || !readShape(*ShapeValue, Out)) {
    return refuse(
        From,
        fmt::format("tensor \"{}\" has no shape of integers of at least 1"sv,
                    logText(Name)));
  }
  uint64_t Size = 0;
  if (Out.Overflow || __builtin_mul_overflow(Out.Elements, *Bytes, &Size) ||
      Size > FileSize) {
    return refuse(From, fmt::format("tensor \"{}\" is larger than the file"sv,
                                    logText(Name)));
  }
  Out.Bytes = Size;
  return true;
}

bool readMetadata(const Source &From, JsonElement Value,
                  std::vector<std::string_view> &Keys) noexcept {
  JsonObject Metadata;
  if (Value.get(Metadata) != simdjson::SUCCESS) {
    return refuse(From, "__metadata__ is not an object"sv);
  }
  if (const auto Key = repeatedKey(Metadata, Keys)) {
    return refuse(From, fmt::format("__metadata__ has the key \"{}\" twice"sv,
                                    logText(*Key)));
  }
  for (const auto Member : Metadata) {
    if (Member.value.type() != JsonType::STRING) {
      return refuse(
          From,
          fmt::format(
              "__metadata__ has a value for \"{}\" that is not a string"sv,
              logText(Member.key)));
    }
    if (From.Kind == AdapterKind::Lora && Member.key == kAdapterConfigKey) {
      return refuse(
          From,
          "__metadata__ has adapter_config, which this check cannot judge"sv);
    }
  }
  return true;
}

// Finds the LoRA part at the end of a tensor name and checks the shape for that
// part.
bool readLoraTensor(const Source &From, std::string_view Name,
                    const Shape &TensorShape,
                    std::vector<LoraTensor> &Out) noexcept {
  const auto Suffix =
      std::find_if(kLoraSuffixes.begin(), kLoraSuffixes.end(),
                   [Name](const LoraSuffix &Candidate) noexcept {
                     return endsWith(Name, Candidate.Text);
                   });
  if (Suffix == kLoraSuffixes.end()) {
    return refuse(
        From,
        fmt::format("tensor \"{}\" is not a LoRA down, up, or alpha tensor"sv,
                    logText(Name)));
  }
  const auto Prefix = Name.substr(0, Name.size() - Suffix->Text.size());
  if (Prefix.empty()) {
    return refuse(From,
                  fmt::format("tensor \"{}\" has nothing before its suffix"sv,
                              logText(Name)));
  }
  const auto Count = TensorShape.DimCount;
  const auto &Dims = TensorShape.Dims;
  uint64_t Rank = 0;
  switch (Suffix->Part) {
  case LoraPart::Down:
    if (Count != 2 && Count != 4) {
      return refuse(
          From,
          fmt::format("down tensor \"{}\" is not [r, in] or [r, in, kh, kw]"sv,
                      logText(Name)));
    }
    // The merge code of the library fails an assertion on this shape. The same
    // applies to the up tensor below.
    if (Count == 4 && Dims[0] == 1 && Dims[1] == 1 && Dims[2] != 1) {
      return refuse(
          From,
          fmt::format("down tensor \"{}\" is [1, 1, kh, kw] with kh > 1"sv,
                      logText(Name)));
    }
    Rank = Dims[0];
    break;
  case LoraPart::Up:
    if (Count != 2 && (Count != 4 || Dims[2] != 1 || Dims[3] != 1)) {
      return refuse(
          From,
          fmt::format("up tensor \"{}\" is not [out, r] or [out, r, 1, 1]"sv,
                      logText(Name)));
    }
    if (Count == 4 && Dims[0] == 1 && Dims[1] != 1) {
      return refuse(From,
                    fmt::format("up tensor \"{}\" is [1, r, 1, 1] with r > 1"sv,
                                logText(Name)));
    }
    Rank = Dims[1];
    break;
  case LoraPart::Alpha:
    if (TensorShape.Elements != 1) {
      return refuse(From,
                    fmt::format("alpha \"{}\" has more than one element"sv,
                                logText(Name)));
    }
    break;
  }
  Out.push_back({Prefix, Suffix->Part, Suffix->Style, Rank});
  return true;
}

// Each prefix must have one down tensor and one up tensor with the same
// spelling and rank, and one alpha at most. The file must have one pair or
// more.
bool checkPairs(const Source &From, std::vector<LoraTensor> &Tensors) noexcept {
  std::sort(Tensors.begin(), Tensors.end(),
            [](const LoraTensor &A, const LoraTensor &B) noexcept {
              return std::tie(A.Prefix, A.Part) < std::tie(B.Prefix, B.Part);
            });
  size_t Pairs = 0;
  for (auto Begin = Tensors.begin(); Begin != Tensors.end();) {
    const auto Prefix = Begin->Prefix;
    const auto End = std::find_if(Begin, Tensors.end(),
                                  [Prefix](const LoraTensor &Tensor) noexcept {
                                    return Tensor.Prefix != Prefix;
                                  });
    std::array<const LoraTensor *, kPartNames.size()> Parts{};
    for (auto It = Begin; It != End; ++It) {
      const auto Index = static_cast<size_t>(It->Part);
      if (Parts[Index] != nullptr) {
        return refuse(From, fmt::format("\"{}\" has more than one {} tensor"sv,
                                        logText(Prefix), kPartNames[Index]));
      }
      Parts[Index] = &*It;
    }
    const auto *Down = Parts[static_cast<size_t>(LoraPart::Down)];
    const auto *Up = Parts[static_cast<size_t>(LoraPart::Up)];
    if (Down == nullptr || Up == nullptr) {
      return refuse(
          From,
          fmt::format("\"{}\" does not have both a down and an up tensor"sv,
                      logText(Prefix)));
    }
    if (Down->Style != Up->Style) {
      return refuse(
          From,
          fmt::format("\"{}\" mixes lora_down/lora_up with lora_A/lora_B"sv,
                      logText(Prefix)));
    }
    if (Down->Rank != Up->Rank) {
      return refuse(
          From,
          fmt::format("\"{}\" has a down rank of {} and an up rank of {}"sv,
                      logText(Prefix), Down->Rank, Up->Rank));
    }
    ++Pairs;
    Begin = End;
  }
  if (Pairs == 0) {
    return refuse(From, "the file has no down and up tensor pair"sv);
  }
  return true;
}

bool judge(const Source &From, uint64_t FileSize, std::string_view HeaderJson,
           uint64_t *Rows) noexcept {
  if (From.Kind == AdapterKind::Embedding &&
      FileSize > Limits::MaxEmbeddingFileBytes) {
    return refuse(From, fmt::format("the file is larger than {} bytes"sv,
                                    Limits::MaxEmbeddingFileBytes));
  }
  simdjson::dom::parser Parser;
  const simdjson::padded_string Padded(HeaderJson.data(), HeaderJson.size());
  JsonElement Document;
  if (const auto Err = Parser.parse(Padded).get(Document);
      Err != simdjson::SUCCESS) {
    return refuse(From, fmt::format("the header is not JSON: {}"sv,
                                    simdjson::error_message(Err)));
  }
  JsonObject Root;
  if (Document.get(Root) != simdjson::SUCCESS) {
    return refuse(From, "the header is not a JSON object"sv);
  }
  std::vector<std::string_view> Keys;
  if (const auto Key = repeatedKey(Root, Keys)) {
    return refuse(From, fmt::format("the header has the key \"{}\" twice"sv,
                                    logText(*Key)));
  }
  const uint64_t DataBytes = FileSize >= kLengthBytes + HeaderJson.size()
                                 ? FileSize - kLengthBytes - HeaderJson.size()
                                 : 0;
  uint64_t TensorBytes = 0;
  size_t TensorCount = 0;
  uint64_t MostRows = 1;
  std::vector<LoraTensor> LoraTensors;
  for (const auto Member : Root) {
    if (Member.key == kMetadataKey) {
      if (!readMetadata(From, Member.value, Keys)) {
        return false;
      }
      continue;
    }
    Shape TensorShape;
    if (!readTensor(From, FileSize, Member.key, Member.value, Keys,
                    TensorShape)) {
      return false;
    }
    // Tensors can share one data range of a small file. Each tensor alone
    // passes, but together they need more than the work area of the library.
    if (__builtin_add_overflow(TensorBytes, TensorShape.Bytes, &TensorBytes) ||
        TensorBytes > DataBytes) {
      return refuse(From,
                    fmt::format("the tensors together are larger than the {} "
                                "bytes of data after the header"sv,
                                DataBytes));
    }
    ++TensorCount;
    if (From.Kind == AdapterKind::Lora) {
      if (!readLoraTensor(From, Member.key, TensorShape, LoraTensors)) {
        return false;
      }
    } else if (TensorShape.DimCount < 1 || TensorShape.DimCount > 2) {
      return refuse(From,
                    fmt::format("embedding tensor \"{}\" has {} "
                                "dimensions, not one or two"sv,
                                logText(Member.key), TensorShape.DimCount));
    } else if (TensorShape.DimCount == 2) {
      MostRows = std::max(MostRows, TensorShape.Dims[0]);
    }
  }
  if (From.Kind == AdapterKind::Embedding) {
    if (TensorCount == 0) {
      return refuse(From, "the file has no tensor"sv);
    }
    if (Rows != nullptr) {
      *Rows = MostRows;
    }
    return true;
  }
  return checkPairs(From, LoraTensors);
}

bool fail(std::string &Why, std::string_view Reason) noexcept {
  Why.assign(Reason);
  return false;
}

// Reads the file size and the header. It uses the header length only after a
// check against the limit and the file size. It logs nothing. On failure, Why
// gives the reason.
bool readHeader(const std::filesystem::path &HostPath, uint64_t &FileSize,
                std::string &Header, std::string &Why) noexcept {
  std::error_code Error;
  const bool Regular = std::filesystem::is_regular_file(HostPath, Error);
  if (Error) {
    return fail(Why, fmt::format("cannot be examined: {}"sv, Error.message()));
  }
  if (!Regular) {
    return fail(Why, "is not a regular file"sv);
  }
  FileSize = std::filesystem::file_size(HostPath, Error);
  if (Error) {
    return fail(Why,
                fmt::format("has no readable size: {}"sv, Error.message()));
  }
  if (FileSize < kLengthBytes) {
    return fail(Why, "is shorter than 8 bytes"sv);
  }
  std::ifstream File(HostPath, std::ios::binary);
  if (!File.is_open()) {
    return fail(Why, "cannot be opened"sv);
  }
  std::array<char, kLengthBytes> LengthBytes{};
  File.read(LengthBytes.data(), static_cast<std::streamsize>(kLengthBytes));
  if (!File || File.gcount() != static_cast<std::streamsize>(kLengthBytes)) {
    return fail(Why, "has no readable header length"sv);
  }
  uint64_t Length = 0;
  for (size_t I = 0; I < kLengthBytes; ++I) {
    Length |= static_cast<uint64_t>(static_cast<unsigned char>(LengthBytes[I]))
              << (8 * I);
  }
  if (Length == 0) {
    return fail(Why, "has a header length of 0"sv);
  }
  if (Length > Limits::MaxSafetensorsHeaderBytes) {
    return fail(Why, fmt::format("has a header length of {}, above {} bytes"sv,
                                 Length, Limits::MaxSafetensorsHeaderBytes));
  }
  if (Length > FileSize - kLengthBytes) {
    return fail(
        Why,
        fmt::format("has a header length of {}, past the end of the file"sv,
                    Length));
  }
  Header.assign(static_cast<size_t>(Length), '\0');
  File.read(Header.data(), static_cast<std::streamsize>(Length));
  if (!File || File.gcount() != static_cast<std::streamsize>(Length)) {
    return fail(Why, "has no readable header"sv);
  }
  return true;
}

} // namespace

ErrNo checkAdapterHeader(AdapterKind Kind, uint64_t FileSize,
                         std::string_view HeaderJson, uint64_t *Rows) noexcept {
  return judge({Kind, nullptr}, FileSize, HeaderJson, Rows) ? ErrNo::Success
                                                            : ErrNo::LoadFailed;
}

ErrNo checkAdapterFile(AdapterKind Kind, const std::filesystem::path &HostPath,
                       uint64_t *Rows) noexcept {
  const Source From{Kind, &HostPath};
  uint64_t FileSize = 0;
  std::string Header;
  std::string Why;
  if (!readHeader(HostPath, FileSize, Header, Why)) {
    refuse(From, Why);
    return ErrNo::LoadFailed;
  }
  return judge(From, FileSize, Header, Rows) ? ErrNo::Success
                                             : ErrNo::LoadFailed;
}

bool isUnquantizedSafetensors(const std::filesystem::path &HostPath) noexcept {
  uint64_t FileSize = 0;
  std::string Header;
  std::string Why;
  if (!readHeader(HostPath, FileSize, Header, Why)) {
    return false;
  }
  simdjson::dom::parser Parser;
  const simdjson::padded_string Padded(Header.data(), Header.size());
  JsonElement Document;
  JsonObject Root;
  if (Parser.parse(Padded).get(Document) != simdjson::SUCCESS ||
      Document.get(Root) != simdjson::SUCCESS) {
    return false;
  }
  for (const auto Member : Root) {
    if (endsWith(Member.key, kComfyQuantSuffix)) {
      return false;
    }
    JsonObject Tensor;
    if (Member.value.get(Tensor) != simdjson::SUCCESS) {
      continue;
    }
    for (const auto Field : Tensor) {
      std::string_view Dtype;
      if (Field.key == "dtype"sv &&
          Field.value.get(Dtype) == simdjson::SUCCESS &&
          std::find(kFp8Dtypes.begin(), kFp8Dtypes.end(), Dtype) !=
              kFp8Dtypes.end()) {
        return false;
      }
    }
  }
  return true;
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
