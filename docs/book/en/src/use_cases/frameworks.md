# App Frameworks and Platforms

WasmEdge applications can be plugged into existing application frameworks or platforms. WasmEdge provides a safe and efficient extension mechanism for those frameworks.

In this chapter, we will introduce several such frameworks and platforms.

* [Service mesh and frameworks](frameworks/mesh.md) support WasmEdge to run as containers for microservices. We will cover distributed application framework [Dapr](frameworks/mesh/dapr.md), service mesh [MOSN](frameworks/mesh/mosn.md), and event mesh [Apache EventMesh](frameworks/mesh/eventmesh.md).
* [Application frameworks](frameworks/app.md) support WasmEdge as an embedded function or plug-in runtime. We will cover streaming data framework [YoMo](frameworks/app/yomo.md) and Go function schedulder / framework [Reactr](frameworks/app/reactr.md).
* [Serverless platforms](frameworks/serverless.md) allows WasmEdge programs to run as serverless functions in their infrastructure. We will cover [AWS Lambda](frameworks/serverless/aws.md), [Tencent Serverless Cloud Functions](frameworks/serverless/tencent.md), [Vercel Serverless Functions](frameworks/serverless/vercel.md), [Netlify Functions](frameworks/serverless/netlify.md), and [Second State Functions](frameworks/serverless/secondstate.md).
