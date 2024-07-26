
all: mandelbrot.so main

mandelbrot.o: mandelbrot.c
	clang -O3 --no-standard-libraries --target=wasm32 -c -o $@ $^

mandelbrot.wasm: mandelbrot.o
	wasm-ld --no-entry $^ -o $@ --import-memory --export-all --shared-memory --features=mutable-globals,atomics,bulk-memory

mandelbrot.wat: mandelbrot.wasm
	wasm2wat --enable-all mandelbrot.wasm -o mandelbrot.wat

mandelbrot.so: mandelbrot.wasm
	wasmedge compile --enable-threads mandelbrot.wasm mandelbrot.so

main: main.cc
	clang++ -lwasmedge -std=c++17 -pthread -o $@ $^

clean:
	rm -f main mandelbrot.wasm mandelbrot.so mandelbrot.o mandelbrot.wat output-*.bin
