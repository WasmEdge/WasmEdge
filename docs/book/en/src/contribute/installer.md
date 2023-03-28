# Installer Guide

## Overview

WasmEdge installer is designed for installing the Core Tools (`wasmedge`, `wasmedgec`), the Libraries (`libwasmedge`), the Extensions(`wasmedge-tensorflow`), and the Plugins(`wasi-nn`, `wasi-crytpo`).

## Dependencies

In the first version of the installer, WasmEdge provides a pure shell script implementation. However, it's not easy to maintain and is not suitable when we want to include the extensions and plugins matrix.

To reduce the cost of maintenance and improve the development performance, we decided to move forward to a brand new installer that is written in Python and is compatible with both Python 2 and 3.

To be compatible with the old one, we use the same entry point, `install.sh`.

## Usage

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- ${OPTIONS}
```

## Roles

### `install.sh`

The installer entry point.

#### Process

1. Check if the `git` is installed; otherwise, exit with an error `Please install git`.
2. If `PYTHON_EXECUTABLE` is given, then try to use `$PYTHON_EXECUTABLE` to execute the `install.py`. Otherwise, go to step 3.
3. If `PYTHON_EXECUTABLE` is not set, `which` command is needed to determine the python-X executable. If it is not found installer exits else it moves on to the next step.
4. Check if the `python3` is installed. If so, go to step 6. Otherwise, go to step 5.
5. Check if the `python2` is installed. If so, go to step 6. Otherwise, go to step 6.
6. Check if the `python` is installed. If so, go to step 7. Otherwise, exit with an error `Please install python or provide python path via $PYTHON_EXECUTABLE`.
7. Print the detected python version `Using Python: $PYTHON_EXECUTABLE`.
8. Download `install.py` with `curl` or `wget`. If the URL of `install.py` is unreachable due to a network issue, exit with an error `$INSTALL_PY_URL not reachable`. If the `curl` and `wget` are not available, exit with an error `curl or wget could not be found`.
9. Execute the `install.py` with all received arguments.

### `install.py`

The real installer handles all stuff. It supports python2.7 (not tested on earlier versions) as well as the latest python versions python3.x.

## Options

### Help Msg

* Short Option: `-h`
* Full Option: `--help`
* Description: Show this help message and exit.

### Verbose

* Short Option: `-D`
* Full Option: `--debug`
* Description: Enable verbosity debug

### Specify the version of WasmEdge to install

* Short Option: `-v VERSION`
* Full Option: `--version VERSION`
* Description: Install the given VERSION of WasmEdge
* Available Value: VERSION `0.11.2` or other valid release versions.
* Note - In the case of supplied an invalid or nonexistent version, the installer exists with an error.

### Installation path

* Short Option: `-p PATH`
* Full Option: `--path PATH`
* Description: Install WasmEdge into the given PATH. The default Path is `$HOME/.wasmedge`.
* Note - In any path other than the ones starting with `/usr` are treated as non system paths in the internals of the installer. The consequences are different directory structures for both.
* Note - If the path not exists, the folder will be created.

### Uninstallation

#### Run uninstaller before installing

* Short Option: `-r {yes,no}`
* Full Option: `--remove-old {yes, no}`
* Description: Run the uninstaller script before installing. Default `yes`.

#### Use a specific version of the uninstaller

* Short Option: `-u UNINSTALL_SCRIPT_TAG`
* Full Option: `--uninstall-script-tag UNINSTALL_SCRIPT_TAG`
* Description: Use the given GitHub tag to uninstall the script

### Install Extensions

* Short Option: `-e [EXTENSIONS [EXTENSIONS ...]]`
* Full Option: `--extension [EXTENSIONS [EXTENSIONS ...]]`
* Description: Install wasmedge-extension tools.
* Available Value (case sensitive): Supported Extensions `'tensorflow', 'image', 'all'`.

#### Tensorflow Extensions Library Version

* Full Option: `--tf-version TF_VERSION`
* Description: Install the given VERSION of the library of the Tensorflow and Tensorflow lite extension. Only available when the `Extensions` is set to `all` or `tensorflow`.
* Note - It's the same as the WasmEdge version if not specified.

#### Tensorflow Extensions Dependencies Version

* Full Option: `--tf-deps-version TF_DEPS_VERSION`
* Description: Install the given VERSION of the dependencies of the Tensorflow and Tensorflow lite extension. Only available when the `Extensions` is set to `all` or `tensorflow`.
* Note - It's the same as the WasmEdge version if not specified.

#### Tensorflow Extensions Tools Version

* Full Option: `--tf-tools-version TF_TOOLS_VERSION`
* Description: Install the given VERSION of the tools of the Tensorflow and Tensorflow lite extension. Only available when the `Extensions` is set to `all` or `tensorflow`.
* Note - It's the same as the WasmEdge version if not specified.

#### Image Extensions Version

* Full Option: `--image-version IMAGE_VERSION`
* Description: Install the given VERSION of the Image extension. Only available when the `Extensions` is set to `all` or `image`.
* Note - It's the same as the WasmEdge version if not specified.

### Plugins

* Note - Currently `--plugins` is an experimental option.

* Full Option: `--plugins wasi_crypto:0.11.0`

* Note - The format for this argument is `<plugin_name>:<version_number>`. `<version_number>` is not compulsory. For example `--plugins wasi_crypto` is a valid option.
* Note - `<plugin_name>` is cases sensitive. Allowed values are stated [here](https://wasmedge.org/book/en/plugin.html) in the `Rust Crate` column. The logic is that the release name should be the same.
* Note - It's the same as the WasmEdge version if not specified.

### DIST

* Full Option: `--dist ubuntu20.04` or `--dist manylinux2014`
* Note - the `ubuntu20.04` and `manylinux2014` values are case insensitive and only these two are currently supported.
* Note - Specifying `--dist` value for `Darwin` has no effect.
* Note - For `Linux` platform if the distribution matches exactly as `Ubuntu 20.04` which is checked using `lsb_release` and python's `platform.dist()` functionality then it is set to `ubuntu20.04` if not specified, else it is used without questioning. However different release packages for WasmEdge are available only after `0.11.1` release below which there is no effect of specifying this option.

### Platform and OS

* Full Option: `--platform PLATFORM` or `--os OS`
* Description: Install the given `PLATFORM` or `OS` version of WasmEdge. This value should be case insensitive to make the maximum compatibility.
* Available Value (case insensitive): "Linux", "Darwin", "Windows".

### Machine and Arch

* Full Option: `--machine MACHINE` or `--arch ARCH`
* Description: Install the given `MACHINE` or `ARCH` version of WasmEdge.
* Available Value: "x86_64", "aarch64".

## Behaviour

* If there exists an installation at `$HOME/.wasmedge` which is to be noted as the default installation path, it is removed with or without uninstaller's invocation.
* WasmEdge installation appends all the files it installs to a file which is located in the installer directory named `env` with it's path as `$INSTALLATION_PATH/env`

### Shell and it's configuration

* Source string in shell configuration is given as `. $INSTALLATION_PATH/env` so that it exports the necessary environment variables for WasmEdge.
* Shell configuration file is appended with source string if it cannot find the source string in that file.
* Currently it detects only `Bash` and `zsh` shells.
* If the above shells are found, then their respective configuration files `$HOME/.bashrc` and `$HOME/.zshrc` are updated along with `$HOME/.zprofile` and `$HOME/.bash_profile` in case of Linux.
* In case of `Darwin`, only `$HOME/.zprofile` is updated with the source string.
