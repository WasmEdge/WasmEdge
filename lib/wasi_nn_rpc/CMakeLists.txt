find_package(PkgConfig REQUIRED)
pkg_check_modules(PROTOBUF protobuf REQUIRED)
pkg_check_modules(GRPC grpc REQUIRED)
pkg_check_modules(GRPCPP grpc++ REQUIRED)

# gRPC::grpc_cpp_plugin can't be used because libgrpc-dev for Ubuntu 22.04 does not contain cmake files.
# https://packages.ubuntu.com/search?keywords=libgrpc-dev
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# protobuf_generate() can't be used on Debian, Ubuntu, etc., as libprotobuf-dev does not contain cmake files:
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1027876
# The situation is similar for other distros too.
# As a workaround, we execute the protoc command directly.
execute_process(
  COMMAND protoc --proto_path=${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/wasi_ephemeral_nn.proto
  COMMAND protoc --proto_path=${CMAKE_CURRENT_SOURCE_DIR} --grpc_out=${CMAKE_CURRENT_BINARY_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${CMAKE_CURRENT_SOURCE_DIR}/wasi_ephemeral_nn.proto
)

# wasiNNRPC has to be a separate shared library to avoid
# `[libprotobuf ERROR google/protobuf/descriptor_database.cc:120] File already exists in database: wasi_ephemeral_nn.proto`,
# because both wasi_nn_rpcserver and libwasmedgePluginWasiNN.so depend on it.
# https://github.com/protocolbuffers/protobuf/issues/1941#issuecomment-284582895
if(NOT WASMEDGE_BUILD_SHARED_LIB)
  message(FATAL_ERROR "WASMEDGE_BUILD_WASI_NN_RPC depends on WASMEDGE_BUILD_SHARED_LIB")
endif()
wasmedge_add_library(wasiNNRPC SHARED
  wasi_ephemeral_nn.grpc.pb.cc
  wasi_ephemeral_nn.grpc.pb.h
  wasi_ephemeral_nn.pb.cc
  wasi_ephemeral_nn.pb.h
)
set_target_properties(wasiNNRPC PROPERTIES
  CXX_VISIBILITY_PRESET default
  VERSION "${WASMEDGE_WASI_NN_VERSION}"
  SOVERSION "${WASMEDGE_WASI_NN_SOVERSION}"
)
install(TARGETS wasiNNRPC
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

target_include_directories(wasiNNRPC PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(wasiNNRPC PUBLIC ${PROTOBUF_CFLAGS})
target_compile_options(wasiNNRPC PUBLIC ${GRPCPP_CFLAGS})
target_link_libraries(wasiNNRPC PUBLIC ${PROTOBUF_LDFLAGS})
target_link_libraries(wasiNNRPC PUBLIC ${GRPCPP_LDFLAGS})
# grpc++_reflection helps debugging the server with grpcurl
# target_link_libraries(wasiNNRPC PUBLIC grpc++_reflection)
