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


configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/ethereum
  ${CMAKE_CURRENT_BINARY_DIR}/ethereum
  COPYONLY
)

add_executable(ssvm
  main.cpp
)

target_link_libraries(ssvm
  PRIVATE
  ssvmVM
)
