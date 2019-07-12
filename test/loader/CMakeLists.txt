macro(configure_files srcDir destDir)
message(STATUS "Generating testing input directory ${destDir}")
make_directory(${destDir})

file(GLOB templateFiles RELATIVE ${srcDir} ${srcDir}/*)
foreach(templateFile ${templateFiles})
    set(srcTemplatePath ${srcDir}/${templateFile})
    if(NOT IS_DIRECTORY ${srcTemplatePath})
        message(STATUS "Generating file ${templateFile}")
        configure_file(
                ${srcTemplatePath}
                ${destDir}/${templateFile}
                COPYONLY)
    endif(NOT IS_DIRECTORY ${srcTemplatePath})
endforeach(templateFile)
endmacro(configure_files)

add_executable(ssvmLoaderFileMgrTests
  filemgrTest.cpp
)

add_executable(ssvmLoaderASTTests
  moduleTest.cpp
  sectionTest.cpp
  descriptionTest.cpp
  segmentTest.cpp
  typeTest.cpp
  expressionTest.cpp
  instructionTest.cpp
)

configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/filemgr
  ${CMAKE_CURRENT_BINARY_DIR}/filemgr
  COPYONLY
)

target_link_libraries(ssvmLoaderFileMgrTests
  PRIVATE
  utilGoogleTest
  ssvmLoader
)

target_link_libraries(ssvmLoaderASTTests
  PRIVATE
  ssvmLoader
  gtest
)