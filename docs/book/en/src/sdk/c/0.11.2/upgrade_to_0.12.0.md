# Upgrade to WasmEdge 0.12.0

Due to the WasmEdge C API breaking changes, this document shows the guideline for programming with WasmEdge C API to upgrade from the `0.11.2` to the `0.12.0` version.

## Concepts

1. Removed the members of the `WasmEdge_HostRegistration` enumeration.

    The following members of the `WasmEdge_HostRegistration` enumeration are removed:

    * `WasmEdge_HostRegistration_WasmEdge_Process`
    * `WasmEdge_HostRegistration_WasiNN`
    * `WasmEdge_HostRegistration_WasiCrypto_Common`
    * `WasmEdge_HostRegistration_WasiCrypto_AsymmetricCommon`
    * `WasmEdge_HostRegistration_WasiCrypto_Kx`
    * `WasmEdge_HostRegistration_WasiCrypto_Signatures`
    * `WasmEdge_HostRegistration_WasiCrypto_Symmetric`

    The `WasmEdge_VMContext` will create and register the host modules automatically now.
    If the plug-ins are not loaded, the `WasmEdge_VMContext` will create and register the mock modules to prevent from import failed.

2. Removed the module instance creation functions of the plug-ins.

    The following APIs are removed:

    * `WasmEdge_ModuleInstanceCreateWasiNN()`
    * `WasmEdge_ModuleInstanceCreateWasiCryptoCommon()`
    * `WasmEdge_ModuleInstanceCreateWasiCryptoAsymmetricCommon()`
    * `WasmEdge_ModuleInstanceCreateWasiCryptoKx()`
    * `WasmEdge_ModuleInstanceCreateWasiCryptoSignatures()`
    * `WasmEdge_ModuleInstanceCreateWasiCryptoSymmetric()`
    * `WasmEdge_ModuleInstanceCreateWasmEdgeProcess()`

    For the new examples for creating the module instances from plug-ins, please refer to [the example below](#creating-the-module-instance-from-a-plug-in).

3. New module instance retrieving API of `VM` context.

    * Added `WasmEdge_VMGetRegisteredModule()` for retrieving registered named module in VM context quickly.
    * Added `WasmEdge_VMListRegisteredModuleLength()` and `WasmEdge_VMListRegisteredModule()` for listing registered named modules in VM context quickly.

## The `WasmEdge_HostRegistration` for plug-ins is not necessary in VM contexts

Before the version `0.11.2`, developers should add the configurations when they want to load the plug-ins in VM:

```c
/* Assume that wasi_crypto plug-in is installed in the default plug-in path. */
WasmEdge_PluginLoadWithDefaultPaths();
WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
WasmEdge_ConfigureAddHostRegistration(
    Conf, WasmEdge_HostRegistration_WasiCrypto_Common);
WasmEdge_ConfigureAddHostRegistration(
    Conf, WasmEdge_HostRegistration_WasiCrypto_AsymmetricCommon);
WasmEdge_ConfigureAddHostRegistration(Conf,
                                      WasmEdge_HostRegistration_WasiCrypto_Kx);
WasmEdge_ConfigureAddHostRegistration(
    Conf, WasmEdge_HostRegistration_WasiCrypto_Signatures);
WasmEdge_ConfigureAddHostRegistration(
    Conf, WasmEdge_HostRegistration_WasiCrypto_Symmetric);
WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(Conf, NULL);
WasmEdge_ConfigureDelete(Conf);

WasmEdge_String Names[32];
WasmEdge_StoreContext *StoreCxt = WasmEdge_VMGetStoreContext(VMCxt);
uint32_t ModuleLen = WasmEdge_StoreListModule(StoreCxt, Names, 32);
for (uint32_t I = 0; I < ModuleLen; I++) {
  printf("%s\n", Names[I].Buf);
}
/*
 * Will print:
 * wasi_ephemeral_crypto_asymmetric_common
 * wasi_ephemeral_crypto_common
 * wasi_ephemeral_crypto_kx
 * wasi_ephemeral_crypto_signatures
 * wasi_ephemeral_crypto_symmetric
 */
WasmEdge_VMDelete(VMCxt);
```

After `0.12.0`, the plug-ins will automatically loaded after the `WasmEdge_PluginLoadWithDefaultPaths()` called or the specific path given into the `WasmEdge_PluginLoadFromPath()` API.

For the plug-ins not installed, the mocked modules will be registered into VM context and will print the error message when invoking the host functions to notice the users to install the plug-in.

```c
WasmEdge_PluginLoadWithDefaultPaths();
WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
/* The `WasmEdge_HostRegistration_Wasi` is still needed. */
WasmEdge_ConfigureAddHostRegistration(Conf, WasmEdge_HostRegistration_Wasi);
WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(Conf, NULL);
WasmEdge_ConfigureDelete(Conf);

WasmEdge_String Names[32];
uint32_t ModuleLen = WasmEdge_VMListRegisteredModule(VMCxt, Names, 32);
for (uint32_t I = 0; I < ModuleLen; I++) {
  printf("%s\n", Names[I].Buf);
}
/*
 * Will print:
 * wasi_ephemeral_crypto_asymmetric_common
 * wasi_ephemeral_crypto_common
 * wasi_ephemeral_crypto_kx
 * wasi_ephemeral_crypto_signatures
 * wasi_ephemeral_crypto_symmetric
 * wasi_ephemeral_nn
 * wasi_snapshot_preview1
 * wasmedge_httpsreq
 * wasmedge_process
 */
WasmEdge_VMDelete(VMCxt);
```

> Note: The `WasmEdge_HostRegistration_Wasi` configuration is still needed if developers want to use the WASI.

## Creating the module instance from a plug-in

When developers didn't use the VM context to load the plug-ins, they can use the creation functions before the `0.11.2` versions:

```c
/* Assume that wasi_crypto plug-in is installed in the default plug-in path. */
WasmEdge_PluginLoadWithDefaultPaths();

WasmEdge_ModuleInstance *WasiCryptoCommonCxt =
    WasmEdge_ModuleInstanceCreateWasiCryptoCommon();

WasmEdge_ModuleInstanceDelete(WasiCryptoCommonCxt);
```

But it not make sense and not extensionable if developers should use different APIs for the different plug-ins.

After the `0.12.0` version, the `WasmEdge_PluginContext` is added, and developers can use the general API to create the module instances:

```c
/* Assume that wasi_crypto plug-in is installed in the default plug-in path. */
WasmEdge_PluginLoadWithDefaultPaths();

const char CryptoPName[] = "wasi_crypto";
const char CryptoMName[] = "wasi_crypto_common";
WasmEdge_String PluginName =
    WasmEdge_StringWrap(CryptoPName, strlen(CryptoPName));
WasmEdge_String ModuleName =
    WasmEdge_StringWrap(CryptoMName, strlen(CryptoMName));
const WasmEdge_PluginContext *PluginCxt = WasmEdge_PluginFind(PluginName);

WasmEdge_ModuleInstance *ModCxt =
    WasmEdge_PluginCreateModule(PluginCxt, ModuleName);

WasmEdge_ModuleInstanceDelete(ModCxt);
```

## Rereieving the module instances from the VM context

Before the version `0.11.2`, developers can retrieve the `WASI` or the modules from plug-ins with the `WasmEdge_HostRegistration` values, or retrieve the registered modules from the store context.

```c
/* Assume that wasi_crypto plug-in is installed in the default plug-in path. */
WasmEdge_PluginLoadWithDefaultPaths();
WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
/* Add the WASI-Crypto related configurations. */
WasmEdge_ConfigureAddHostRegistration(
    Conf, WasmEdge_HostRegistration_WasiCrypto_Common);
WasmEdge_ConfigureAddHostRegistration(
    Conf, WasmEdge_HostRegistration_WasiCrypto_AsymmetricCommon);
WasmEdge_ConfigureAddHostRegistration(Conf,
                                      WasmEdge_HostRegistration_WasiCrypto_Kx);
WasmEdge_ConfigureAddHostRegistration(
    Conf, WasmEdge_HostRegistration_WasiCrypto_Signatures);
WasmEdge_ConfigureAddHostRegistration(
    Conf, WasmEdge_HostRegistration_WasiCrypto_Symmetric);
/* Add the WASI configurations. */
WasmEdge_ConfigureAddHostRegistration(Conf, WasmEdge_HostRegistration_Wasi);
WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(Conf, NULL);
WasmEdge_ConfigureDelete(Conf);

/* Get the WASI module instance. */
WasmEdge_ModuleInstance *WASIModInst =
    WasmEdge_VMGetImportModuleContext(VMCxt, WasmEdge_HostRegistration_Wasi);
/* Get the WASI-crypto-common module instance. */
WasmEdge_ModuleInstance *WASICryptoCommonModInst =
    WasmEdge_VMGetImportModuleContext(
        VMCxt, WasmEdge_HostRegistration_WasiCrypto_Common);

/* Get the registered module instance by name. */
WasmEdge_StoreContext *StoreCxt = WasmEdge_VMGetStoreContext(VMCxt);
WasmEdge_String ModName =
    WasmEdge_StringCreateByCString("wasi_ephemeral_crypto_kx");
const WasmEdge_ModuleInstance *WASICryptoKxModInst =
    WasmEdge_StoreFindModule(StoreCxt, ModName);
WasmEdge_StringDelete(ModName);

WasmEdge_VMDelete(VMCxt);
```

After the `0.12.0` version, due to removing the plug-in related configurations and automatically registering the plug-ins into VM contexts, developers cannot use the `WasmEdge_VMGetImportModuleContext()` API to retrieve the module instances except the built-in host modules (`WASI` currently).

However, developers can use the new APIs to retrieve the registered module instances more quickly.

```c
/* Assume that wasi_crypto plug-in is installed in the default plug-in path. */
WasmEdge_PluginLoadWithDefaultPaths();
WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
/* Add the WASI configurations. */
WasmEdge_ConfigureAddHostRegistration(Conf, WasmEdge_HostRegistration_Wasi);
WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(Conf, NULL);
WasmEdge_ConfigureDelete(Conf);

/* Get the WASI module instance. */
WasmEdge_ModuleInstance *WASIModInst =
    WasmEdge_VMGetImportModuleContext(VMCxt, WasmEdge_HostRegistration_Wasi);
/* Get the registered WASI-crypto-common module instance by name. */
WasmEdge_String ModName =
    WasmEdge_StringCreateByCString("wasi_ephemeral_crypto_kx");
const WasmEdge_ModuleInstance *WASICryptoKxModInst =
    WasmEdge_VMGetImportModuleContext(VMCxt, ModName);
WasmEdge_StringDelete(ModName);

WasmEdge_VMDelete(VMCxt);
```
