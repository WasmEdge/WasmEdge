// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llvm/codegen.h"

#include "aot/version.h"
#include "common/defines.h"
#include "data.h"
#include "llvm.h"

#include <charconv>
#include <fstream>
#include <lld/Common/Driver.h>
#include <random>
#include <sstream>

#if LLVM_VERSION_MAJOR >= 14
#include <lld/Common/CommonLinkerContext.h>
#endif
#if LLVM_VERSION_MAJOR >= 17
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

namespace LLVM = WasmEdge::LLVM;
using namespace std::literals;

namespace {

using namespace WasmEdge;

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
// Get current SDK version in pair
std::pair<uint32_t, uint32_t> getSDKVersionPair() noexcept {
  // TODO: parse SDKSettings.json to get real version
  return {UINT32_C(12), UINT32_C(1)};
}
#endif

Expect<void> WriteByte(std::ostream &OS, uint8_t Data) noexcept {
  OS.put(static_cast<char>(Data));
  return {};
}

Expect<void> WriteU32(std::ostream &OS, uint32_t Data) noexcept {
  do {
    uint8_t Byte = static_cast<uint8_t>(Data & UINT32_C(0x7f));
    Data >>= 7;
    if (Data > UINT32_C(0)) {
      Byte |= UINT8_C(0x80);
    }
    WriteByte(OS, Byte);
  } while (Data > UINT32_C(0));
  return {};
}

Expect<void> WriteU64(std::ostream &OS, uint64_t Data) noexcept {
  do {
    uint8_t Byte = static_cast<uint8_t>(Data & UINT64_C(0x7f));
    Data >>= 7;
    if (Data > UINT64_C(0)) {
      Byte |= UINT8_C(0x80);
    }
    WriteByte(OS, Byte);
  } while (Data > UINT64_C(0));
  return {};
}

Expect<void> WriteName(std::ostream &OS, std::string_view Data) noexcept {
  WriteU32(OS, static_cast<uint32_t>(Data.size()));
  for (const auto C : Data) {
    WriteByte(OS, static_cast<uint8_t>(C));
  }
  return {};
}

inline constexpr bool startsWith(std::string_view Value,
                                 std::string_view Prefix) noexcept {
  return Value.size() >= Prefix.size() &&
         Value.substr(0, Prefix.size()) == Prefix;
}

std::filesystem::path uniquePath(const std::filesystem::path Model) noexcept {
  using size_type = std::filesystem::path::string_type::size_type;
  using value_type = std::filesystem::path::value_type;
  static const auto Hex = "0123456789abcdef"sv;
  std::random_device Device;
  std::default_random_engine Engine(Device());
  std::uniform_int_distribution<size_type> Distribution(0, Hex.size() - 1);
  auto String = Model.native();
  for (size_type N = String.size(), I = 0; I < N; ++I) {
    if (String[I] == static_cast<value_type>('%')) {
      String[I] = static_cast<value_type>(Hex[Distribution(Engine)]);
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

// Write output object and link
Expect<void> outputNativeLibrary(const std::filesystem::path &OutputPath,
                                 const LLVM::MemoryBuffer &OSVec) noexcept {
  spdlog::info("output start");
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
      spdlog::error("so file creation failed:{}", OPath.u8string());
      return Unexpect(ErrCode::Value::IllegalPath);
    }
    std::ofstream OS(ObjectName, std::ios_base::binary);
    OS.write(OSVec.data(), static_cast<std::streamsize>(OSVec.size()));
    OS.close();
  }

  // link
  bool LinkResult = false;
#if WASMEDGE_OS_MACOS
  const auto OSVersion = getOSVersion();
  const auto SDKVersion = getSDKVersion();
#if LLVM_VERSION_MAJOR >= 14
  // LLVM 14 replaces the older mach_o lld implementation with the new one.
  // So we need to change the namespace after LLVM 14.x released.
  // Reference: https://reviews.llvm.org/D114842
  LinkResult = lld::macho::link(
#else
  LinkResult = lld::mach_o::link(
#endif
      std::initializer_list<const char *> {
        "lld", "-arch",
#if defined(__x86_64__)
            "x86_64",
#elif defined(__aarch64__)
            "arm64",
#else
#error Unsupported architecture on the MacOS!
#endif
#if LLVM_VERSION_MAJOR >= 14
            // LLVM 14 replaces the older mach_o lld implementation with the new
            // one. And it require -arch and -platform_version to always be
            // specified. Reference: https://reviews.llvm.org/D97799
            "-platform_version", "macos", OSVersion.c_str(), SDKVersion.c_str(),
#else
            "-sdk_version", SDKVersion.c_str(),
#endif
            "-dylib", "-demangle", "-macosx_version_min", OSVersion.c_str(),
            "-syslibroot",
            "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
            ObjectName.u8string().c_str(), "-o", OutputPath.u8string().c_str()
      },
#elif WASMEDGE_OS_LINUX
  LinkResult = lld::elf::link(
      std::initializer_list<const char *>{"ld.lld", "--eh-frame-hdr",
                                          "--shared", "--gc-sections",
                                          "--discard-all", ObjectName.c_str(),
                                          "-o", OutputPath.u8string().c_str()},
#elif WASMEDGE_OS_WINDOWS
  LinkResult = lld::coff::link(
      std::initializer_list<const char *>{
          "lld-link", "-dll", "-base:0", "-nologo",
          ObjectName.u8string().c_str(),
          ("-out:" + OutputPath.u8string()).c_str()},
#endif

#if LLVM_VERSION_MAJOR >= 14
      llvm::outs(), llvm::errs(), false, false
#elif LLVM_VERSION_MAJOR >= 10
      false, llvm::outs(), llvm::errs()
#else
      false, llvm::errs()
#endif
  );

#if LLVM_VERSION_MAJOR >= 14
  lld::CommonLinkerContext::destroy();
#endif

  if (LinkResult) {
    std::error_code Error;
    std::filesystem::remove(ObjectName, Error);
#if WASMEDGE_OS_WINDOWS
    std::filesystem::path LibPath(OutputPath);
    LibPath.replace_extension(".lib"sv);
    std::filesystem::remove(LibPath, Error);
#endif

    spdlog::info("codegen done");
  } else {
    spdlog::error("link error");
  }

#if WASMEDGE_OS_MACOS
  // codesign
  if (LinkResult) {
    pid_t PID = ::fork();
    if (PID == -1) {
      spdlog::error("codesign error on fork:{}", std::strerror(errno));
    } else if (PID == 0) {
      execlp("/usr/bin/codesign", "codesign", "-s", "-",
             OutputPath.u8string().c_str(), nullptr);
      std::exit(256);
    } else {
      int ChildStat;
      waitpid(PID, &ChildStat, 0);
      if (const int Status = WEXITSTATUS(ChildStat); Status != 0) {
        spdlog::error("codesign exited with status {}", Status);
      }
    }
  }
#endif

  return {};
}

Expect<void> outputWasmLibrary(LLVM::Context LLContext,
                               const std::filesystem::path &OutputPath,
                               Span<const Byte> Data,
                               const LLVM::MemoryBuffer &OSVec) noexcept {
  std::filesystem::path SharedObjectName;
  {
    // tempfile
    std::filesystem::path SOPath(OutputPath);
    SOPath.replace_extension("%%%%%%%%%%" WASMEDGE_LIB_EXTENSION);
    SharedObjectName = createTemp(SOPath);
    if (SharedObjectName.empty()) {
      // TODO:return error
      spdlog::error("so file creation failed:{}", SOPath.u8string());
      return Unexpect(ErrCode::Value::IllegalPath);
    }
    std::ofstream OS(SharedObjectName, std::ios_base::binary);
    OS.write(OSVec.data(), static_cast<std::streamsize>(OSVec.size()));
    OS.close();
  }

  if (auto Res = outputNativeLibrary(SharedObjectName, OSVec); unlikely(!Res)) {
    return Unexpect(Res);
  }

  LLVM::MemoryBuffer SOFile;
  if (auto [Res, ErrorMessage] =
          LLVM::MemoryBuffer::getFile(SharedObjectName.u8string().c_str());
      unlikely(ErrorMessage)) {
    spdlog::error("object file open error:{}", ErrorMessage.string_view());
    return Unexpect(ErrCode::Value::IllegalPath);
  } else {
    SOFile = std::move(Res);
  }

  LLVM::Binary ObjFile;
  if (auto [Res, ErrorMessage] = LLVM::Binary::create(SOFile, LLContext);
      unlikely(ErrorMessage)) {
    spdlog::error("object file parse error:{}", ErrorMessage.string_view());
    return Unexpect(ErrCode::Value::IllegalPath);
  } else {
    ObjFile = std::move(Res);
  }

  std::string OSCustomSecVec;
  {
    std::ostringstream OS;
    WriteName(OS, "wasmedge"sv);
    WriteU32(OS, AOT::kBinaryVersion);

#if WASMEDGE_OS_LINUX
    WriteByte(OS, UINT8_C(1));
#elif WASMEDGE_OS_MACOS
    WriteByte(OS, UINT8_C(2));
#elif WASMEDGE_OS_WINDOWS
    WriteByte(OS, UINT8_C(3));
#else
#error Unsupported operating system!
#endif

#if defined(__x86_64__)
    WriteByte(OS, UINT8_C(1));
#elif defined(__aarch64__)
    WriteByte(OS, UINT8_C(2));
#elif defined(__riscv) && __riscv_xlen == 64
    WriteByte(OS, UINT8_C(3));
#elif defined(__arm__) && __ARM_ARCH == 7
    WriteByte(OS, UINT8_C(4));
#else
#error Unsupported hardware architecture!
#endif

    std::vector<std::pair<std::string, uint64_t>> SymbolTable;
#if !WASMEDGE_OS_WINDOWS
    for (auto Symbol = ObjFile.symbols();
         Symbol && !ObjFile.isSymbolEnd(Symbol); Symbol.next()) {
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
    uint64_t VersionAddress = 0, IntrinsicsAddress = 0;
    std::vector<uint64_t> Types;
    std::vector<uint64_t> Codes;
    uint64_t CodesMin = std::numeric_limits<uint64_t>::max();
    for (const auto &[Name, Address] : SymbolTable) {
      if (Name == SYMBOL("version"sv)) {
        VersionAddress = Address;
      } else if (Name == SYMBOL("intrinsics"sv)) {
        IntrinsicsAddress = Address;
      } else if (startsWith(Name, SYMBOL("t"sv))) {
        uint64_t Index = 0;
        std::from_chars(Name.data() + SYMBOL("t"sv).size(),
                        Name.data() + Name.size(), Index);
        if (Types.size() < Index + 1) {
          Types.resize(Index + 1);
        }
        Types[Index] = Address;
      } else if (startsWith(Name, SYMBOL("f"sv))) {
        uint64_t Index = 0;
        std::from_chars(Name.data() + SYMBOL("f"sv).size(),
                        Name.data() + Name.size(), Index);
        if (Codes.size() < Index + 1) {
          Codes.resize(Index + 1);
        }
        CodesMin = std::min(CodesMin, Index);
        Codes[Index] = Address;
      }
    }
    if (CodesMin != std::numeric_limits<uint64_t>::max()) {
      Codes.erase(Codes.begin(),
                  Codes.begin() + static_cast<int64_t>(CodesMin));
    }
    WriteU64(OS, VersionAddress);
    WriteU64(OS, IntrinsicsAddress);
    WriteU64(OS, Types.size());
    for (const uint64_t TypeAddress : Types) {
      WriteU64(OS, TypeAddress);
    }
    WriteU64(OS, Codes.size());
    for (const uint64_t CodeAddress : Codes) {
      WriteU64(OS, CodeAddress);
    }

    uint32_t SectionCount = 0;
    for (auto Section = ObjFile.sections(); !ObjFile.isSectionEnd(Section);
         Section.next()) {
      if (Section.getSize() == 0) {
        continue;
      }
      if (!Section.isEHFrame() && !Section.isPData() && !Section.isText() &&
          !Section.isData() && !Section.isBSS()) {
        continue;
      }
      ++SectionCount;
    }
    WriteU32(OS, SectionCount);

    for (auto Section = ObjFile.sections(); !ObjFile.isSectionEnd(Section);
         Section.next()) {
      if (Section.getSize() == 0) {
        continue;
      }
      std::vector<char> Content;
      if (auto Res = Section.getContents(); unlikely(Res.empty())) {
        assumingUnreachable();
      } else {
        Content.assign(Res.begin(), Res.end());
      }
      if (Section.isEHFrame() || Section.isPData()) {
        WriteByte(OS, UINT8_C(4));
      } else if (Section.isText()) {
        WriteByte(OS, UINT8_C(1));
      } else if (Section.isData()) {
        WriteByte(OS, UINT8_C(2));
      } else if (Section.isBSS()) {
        WriteByte(OS, UINT8_C(3));
      } else {
        continue;
      }

      WriteU64(OS, Section.getAddress());
      WriteU64(OS, Content.size());
      WriteName(OS, std::string_view(Content.data(), Content.size()));
    }
    OSCustomSecVec = OS.str();
  }

  spdlog::info("output start");

  std::ofstream OS(OutputPath, std::ios_base::binary);
  if (!OS) {
    spdlog::error("output failed.");
    return Unexpect(ErrCode::Value::IllegalPath);
  }
  OS.write(reinterpret_cast<const char *>(Data.data()),
           static_cast<std::streamsize>(Data.size()));
  // Custom section id
  WriteByte(OS, UINT8_C(0x00));
  WriteName(OS, std::string_view(OSCustomSecVec.data(), OSCustomSecVec.size()));

  std::error_code Error;
  std::filesystem::remove(SharedObjectName, Error);

  spdlog::info("output done");
  return {};
}

} // namespace

namespace WasmEdge::LLVM {

Expect<void> CodeGen::codegen(Span<const Byte> WasmData, Data D,
                              std::filesystem::path OutputPath) noexcept {
  auto LLContext = D.extract().LLContext();
  auto &LLModule = D.extract().LLModule;
  auto &TM = D.extract().TM;
  std::filesystem::path LLPath(OutputPath);
  LLPath.replace_extension("ll"sv);

#if WASMEDGE_OS_WINDOWS
  {
    // create dummy dllmain function
    auto FTy = LLVM::Type::getFunctionType(LLContext.getInt32Ty(), {});
    auto F =
        LLModule.addFunction(FTy, LLVMExternalLinkage, "_DllMainCRTStartup");
    F.setVisibility(LLVMProtectedVisibility);
    F.setDSOLocal(true);
    F.addFnAttr(
        LLVM::Attribute::createString(LLContext, "no-stack-arg-probe"sv, {}));
    F.addFnAttr(
        LLVM::Attribute::createEnum(LLContext, LLVM::Core::StrictFP, 0));
    F.addFnAttr(LLVM::Attribute::createEnum(LLContext, LLVM::Core::UWTable,
                                            LLVM::Core::UWTableDefault));
    F.addFnAttr(
        LLVM::Attribute::createEnum(LLContext, LLVM::Core::NoReturn, 0));
    LLVM::Builder Builder(LLContext);
    Builder.positionAtEnd(LLVM::BasicBlock::create(LLContext, F, "entry"));
    Builder.createRet(LLContext.getInt32(1u));

    auto A = LLModule.addAlias(F.getType(), F, "_fltused");
    A.setLinkage(LLVMExternalLinkage);
    A.setVisibility(LLVMProtectedVisibility);
    A.setDSOLocal(true);
  }
#endif
#if WASMEDGE_OS_MACOS
  {
    const auto [Major, Minor] = getSDKVersionPair();
    LLModule.addFlag(LLVMModuleFlagBehaviorError, "SDK Version"sv,
                     LLVM::Value::getConstVector32(LLContext, {Major, Minor}));
  }
#endif

  if (Conf.getCompilerConfigure().getOutputFormat() !=
      CompilerConfigure::OutputFormat::Wasm) {
    // create wasm.code and wasm.size
    auto Int32Ty = LLContext.getInt32Ty();
    auto Content = LLVM::Value::getConstString(
        LLContext,
        {reinterpret_cast<const char *>(WasmData.data()), WasmData.size()},
        true);
    LLModule.addGlobal(Content.getType(), true, LLVMExternalLinkage, Content,
                       "wasm.code");
    LLModule.addGlobal(Int32Ty, true, LLVMExternalLinkage,
                       LLVM::Value::getConstInt(Int32Ty, WasmData.size()),
                       "wasm.size");
    for (auto Fn = LLModule.getFirstFunction(); Fn; Fn = Fn.getNextFunction()) {
      if (Fn.getLinkage() == LLVMInternalLinkage) {
        Fn.setLinkage(LLVMExternalLinkage);
        Fn.setVisibility(LLVMProtectedVisibility);
        Fn.setDSOLocal(true);
        Fn.setDLLStorageClass(LLVMDLLExportStorageClass);
      }
    }
  } else {
    for (auto Fn = LLModule.getFirstFunction(); Fn; Fn = Fn.getNextFunction()) {
      if (Fn.getLinkage() == LLVMInternalLinkage) {
        Fn.setLinkage(LLVMPrivateLinkage);
        Fn.setDSOLocal(true);
        Fn.setDLLStorageClass(LLVMDefaultStorageClass);
      }
    }
  }

  // set dllexport
  for (auto GV = LLModule.getFirstGlobal(); GV; GV = GV.getNextGlobal()) {
    if (GV.getLinkage() == LLVMExternalLinkage) {
      GV.setVisibility(LLVMProtectedVisibility);
      GV.setDSOLocal(true);
      GV.setDLLStorageClass(LLVMDLLExportStorageClass);
    }
  }

  if (Conf.getCompilerConfigure().isDumpIR()) {
    if (auto ErrorMessage = LLModule.printModuleToFile("wasm.ll");
        unlikely(ErrorMessage)) {
      spdlog::error("wasm.ll open error:{}", ErrorMessage.string_view());
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
    }
  }

  spdlog::info("codegen start");
  // codegen
  {
    if (Conf.getCompilerConfigure().isDumpIR()) {
      if (auto ErrorMessage = LLModule.printModuleToFile("wasm-opt.ll")) {
        // TODO:return error
        spdlog::error("printModuleToFile failed");
        return Unexpect(ErrCode::Value::IllegalPath);
      }
    }

    auto [OSVec, ErrorMessage] =
        TM.emitToMemoryBuffer(LLModule, LLVMObjectFile);
    if (ErrorMessage) {
      // TODO:return error
      spdlog::error("addPassesToEmitFile failed");
      return Unexpect(ErrCode::Value::IllegalPath);
    }

    if (Conf.getCompilerConfigure().getOutputFormat() ==
        CompilerConfigure::OutputFormat::Wasm) {
      if (auto Res = outputWasmLibrary(LLContext, OutputPath, WasmData, OSVec);
          unlikely(!Res)) {
        return Unexpect(Res);
      }
    } else {
      if (auto Res = outputNativeLibrary(OutputPath, OSVec); unlikely(!Res)) {
        return Unexpect(Res);
      }
    }
  }

  return {};
}

} // namespace WasmEdge::LLVM
