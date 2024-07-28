#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <random>
#include <utility>

namespace WasmEdge {
namespace Host {

class ResourceElementBase {
public:
  using HandleT = std::uint32_t;
  HandleT Parent;
  ResourceElementBase() {}
  virtual ~ResourceElementBase(){};
  virtual std::list<HandleT> *GetChildren() { return nullptr; };
};

class ResourceTable {
public:
  using HandleT = ResourceElementBase::HandleT;
  static const HandleT NullParent = UINT32_MAX;
  HandleT GetNewHandle() {
    static std::random_device Rd;
    static std::default_random_engine Engine(Rd());
    static std::uniform_int_distribution<HandleT> Dis(1, UINT32_MAX - 1);

    HandleT Res = 0;
    do {
      Res = Dis(Engine);
    } while (Table.find(Res) != Table.end());
    return Res;
  }

  template <typename T, typename... Args>
  HandleT Push(HandleT Parent, Args &&...args) {
    // Note: this is not thread safe
    HandleT Handle = GetNewHandle();
    auto Ptr = std::make_unique<T>(std::forward<Args>(args)...);
    Ptr->Parent = Parent;
    Table.emplace(Handle, std::move(Ptr));
    return Handle;
  }

  template <typename T> T *GetById(HandleT Index) {
    auto Res = Table.find(Index);
    if (Res == Table.end()) {
      return nullptr;
    }
    // if type not matched, it return nullptr and it is fine
    return dynamic_cast<T *>(Res->second.get());
  }

  // TODO: delayed drop
  bool Drop(HandleT Index) {
    auto Res = Table.find(Index);
    if (Res == Table.end())
      return false;

    if (auto List = Res->second->GetChildren(); List != nullptr) {
      for (auto Childres : *List) {
        Drop(Childres);
      }
    }
    return Table.erase(Index);
  }

private:
  std::map<HandleT, std::unique_ptr<ResourceElementBase>> Table;
};
} // namespace Host
} // namespace WasmEdge
