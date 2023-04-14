# Upgrade to WasmEdge-Go v0.12.0

Due to the WasmEdge-Go API breaking changes, this document shows the guideline of programming with WasmEdge-Go API to upgrade from the `v0.11.2` to the `v0.12.0` version.

## Concepts

1. Removed the members of the `wasmedge.HostRegistration` related const values.

    The following const values are removed:

    * `wasmedge.WasmEdge_Process`
    * `wasmedge.WasiNN`
    * `wasmedge.WasiCrypto_Common`
    * `wasmedge.WasiCrypto_AsymmetricCommon`
    * `wasmedge.WasiCrypto_Kx`
    * `wasmedge.WasiCrypto_Signatures`
    * `wasmedge.WasiCrypto_Symmetric`

    The `wasmedge.VM` objects will create and register the host modules automatically now.
    If the plug-ins are not loaded, the `VM` objects will create and register the mock modules to prevent from import failed.

2. Removed the module instance creation functions of the plug-ins.

    The following APIs are removed:

    * `wasmedge.NewWasiNNModule()`
    * `wasmedge.NewWasiCryptoCommonModule()`
    * `wasmedge.NewWasiCryptoAsymmetricCommonModule()`
    * `wasmedge.NewWasiCryptoKxModule()`
    * `wasmedge.NewWasiCryptoSignaturesModule()`
    * `wasmedge.NewWasiCryptoSymmetricModule()`
    * `wasmedge.NewWasmEdgeProcessModule()`

    For the new examples for creating the module instances from plug-ins, please refer to [the example below](#creating-the-module-instance-from-a-plug-in).

3. New module instance retrieving API of `VM` objects.

    * Added `(*wasmedge.VM).GetRegisteredModule()` for retrieving registered named module in `VM` objects quickly.
    * Added `(*wasmedge.VM).ListRegisteredModule()` for listing registered named modules in `VM` objects quickly.

## The `wasmedge.HostRegistration` const values for plug-ins are not necessary in VM objects

Before the version `v0.11.2`, developers should add the configurations when they want to load the plug-ins in VM:

```go
// Assume that wasi_crypto plug-in is installed in the default plug-in path.
wasmedge.LoadPluginDefaultPaths()

conf := wasmedge.NewConfigure(wasmedge.WASI)
comf.AddConfig(wasmedge.WasiCrypto_Common)
comf.AddConfig(wasmedge.WasiCrypto_AsymmetricCommon)
comf.AddConfig(wasmedge.WasiCrypto_Kx)
comf.AddConfig(wasmedge.WasiCrypto_Signatures)
comf.AddConfig(wasmedge.WasiCrypto_Symmetric)
vm := wasmedge.NewVMWithConfig(conf)
conf.Release()

store := vm.GetStore()
modulelist := store.ListModule()
for _, name := range modulelist {
  fmt.Println(name)
}
// Will print:
//   wasi_ephemeral_crypto_asymmetric_common
//   wasi_ephemeral_crypto_common
//   wasi_ephemeral_crypto_kx
//   wasi_ephemeral_crypto_signatures
//   wasi_ephemeral_crypto_symmetric
vm.Release()
```

After `v0.12.0`, the plug-ins will automatically loaded after the `wasmedge.LoadPluginDefaultPaths()` called or the specific path given into the `wasmedge.LoadPluginFromPath()` API.

For the plug-ins not installed, the mocked modules will be registered into `VM` objects and will print the error message when invoking the host functions to notice the users to install the plug-in.

```go
wasmedge.LoadPluginDefaultPaths()
// The `wasmedge.WASI` is still needed.
conf := wasmedge.NewConfigure(wasmedge.WASI)
vm := wasmedge.NewVMWithConfig(conf)
conf.Release()

modulelist := vm.ListRegisteredModule()
for _, name := range modulelist {
  fmt.Println(name)
}
// Will print:
//   wasi_ephemeral_crypto_asymmetric_common
//   wasi_ephemeral_crypto_common
//   wasi_ephemeral_crypto_kx
//   wasi_ephemeral_crypto_signatures
//   wasi_ephemeral_crypto_symmetric
//   wasi_ephemeral_nn
//   wasi_snapshot_preview1
//   wasmedge_httpsreq
//   wasmedge_process
vm.Release()
```

> Note: The `wasmedge.WASI` configuration is still needed if developers want to use the WASI.

## Creating the module instance from a plug-in

When developers didn't use the VM objects to load the plug-ins, they can use the creation functions before the `v0.11.2` versions:

```go
// Assume that wasi_crypto plug-in is installed in the default plug-in path.
wasmedge.LoadPluginDefaultPaths()

cryptocommonmod := wasmedge.NewWasiCryptoCommonModule()

cryptocommonmod.Release()
```

But it not make sense and not extensionable if developers should use different APIs for the different plug-ins.

After the `v0.12.0` version, the `wasmedge.Plugin` struct is added, and developers can use the general API to create the module instances:

```go
// Assume that wasi_crypto plug-in is installed in the default plug-in path.
wasmedge.LoadPluginDefaultPaths()

cryptoplugin := wasmedge.FindPlugin("wasi_crypto")
if ctyptoplugin == nil {
  fmt.Println("FAIL: Cannot find the wasi_crypto plugin.")
  return
}

cryptocommonmod := cryptoplugin.CreateModule("wasi_crypto_common")

cryptocommonmod.Release()
```

## Retrieving the module instances from the VM context

Before the version `v0.11.2`, developers can retrieve the `WASI` or the modules from plug-ins with the `wasmedge.HostRegistration` const values, or retrieve the registered modules from the `store` object.

```go
// Assume that wasi_crypto plug-in is installed in the default plug-in path.
wasmedge.LoadPluginDefaultPaths()
conf := wasmedge.NewConfigure(wasmedge.WASI)
comf.AddConfig(wasmedge.WasiCrypto_Common)
comf.AddConfig(wasmedge.WasiCrypto_AsymmetricCommon)
comf.AddConfig(wasmedge.WasiCrypto_Kx)
comf.AddConfig(wasmedge.WasiCrypto_Signatures)
comf.AddConfig(wasmedge.WasiCrypto_Symmetric)
vm := wasmedge.NewVMWithConfig(conf)
conf.Release()

// Get the WASI module instance.
wasimod := vm.GetImportModule(wasmedge.WASI)
// Get the WASI-crypto-common module instance.
cryptocommonmod := vm.GetImportModule(wasmedge.WasiCrypto_Common)

// Get the registered module instance by name.
store := vm.GetStore()
cryptokxmod := store.FindModule("wasi_ephemeral_crypto_kx")

vm.Release()
```

After the `v0.12.0` version, due to removing the plug-in related configurations and automatically registering the plug-ins into VM objects, developers cannot use the `(*wasmedge.VM).GetImportModule()` API to retrieve the module instances except the built-in host modules (`WASI` currently).

However, developers can use the new APIs to retrieve the registered module instances more quickly.

```go
// Assume that wasi_crypto plug-in is installed in the default plug-in path.
wasmedge.LoadPluginDefaultPaths()
// Add the WASI configurations.
conf := wasmedge.NewConfigure(wasmedge.WASI)
vm := wasmedge.NewVMWithConfig(conf)
conf.Release()

// Get the WASI module instance.
wasimod := vm.GetImportModule(wasmedge.WASI)
// Get the registered WASI-crypto-common module instance by name.
cryptocommonmod := vm.GetRegisteredModule("wasi_ephemeral_crypto_common")

vm.Release()
```
