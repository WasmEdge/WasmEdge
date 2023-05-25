# `wasmedge run` CLI

After [installation](../quick_start/install.md), users can execute the `wasmedge run` command.

`wasmedge run` is an alias of `wasmedge` without `-v|--version` option. The usage of the `wasmedge run` tool will be:

```bash
$ wasmedge run -h
USAGE
   wasmedge run [OPTIONS] [--] WASM_OR_SO [ARG ...]

...
```

## Options

`wasmedge run` is an alias of `wasmedge` without `-v|--version` option.

In the other words, if users want to execute the following command.

```bash
wasmedge --reactor fibonacci.wasm fib 10
```

It is also accepted to add subcommand `run` and will have no difference on execution process and result.

```bash
wasmedge run --reactor fibonacci.wasm fib 10
```
