ldconfig
export LD_LIBRARY_PATH="$(pwd)/build/lib/api:$LD_LIBRARY_PATH"

cd build
ctest
cd -
