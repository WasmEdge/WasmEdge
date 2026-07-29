# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

if(CPACK_GENERATOR STREQUAL "DEB")
else()
  list(REMOVE_ITEM CPACK_COMPONENTS_ALL "static_debian")
endif()
