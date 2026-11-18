// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "lld.h"

#include "aot/version.h"
#include "common/defines.h"
#include "common/filesystem.h"
#include "common/hash.h"
#include "common/spdlog.h"
#include "linker/universal_wasm_writer.h"

#include <lld/Common/Driver.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#if LLVM_VERSION_MAJOR < 14
#else
#include <lld/Common/CommonLinkerContext.h>
#endif
#if LLVM_VERSION_MAJOR < 17
#else
#if WASMEDGE_OS_MACOS
LLD_HAS_DRIVER(macho)
#elif WASMEDGE_OS_LINUX
LLD_HAS_DRIVER(elf)
#elif WASMEDGE_OS_WINDOWS
LLD_HAS_DRIVER(coff)
#endif
#endif

#if WASMEDGE_OS_MACOS
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#if WASMEDGE_OS_WINDOWS
#include <llvm/Object/COFF.h>
#endif

#if WASMEDGE_OS_LINUX
#define SYMBOL(X) X
#elif WASMEDGE_OS_MACOS
#define SYMBOL(X) "_" X
#elif WASMEDGE_OS_WINDOWS
#define SYMBOL(X) X
#endif

using namespace std::literals;

namespace WasmEdge::LLVM::LLD {

namespace {

static_assert(AOT::kHostOSType != AOT::OSType::Unsupported,
              "Unsupported operating system!");
static_assert(AOT::kHostArchitecture != AOT::Architecture::Unsupported,
              "Unsupported hardware architecture!");

#if WASMEDGE_OS_MACOS
// Get current OS version
std::string getOSVersion() noexcept {
  struct utsname Info;
  if (::uname(&Info)) {
    // default os version
    return "13.0.0"s;
  }
  std::string_view Release = Info.release;
  auto GetNum = [](std::string_view &String) noexcept {
    uint64_t Result = 0;
    while (!String.empty() && std::isdigit(String[0])) {
      Result = Result * 10 + (String[0] - '0');
      String = String.substr(1);
    }
    return Result;
  };
  auto SkipDot = [](std::string_view &String) noexcept {
    if (!String.empty() && String[0] == '.')
      String = String.substr(1);
  };
  uint64_t Major = GetNum(Release);
  SkipDot(Release);
  uint64_t Minor = GetNum(Release);
  SkipDot(Release);
  uint64_t Micro = GetNum(Release);

  if (Major == 0) {
    Major = 8;
  }
  if (Major <= 19) {
    Micro = 0;
    Minor = Major - 4;
    Major = 10;
  } else {
    Micro = 0;
    Minor = 0;
    Major = 11 + Major - 20;
  }

  return fmt::format("{}.{}.{}"sv, Major, Minor, Micro);
}
// Get current SDK version
std::string getSDKVersion() noexcept {
  // TODO: parse SDKSettings.json to get real version
  return "12.1"s;
}
#endif

inline constexpr bool startsWith(std::string_view Value,
                                 std::string_view Prefix) noexcept {
  return Value.size() >= Prefix.size() &&
         Value.substr(0, Prefix.size()) == Prefix;
}

std::filesystem::path uniquePath(const std::filesystem::path Model) noexcept {
  using size_type = std::filesystem::path::string_type::size_type;
  using value_type = std::filesystem::path::value_type;
  static const auto Hex = "0123456789abcdef"sv;
  std::uniform_int_distribution<size_type> Distribution(0, Hex.size() - 1);
  auto String = Model.native();
  for (size_type N = String.size(), I = 0; I < N; ++I) {
    if (String[I] == static_cast<value_type>('%')) {
      String[I] = static_cast<value_type>(Hex[Distribution(Hash::RandEngine)]);
    }
  }
  return String;
}

std::filesystem::path createTemp(const std::filesystem::path Model) noexcept {
  while (true) {
    auto Result = uniquePath(Model);
    std::error_code Error;
    if (!std::filesystem::exists(Result, Error)) {
      if (Error) {
        return {};
      }
      return Result;
    }
  }
}

struct TempFileRemover {
  std::filesystem::path Path;
  ~TempFileRemover() noexcept {
    std::error_code Error;
    std::filesystem::remove(Path, Error);
  }
};

} // namespace

void prepareModule([[maybe_unused]] Context LLContext,
                   [[maybe_unused]] Module &LLModule) noexcept {
#if WASMEDGE_OS_WINDOWS
  // create dummy dllmain function
  auto FTy = Type::getFunctionType(LLContext.getInt32Ty(), {});
  auto F = LLModule.addFunction(FTy, LLVMExternalLinkage, "_DllMainCRTStartup");
  F.setVisibility(LLVMProtectedVisibility);
  F.setDSOLocal(true);
  F.addFnAttr(Attribute::createString(LLContext, "no-stack-arg-probe"sv, {}));
  F.addFnAttr(Attribute::createEnum(LLContext, Core::StrictFP, 0));
  F.addFnAttr(
      Attribute::createEnum(LLContext, Core::UWTable, Core::UWTableDefault));
  F.addFnAttr(Attribute::createEnum(LLContext, Core::NoReturn, 0));
  Builder Builder(LLContext);
  Builder.positionAtEnd(BasicBlock::create(LLContext, F, "entry"));
  Builder.createRet(LLContext.getInt32(1u));

  auto A = LLModule.addAlias(FTy, F, "_fltused");
  A.setLinkage(LLVMExternalLinkage);
  A.setVisibility(LLVMProtectedVisibility);
  A.setDSOLocal(true);
#endif
}

Expect<void> outputNativeLibrary(const std::filesystem::path &OutputPath,
                                 const MemoryBuffer &OSVec) noexcept {
  spdlog::info("output start"sv);
  std::filesystem::path ObjectName;
  {
    // tempfile
    std::filesystem::path OPath(OutputPath);
#if WASMEDGE_OS_WINDOWS
    OPath.replace_extension("%%%%%%%%%%.obj"sv);
#else
    OPath.replace_extension("%%%%%%%%%%.o"sv);
#endif
    ObjectName = createTemp(OPath);
    if (ObjectName.empty()) {
      // TODO:return error
      spdlog::error("so file creation failed:{}"sv, u8string(OPath));
      return Unexpect(ErrCode::Value::IllegalPath);
    }
    std::ofstream OS(ObjectName, std::ios_base::binary);
    OS.write(OSVec.data(), static_cast<std::streamsize>(OSVec.size()));
    OS.close();
  }
  TempFileRemover ObjectRemover{ObjectName};

  // link
  // Serialize LLD invocations: CommonLinkerContext is a global singleton, so
  // concurrent link() calls from multiple threads would corrupt it.
  static std::mutex LldMutex;
  std::lock_guard<std::mutex> Lock(LldMutex);
  bool LinkResult = false;
#if WASMEDGE_OS_MACOS
  const auto OSVersion = getOSVersion();
  const auto SDKVersion = getSDKVersion();
#if LLVM_VERSION_MAJOR < 14
  LinkResult = lld::mach_o::link(
#else
  // LLVM 14 replaces the older mach_o lld implementation with the new one.
  // So we need to change the namespace after LLVM 14.x was released.
  // Reference: https://reviews.llvm.org/D114842
  LinkResult = lld::macho::link(
#endif
      std::initializer_list<const char *>{
          "lld", "-arch",
#if defined(__x86_64__)
          "x86_64",
#elif defined(__aarch64__)
          "arm64",
#else
#error Unsupported architecture on the MacOS!
#endif
#if LLVM_VERSION_MAJOR < 14
          "-sdk_version", SDKVersion.c_str(),
#else
          // LLVM 14 replaces the older mach_o lld implementation with the new
          // one. And it require -arch and -platform_version to always be
          // specified. Reference: https://reviews.llvm.org/D97799
          "-platform_version", "macos", OSVersion.c_str(), SDKVersion.c_str(),
#endif
          "-dylib", "-demangle", "-macosx_version_min", OSVersion.c_str(),
          "-syslibroot", "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
          u8string(ObjectName).c_str(), "-o", u8string(OutputPath).c_str()},
#elif WASMEDGE_OS_LINUX
  LinkResult = lld::elf::link(
      std::initializer_list<const char *>{"ld.lld", "--eh-frame-hdr",
                                          "--shared", "--gc-sections",
                                          "--discard-all", ObjectName.c_str(),
                                          "-o", u8string(OutputPath).c_str()},
#elif WASMEDGE_OS_WINDOWS
  LinkResult = lld::coff::link(
      std::initializer_list<const char *>{
          "lld-link", "-dll", "-base:0", "-nologo",
          u8string(ObjectName).c_str(),
          ("-out:" + u8string(OutputPath)).c_str()},
#endif

#if LLVM_VERSION_MAJOR < 10
      false, llvm::errs()
#elif LLVM_VERSION_MAJOR < 14
      false, llvm::outs(), llvm::errs()
#else
      llvm::outs(), llvm::errs(), false, false
#endif
  );

#if LLVM_VERSION_MAJOR < 14
#else
  lld::CommonLinkerContext::destroy();
#endif

  if (!LinkResult) {
    spdlog::error("link error"sv);
    return Unexpect(ErrCode::Value::IllegalPath);
  }
#if WASMEDGE_OS_WINDOWS
  {
    std::error_code Error;
    std::filesystem::path LibPath(OutputPath);
    LibPath.replace_extension(".lib"sv);
    std::filesystem::remove(LibPath, Error);
  }
#endif
  spdlog::info("codegen done"sv);

#if WASMEDGE_OS_MACOS
  // codesign
  {
    pid_t PID = ::fork();
    if (PID == -1) {
      spdlog::error("codesign error on fork:{}"sv, std::strerror(errno));
    } else if (PID == 0) {
      execlp("/usr/bin/codesign", "codesign", "-s", "-",
             u8string(OutputPath).c_str(), nullptr);
      ::_exit(127);
    } else {
      int ChildStat;
      waitpid(PID, &ChildStat, 0);
      if (const int Status = WEXITSTATUS(ChildStat); Status != 0) {
        spdlog::error("codesign exited with status {}"sv, Status);
      }
    }
  }
#endif

  return {};
}

Expect<void> outputWasmLibrary(Context LLContext,
                               const std::filesystem::path &OutputPath,
                               Span<const Byte> Data,
                               const MemoryBuffer &OSVec) noexcept {
  std::filesystem::path SharedObjectName;
  {
    // tempfile
    std::filesystem::path SOPath(OutputPath);
    SOPath.replace_extension("%%%%%%%%%%" WASMEDGE_LIB_EXTENSION);
    SharedObjectName = createTemp(SOPath);
    if (SharedObjectName.empty()) {
      // TODO:return error
      spdlog::error("so file creation failed:{}"sv, u8string(SOPath));
      return Unexpect(ErrCode::Value::IllegalPath);
    }
    std::ofstream OS(SharedObjectName, std::ios_base::binary);
    OS.write(OSVec.data(), static_cast<std::streamsize>(OSVec.size()));
    OS.close();
  }
  TempFileRemover SharedObjectRemover{SharedObjectName};

  EXPECTED_TRY(outputNativeLibrary(SharedObjectName, OSVec));

  MemoryBuffer SOFile;
  if (auto [Res, ErrorMessage] =
          MemoryBuffer::getFile(u8string(SharedObjectName).c_str());
      unlikely(ErrorMessage)) {
    spdlog::error("object file open error:{}"sv, ErrorMessage.string_view());
    return Unexpect(ErrCode::Value::IllegalPath);
  } else {
    SOFile = std::move(Res);
  }

  Binary ObjFile;
  if (auto [Res, ErrorMessage] = Binary::create(SOFile, LLContext);
      unlikely(ErrorMessage)) {
    spdlog::error("object file parse error:{}"sv, ErrorMessage.string_view());
    return Unexpect(ErrCode::Value::IllegalPath);
  } else {
    ObjFile = std::move(Res);
  }

  std::vector<std::pair<std::string, uint64_t>> SymbolTable;
#if !WASMEDGE_OS_WINDOWS
  for (auto Symbol = ObjFile.symbols(); Symbol && !ObjFile.isSymbolEnd(Symbol);
       Symbol.next()) {
    SymbolTable.emplace_back(Symbol.getName(), Symbol.getAddress());
  }
#else
  for (auto &Symbol :
       llvm::object::unwrap<llvm::object::COFFObjectFile>(ObjFile.unwrap())
           ->export_directories()) {
    llvm::StringRef Name;
    if (auto Error = Symbol.getSymbolName(Name); unlikely(!!Error)) {
      continue;
    } else if (Name.empty()) {
      continue;
    }
    uint32_t Offset = 0;
    if (auto Error = Symbol.getExportRVA(Offset); unlikely(!!Error)) {
      continue;
    }
    SymbolTable.emplace_back(Name.str(), Offset);
  }
#endif
  Linker::UniversalWasmTables Tables;
  uint64_t CodesMin = std::numeric_limits<uint64_t>::max();
  for (const auto &[Name, Address] : SymbolTable) {
    if (!startsWith(Name, SYMBOL(""sv))) {
      continue;
    }
    const auto Semantic =
        AOT::semanticSymbol(std::string_view(Name).substr(SYMBOL(""sv).size()));
    if (!Semantic) {
      continue;
    }
    switch (Semantic->Kind) {
    case AOT::SemanticSymbolKind::Version:
      Tables.Version = Address;
      break;
    case AOT::SemanticSymbolKind::Intrinsics:
      Tables.Intrinsics = Address;
      break;
    case AOT::SemanticSymbolKind::Type:
      if (Tables.Types.size() < Semantic->Index + 1) {
        Tables.Types.resize(Semantic->Index + 1);
      }
      Tables.Types[Semantic->Index] = Address;
      break;
    case AOT::SemanticSymbolKind::Code:
      if (Tables.Codes.size() < Semantic->Index + 1) {
        Tables.Codes.resize(Semantic->Index + 1);
      }
      CodesMin = std::min(CodesMin, Semantic->Index);
      Tables.Codes[Semantic->Index] = Address;
      break;
    }
  }
  if (CodesMin != std::numeric_limits<uint64_t>::max()) {
    Tables.Codes.erase(Tables.Codes.begin(),
                       Tables.Codes.begin() + static_cast<int64_t>(CodesMin));
  }

  std::vector<Linker::UniversalWasmSection> Sections;
  for (auto Section = ObjFile.sections(); !ObjFile.isSectionEnd(Section);
       Section.next()) {
    if (Section.getSize() == 0) {
      continue;
    }
    AOT::SectionKind Kind;
    if (Section.isEHFrame() || Section.isPData()) {
      Kind = AOT::SectionKind::Unwind;
    } else if (Section.isText()) {
      Kind = AOT::SectionKind::Text;
    } else if (Section.isData()) {
      Kind = AOT::SectionKind::Data;
    } else if (Section.isBSS()) {
      Kind = AOT::SectionKind::BSS;
    } else {
      continue;
    }
    std::vector<Byte> Content(Section.getSize());
    if (!Section.isVirtual()) {
      if (auto Res = Section.getContents(); unlikely(Res.empty())) {
        assumingUnreachable();
      } else {
        std::copy(Res.begin(), Res.end(),
                  reinterpret_cast<char *>(Content.data()));
      }
    }
    Sections.push_back(Linker::UniversalWasmSection{
        Kind, Section.getAddress(), Content.size(), std::move(Content)});
  }

  std::vector<Byte> Bytes;
  Linker::Writer Encoded(Bytes);
  if (!Linker::UniversalWasmWriter::encode(Encoded, Data, Tables, Sections)) {
    spdlog::error("universal Wasm encoding failed."sv);
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  std::ofstream OS(OutputPath, std::ios_base::binary);
  OS.write(reinterpret_cast<const char *>(Bytes.data()),
           static_cast<std::streamsize>(Bytes.size()));
  OS.close();
  if (!OS) {
    spdlog::error("output failed."sv);
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  spdlog::info("output done"sv);
  return {};
}

} // namespace WasmEdge::LLVM::LLD
