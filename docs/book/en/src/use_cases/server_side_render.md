# Server Side Rendering Modern Web UI

Traditional web applications follows the client-server model. In the past era of application servers, the entire UI is dynamically generated from the server. The browser is simply a thin client that displays the rendered web pages at real time. However, as the browser becomes more capable and sophisticated, the client can now take on more workload to improve application UX, performance, and security.

That gives rise to the era of Jamstack. There is now a clear separation between frontend and backend services. The frontend is a static web site (HTML + JavaScript + WebAssembly) generated from UI frameworks such as React.js, Vue.js, Yew or Percy, and the backend consists of microservices. Yet, as Jamstack gains popularity, the diversity of clients (both browsers and apps) makes it very difficult to achieve great performance across all use cases.

The solution is server-side rendering (SSR). That is to have edge servers run the "client side" UI code (ie the React generated JavaScript OR Percy generated WebAssembly), and send back the rendered HTML DOM objects to the browser. In this case, the edge server must execute the exact same code (i.e. [JavaScript](../write_wasm/js.md) and WebAssembly) as the browser to render the UI. That is called isomorphic Jamstack applications. The WasmEdge runtime provides a lightweight, high performance, OCI complaint, and polyglot container to run all kinds of SSR functions on edge servers.

* [React JS SSR function](../write_wasm/js/ssr.md)
* Vue JS SSR function (coming soon)
* Yew Rust compiled to WebAssembly SSR function (coming soon)
* [Percy Rust compiled to WebAssembly SSR function](../write_wasm/rust/ssr.md)

We also exploring ways to render more complex UI and interactions on WasmEdge-based edge servers, and then stream the rendered results to the client application. Potential examples include

* Render Unity3D animations on the edge server (based on [WebAssembly rendering of Unity3D](https://docs.unity3d.com/2020.1/Documentation/Manual/webgl-gettingstarted.html))
* Render interactive video (generated from AI) on the edge server

Of course, the edge cloud could grow well beyond SSR for UI components. It could also host high-performance microservices for business logic and serverless functions. Read on to the next chapter.
