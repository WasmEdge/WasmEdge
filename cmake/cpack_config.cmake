# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

if(CPACK_GENERATOR STREQUAL "DEB")
else()
  list(REMOVE_ITEM CPACK_COMPONENTS_ALL "static_debian")
endif()
