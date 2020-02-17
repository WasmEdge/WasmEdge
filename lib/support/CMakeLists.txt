add_library(ssvmSupport
  log.cpp
)

target_link_libraries(ssvmSupport
  PRIVATE
  utilLog
)