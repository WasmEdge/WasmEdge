#/usr/bin/env bash
git config --global --add safe.directory $(pwd)
rm -rf ${BUILD_DIR}/*
cmake -B ${BUILD_DIR} -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DWASMEDGE_BUILD_TESTS=On \
    -DWASMEDGE_USE_LLVM=Off \
    -DWASMEDGE_BUILD_TOOLS=Off \
    -DOPENSSL_ROOT_DIR=${OpenSSL_DIR} \
    ${WASMEDGE_OPTIONS}
cmake --build ${BUILD_DIR} --target ${WASMEDGE_TARGET}
