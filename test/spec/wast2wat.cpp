// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

/// \file
/// This tool extracts the WAT modules from the WAST files. The modules become
/// the seed corpus of a fuzzer.
/// Usage: wast2wat [--keep-going] <input-dir> <output-dir>
///
/// The output keeps the directory tree of the input. For example, the modules
/// of "wasm-3.0/memory/memory.wast" go to "wasm-3.0/memory/memory-000.wat",
/// "wasm-3.0/memory/memory-001.wat", and so on. Two input files then never
/// write to the same output file. A WAST file that does not parse fails the
/// run, unless --keep-going makes the run a best-effort extraction.

#include "common/filesystem.h"
#include "wast.h"
#include "wast_parser.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
using namespace std::literals;

namespace {
// Append the text modules of Commands to Modules in source order. A thread
// holds its commands in SubCommands, so the function also walks them.
void collectTextModules(
    const std::vector<WasmEdge::Wast::ScriptCommand> &Commands,
    std::vector<std::string_view> &Modules) {
  for (const auto &Cmd : Commands) {
    if (!Cmd.ModuleSource.empty() &&
        Cmd.ModType == WasmEdge::Wast::ModuleType::Text) {
      Modules.push_back(Cmd.ModuleSource);
    }
    collectTextModules(Cmd.SubCommands, Modules);
  }
}
} // namespace

int main(int Argc, char *Argv[]) {
  bool KeepGoing = false;
  int ArgIdx = 1;
  if (Argc > 1 && Argv[1] == "--keep-going"sv) {
    KeepGoing = true;
    ++ArgIdx;
  }
  if (Argc - ArgIdx != 2) {
    std::fprintf(stderr, "Usage: %s [--keep-going] <input-dir> <output-dir>\n",
                 Argv[0]);
    return 1;
  }

  fs::path InputDir(Argv[ArgIdx]);
  fs::path OutputDir(Argv[ArgIdx + 1]);

  if (!fs::is_directory(InputDir)) {
    std::fprintf(stderr, "Error: %s is not a directory\n", Argv[ArgIdx]);
    return 1;
  }

  uint32_t TotalFiles = 0;
  uint32_t TotalModules = 0;
  uint32_t ParseErrors = 0;
  uint32_t WriteErrors = 0;

  for (auto &Entry : fs::recursive_directory_iterator(InputDir)) {
    if (!Entry.is_regular_file() || Entry.path().extension() != ".wast") {
      continue;
    }

    auto Res = WasmEdge::Wast::parseWast(Entry.path());
    if (!Res) {
      std::fprintf(stderr, "%s: failed to parse %s\n",
                   KeepGoing ? "Warning" : "Error",
                   WasmEdge::u8string(Entry.path()).c_str());
      ++ParseErrors;
      continue;
    }
    ++TotalFiles;

    const auto RelPath = fs::relative(Entry.path(), InputDir);
    const auto OutDir = OutputDir / RelPath.parent_path();
    const auto Stem = RelPath.stem();
    std::error_code EC;
    fs::create_directories(OutDir, EC);
    if (EC) {
      std::fprintf(stderr, "Error: cannot create %s\n",
                   WasmEdge::u8string(OutDir).c_str());
      ++WriteErrors;
      continue;
    }

    std::vector<std::string_view> Modules;
    collectTextModules(Res->Commands, Modules);
    uint32_t ModIdx = 0;
    for (const auto Module : Modules) {
      char Suffix[16];
      std::snprintf(Suffix, sizeof(Suffix), "-%03u.wat", ModIdx++);
      auto FileName = Stem;
      FileName += Suffix;
      const auto OutPath = OutDir / FileName;

      // A stream that failed to open is in a failed state, so the check after
      // the write also covers the open. The stream can keep the bytes in its
      // buffer, so close it before the check. The close writes the buffer.
      std::ofstream Out(OutPath, std::ios::binary);
      Out.write(Module.data(), static_cast<std::streamsize>(Module.size()));
      Out.close();
      if (!Out) {
        std::fprintf(stderr, "Error: cannot write %s\n",
                     WasmEdge::u8string(OutPath).c_str());
        ++WriteErrors;
        continue;
      }
      ++TotalModules;
    }
  }

  std::printf("Processed %u WAST files, extracted %u modules", TotalFiles,
              TotalModules);
  if (ParseErrors > 0) {
    std::printf(", %u parse errors", ParseErrors);
  }
  if (WriteErrors > 0) {
    std::printf(", %u write errors", WriteErrors);
  }
  std::printf("\n");
  if (WriteErrors > 0 || (ParseErrors > 0 && !KeepGoing)) {
    return 1;
  }
  return 0;
}
