pybind11_add_module(WasmEdge 
    src/WasmEdge.cpp
    src/configure.cpp
    src/function.cpp
    src/import_object.cpp
    src/result.cpp
    src/store.cpp
    src/value.cpp
    src/vm.cpp
    src/ast_module.cpp
    src/loader.cpp
    src/validator.cpp
    src/executor.cpp
    src/memory.cpp
    src/global.cpp
    src/table.cpp
    src/statistics.cpp
    src/export.cpp
    src/compiler.cpp
    src/async.cpp
)
INCLUDE_DIRECTORIES("include/")
set_target_properties(WasmEdge PROPERTIES 
    CXX_STANDARD 11
)
# FILE(COPY classes.py DESTINATION .)
# ADD_TEST(NAME 02-ExposingClasses COMMAND ${PYTHON_EXECUTABLE} classes.py)