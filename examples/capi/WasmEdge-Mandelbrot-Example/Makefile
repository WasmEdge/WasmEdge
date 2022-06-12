
all: mandelbrot.so main

mandelbrot.o: mandelbrot.c
	clang-12 -O3 --no-standard-libraries --target=wasm32 -c -o $@ $^

mandelbrot.wasm: mandelbrot.o
	wasm-ld --no-entry $^ -o $@ --import-memory --export-all --shared-memory --features=mutable-globals,atomics,bulk-memory

mandelbrot.wat: mandelbrot.wasm
	wasm2wat --enable-all mandelbrot.wasm -o mandelbrot.wat

mandelbrot.so: mandelbrot.wasm 
	wasmedgec --enable-threads mandelbrot.wasm mandelbrot.so

main: main.cc
	clang++-12 -lwasmedge_c -std=c++17 -pthread -o $@ $^

clean:
	rm -f main mandelbrot.wasm mandelbrot.so mandelbrot.o mandelbrot.wat output-*.bin

