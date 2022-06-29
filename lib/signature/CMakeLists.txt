# SPDX-License-Identifier: Apache-2.0
wasmedge_add_library(wasmedgeSignature
  signature.cpp
  sig_algorithm.cpp
)

message([STATUS] "orlp: ${orlp-ed25519_SOURCE_DIR}")
target_include_directories(wasmedgeSignature
  PUBLIC
  ${orlp-ed25519_SOURCE_DIR}/src
)

target_link_libraries(wasmedgeSignature
  PUBLIC
  wasmedgeCommon
  wasmedgeLoader
  ED25519
)