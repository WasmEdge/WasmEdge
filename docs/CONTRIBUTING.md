# Contributing to WasmEdge

## Welcome

[**WasmEdge**](https://github.com/WasmEdge/WasmEdge) (previously known as SSVM) is a high-performance WebAssembly (Wasm) VM optimized for Edge Computing, including Edge Clouds and Software Defined Vehicles.

WasmEdge is developed in the open, and is constantly being improved by our **users, contributors, and maintainers**. It is because of you that we can bring great software to the community.

This guide provides information on filing issues and guidelines for open source contributors. **Please leave comments / suggestions if you find something is missing or incorrect.**

If you are looking for ideas for contribution, [here is a wish list](wish_list.md) of items we'd like to get some help with!

> The WasmEdge project adopts [DCO](https://community.openhab.org/t/dco-check-signing-off-with-github-web-editor-explanation/83330) to manage all contributions. Please make sure you add your `sign-off-statement` through the `-s` flag or the GitHub Web UI before committing the pull request message.

## Getting Started

### Fork Repository

Fork the WasmEdge repository on GitHub to your personal account.

```bash
git clone git@github.com:WasmEdge/WasmEdge.git
cd WasmEdge
```

Notes: Note the WasmEdge team builds lots of extensions of Server-side WebAssembly, see [TensorFlow](https://github.com/second-state/WasmEdge-tensorflow), [Storage](https://github.com/second-state/WasmEdge-storage), [Command interface](https://github.com/second-state/wasmedge_process_interface), [Ethereum](https://github.com/second-state/WasmEdge-evmc), [Substrate](https://github.com/ParaState/substrate-ssvm-node). If you want to contribute to the extensions, please go to those repositories.

### Setup Development Environment

The WasmEdge is developed on Ubuntu 20.04 to take advantage of advanced LLVM features for the AOT compiler. The WasmEdge team also builds and releases statically linked WasmEdge binaries for older Linux distributions.

Our development environment requires libLLVM-10 and >=GLIBCXX_3.4.26.

If you are using an operating system older than Ubuntu 20.04, please use our special docker image to build WasmEdge. If you are looking for the pre-built binaries for the older operating system, we also provide several pre-built binaries based on manylinux* distribution.

### Docker image

```bash
docker pull wasmedge/wasmedge
```

### Setup the environment manually

```bash
# Tools and libraries
sudo apt install -y \
    software-properties-common \
    cmake \
    libboost-all-dev
# And you will need to install llvm for wasmedgec tool
sudo apt install -y \
    llvm-dev \
    liblld-10-dev
# WasmEdge supports both clang++ and g++ compilers# You can choose one of them for building this project
sudo apt install -y gcc g++
sudo apt install -y clang
```

## Contribute Workflow

PRs are always welcome, even if they only contain small fixes like typos or a few lines of code. If there will be a significant effort, please document it as an issue and get a discussion going before starting to work on it.

Please submit a PR broken down into small changes bit by bit. A PR consisting of a lot features and code changes may be hard to review. It is recommended to submit PRs in an incremental fashion.

Note: If you split your pull request into small changes, please make sure any of the changes that goes to master will not break anything. Otherwise, it can not be merged until this feature is complete.

### Fork and clone

Fork [the WasmEdge repository](https://github.com/WasmEdge/WasmEdge) and clone the code to your local workspace

### Branch

Changes should be made on your own fork in a new branch. The branch should be named XXX-description where XXX is the number of the issue. PR should be rebased on top of master without multiple branches mixed into the PR. If your PR does not merge cleanly, use the commands listed below to get it up to date.

### Develop, Build and Test

Write code on the new branch in your fork.

```bash
# After pulling our wasmedge docker image
docker run -it --rm \
    -v <path/to/your/wasmedge/source/folder>:/root/wasmedge \
    wasmedge/wasmedge:latest
# In docker
cd /root/wasmedge
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_TESTS=ON .. && make -j
```

**Run tests**
The following built-in tests are only available when the build flag WASMEDGE_BUILD_TESTS sets to ON.
You can use these tests to verify the correctness of WasmEdge binaries.

```bash
cd <path/to/wasmedge/build_folder>
LD_LIBRARY_PATH=$(pwd)/lib/api ctest
```

### Push and Create PR

When ready for review, push your branch to your fork repository on github.com.

Then visit your fork at <https://github.com/$user/WasmEdge> and click the Compare & Pull Request button next to your branch to create a new pull request (PR). Description of a pull request should refer to all the issues that it addresses. Remember to put a reference to issues (such as Closes #XXX and Fixes #XXX) in commits so that the issues can be closed when the PR is merged.

Once your pull request has been opened it will be assigned to one or more reviewers. Those reviewers will do a thorough code review, looking for correctness, bugs, opportunities for improvement, documentation and comments, and style.

Commit changes made in response to review comments to the same branch on your fork.

## Reporting issues

It is a great way to contribute to WasmEdge by reporting an issue. Well-written and complete bug reports are always welcome! Please open an issue on Github.

Before opening any issue, please look up the existing [issues](https://github.com/WasmEdge/WasmEdge/issues) to avoid submitting a duplication. If you find a match, you can "subscribe" to it to get notified on updates. If you have additional helpful information about the issue, please leave a comment.

When reporting issues, always include:

* Version of your system
* Configuration files of WasmEdge

Because the issues are open to the public, when submitting the log and configuration files, be sure to remove any sensitive information, e.g. user name, password, IP address, and company name. You can replace those parts with "REDACTED" or other strings like "****".
Be sure to include the steps to reproduce the problem if applicable. It can help us understand and fix your issue faster.

## Documenting

Update the documentation if you are creating or changing features. Good documentation is as important as the code itself.
Documents are written with Markdown. See [Writing on GitHub](https://help.github.com/categories/writing-on-github/) for more details.

## Design new features

You can propose new designs for existing WasmEdge features. You can also design entirely new features, please submit a proposal via GitHub issues.

WasmEdge maintainers will review this proposal as soon as possible. This is necessary to ensure the overall architecture is consistent and to avoid duplicated work in the roadmap.
