# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgePluginOpenCvImgcodecs 
	SHARED
	imgcodecsenv.cpp
	imgcodecsfunc.cpp
	imgcodecsmodule.cpp
)

target_include_directories(wasmedgePluginOpenCvImgcodecs
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgePluginOpenCvImgcodecs
  PUBLIC
  ${OpenCV_LIBS}
)

if(WASMEDGE_LINK_PUGLINS_STATIC)
  target_link_libraries(wasmedgePluginOpenCvImgcodecs
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginOpenCvImgcodecs
    PRIVATE
    wasmedge_shared
  )
endif()
