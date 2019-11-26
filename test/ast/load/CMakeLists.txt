add_executable(ssvmASTLoadTests
  moduleTest.cpp
  sectionTest.cpp
  descriptionTest.cpp
  segmentTest.cpp
  typeTest.cpp
  expressionTest.cpp
  instructionTest.cpp
)

target_link_libraries(ssvmASTLoadTests
  PRIVATE
  utilGoogleTest
  ssvmASTLoad
)
