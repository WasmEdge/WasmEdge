curl -L https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.6/llvm-project-16.0.6.src.tar.xz -O
tar --xz -xvf llvm-project-16.0.6.src.tar.xz > /dev/null
cd llvm-project-16.0.6.src
cmake -S llvm -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cd build
make install
clang --version 
