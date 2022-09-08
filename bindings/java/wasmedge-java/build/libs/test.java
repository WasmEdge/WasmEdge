import java.util.ArrayList;
import java.util.List;

import org.wasmedge.ConfigureContext;
import org.wasmedge.StoreContext;
import org.wasmedge.WasmEdge;
import org.wasmedge.WasmEdgeAsync;
import org.wasmedge.WasmEdgeI32Value;
import org.wasmedge.WasmEdgeVM;
import org.wasmedge.WasmEdgeValue;

public class test {
    public static void main(String[] args) {
        WasmEdge.init();
        ConfigureContext configureContext = new ConfigureContext();
        StoreContext storeContext = new StoreContext();
        WasmEdgeVM vm = new WasmEdgeVM(configureContext, storeContext);
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        // Create return list
        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());
        
        // Run a wasm file
        WasmEdgeAsync async = vm.asyncRunWasmFromFile("/root/wasmedge/bindings/java/wasmedge-java/build/libs/fibonacci.wasm", "fib", params);
        //async.wasmEdge_AsyncWait();
        /*if(b){
            System.out.println("finish");
        }
        else{
            System.out.println("NO finish");
        }*/
        //int len = async.wasmEdge_AsyncGetReturnsLength();
        async.wasmEdge_AsyncGet(returns);
        // Get return values
        //System.out.println(len);
        System.out.println(((WasmEdgeI32Value) returns.get(0)).getValue());
        if(((WasmEdgeI32Value) returns.get(0)).getValue() == 3){
            System.out.println("equ");
        }
        return ;
        // WasmEdgeVM vm = new WasmEdgeVM(configureContext, storeContext)
    } 
}
