#pragma once

namespace SSVM {
namespace Support {

template<typename X, typename Y>
inline bool isa(const Y ptr) {
    return dynamic_cast<const X*>(ptr) != nullptr;
}

} // namespace Support
} // namespace SSVM
