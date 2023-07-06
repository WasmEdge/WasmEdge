source /opt/intel/openvino_2023/bin/setupvars.sh
ldconfig
export LD_LIBRARY_PATH="$(pwd)/build/lib/api:$LD_LIBRARY_PATH"

cd build
ctest
cd -
