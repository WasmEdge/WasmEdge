// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

/// \file
/// This tool extracts the WAT modules from the WAST files. The modules become
/// the seed corpus of a fuzzer.
/// Usage: wast2wat <input-dir> <output-dir>

#include "common/filesystem.h"
#include "wast.h"
#include "wast_parser.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// Build the prefix from the suite name and the test name. For example,
// "wasm-3.0/memory/memory.wast" gives "wasm-3.0-memory". The stem of the file
// name is left out when it repeats the name of the parent directory.
std::string makePrefix(const fs::path &RelPath) {
  std::string Prefix;
  for (const auto &Part : RelPath.parent_path()) {
    if (!Prefix.empty()) {
      Prefix += '-';
    }
    Prefix += Part.string();
  }
  const auto Stem = RelPath.stem().string();
  if (Prefix.empty() || Stem != RelPath.parent_path().filename().string()) {
    if (!Prefix.empty()) {
      Prefix += '-';
    }
    Prefix += Stem;
  }
  return Prefix;
}

} // namespace

int main(int Argc, char *Argv[]) {
  if (Argc != 3) {
    std::fprintf(stderr, "Usage: %s <input-dir> <output-dir>\n", Argv[0]);
    return 1;
  }

  fs::path InputDir(Argv[1]);
  fs::path OutputDir(Argv[2]);

  if (!fs::is_directory(InputDir)) {
    std::fprintf(stderr, "Error: %s is not a directory\n", Argv[1]);
    return 1;
  }
  fs::create_directories(OutputDir);

  uint32_t TotalFiles = 0;
  uint32_t TotalModules = 0;
  uint32_t ParseErrors = 0;
  uint32_t WriteErrors = 0;

  for (auto &Entry : fs::recursive_directory_iterator(InputDir)) {
    if (!Entry.is_regular_file() || Entry.path().extension() != ".wast") {
      continue;
    }

    auto RelPath = fs::relative(Entry.path(), InputDir);
    const auto Prefix = makePrefix(RelPath);

    auto Res = WasmEdge::Wast::parseWast(Entry.path());
    if (!Res) {
      std::fprintf(stderr, "Warning: failed to parse %s\n",
                   WasmEdge::u8string(Entry.path()).c_str());
      ++ParseErrors;
      continue;
    }
    ++TotalFiles;

    uint32_t ModIdx = 0;
    for (const auto &Cmd : Res->Commands) {
      if (Cmd.ModuleSource.empty()) {
        continue;
      }
      if (Cmd.ModType != WasmEdge::Wast::ModuleType::Text) {
        continue;
      }

      char Suffix[16];
      std::snprintf(Suffix, sizeof(Suffix), "-%03u.wat", ModIdx++);
      auto OutPath = OutputDir / (Prefix + Suffix);

      // A stream that failed to open is in a failed state, so the check after
      // the write also covers the open.
      std::ofstream Out(OutPath, std::ios::binary);
      Out.write(Cmd.ModuleSource.data(),
                static_cast<std::streamsize>(Cmd.ModuleSource.size()));
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
  return WriteErrors > 0 ? 1 : 0;
}
