# SPDX-License-Identifier: Apache-2.0

if(CPACK_GENERATOR STREQUAL "DEB")
else()
  list(REMOVE_ITEM CPACK_COMPONENTS_ALL "static_debian")
endif()
