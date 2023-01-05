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
3. Check if the `python3` is installed. If so, go to step 6. Otherwise, go to step 4.
4. Check if the `python2` is installed. If so, go to step 6. Otherwise, go to step 5.
5. Check if the `python` is installed. If so, go to step 6. Otherwise, exit with an error `Please install python or provide python path via $PYTHON_EXECUTABLE`.
6. Print the detected python version `Using Python: $PYTHON_EXECUTABLE`.
7. Download `install.py` with `curl` or `wget`. If the URL of `install.py` is unreachable due to a network issue, exit with an error `$INSTALL_PY_URL not reachable`. If the `curl` and `wget` are not available, exit with an error `curl or wget could not be found`.
8. Execute the `install.py` with all received arguments.

### `install.py`

The real installer handles all stuff.

## Options

### Help Msg
* Short Option: `-h`
* Full Option: `--help`
* Description: Show this help message and exit.

### Verbose
* Short Option: `-D`
* Full Option: `--debug`
* Description: Enable verbosity debug

### Specify Installed Version
* Short Option: `-v VERSION`
* Full Option: `--version VERSION`
* Description: Install the given VERSION of WasmEdge
* Available Value: VERSION `0.11.2` or other valid release versions.

### Install Path
* Short Option: `-p PATH`
* Full Option: `--path PATH`
* Description: Install WasmEdge into the given PATH. The default Path is `$HOME/.wasmedge`.

### Uninstall existed version

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
* Available Value: Supported Extensions `'tensorflow', 'image', 'all'`.

#### Tensorflow Extensions Library Version
* Full Option: `--tf-version TF_VERSION`
* Description: Install the given VERSION of the library of the Tensorflow and Tensorflow lite extension. Only available when the `Extensions` is set to `all` or `tensorflow`.

#### Tensorflow Extensions Dependencies Version
* Full Option: `--tf-deps-version TF_DEPS_VERSION`
* Description: Install the given VERSION of the dependencies of the Tensorflow and Tensorflow lite extension. Only available when the `Extensions` is set to `all` or `tensorflow`.

#### Tensorflow Extensions Tools Version
* Full Option: `--tf-tools-version TF_TOOLS_VERSION`
* Description: Install the given VERSION of the tools of the Tensorflow and Tensorflow lite extension. Only available when the `Extensions` is set to `all` or `tensorflow`.

#### Image Extensions Version
* Full Option: `--image-version IMAGE_VERSION`
* Description: Install the given VERSION of the Image extension. Only available when the `Extensions` is set to `all` or `image`.

#### Image Extensions Dependencies Version
* Full Option: `--image-deps-version IMAGE_DEPS_VERSION`
* Description: Install the given VERSION of the dependencies of the Image extension. Only available when the `Extensions` is set to `all` or `image`.

### Plugins

TBD.

### DIST

TBD.

### Platform and OS
* Full Option: `--platform PLATFORM` or `--os OS`
* Description: Install the given `PLATFORM` or `OS` version of WasmEdge. This value should be case insensitive to make the maximum compatibility.
* Available Value (case insensitive): "Linux", "Darwin", "Windows".

### Machine and Arch
* Full Option: `--machine MACHINE` or `--arch ARCH`
* Description: Install the given `MACHINE` or `ARCH` version of WasmEdge.
* Available Value: "x86_64", "aarch64".
