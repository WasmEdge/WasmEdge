PYTHON_ADD_MODULE(WasmEdge src/WasmEdge.cpp)
INCLUDE_DIRECTORIES("include/")
# FILE(COPY classes.py DESTINATION .)
# ADD_TEST(NAME 02-ExposingClasses COMMAND ${PYTHON_EXECUTABLE} classes.py)