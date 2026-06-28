// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "bindings.h"
#include "plugin/plugin.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

class WasmEdgeFFmpegEnv {
public:
  // Singleton
  static std::shared_ptr<WasmEdgeFFmpegEnv> getInstance() noexcept {
    std::unique_lock Lock(Mutex);
    std::shared_ptr<WasmEdgeFFmpegEnv> EnvPtr = Instance.lock();
    if (!EnvPtr) {
      EnvPtr.reset(new WasmEdgeFFmpegEnv());
      Instance = EnvPtr;
    }
    return EnvPtr;
  }

  // Avoid copy constructor and overloading functions.
  WasmEdgeFFmpegEnv(const WasmEdgeFFmpegEnv &) = delete;
  void operator=(const WasmEdgeFFmpegEnv &) = delete;

  void alloc(void *Data, uint32_t *DataPtr) {
    FfmpegPtrMap[FfmpegPtrAllocateKey++] = Data;
    *DataPtr = FfmpegPtrAllocateKey - 1;
    KeysByValue.emplace(Data, *DataPtr);
  }

  // Stores a borrowed (non-owning) pointer. The guest gets an id for an object
  // owned by another structure, so freeing that id must not free the underlying
  // object.
  void allocRef(void *Data, uint32_t *DataPtr) {
    alloc(Data, DataPtr);
    BorrowedKeys.insert(*DataPtr);
  }

  bool isBorrowed(uint32_t Index) const {
    return BorrowedKeys.find(Index) != BorrowedKeys.end();
  }

  // Stores a borrowed child owned by ParentId. The child id is dropped when the
  // parent is freed via deallocChildren, so it cannot outlive the object it
  // points into.
  void allocChild(void *Data, uint32_t *DataPtr, uint32_t ParentId) {
    allocRef(Data, DataPtr);
    if (ParentIdByChild.emplace(*DataPtr, ParentId).second) {
      ChildIdsByParent.emplace(ParentId, *DataPtr);
    }
  }

  // Marks an already-stored id as borrowed (non-owning) without linking it to a
  // parent. Used when ownership of a node transfers to a sibling structure that
  // becomes responsible for freeing it (e.g. an AVFilterInOut linked into
  // another node's chain via AVFilterInOutSetNext), so freeing this id on its
  // own must not free the underlying object.
  void markBorrowed(uint32_t Index) { BorrowedKeys.insert(Index); }

  // Transfers ownership of an already-stored id to ParentId: the id becomes
  // borrowed, so freeing it no longer frees the underlying object, and it is
  // dropped when the parent is freed via deallocChildren.
  void markBorrowedChild(uint32_t Index, uint32_t ParentId) {
    BorrowedKeys.insert(Index);
    if (ParentIdByChild.emplace(Index, ParentId).second) {
      ChildIdsByParent.emplace(ParentId, Index);
    }
  }

  void deallocChildren(uint32_t ParentId) {
    auto Range = ChildIdsByParent.equal_range(ParentId);
    std::vector<uint32_t> Children;
    for (auto It = Range.first; It != Range.second; ++It) {
      Children.push_back(It->second);
      ParentIdByChild.erase(It->second);
    }
    ChildIdsByParent.erase(ParentId);
    for (uint32_t const ChildId : Children) {
      dealloc(ChildId);
    }
  }

  void *fetchData(const size_t Index) {
    if (Index >= FfmpegPtrAllocateKey) {
      return nullptr;
    }
    // Use find, not operator[]: a guest id that is absent (never allocated or
    // already freed) must not insert a null entry, which would grow the table
    // without bound and leave a key with no KeysByValue mirror.
    auto It = FfmpegPtrMap.find(static_cast<uint32_t>(Index));
    if (It == FfmpegPtrMap.end()) {
      return nullptr;
    }
    return It->second;
  }

  void dealloc(size_t Index) {

    if (Index >= FfmpegPtrAllocateKey) {
      return;
    }

    auto It = FfmpegPtrMap.find(static_cast<uint32_t>(Index));
    if (It != FfmpegPtrMap.end()) {
      eraseKeyByValue(It->second, static_cast<uint32_t>(Index));
      FfmpegPtrMap.erase(It);
    }
    BorrowedKeys.erase(static_cast<uint32_t>(Index));
    eraseChildLink(static_cast<uint32_t>(Index));
  }

  // Restores ownership of every id aliasing Data by clearing its borrowed mark,
  // so freeing such an id frees the underlying object again. Used when a node
  // is detached from the chain that owned it (e.g. AVFilterInOutSetNext
  // replacing or clearing ->next): the detached node is no longer released by
  // any chain head, so its handle must regain the right to free it; otherwise
  // it would leak for the VM's lifetime.
  void unmarkBorrowedByValue(void *Data) {
    if (Data == nullptr) {
      return;
    }
    auto Range = KeysByValue.equal_range(Data);
    for (auto It = Range.first; It != Range.second; ++It) {
      BorrowedKeys.erase(It->second);
    }
  }

  // Removes every key that maps to Data, including each id's parent/child
  // bookkeeping. Used when an underlying object is freed while more than one id
  // may still alias it, so no id is left dangling and no stale child link
  // survives in ChildIdsByParent for a later deallocChildren to trip over.
  void deallocByValue(void *Data) {
    if (Data == nullptr) {
      return;
    }
    auto Range = KeysByValue.equal_range(Data);
    for (auto It = Range.first; It != Range.second; ++It) {
      FfmpegPtrMap.erase(It->second);
      BorrowedKeys.erase(It->second);
      eraseChildLink(It->second);
    }
    KeysByValue.erase(Data);
  }

  WasmEdgeFFmpegEnv() noexcept {}

private:
  // Removes the single KeysByValue entry mapping Data to Key.
  void eraseKeyByValue(void *Data, uint32_t Key) {
    auto Range = KeysByValue.equal_range(Data);
    for (auto It = Range.first; It != Range.second; ++It) {
      if (It->second == Key) {
        KeysByValue.erase(It);
        return;
      }
    }
  }

  // Drops the parent/child bookkeeping for ChildId, if any. Freeing a
  // parent-owned child id on its own (e.g. a repeatedly snapshotted metadata
  // handle) must not leave a stale ChildIdsByParent entry behind, which would
  // otherwise grow without bound for the parent's lifetime.
  void eraseChildLink(uint32_t ChildId) {
    auto It = ParentIdByChild.find(ChildId);
    if (It == ParentIdByChild.end()) {
      return;
    }
    auto Range = ChildIdsByParent.equal_range(It->second);
    for (auto CIt = Range.first; CIt != Range.second; ++CIt) {
      if (CIt->second == ChildId) {
        ChildIdsByParent.erase(CIt);
        break;
      }
    }
    ParentIdByChild.erase(It);
  }

  //  Using zero as NULL Value.
  uint32_t FfmpegPtrAllocateKey = 1;
  // Can update this to uint64_t to get more memory.
  std::map<uint32_t, void *> FfmpegPtrMap;
  std::set<uint32_t> BorrowedKeys;
  std::multimap<uint32_t, uint32_t> ChildIdsByParent;
  // Reverse index of ChildIdsByParent: maps a child id back to its parent so a
  // child freed on its own can drop its ChildIdsByParent entry in O(log n)
  // instead of leaking it for the parent's lifetime.
  std::map<uint32_t, uint32_t> ParentIdByChild;
  // Reverse index of FfmpegPtrMap: maps a stored pointer to every id that
  // aliases it, so deallocByValue need not scan the whole table.
  std::multimap<void *, uint32_t> KeysByValue;
  static std::weak_ptr<WasmEdgeFFmpegEnv> Instance;
  static std::shared_mutex Mutex;
};

// Utils functions.
#define MEMINST_CHECK(Out, CallFrame, Index)                                   \
  auto *Out = CallFrame.getMemoryByIndex(Index);                               \
  if (unlikely(Out == nullptr)) {                                              \
    spdlog::error("[WasmEdge-FFmpeg] Memory instance not found."sv);           \
    return static_cast<int32_t>(ErrNo::MissingMemory);                         \
  }

#define FFMPEG_PTR_FETCH(StructPtr, FFmpegStructId, Type)                      \
  Type *StructPtr = nullptr;                                                   \
  if (FFmpegStructId != 0)                                                     \
    StructPtr = static_cast<Type *>(Env.get()->fetchData(FFmpegStructId));

// Returns Ret from the calling host function when a guest-supplied handle id
// did not resolve to a live pointer. FFMPEG_PTR_FETCH yields nullptr for
// unknown, stale, or forged ids, and dereferencing it would let the guest crash
// the host (DoS). The id is intentionally not logged, so a guest cannot turn a
// stream of bad ids into a log-spam amplification vector.
#define FFMPEG_PTR_CHECK(StructPtr, Ret)                                       \
  if (unlikely(StructPtr == nullptr)) {                                        \
    return Ret;                                                                \
  }

#define MEM_SPAN_CHECK(OutSpan, MemInst, Type, BufPtr, BufLen, Message)        \
  auto OutSpan = MemInst->getSpan<Type>(BufPtr, BufLen);                       \
  if (unlikely(OutSpan.size() != BufLen)) {                                    \
    spdlog::error("[WasmEdge-FFmpeg] "sv Message);                             \
    return static_cast<uint32_t>(ErrNo::MissingMemory);                        \
  }

#define FFMPEG_PTR_STORE(StructPtr, FFmpegStructId)                            \
  Env.get()->alloc(StructPtr, FFmpegStructId);

#define FFMPEG_PTR_STORE_REF(StructPtr, FFmpegStructId)                        \
  Env.get()->allocRef(StructPtr, FFmpegStructId);

#define FFMPEG_PTR_STORE_CHILD(StructPtr, FFmpegStructId, ParentId)            \
  Env.get()->allocChild(StructPtr, FFmpegStructId, ParentId);

#define FFMPEG_PTR_DELETE(FFmpegStructId) Env.get()->dealloc(FFmpegStructId);

#define MEM_PTR_CHECK(OutPtr, MemInst, Type, Offset, Message)                  \
  Type *OutPtr = MemInst->getPointerOrNull<Type *>(Offset);                    \
  if (unlikely(OutPtr == nullptr)) {                                           \
    spdlog::error("[WasmEdge-FFmpeg] "sv Message);                             \
    return static_cast<int32_t>(ErrNo::MissingMemory);                         \
  }

// Starting from 200 because, posix codes take values till 131.
// Hence using 200.
enum class ErrNo : int32_t {
  Success = 0,          // No error occurred.
  MissingMemory = -201, // Caller module is missing a memory export.
  NullStructId = -202,  // Rust Sdk Passes null id.
  InternalError = -203,
  UnImplemented = -204 // Unimplemented funcs.
};

// Copies the NUL-terminated C string Src into the destination buffer without
// writing past DstLen bytes or reading past the terminator of Src. The length
// getters report strlen(Src), so a guest buffer sized exactly to that length
// receives the full string; the terminator is appended only when the buffer
// has room for it. When Src is null the destination is left as an empty
// string (when DstLen permits) and false is returned so callers can still
// detect the absent source.
inline bool copyCStringToBuffer(char *Dst, uint32_t DstLen, const char *Src) {
  if (Src == nullptr) {
    if (DstLen > 0) {
      Dst[0] = '\0';
    }
    return false;
  }
  size_t const SrcLen = std::strlen(Src);
  size_t const N = std::min<size_t>(DstLen, SrcLen + 1);
  std::copy_n(Src, N, Dst);
  return true;
}

// Returns &Array[Idx], or nullptr when Array is null or Idx is out of range.
// Centralizes the bounds check for indexed ffmpeg arrays (streams, chapters,
// ...) so a guest-supplied index can never walk past the end of the array.
template <typename T>
T **checkedArraySlot(T **Array, uint32_t Count, uint32_t Idx) {
  if (Array == nullptr || Idx >= Count) {
    return nullptr;
  }
  return &Array[Idx];
}

// Replaces the metadata dictionary at *Dst with an independent copy of Src, so
// the target owns its own dictionary and the guest-supplied handle keeps owning
// its own; freeing either side is then safe. A self-assignment (Src already at
// *Dst) is a no-op. The previous *Dst is freed: metadata handles are borrowed
// pointers to the field itself, never snapshots of its value, so they observe
// the replacement instead of dangling. Returns the av_dict_copy code, negative
// on failure.
inline int setMetadataCopy(AVDictionary **Dst, AVDictionary *Src) {
  if (Src == *Dst) {
    return 0;
  }
  AVDictionary *Copy = nullptr;
  if (Src != nullptr) {
    int const Ret = av_dict_copy(&Copy, Src, 0);
    if (Ret < 0) {
      return Ret;
    }
  }
  av_dict_free(Dst);
  *Dst = Copy;
  return 0;
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
