(unreleased)
------------
- [PySDK] Several Changes. [Shreyas Atre]

  * Moved str definitions to separate doc_strings header
  * Created major Wrappers around Store,Configure and VM contexts
  * There is a minor issue in passing Store and Configure objects to VM
  * Passes basic Fibonacci test
- [PySDK] Debug flag addition. [Shreyas Atre]
- [PySDK] Configure class: Add host registration. [Shreyas Atre]

  * Overloading of python class Configure to support adding and removing of Host.
  * Wraps Host Registration.
- [PySDK] Configure, Proposal support. [Shreyas Atre]

  * Adds configuration support which is an api wrapped over contexts.
  * Proposal are just enums
- [PySDK] Boilerplate for PySDK based on Boost. [Shreyas Atre]

  * Currently it overloads WasmEdge Result as a boolean and supports version
  as a method.
  * Logging is implemented as a class.
  * Result supports message and code methods.
- [PySDK] Docs: Fix mkdocs yaml for HISTORY. [Shreyas Atre]
- [PySDK] Docs: Fix HISTORY generation. [Shreyas Atre]
- [PySDK] Version String function. [Shreyas Atre]

  * Returns version string
- [Docs] Fix docs example. [Shreyas Atre]
- [Test] PySDK: Test commit. [Shreyas Atre]

  * It is a very basic template for python port
- Initial Commit for python SDK. [Shreyas Atre]


