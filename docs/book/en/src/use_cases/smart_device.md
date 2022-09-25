# WasmEdge On Smart Devices

Smart device apps could embed WasmEdge as a middleware runtime to render interactive content on the UI, connect to native device drivers, and access specialized hardware features (i.e, the GPU for AI inference). The benefits of the WasmEdge runtime over native-compiled machine code include security, safety, portability, manageability, OTA upgradability, and developer productivity. WasmEdge runs on the following device OSes.

* [Android](../contribute/build_from_src/android.md)
* [OpenHarmony](../contribute/build_from_src/openharmony.md)
* [Raspberry Pi](../contribute/build_from_src/raspberrypi.md)
* [The seL4 RTOS](../contribute/build_from_src/sel4.md)

With WasmEdge on both the device and the edge server, we can support [isomorphic Server-Side Rendering (SSR)](server_side_render.md) and [microservices](microservice.md) for rich-client mobile applications that is both portable and upgradable.
