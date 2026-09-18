set(Expected "path with space;segment\\;leaf")
if(NOT ROUNDTRIP STREQUAL Expected)
  message(FATAL_ERROR "received '${ROUNDTRIP}', expected '${Expected}'")
endif()
