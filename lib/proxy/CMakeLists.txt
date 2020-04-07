# SPDX-License-Identifier: Apache-2.0
add_library(ssvmProxy
  cmdparser.cpp
  proxy.cpp
)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#include <iostream>
#if !defined(__GLIBCXX__) || (defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE >= 9)
#error
#endif
int main() {}
" NEED_STDCXXFS)

target_link_libraries(ssvmProxy
  PUBLIC
  $<$<BOOL:${NEED_STDCXXFS}>:stdc++fs>
  PRIVATE
  ssvmExpVM
)
