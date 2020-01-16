# EVMC: Ethereum Client-VM Connector API.
# Copyright 2018-2019 The EVMC Authors.
# Licensed under the Apache License, Version 2.0.

add_library(
    loader STATIC
    ${include_dir}/evmc/loader.h
    loader.c
)

add_library(evmc::loader ALIAS loader)
set_target_properties(loader PROPERTIES
    OUTPUT_NAME evmc-loader
    POSITION_INDEPENDENT_CODE TRUE
)
target_include_directories(loader PUBLIC
    $<BUILD_INTERFACE:${include_dir}>$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(loader INTERFACE ${CMAKE_DL_LIBS})

if(EVMC_INSTALL)
    install(TARGETS loader EXPORT evmcTargets DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()