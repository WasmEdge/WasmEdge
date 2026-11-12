all:
	emcc server.cpp -o server.wasm -sERROR_ON_UNDEFINED_SYMBOLS=0 -sSTANDALONE_WASM
	emcc client.cpp -o client.wasm -sERROR_ON_UNDEFINED_SYMBOLS=0 -sSTANDALONE_WASM

test:
	-rm /tmp/WasmEdge.socket
	-rm /tmp/WasmEdge.Client.socket
	wasmedge --allow-af-unix --dir /tmp:/tmp server.wasm &
	wasmedge --allow-af-unix --dir /tmp:/tmp client.wasm

clean:
	rm server.wasm client.wasm
