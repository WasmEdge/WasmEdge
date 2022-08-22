if [ ! -d ./libtorch ]; then
    curl -s -L -O --remote-name-all https://download.pytorch.org/libtorch/lts/1.8/cpu/libtorch-cxx11-abi-shared-with-deps-1.8.2%2Bcpu.zip
    echo "b76d6dd4380e2233ce6f7654e672e13aae7c871231d223a4267ef018dcbfb616 libtorch-cxx11-abi-shared-with-deps-1.8.2%2Bcpu.zip" | sha256sum -c
    unzip -q "libtorch-cxx11-abi-shared-with-deps-1.8.2%2Bcpu.zip"
fi
