// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/gc/allocator.h - GC memory allocator ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of allocator class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/errcode.h"
#include "common/hash.h"
#include "system/allocator.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>

namespace WasmEdge::GC {

class TypePack {
public:
  TypePack(const AST::CompositeType &CompType) {
    Fields.reserve(CompType.getFieldTypes().size() + 1);
    Fields.push_back(CompType.getContentTypeCode());
    for (const auto &Field : CompType.getFieldTypes()) {
      Fields.push_back(Field.getStorageType().getCode());
    }
  }

  [[nodiscard]] bool operator==(const TypePack &TP) const noexcept {
    return Fields == TP.Fields;
  }

  [[nodiscard]] bool operator!=(const TypePack &TP) const noexcept {
    return Fields != TP.Fields;
  }

  TypeCode operator[](size_t I) const noexcept { return Fields[I]; }

  size_t size() const noexcept { return Fields.size(); }

  auto begin() const noexcept { return Fields.begin(); }
  auto begin() noexcept { return Fields.begin(); }
  auto end() const noexcept { return Fields.end(); }
  auto end() noexcept { return Fields.end(); }

  const std::basic_string<TypeCode> &fields() const noexcept { return Fields; }

private:
  std::basic_string<TypeCode> Fields;
};

struct TypePackHash : Hash::Hash {
  inline uint64_t operator()(const TypePack &Pack) const noexcept {
    return Hash::Hash::operator()(Pack.fields());
  }
};

class Allocator {
public:
  struct Header {
    uint32_t Size = 0;
    std::atomic<uint32_t> RefCount = 1;
    std::atomic<bool> IsGray = true;
    const TypePack *Type;
  };
  struct Work {
    Work *Next = nullptr;
    uint32_t Size = 0;
    Header *Data[];
  };

  Allocator();

  ~Allocator() noexcept;

  [[nodiscard]] void *allocate(const AST::CompositeType &CompType,
                               uint32_t Size) noexcept {
    uint8_t *Pointer = do_allocate(sizeof(Header) + Size);
    if (unlikely(!Pointer)) {
      return nullptr;
    }
    auto H = reinterpret_cast<Header *>(Pointer);
    new (H) Header();
    H->Type = &*TypePacks.emplace(CompType).first;
    H->Size = Size;
    {
      std::unique_lock<std::mutex> Locker(RootMutex);
      Root.emplace(H);
    }
    {
      std::unique_lock<std::mutex> Locker(GrayMutex);
      Gray.push_back(H);
    }

    return static_cast<void *>(Pointer + sizeof(Header));
  }

  void refCountInc(uint8_t *P) noexcept {
    auto H = reinterpret_cast<Header *>(P - sizeof(Header));
    H->RefCount.fetch_add(1, std::memory_order_relaxed);
  }

  void refCountDec(uint8_t *P) noexcept {
    auto H = reinterpret_cast<Header *>(P - sizeof(Header));
    if (H->RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      std::unique_lock<std::mutex> Locker(RootMutex);
      Root.erase(H);
    }
  }

private:
  [[nodiscard]] uint8_t *do_allocate(uint32_t N) noexcept;

  void do_deallocate(uint8_t *P, uint32_t Size) noexcept;

  void do_collect() noexcept;

  std::atomic<bool> Stop = false;
  std::atomic<uint32_t> WorkerCount;
  std::atomic<std::chrono::steady_clock::rep> NextGC =
      std::chrono::steady_clock::now().time_since_epoch().count() +
      std::chrono::seconds(1).count();
  std::atomic<uint64_t> Threshold = 1024 * 1024;
  std::atomic<uint64_t> Used = 0;

  std::mutex RootMutex{};
  std::unordered_set<Header *> Root;
  std::mutex Set1Mutex{};
  std::unordered_set<Header *> Set1;
  std::mutex Set2Mutex{};
  std::unordered_set<Header *> Set2;
  std::mutex GrayMutex{};
  std::condition_variable GrayNotEmptyCV;
  std::deque<Header *> Gray;
  std::mutex TypePacksMutex{};
  std::unordered_set<TypePack, TypePackHash> TypePacks;

  std::mutex *BlackMutex = &Set1Mutex;
  std::unordered_set<Header *> *Black = &Set1;
  std::mutex *WhiteMutex = &Set2Mutex;
  std::unordered_set<Header *> *White = &Set2;

  std::vector<std::thread> Collectors;
};

} // namespace WasmEdge::GC
