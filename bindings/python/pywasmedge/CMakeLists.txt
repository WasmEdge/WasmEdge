pybind11_add_module(WasmEdge 
    src/WasmEdge.cpp
    src/configure.cpp
    src/function.cpp
    src/import_object.cpp
    src/result.cpp
    src/store.cpp
    src/value.cpp
    src/vm.cpp
)
INCLUDE_DIRECTORIES("include/")
set_target_properties(WasmEdge PROPERTIES 
    CXX_STANDARD 17
)
# FILE(COPY classes.py DESTINATION .)
# ADD_TEST(NAME 02-ExposingClasses COMMAND ${PYTHON_EXECUTABLE} classes.py)