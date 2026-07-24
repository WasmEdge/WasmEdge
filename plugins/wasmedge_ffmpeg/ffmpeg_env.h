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
#include <type_traits>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

namespace detail {
// Address-based type tag: &HandleTypeId<T>::Id is unique per distinct T and
// well-defined even for the forward-declared ffmpeg types this header never
// completes. A fetch whose recorded tag mismatches the requested type is
// rejected instead of reinterpreting the pointer.
template <typename T> struct HandleTypeId {
  static const char Id;
};
template <typename T> const char HandleTypeId<T>::Id = 0;

// Collapses cv-qualifiers and one level of pointer so that AVCodec,
// const AVCodec and const AVCodec * all share a tag.
template <typename T>
using NormalizeHandle =
    std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<T>>>;

template <typename T> inline const void *handleTag() {
  return &HandleTypeId<NormalizeHandle<T>>::Id;
}
} // namespace detail

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

  // Maps a null pointer to the reserved id 0 and stores no entry; a null
  // entry would be unreclaimable and grow the handle maps without bound.
  template <typename T> void alloc(T *Data, uint32_t *DataPtr) {
    if (Data == nullptr) {
      *DataPtr = 0;
      return;
    }
    void *const Raw = const_cast<void *>(static_cast<const void *>(Data));
    FfmpegPtrMap[FfmpegPtrAllocateKey++] = Entry{Raw, detail::handleTag<T>()};
    *DataPtr = FfmpegPtrAllocateKey - 1;
    KeysByValue.emplace(Raw, *DataPtr);
  }

  // Stores a borrowed (non-owning) pointer: the object is owned by another
  // structure, so freeing the returned id must not free it.
  template <typename T> void allocRef(T *Data, uint32_t *DataPtr) {
    alloc(Data, DataPtr);
    if (*DataPtr != 0) {
      BorrowedKeys.insert(*DataPtr);
    }
  }

  bool isBorrowed(uint32_t Index) const {
    return BorrowedKeys.find(Index) != BorrowedKeys.end();
  }

  // Stores a borrowed child owned by ParentId; deallocChildren drops the child
  // id with its parent, so it cannot outlive the object it points into.
  template <typename T>
  void allocChild(T *Data, uint32_t *DataPtr, uint32_t ParentId) {
    allocRef(Data, DataPtr);
    if (*DataPtr != 0 && ParentIdByChild.emplace(*DataPtr, ParentId).second) {
      ChildIdsByParent.emplace(ParentId, *DataPtr);
    }
  }

  // Marks an already-stored id borrowed without linking it to a parent, e.g.
  // an AVFilterInOut adopted by another chain via AVFilterInOutSetNext.
  void markBorrowed(uint32_t Index) { BorrowedKeys.insert(Index); }

  // Transfers an already-stored id to ParentId: it becomes borrowed and is
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
    // find(), not operator[]: an absent id must not insert a null entry.
    auto It = FfmpegPtrMap.find(static_cast<uint32_t>(Index));
    if (It == FfmpegPtrMap.end()) {
      return nullptr;
    }
    return It->second.Ptr;
  }

  // Returns the stored pointer only when its recorded tag matches T.
  template <typename T> T *fetchTyped(const size_t Index) {
    if (Index >= FfmpegPtrAllocateKey) {
      return nullptr;
    }
    auto It = FfmpegPtrMap.find(static_cast<uint32_t>(Index));
    if (It == FfmpegPtrMap.end() || It->second.Tag != detail::handleTag<T>()) {
      return nullptr;
    }
    return static_cast<T *>(It->second.Ptr);
  }

  void dealloc(size_t Index) {

    if (Index >= FfmpegPtrAllocateKey) {
      return;
    }

    auto It = FfmpegPtrMap.find(static_cast<uint32_t>(Index));
    if (It != FfmpegPtrMap.end()) {
      eraseKeyByValue(It->second.Ptr, static_cast<uint32_t>(Index));
      FfmpegPtrMap.erase(It);
    }
    BorrowedKeys.erase(static_cast<uint32_t>(Index));
    eraseChildLink(static_cast<uint32_t>(Index));
  }

  // Clears the borrowed mark on every id aliasing Data. Used when a node is
  // detached from its owning chain and must regain the right to be freed.
  void unmarkBorrowedByValue(void *Data) {
    if (Data == nullptr) {
      return;
    }
    auto Range = KeysByValue.equal_range(Data);
    for (auto It = Range.first; It != Range.second; ++It) {
      BorrowedKeys.erase(It->second);
    }
  }

  // Removes every id mapping to Data plus its parent/child bookkeeping, so no
  // alias id dangles when the underlying object is freed.
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

  // Reclaims the FilterPadView entries still in the table: they are the only
  // plugin-owned objects in it, and a view whose id the guest discarded would
  // otherwise leak. Defined in ffmpeg_env.cpp, where FilterPadView is complete.
  ~WasmEdgeFFmpegEnv();

private:
  void eraseKeyByValue(void *Data, uint32_t Key) {
    auto Range = KeysByValue.equal_range(Data);
    for (auto It = Range.first; It != Range.second; ++It) {
      if (It->second == Key) {
        KeysByValue.erase(It);
        return;
      }
    }
  }

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

  // A stored pointer plus the tag of the type it was stored as.
  struct Entry {
    void *Ptr;
    const void *Tag;
  };

  //  Using zero as NULL Value.
  uint32_t FfmpegPtrAllocateKey = 1;
  // Can update this to uint64_t to get more memory.
  std::map<uint32_t, Entry> FfmpegPtrMap;
  std::set<uint32_t> BorrowedKeys;
  std::multimap<uint32_t, uint32_t> ChildIdsByParent;
  // Reverse index of ChildIdsByParent, so a child freed on its own can drop
  // its entry without leaking it for the parent's lifetime.
  std::map<uint32_t, uint32_t> ParentIdByChild;
  // Reverse index of FfmpegPtrMap: every id aliasing a stored pointer.
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
  Type *StructPtr = Env.get()->fetchTyped<Type>(FFmpegStructId);

// Returns Ret when a guest-supplied handle id did not resolve. The id is
// intentionally not logged, so a stream of bad ids cannot become log spam.
#define FFMPEG_PTR_CHECK(StructPtr, Ret)                                       \
  if (unlikely(StructPtr == nullptr)) {                                        \
    return Ret;                                                                \
  }

// For handles ffmpeg accepts as NULL (drain/flush packets and frames,
// optional filters, dicts, and codecs): id 0 keeps meaning "pass NULL", but a
// nonzero id that failed the typed fetch is rejected instead of silently
// triggering the NULL semantics.
#define FFMPEG_PTR_CHECK_NONZERO(StructPtr, FFmpegStructId, Ret)               \
  if (unlikely((FFmpegStructId) != 0 && (StructPtr) == nullptr)) {             \
    return Ret;                                                                \
  }

// Keeps the free/drop wrappers idempotent: id 0 and stale ids succeed as
// no-ops, mirroring FFmpeg's free(NULL), while a nonzero id resolving to a
// live handle of another type is rejected before an id-keyed deallocation
// could erase that handle.
#define FFMPEG_PTR_CHECK_FREE(StructPtr, FFmpegStructId, Ret)                  \
  if (unlikely((StructPtr) == nullptr)) {                                      \
    if ((FFmpegStructId) != 0 &&                                               \
        Env.get()->fetchData(FFmpegStructId) != nullptr) {                     \
      return Ret;                                                              \
    }                                                                          \
    return static_cast<int32_t>(ErrNo::Success);                               \
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

// Bounded C-string copy: never writes past DstLen, treats a null Src as an
// empty string, and appends the terminator only when the buffer has room, so
// a buffer sized exactly to the reported strlen still gets the full string.
inline void copyCStringToBuffer(char *Dst, uint32_t DstLen, const char *Src) {
  if (Src == nullptr) {
    if (DstLen > 0) {
      Dst[0] = '\0';
    }
    return;
  }
  size_t const SrcLen = std::strlen(Src);
  size_t const N = std::min<size_t>(DstLen, SrcLen + 1);
  std::copy_n(Src, N, Dst);
}

// Returns &Array[Idx], or nullptr when Array is null or Idx is out of range.
template <typename T>
T **checkedArraySlot(T **Array, uint32_t Count, uint32_t Idx) {
  if (Array == nullptr || Idx >= Count) {
    return nullptr;
  }
  return &Array[Idx];
}

// Replaces *Dst with an independent copy of Src (self-assignment is a no-op)
// and frees the previous *Dst: metadata handles are borrowed pointers to the
// field itself, so they observe the replacement instead of dangling. Returns
// the av_dict_copy code, negative on failure.
inline int setMetadataCopy(AVDictionary **Dst, AVDictionary *Src) {
  if (Src == *Dst) {
    return 0;
  }
  AVDictionary *Copy = nullptr;
  if (Src != nullptr) {
    int const Ret = av_dict_copy(&Copy, Src, 0);
    if (Ret < 0) {
      av_dict_free(&Copy);
      return Ret;
    }
  }
  av_dict_free(Dst);
  *Dst = Copy;
  return 0;
}

// setMetadataCopy for a guest dict handle, treating a null handle as an empty
// source. Shared by the stream/frame/chapter/format metadata setters.
inline int32_t applyMetadataCopy(AVDictionary **Dst, AVDictionary **Handle) {
  AVDictionary *const Src = (Handle == nullptr) ? nullptr : *Handle;
  if (setMetadataCopy(Dst, Src) < 0) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
