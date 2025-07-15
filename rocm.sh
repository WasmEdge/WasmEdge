export build_number=b5896
export wasmedge_version=0.14.1
export build_dir=build_${wasmedge_version}_${build_number}
export output_name=WasmEdge-plugin-wasi_nn-ggml-${build_number}-rocm-${wasmedge_version}-ubuntu24.04_x86_64.tar.gz
export output_dir=${build_dir}/plugins/wasi_nn
export build_options="-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=GGML -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS=OFF"
export tar_name=wasi_nn-ggml
export output_bin=libwasmedgePluginWasiNN.so
export CXXFLAGS="-Wno-error"

git config --global --add safe.directory $(pwd)
rm -rf ${build_dir}
export HIPCXX="$(hipconfig -l)/clang"
export HIP_PATH="$(hipconfig -R)"
export CC="$(hipconfig -l)/clang"
export CXX="$(hipconfig -l)/clang"
export ROCM_PATH="/opt/rocm-6.4.0"
export AMDGPU_TARGETS="gfx942"

HIPCXX="$(hipconfig -l)/clang" HIP_PATH="$(hipconfig -R)" cmake -B${build_dir} -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
	-DLDFLAGS="-L$ROCM_PATH/lib -Wl,-rpath=$ROCM_PATH/lib -L$ROCM_PATH/lib64 -Wl,-rpath=$ROCM_PATH/lib64 -lhipblas -lamdhip64 -lrocblas" \
	-DCPPFLAGS="-DGGML_USE_HIP -DGGML_USE_CUDA" \
	-D_GLIBCXX_USE_CXX11_ABI=1 \
        -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_NATIVE=OFF \
        -DWASMEDGE_BUILD_AOT_RUNTIME=OFF \
        -DWASMEDGE_USE_LLVM=OFF \
        -DWASMEDGE_BUILD_TOOLS=OFF \
        ${build_options} \
	-DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP=ON

cmake --build ${build_dir}

echo "Copying ${tar_name} backend:"
cp -f ${output_dir}/${output_bin} ${output_bin}
tar -zcvf plugin_${tar_name}.tar.gz ${output_bin}

mv plugin_wasi_nn-ggml.tar.gz ${output_name}
