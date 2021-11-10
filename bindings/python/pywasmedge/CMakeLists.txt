PYTHON_ADD_MODULE(WasmEdge src/WasmEdge.cpp)
INCLUDE_DIRECTORIES("include/")
set_target_properties(WasmEdge PROPERTIES 
    CXX_STANDARD 17
)
# FILE(COPY classes.py DESTINATION .)
# ADD_TEST(NAME 02-ExposingClasses COMMAND ${PYTHON_EXECUTABLE} classes.py)