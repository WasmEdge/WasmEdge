// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

/// \file
/// This tool extracts the WAT modules from the WAST files. The modules become
/// the seed corpus of a fuzzer.
/// Usage: wast2wat <input-dir> <output-dir>

#include "wast.h"
#include "wast_parser.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

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
  uint32_t TotalErrors = 0;

  for (auto &Entry : fs::recursive_directory_iterator(InputDir)) {
    if (!Entry.is_regular_file() || Entry.path().extension() != ".wast") {
      continue;
    }

    auto RelPath = fs::relative(Entry.path(), InputDir);
    // Build the prefix from the suite name and the test name. For example,
    // "wasm-3.0/memory/memory.wast" gives "wasm-3.0-memory".
    std::string Prefix;
    std::string LastDir;
    bool HasParent = false;
    for (auto It = RelPath.begin(); It != RelPath.end(); ++It) {
      auto Part = It->string();
      // Skip the last component, which is the file name. Use the stem.
      auto Next = std::next(It);
      if (Next == RelPath.end()) {
        // This is the file name component. Use its stem only if the stem is
        // different from the parent directory. A .wast file directly in
        // InputDir has no parent, so use the stem. This loop keeps the parent
        // as it moves forward, because a path iterator does not support
        // std::prev on every platform.
        auto Stem = RelPath.stem().string();
        if (!HasParent || Stem != LastDir) {
          if (!Prefix.empty()) {
            Prefix += '-';
          }
          Prefix += Stem;
        }
        break;
      }
      if (!Prefix.empty()) {
        Prefix += '-';
      }
      Prefix += Part;
      LastDir = Part;
      HasParent = true;
    }

    auto Res = WasmEdge::Wast::parseWast(Entry.path());
    if (!Res) {
      std::fprintf(stderr, "Warning: failed to parse %s\n",
                   Entry.path().u8string().c_str());
      ++TotalErrors;
      continue;
    }
    ++TotalFiles;

    uint32_t ModIdx = 0;
    for (const auto &Cmd : Res->Commands) {
      if (Cmd.ModuleSource.empty()) {
        continue;
      }
      if (Cmd.ModType != WasmEdge::Wast::ModuleType::Text &&
          Cmd.ModType != WasmEdge::Wast::ModuleType::Quote) {
        continue;
      }

      char Suffix[16];
      std::snprintf(Suffix, sizeof(Suffix), "-%03u.wat", ModIdx++);
      auto OutPath = OutputDir / (Prefix + Suffix);

      std::ofstream Out(OutPath, std::ios::binary);
      if (!Out) {
        std::fprintf(stderr, "Error: cannot write %s\n",
                     OutPath.u8string().c_str());
        continue;
      }
      Out.write(Cmd.ModuleSource.data(),
                static_cast<std::streamsize>(Cmd.ModuleSource.size()));
      ++TotalModules;
    }
  }

  std::printf("Processed %u WAST files, extracted %u modules", TotalFiles,
              TotalModules);
  if (TotalErrors > 0) {
    std::printf(", %u parse errors", TotalErrors);
  }
  std::printf("\n");
  return 0;
}
