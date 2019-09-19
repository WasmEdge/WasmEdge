#include "executor/instance/entity.h"

namespace SSVM {
namespace Executor {
namespace Instance {

/// Setter of module and entity name. See "include/executor/instance/entity.h".
ErrCode Entity::setNames(const std::string &Mod, const std::string &Entity) {
  ModName = Mod;
  EntityName = Entity;
  return ErrCode::Success;
}

/// Match the module and entity name. See "include/executor/instance/entity.h".
bool Entity::isName(const std::string &Mod, const std::string &Entity) {
  return Mod == ModName && Entity == EntityName;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM