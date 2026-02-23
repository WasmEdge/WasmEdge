# WasmEdge Roadmap

## About This Document

This document serves as a reference point for WasmEdge users and contributors to understand where the project is heading, and help to determine if a contribution could be conflicting with a longer term plan. Additionally, this document also describes the rules and steps of proposing and retiring roadmap entries.

## Planning for Roadmap

WasmEdge roadmap will be updated quarterly with the following steps.

### New Roadmap Discussion

1. WasmEdge maintainers will open an issue on the upcoming roadmap at least 14 days before the start of a new quarter.
2. Any participation can request a roadmap entry by commenting on the issue and volunteering as an assignee.
3. WasmEdge maintainers will collect the proposed roadmap entries along with their respective timelines and assignees, then update them within the issue.
4. When the new quarter starts, the roadmap discussion issue will be finalized, this document will be updated, and new issues for the roadmap entries will be opened.

### Updating Roadmap Status

1. The completed roadmap entries will be marked as `"Completed"` when in a quarterly discussion.
2. Existing roadmap entries which not reach their deadlines will be retained in the new quarter and marked as `"Active"`.

### Stale And Close

1. For roadmap entries which have reached their deadlines, mark them as `"Staled"`.
2. The assignees of roadmap entries can request for updating the timeline.
3. The staled roadmap entries will be marked as `"Inactive"` or `"Closed"` if no response from their assignees in the next quarterly discussion. The assignee will be removed, and we welcome everyone work on it if they interest in.
4. The inactive roadmap entries will be closed if they will not be planned to support, and their related issues and pull requests will also be closed.

## Current Roadmap

Last Updated: Q1 / 2026

| Theme    | Description | Timeline | Assignee |
| ---      | ----------- | -------- | -------- |

| Document | WasmEdge documentation refactoring | Q1 / 2026 | [@q82419](https://github.com/q82419) |
| Proposal | [Component Model completion](https://github.com/WasmEdge/WasmEdge/issues/4236) | Q2 / 2026 | [@q82419](https://github.com/q82419) |
| Proposal | [Component Model canonical section refactoring](https://github.com/WasmEdge/WasmEdge/issues/4334) | Q2 / 2026 | [@q82419](https://github.com/q82419) |
| Proposal | [WASI preview2 as plugins](https://github.com/WasmEdge/WasmEdge/issues/4236) | Q2 / 2026 | [@q82419](https://github.com/q82419) |
| Feature | [LFX (2026/term1) Extend sub-command of WasmEdge CLI tool](https://github.com/WasmEdge/WasmEdge/issues/4513) | Q2 / 2026 | |
| Feature | [LFX (2026/term1) Module instance dependency tree in WASM store](https://github.com/WasmEdge/WasmEdge/issues/4514) | Q2 / 2026 | |
| Feature | [LFX (2026/term1) Enable JIT mode support for per-function compilation](https://github.com/WasmEdge/WasmEdge/issues/4516) | Q2 / 2026 | |

## Inactive Roadmap

| Theme | Description |
| ----- | ----------- |
| Language Binding | [Update Java binding with 0.14](https://github.com/WasmEdge/WasmEdge/pull/3663) |
| Proposal | [WASM C API proposal](https://github.com/WasmEdge/WasmEdge/pull/346) |
| Language Binding | Move Go binding back into WasmEdge org |
| Feature | [Redirection of stdin, stdout, and stderr in C SDK](https://github.com/WasmEdge/WasmEdge/issues/2936) |
| Proposal | [WASI-NN GGML plugin with CANN support](https://github.com/WasmEdge/WasmEdge/issues/3768) |
| Proposal | [WASI-NN TensorRT plugin initial support](https://github.com/WasmEdge/WasmEdge/pull/3878) |
| Feature | Update installer to use manylinux_2_28 from 0.15.0 |
| Proposal | Support TensorRT in wasi_nn plugin |
| Feature | Apply `-Wshadow` and `-Wshadow-field` to enhance the codebase |
| Proposal | [WASI signature proposal](https://github.com/WasmEdge/WasmEdge/pull/517) |
| Feature | Wasm coredump |
| Feature | DWARF symbol |
| Languages Bindings | [Python SDK](https://github.com/WasmEdge/WasmEdge/pull/633) |
| Feature | Support stack tracing in the JIT and AOT modes |
| Feature | [Setup workflows for](https://github.com/WasmEdge/WasmEdge/issues/3840) [cpp-plugins repo](https://github.com/WasmEdge/cpp-plugins) |
| Feature | A new WasmEdge installer and plugin manager |
| Proposal | Exception-Handling proposal for AOT/JIT |
| Proposal | Typed continuation proposal |
| Proposal | Stack-switch proposal |
| Proposal | WASI-NN Whisper.cpp plugin: update for supporting the greedy sampling |
| Proposal | [Runtime GC support in WasmEdge](https://github.com/WasmEdge/WasmEdge/pull/4156) |

## Previous Roadmap

### Q4/2025

| Theme | Description | Timeline | Assignee | Status |
| --- | --- | --- | --- | --- |
| Feature | WasmEdge 0.16.0 release | Q4 / 2025 | [@hydai](https://github.com/hydai) | Completed |
| Proposal | [WASM 3.0 supporting](https://github.com/WasmEdge/WasmEdge/issues/4382) | Q4 / 2025 | [@q82419](https://github.com/q82419) | Completed |
| Proposal | [Component Model value and value type refactoring](https://github.com/WasmEdge/WasmEdge/issues/4333) | Q4 / 2025 | [@q82419](https://github.com/q82419) | Completed |
| Feature | [LFX (2025/term3) Pointer alignment checking for WASI host function arguments](https://github.com/WasmEdge/WasmEdge/issues/4362) | Q4 / 2025 | [@Minimega12121](https://github.com/Minimega12121) | Completed |
| Proposal | WASI-NN GGML plugin with latest llama.cpp integration for Q4/2025 | Q4 / 2025 | [@dm4](https://github.com/dm4) | Completed |
| Proposal | [Runtime GC support in WasmEdge](https://github.com/WasmEdge/WasmEdge/pull/4156) | Q4 / 2025 | [@ibmibmibm](https://github.com/ibmibmibm) | Staled |
| Proposal | Update the Android NDK build examples | Q4 / 2025 | [@dm4](https://github.com/dm4) | Staled |

### Q3/2025

| Theme | Description | Timeline | Assignee | Status |
| --- | --- | --- | --- | --- |
| Proposal | WASI-NN GGML plugin with latest llama.cpp integration | Q3 / 2025 | [@dm4](https://github.com/dm4) | Completed |
| Proposal | [Component Model AST refactoring](https://github.com/WasmEdge/WasmEdge/pull/4123) | Q3 / 2025 | [@q82419](https://github.com/q82419) | Completed |
| Proposal | [Component Model linking phase refactoring](https://github.com/WasmEdge/WasmEdge/pull/4321) | Q3 / 2025 | [@q82419](https://github.com/q82419) | Completed |
| Proposal | [LFX (2025/term2) runwasi multi-node stability testing](https://github.com/WasmEdge/WasmEdge/issues/4155) | Q3 / 2025 | [@CaptainVincent](https://github.com/CaptainVincent) | Completed |
| Feature | A new WasmEdge installer and plugin manager (TBD) | Q3 / 2025 | [@hydai](https://github.com/hydai) | Staled |
| Proposal | Exception-Handling proposal for AOT/JIT | Q3 / 2025 | [@ibmibmibm](https://github.com/ibmibmibm) | Staled |
| Proposal | Typed continuation proposal | Q3 / 2025 | [@q82419](https://github.com/q82419) | Staled |
| Proposal | Stack-switch proposal | Q3 / 2025 | [@q82419](https://github.com/q82419) | Staled |
| Proposal | WASI-NN Whisper.cpp plugin: update for supporting the greedy sampling | Q3 / 2025 | [@hydai](https://github.com/hydai) | Staled |

### Q2/2025

| Theme | Description | Timeline | Assignee | Status |
| --- | --- | --- | --- | --- |
| Proposal | [WASI signature proposal](https://github.com/WasmEdge/WasmEdge/pull/517) | Q2 / 2025 | | Staled |
| Feature | Wasm coredump | Q2 / 2025 | | Staled |
| Feature | DWARF symbol | Q2 / 2025 | | Staled |
| Languages Bindings | [Python SDK](https://github.com/WasmEdge/WasmEdge/pull/633) | Q2 / 2025 | | Staled |
| Feature | Support stack tracing in the JIT and AOT modes | Q2 / 2025 | [@hydai](https://github.com/hydai) | Staled |
| Feature | [Setup workflows for](https://github.com/WasmEdge/WasmEdge/issues/3840) [cpp-plugins repo](https://github.com/WasmEdge/cpp-plugins) | Q2 / 2025 | [@0yi0](https://github.com/0yi0) | Staled |
| Proposal | [LFX (2025/term1) component model validator](https://github.com/WasmEdge/WasmEdge/issues/3966) | Q2 / 2025 | [@dannypsnl](https://github.com/dannypsnl) | Completed |
| Feature | Fix the IWYU reported errors | Q1 / 2025 | [@hydai](https://github.com/hydai) | Completed |
| Proposal | GC proposal for AOT/JIT | Q3 / 2025 | [@q82419](https://github.com/q82419) | Completed |
| Proposal | WASI-NN GGML plugin with latest llama.cpp integration | Q2 / 2025 | [@dm4](https://github.com/dm4) | Completed |
| Proposal | WASI-NN GGML plugin: support libmtmd | Q2 / 2025 | [@dm4](https://github.com/dm4) | Completed |

### Q1/2025

| Theme | Description | Timeline | Assignee | Status |
| --- | --- | --- | --- | --- |
| Proposal | [WASI-NN GGML plugin: support mllama for llama 3.2](https://github.com/WasmEdge/WasmEdge/pull/3929) | Q1 / 2025 | [@q82419](https://github.com/q82419) | Completed |
| Proposal | WASI-NN ChatTTS plugin with llama.cpp integration | Q1 / 2025 | [@dm4](https://github.com/dm4) | Completed |
| Proposal | WASI-NN GGML plugin: support Qwen2VL | Q1 / 2025 | @dm4 | Completed |
| Feature | Update installer to use manylinux_2_28 from 0.15.0 | Q1 / 2025 | [@0yi0](https://github.com/0yi0) | Staled |
| Proposal | [WASI-NN TensorRT plugin initial support](https://github.com/WasmEdge/WasmEdge/pull/3878) | Q1 / 2025 | [@ibmibmibm](https://github.com/ibmibmibm) | Staled |
| Proposal | [WASI preview2 partial as plugins](https://github.com/WasmEdge/WasmEdge/pull/3753) | Q1 / 2025 | [@dannypsnl](https://github.com/dannypsnl) | Staled |
| Feature | [Apply `-Wshadow` and `-Wshadow-field` to enhance the codebase](https://github.com/WasmEdge/WasmEdge/pull/3055) | Q1 / 2025 | [@hydai](https://github.com/hydai) | Staled |
| Feature | Fix the IWYU reported errors | Q1 / 2025 | [@hydai](https://github.com/hydai) | Staled |
| Feature | Upgrade WasmEdge version in Runwasi | Q1 / 2025 | [@CaptainVincent](https://github.com/CaptainVincent) | Completed |
| Feature | Add support for WasmEdge plugins in Runwasi | Q1 / 2025 | [@CaptainVincent](https://github.com/CaptainVincent) | Completed |

### Q4/2024

| Theme | Description | Timeline | Assignee | Status |
| --- | --- | --- | --- | --- |
| Proposal | [Move the Rust (burn.rs) plug-ins into an independent repo](https://github.com/WasmEdge/WasmEdge/issues/3767) | Q4 / 2024 | [@CaptainVincent](https://github.com/CaptainVincent) | Completed |
| Proposal | Component-Model: start section | Q4 / 2024 | [@dannypsnl](https://github.com/dannypsnl) | Completed |
| Proposal | Component-Model: resource type | Q4 / 2024 | [@dannypsnl](https://github.com/dannypsnl) | Completed |
| Feature | [Upgrade CI workflows Ubuntu to 24.04](https://github.com/WasmEdge/WasmEdge/pull/3774) | Q4 / 2024 | [@0yi0](https://github.com/0yi0) | Completed |
| Feature | Refactor structure of CI workflows to reduce duplicates and improve efficiency | Q4 / 2024 | [@0yi0](https://github.com/0yi0) | Completed |
| Feature | Support LLVM19 | Q4 / 2024 | [@ibmibmibm](https://github.com/ibmibmibm) | Completed |
| Proposal | [WASM C API proposal](https://github.com/WasmEdge/WasmEdge/pull/346) | Q4 / 2024 | [@q82419](https://github.com/q82419) | Staled |
| Proposal | [WASM memory64 proposal](https://github.com/WasmEdge/WasmEdge/pull/2964) | Q4 / 2024 | [@dannypsnl](https://github.com/dannypsnl) | Staled |
| Language Binding | Move Go binding back into WasmEdge org | Q4 / 2024 | [@q82419](https://github.com/q82419) | Staled |
| Feature | [Redirection of stdin, stdout, and stderr in C SDK](https://github.com/WasmEdge/WasmEdge/issues/2936) | Q4 / 2024 | [@hydai](https://github.com/hydai) | Staled |
| Proposal | [WASI-NN GGML plugin with CANN support](https://github.com/WasmEdge/WasmEdge/issues/3768) | Q4 / 2024 | [@dm4](https://github.com/dm4) | Staled |
| Feature | Update installer to use manylinux_2_28 from 0.15.0 | Q4 / 2024 | | Staled |

### Q3/2024

| Theme | Description | Timeline | Assignee | Status |
| --- | --- | --- | --- | --- |
| Proposal | [Relaxed-SIMD proposal](https://github.com/WasmEdge/WasmEdge/pull/3311) | Q4 / 2024 | [@LFsWang](https://github.com/LFsWang) | Completed |
| Proposal | [Whisper backend for WASI-NN](https://github.com/WasmEdge/WasmEdge/pull/3484) | Q3 / 2024 | [@q82419](https://github.com/q82419) | Completed |
| Proposal | [WASI-NN rust (burn) plugin and also added more models support](https://github.com/WasmEdge/WasmEdge/pull/3543) | Q3 / 2024 | [@CaptainVincent](https://github.com/CaptainVincent) | Completed |
| Feature | Deprecate manylinux2014 and make sure everything goes well on manylinux_2_28 | Q3 / 2024 | [@0yi0](https://github.com/0yi0) | Completed |
| Proposal | [Instantiation of Component model proposal](https://github.com/WasmEdge/WasmEdge/pull/3218) | Q4 / 2024 | [@dannypsnl](https://github.com/dannypsnl) | Completed |
| Language Binding | [Update Java binding with 0.14](https://github.com/WasmEdge/WasmEdge/pull/3663) | Q3 / 2024 | [@Kuntal271](https://github.com/Kuntal271) | Staled |

### Q2/2024

| Theme | Description | Assignee | Status |
| ----- | ----------- | -------- | ------ |
| Proposal | GC proposal for interpreter | [@q82419](https://github.com/q82419), [@little-willy](https://github.com/little-willy) | Completed |
| Proposal | Exception-Handling proposal for interpreter | [@harry900831](https://github.com/harry900831), [@q82419](https://github.com/q82419) | Completed |
| Proposal | GGML backend for WASI-NN | [@dm4](https://github.com/dm4), [@CaptainVincent](https://github.com/CaptainVincent) | Completed |
| Feature | JIT support | [@ibmibmibm](https://github.com/ibmibmibm) | Completed |

### Q1/2024

| Theme | Description | Assignee | Status |
| ----- | ----------- | -------- | ------ |
| Proposal | Typed-function references proposal | [@q82419](https://github.com/q82419), [@little-willy](https://github.com/little-willy) | Completed |
| Proposal | Loader phase of Component model proposal | [@dannypsnl](https://github.com/dannypsnl) | Completed |
| Feature | WASM serialization | [@dracoooooo](https://github.com/dracoooooo) | Completed |
