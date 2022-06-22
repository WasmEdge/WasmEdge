WASMEDGE_EXECUTABLE='./main'
NODE_EXECUTABLE='node --experimental-wasm-threads --experimental-wasm-bulk-memory main.js'

rm -f output-*

echo "------------------------"
echo "WasmEdge Test"
echo "------------------------"
for i in {1..10}
do
   ${WASMEDGE_EXECUTABLE} ${i} | tee -a log
done
echo "------------------------"
echo "Elapsed Time (ms) of WasmEdge (1 to 10 threads)"
cat log | grep 'Elapsed Time'
rm log
echo "------------------------"
echo "NodeJs Test"
echo "------------------------"
for i in {1..10}
do
   ${NODE_EXECUTABLE} ${i} | tee -a log
done
echo "------------------------"
echo "Elapsed Time (ms) of NodeJS (1 to 10 threads)"
cat log | grep 'Elapsed Time'
rm log

node convert.js output-wasmedge.bin output-wasmedge.png
node convert.js output-node.bin output-node.png
