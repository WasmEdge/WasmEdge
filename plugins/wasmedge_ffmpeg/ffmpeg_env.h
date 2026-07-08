// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "bindings.h"
#include "plugin/plugin.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {

namespace detail {
template <typename T> struct HandleTypeId {
  static const char Id;
};
template <typename T> const char HandleTypeId<T>::Id = 0;

template <typename T>
using NormalizeHandle =
    std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<T>>>;

template <typename T> inline const void *handleTag() {
  return &HandleTypeId<NormalizeHandle<T>>::Id;
}

template <typename T> inline void *rawHandle(T *Data) {
  return const_cast<void *>(static_cast<const void *>(Data));
}

// Picks the next handle id at or after Next, skipping 0 (the null handle)
// and every id still present in Live, so a counter that has wrapped around
// never hands out an id another object owns.
template <typename LiveMap>
inline uint32_t takeFreeHandle(uint32_t &Next, const LiveMap &Live) {
  while (Next == 0 || Live.find(Next) != Live.end()) {
    ++Next;
  }
  return Next++;
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

  template <typename T> void alloc(T *Data, uint32_t *DataPtr) {
    *DataPtr = insert(detail::rawHandle(Data), detail::handleTag<T>());
  }

  // Hands out a handle for an object the guest may ask for any number of
  // times, such as a codec, filter or format descriptor: every request for
  // the same object returns the same id and takes a reference, which a drop
  // of that id gives back.
  template <typename T> void allocShared(T *Data, uint32_t *DataPtr) {
    uint32_t Id = acquireShared<T>(Data);
    if (Id == 0) {
      Id = insertShared(Data, Data);
    }
    *DataPtr = Id;
  }

  // The shared handle registered for Key, with another reference taken, or
  // 0 when there is none.
  template <typename T> uint32_t acquireShared(const void *Key) {
    auto const It = SharedIdsByKey.find(
        {reinterpret_cast<uintptr_t>(Key), detail::handleTag<T>()});
    if (It == SharedIdsByKey.end()) {
      return 0;
    }
    addRef(It->second);
    return It->second;
  }

  // Registers Data under a new id as the shared handle for Key, holding one
  // reference.
  template <typename T> uint32_t insertShared(T *Data, const void *Key) {
    const void *const Tag = detail::handleTag<T>();
    uint32_t const Id = insert(detail::rawHandle(Data), Tag);
    if (Id != 0) {
      SharedKey const Shared{reinterpret_cast<uintptr_t>(Key), Tag};
      SharedKeysById.emplace(Id, Shared);
      SharedIdsByKey.emplace(Shared, Id);
    }
    return Id;
  }

  template <typename T> void allocRef(T *Data, uint32_t *DataPtr) {
    uint32_t const Key =
        insert(detail::rawHandle(Data), detail::handleTag<T>());
    if (Key != 0) {
      BorrowedKeys.insert(Key);
    }
    *DataPtr = Key;
  }

  bool isBorrowed(uint32_t Index) const {
    return BorrowedKeys.find(Index) != BorrowedKeys.end();
  }

  // Hands out a borrowed view of an object owned by ParentId. Asking for the
  // same view again returns the id already handed out and takes another
  // reference to it, so a getter called in a loop does not grow the table and
  // a guest dropping one copy of the id leaves the others valid.
  template <typename T>
  void allocChild(T *Data, uint32_t *DataPtr, uint32_t ParentId) {
    void *const Raw = detail::rawHandle(Data);
    const void *const Tag = detail::handleTag<T>();
    uint32_t Key = findChild(Raw, Tag, ParentId);
    if (Key != 0) {
      addRef(Key);
    } else {
      Key = insert(Raw, Tag);
      if (Key != 0) {
        BorrowedKeys.insert(Key);
        linkChild(Key, ParentId);
      }
    }
    *DataPtr = Key;
  }

  void markBorrowed(uint32_t Index) { BorrowedKeys.insert(Index); }

  void markBorrowedChild(uint32_t Index, uint32_t ParentId) {
    BorrowedKeys.insert(Index);
    linkChild(Index, ParentId);
  }

  void deallocChildren(uint32_t ParentId) {
    auto const Range = childRange(ParentId);
    std::vector<uint32_t> Children;
    for (auto It = Range.first; It != Range.second; ++It) {
      Children.push_back(It->second);
      ParentIdByChild.erase(It->second);
    }
    ChildIdsByParent.erase(Range.first, Range.second);
    for (uint32_t const ChildId : Children) {
      dealloc(ChildId);
    }
  }

  void *fetchData(const size_t Index) {
    if (Index > MaxKey) {
      return nullptr;
    }
    auto It = FfmpegPtrMap.find(static_cast<uint32_t>(Index));
    if (It == FfmpegPtrMap.end()) {
      return nullptr;
    }
    return It->second.Ptr;
  }

  template <typename T> T *fetchTyped(const size_t Index) {
    if (Index > MaxKey) {
      return nullptr;
    }
    auto It = FfmpegPtrMap.find(static_cast<uint32_t>(Index));
    if (It == FfmpegPtrMap.end() || It->second.Tag != detail::handleTag<T>()) {
      return nullptr;
    }
    return static_cast<T *>(It->second.Ptr);
  }

  enum HandleFlag : uint32_t {
    MuxHeaderWritten = 1U << 0,
    MuxTrailerWritten = 1U << 1,
    GraphConfigured = 1U << 2,
    MuxFailed = 1U << 3,
  };

  void setFlag(uint32_t Index, uint32_t Flag) { FlagsById[Index] |= Flag; }

  void clearFlag(uint32_t Index, uint32_t Flag) {
    auto It = FlagsById.find(Index);
    if (It == FlagsById.end()) {
      return;
    }
    It->second &= ~Flag;
    if (It->second == 0) {
      FlagsById.erase(It);
    }
  }

  bool hasFlag(uint32_t Index, uint32_t Flag) const {
    auto It = FlagsById.find(Index);
    return It != FlagsById.end() && (It->second & Flag) != 0;
  }

  // A muxer whose header has been written, or whose avformat_write_header
  // failed, may no longer have its streams or their parameters changed.
  bool isMuxerLocked(uint32_t Index) const {
    return hasFlag(Index, MuxHeaderWritten | MuxFailed);
  }

  // A muxer between its header and its trailer: it takes packets, and it
  // still owes the trailer. Only the trailer releases what the header set up
  // (the pulse output device keeps a thread whose callbacks dereference the
  // context, and has no deinit hook), so such a context is trailed before
  // it is freed, and its pb stays open until then.
  bool isMuxing(uint32_t Index) const {
    return hasFlag(Index, MuxHeaderWritten) &&
           !hasFlag(Index, MuxTrailerWritten);
  }

  uint32_t parentIdOf(uint32_t ChildId) const {
    auto It = ParentIdByChild.find(ChildId);
    return It == ParentIdByChild.end() ? 0 : It->second;
  }

  void dealloc(size_t Index) {
    if (Index > MaxKey) {
      return;
    }

    auto It = FfmpegPtrMap.find(static_cast<uint32_t>(Index));
    if (It != FfmpegPtrMap.end()) {
      eraseKeyByValue(It->second.Ptr, static_cast<uint32_t>(Index));
      FfmpegPtrMap.erase(It);
    }
    BorrowedKeys.erase(static_cast<uint32_t>(Index));
    FlagsById.erase(static_cast<uint32_t>(Index));
    forgetShare(static_cast<uint32_t>(Index));
    eraseChildLink(static_cast<uint32_t>(Index));
  }

  // Gives back one reference to Index, as a guest's free or drop of an id it
  // was handed does; the id goes with the last reference. Returns whether
  // that was the last one.
  bool releaseRef(size_t Index) {
    if (Index <= MaxKey) {
      auto It = RefsById.find(static_cast<uint32_t>(Index));
      if (It != RefsById.end()) {
        if (--It->second > 0) {
          return false;
        }
      }
    }
    dealloc(Index);
    return true;
  }

  void unmarkBorrowedByValue(void *Data) {
    if (Data == nullptr) {
      return;
    }
    auto const Range = keyRange(Data);
    for (auto It = Range.first; It != Range.second; ++It) {
      BorrowedKeys.erase(It->second);
    }
  }

  void deallocByValue(void *Data) {
    if (Data == nullptr) {
      return;
    }
    auto const Range = keyRange(Data);
    for (auto It = Range.first; It != Range.second; ++It) {
      FfmpegPtrMap.erase(It->second);
      BorrowedKeys.erase(It->second);
      FlagsById.erase(It->second);
      forgetShare(It->second);
      eraseChildLink(It->second);
    }
    KeysByValue.erase(Range.first, Range.second);
  }

  WasmEdgeFFmpegEnv() noexcept {}

  ~WasmEdgeFFmpegEnv();

private:
  // (parent id, child id) and (native pointer, handle id) pairs, ordered so
  // that every entry of one parent or pointer forms one contiguous range.
  using LinkSet = std::set<std::pair<uint32_t, uint32_t>>;
  using KeySet = std::set<std::pair<uintptr_t, uint32_t>>;
  // (object, handle type) a shared handle was registered for.
  using SharedKey = std::pair<uintptr_t, const void *>;
  static constexpr uint32_t MaxKey = std::numeric_limits<uint32_t>::max();

  uint32_t insert(void *Raw, const void *Tag) {
    if (Raw == nullptr) {
      return 0;
    }
    uint32_t const Key =
        detail::takeFreeHandle(FfmpegPtrAllocateKey, FfmpegPtrMap);
    forgetHandleState(Key);
    FfmpegPtrMap[Key] = Entry{Raw, Tag};
    KeysByValue.emplace(reinterpret_cast<uintptr_t>(Raw), Key);
    return Key;
  }

  // Takes another reference to an id that is handed out again. An id without
  // an entry in RefsById holds a single reference.
  void addRef(uint32_t Key) { ++RefsById.try_emplace(Key, 1).first->second; }

  void forgetShare(uint32_t Key) {
    RefsById.erase(Key);
    auto const It = SharedKeysById.find(Key);
    if (It != SharedKeysById.end()) {
      SharedIdsByKey.erase(It->second);
      SharedKeysById.erase(It);
    }
  }

  uint32_t findChild(void *Raw, const void *Tag, uint32_t ParentId) const {
    if (Raw == nullptr) {
      return 0;
    }
    auto const Range = keyRange(Raw);
    for (auto It = Range.first; It != Range.second; ++It) {
      uint32_t const Key = It->second;
      auto const Found = FfmpegPtrMap.find(Key);
      if (Found != FfmpegPtrMap.end() && Found->second.Tag == Tag &&
          isBorrowed(Key) && parentIdOf(Key) == ParentId) {
        return Key;
      }
    }
    return 0;
  }

  // Drops every side-table record of an id that is about to be handed out,
  // so a recycled id starts without the borrowed flag, muxer or graph state,
  // parent link, or children of its previous owner.
  void forgetHandleState(uint32_t Key) {
    BorrowedKeys.erase(Key);
    FlagsById.erase(Key);
    forgetShare(Key);
    eraseChildLink(Key);
    auto const Range = childRange(Key);
    for (auto It = Range.first; It != Range.second; ++It) {
      ParentIdByChild.erase(It->second);
    }
    ChildIdsByParent.erase(Range.first, Range.second);
  }

  void linkChild(uint32_t ChildId, uint32_t ParentId) {
    if (ParentIdByChild.emplace(ChildId, ParentId).second) {
      ChildIdsByParent.emplace(ParentId, ChildId);
    }
  }

  std::pair<LinkSet::const_iterator, LinkSet::const_iterator>
  childRange(uint32_t ParentId) const {
    return {ChildIdsByParent.lower_bound({ParentId, 0}),
            ChildIdsByParent.upper_bound({ParentId, MaxKey})};
  }

  std::pair<KeySet::const_iterator, KeySet::const_iterator>
  keyRange(void *Data) const {
    uintptr_t const Raw = reinterpret_cast<uintptr_t>(Data);
    return {KeysByValue.lower_bound({Raw, 0}),
            KeysByValue.upper_bound({Raw, MaxKey})};
  }

  void eraseKeyByValue(void *Data, uint32_t Key) {
    KeysByValue.erase({reinterpret_cast<uintptr_t>(Data), Key});
  }

  void eraseChildLink(uint32_t ChildId) {
    auto It = ParentIdByChild.find(ChildId);
    if (It == ParentIdByChild.end()) {
      return;
    }
    ChildIdsByParent.erase({It->second, ChildId});
    ParentIdByChild.erase(It);
  }

  struct Entry {
    void *Ptr;
    const void *Tag;
  };

  //  Using zero as NULL Value.
  uint32_t FfmpegPtrAllocateKey = 1;
  // Can update this to uint64_t to get more memory.
  std::map<uint32_t, Entry> FfmpegPtrMap;
  std::set<uint32_t> BorrowedKeys;
  LinkSet ChildIdsByParent;
  std::map<uint32_t, uint32_t> ParentIdByChild;
  std::map<uint32_t, uint32_t> FlagsById;
  std::map<uint32_t, uint64_t> RefsById;
  std::map<SharedKey, uint32_t> SharedIdsByKey;
  std::map<uint32_t, SharedKey> SharedKeysById;
  KeySet KeysByValue;
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

#define FFMPEG_PTR_CHECK(StructPtr, Ret)                                       \
  if (unlikely(StructPtr == nullptr)) {                                        \
    return Ret;                                                                \
  }

#define FFMPEG_PTR_CHECK_NONZERO(StructPtr, FFmpegStructId, Ret)               \
  if (unlikely((FFmpegStructId) != 0 && (StructPtr) == nullptr)) {             \
    return Ret;                                                                \
  }

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

#define FFMPEG_PTR_STORE_CHILD(StructPtr, FFmpegStructId, ParentId)            \
  Env.get()->allocChild(StructPtr, FFmpegStructId, ParentId);

#define FFMPEG_PTR_STORE_SHARED(StructPtr, FFmpegStructId)                     \
  Env.get()->allocShared(StructPtr, FFmpegStructId);

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

template <typename T>
T **checkedArraySlot(T **Array, uint32_t Count, uint32_t Idx) {
  if (Array == nullptr || Idx >= Count) {
    return nullptr;
  }
  return &Array[Idx];
}

inline bool hasBuffers(const AVFrame *Frame) {
  return Frame->buf[0] != nullptr || Frame->data[0] != nullptr;
}

inline bool hasVideoGeometry(const AVFrame *Frame) {
  return Frame->width > 0 && Frame->height > 0;
}

inline bool hasAudioGeometry(const AVFrame *Frame) {
  return Frame->nb_samples > 0 || Frame->ch_layout.nb_channels > 0;
}

inline int setMetadataCopy(AVDictionary **Dst, AVDictionary *Src) {
  if (Src == *Dst) {
    return 0;
  }
  AVDictionary *Copy = nullptr;
  if (Src != nullptr) {
    int const Ret = av_dict_copy(&Copy, Src, AV_DICT_MULTIKEY);
    if (Ret < 0) {
      av_dict_free(&Copy);
      return Ret;
    }
  }
  av_dict_free(Dst);
  *Dst = Copy;
  return 0;
}

// Installs the dictionary behind a guest handle as *Dst. An owned handle is
// consumed: its dictionary moves into Dst, the holder is released and the id
// becomes invalid, which is what the SDK expects after disowning it. A borrowed
// handle (a metadata view of another object) is copied entry by entry.
inline int32_t applyMetadata(WasmEdgeFFmpegEnv &Env, AVDictionary **Dst,
                             uint32_t DictId, AVDictionary **Handle) {
  AVDictionary *const Src = (Handle == nullptr) ? nullptr : *Handle;
  if (Handle != nullptr && !Env.isBorrowed(DictId)) {
    if (Src != *Dst) {
      av_dict_free(Dst);
      *Dst = Src;
    }
    *Handle = nullptr;
    Env.deallocByValue(Handle);
    av_free(Handle);
    return static_cast<int32_t>(ErrNo::Success);
  }
  if (Src == *Dst) {
    return static_cast<int32_t>(ErrNo::Success);
  }
  if (setMetadataCopy(Dst, Src) < 0) {
    return static_cast<int32_t>(ErrNo::InternalError);
  }
  return static_cast<int32_t>(ErrNo::Success);
}

} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
