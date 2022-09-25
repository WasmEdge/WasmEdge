# Serverless Software-As-A-Service Functions

WasmEdge can support customized SaaS extensions or applications using serverless functions instead of traditional network APIs. That dramatically improves SaaS users' and developers' productivity.

* WasmEdge could be embedded into SaaS products to execute user-defined functions. In this scenario, the WasmEdge function API replaces the SaaS web API. The embedded WasmEdge functions are much faster, safer, and easier to use than RPC functions over the web.
* Edge servers could provide WasmEdge-based containers to interact with existing SaaS or PaaS APIs without requiring the user to run his own servers (eg callback servers). The serverless API services can be co-located in the same networks as the SaaS to provide optimal performance and security.

The examples below showcase how WasmEdge-based serverless functions connect together SaaS APIs from different services, and process data flows across those SaaS APIs according each user's business logic.

## Slack

* [Build a serverless chatbot for Slack](http://reactor.secondstate.info/en/docs/user_guideline.html)

## Lark

It is also known as `飞书` aka the Chinese Slack. It is created by Byte Dance, the parent company of Tiktok.

* [Build a serverless chatbot for Lark](http://reactor.secondstate.info/en/docs/user_guideline.html)
