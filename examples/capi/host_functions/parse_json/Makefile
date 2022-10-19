CC = gcc

all: host_function parse-json.wasm

host_function: host_function.c
	$(CC) $^ -ljson-c -lwasmedge -o $@

%.wasm: %.wat
	wat2wasm --enable-all $^ -o $@

clean:
	rm -f host_function parse-json.wasm
