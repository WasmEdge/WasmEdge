#!/bin/bash

set -ex

OPTSTRING=":fbvauoc"
# Get options
while getopts ${OPTSTRING} option
do
	case "${option}" in
		f)
			folder=${OPTARG}
			echo "Set output folder to ${folder}"
			;;
		b)
			build_number=${OPTARG}
			echo "Set build number to ${build_number}"
			;;
		v)
			version=${OPTARG}
			echo "Set version to ${version}"
			;;
		a)
			distro_arch=${OPTARG}
			echo "Set distro and arch to ${distro_arch}"
			;;
		u)
			build_on_ubuntu=true
			echo "Build on Ubuntu"
			;;
		o)
			enable_openblas=true
			echo "Enable OpenBLAS"
			;;
		c)
			enable_cublas=true
			echo "Enable cuBLAS"
			;;
		*)
			echo "Unknown option -${OPTARG}"
			exit 1
			;;
	esac
done

# Verify all options are set
if [ -z ${folder+x} ]
then
	echo "folder is unset"
	exit 1
else
	echo "folder is set to '${folder}'"
fi

if [ -z ${build_number+x} ]
then
	echo "build_number is unset"
else
	echo "build_number is set to '${build_number}'"
fi

if [ -z ${version+x} ]
then
	echo "version is unset"
else
	echo "version is set to '${version}'"
fi

if [ -z ${distro_arch+x} ]
then
	echo "distro_arch is unset"
else
	echo "distro_arch is set to '${distro_arch}'"
fi

# Set remaining variables
output_name=WasmEdge-plugin-wasi_nn-ggml-${build_number}-${version}-${distro_arch}.tar.gz
output_folder=${folder}
plugin_folder=${output_folder}/plugins/wasi_nn
plugin_options="-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=GGML"

target_name=wasi_nn-ggml
plugin_bin=libwasmedgePluginWasiNN.so

# Grant safe directory
git config --global --add safe.directory "$(pwd)"

# Check if on ubuntu
if [ "${build_on_ubuntu}" = true ]
then
	echo "Install dependencies on ubuntu"
	apt update
	apt install -y unzip libopenblas-dev pkg-config
fi

if [ "${build_on_ubuntu}" = true ] && [ "${enable_openblas}" = true ]
then
	plugin_options="${plugin_options} -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS=ON"
else
	plugin_options="${plugin_options} -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS=OFF"
fi

if [ "${build_on_ubuntu}" = true ] && [ "${enable_cublas}" = true ]
then
	plugin_options="-DWASMEDGE_PLUGIN_WASI_NN_BACKEND=GGML -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS=OFF -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_CUBLAS=ON"
	export CXXFLAGS="-Wno-error"
	export CUDAARCHS="60;61;70"
	export cuda_options="-DCMAKE_CUDA_ARCHITECTURES='60;61;70' -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc"
	output_name=WasmEdge-plugin-wasi_nn-ggml-cuda-${build_number}-${version}-${distro_arch}.tar.gz

	# Fix the cmake issue
	apt remove --purge --auto-remove cmake
	apt update && \
		apt install -y software-properties-common lsb-release && \
		apt clean all

	wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
	apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
	apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 6AF7F09730B3F0A4
	apt update
	apt install cmake
fi

# Configure
echo "Building ${target_name}"
cmake -B"${output_folder}" -GNinja \
	-DCMAKE_BUILD_TYPE=Release \
	-DWASMEDGE_BUILD_AOT_RUNTIME=OFF \
	-DWASMEDGE_BUILD_TOOLS=OFF \
	"${cuda_options}" "${plugin_options}"

# Build
cmake --build "${output_folder}"

# Package tarball
echo "Copying ${target_name}"
cp -f "${plugin_folder}"/"${plugin_bin}" "${plugin_bin}"
tar -zcvf "${output_name}" "${plugin_bin}"
