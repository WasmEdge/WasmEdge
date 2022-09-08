export CLASSPATH=$CLASSPATH:./wasmedge-java.jar
cd ~/wasmedge/bindings/java/wasmedge-jni
rm -r build
mkdir build && cd build
cmake ..
make
make install
