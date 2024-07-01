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
3. The staled roadmap entries will be marked as `"Closed"` if no response from their assignees in the next quarterly discussion, and their related issues and pull requests will also be closed.

## Current Roadmap

Last Updated: Q3 / 2024

| Theme              | Description | Timeline | Assignee |
| ---                | ----------- | -------- | -------- |
| Proposal           | GC proposal for AOT/JIT     | Q4 / 2024 | [@q82419](https://github.com/q82419) |
| Proposal           | Exception-Handling proposal for AOT/JIT | Q4 / 2024 | [@q82419](https://github.com/q82419) |
| Proposal           | [Relaxed-SIMD proposal](https://github.com/WasmEdge/WasmEdge/pull/3311) | Q4 / 2024 | [@LFsWang](https://github.com/LFsWang) |
| Proposal           | Typed continuation proposal | Q2 / 2025 | |
| Proposal           | Stack-switch proposal       | Q2 / 2025 | |
| Proposal           | [WASI signature proposal](https://github.com/WasmEdge/WasmEdge/pull/517) | Q2 / 2025 | |
| Feature            | Wasm coredump | Q2 / 2025 | |
| Proposal           | [WASM C API proposal](https://github.com/WasmEdge/WasmEdge/pull/346) | Q4 / 2024 | [@q82419](https://github.com/q82419) |
| Proposal           | [WASM memory64 proposal](https://github.com/WasmEdge/WasmEdge/pull/2964) | Q4 / 2024 | [@dannypsnl](https://github.com/dannypsnl) |
| Feature            | DWARF symbol | Q2 / 2025 | |
| Proposal           | [Instantiation of Component model proposal](https://github.com/WasmEdge/WasmEdge/pull/3218) | Q4 / 2024 | [@dannypsnl](https://github.com/dannypsnl) |
| Languages Bindings | [Python SDK](https://github.com/WasmEdge/WasmEdge/pull/633) | Q2 / 2025 | |
| Proposal           | Whisper backend for WASI-NN | Q3 / 2024 | [@q82419](https://github.com/q82419) |
| Proposal           | WASI-NN GGML plugin with latest llama.cpp integration | Q3 / 2024 | [@dm4](https://github.com/dm4) |
| Language Bindings  | Update Java binding with 0.14 | Q3 / 2024 | |
| Language Bindings  | Move Go binding back into WasmEdge org | Q4 / 2024 | [@q82419](https://github.com/q82419) |
| Feature            | Deprecate manylinux2014 and make sure everything goes well on manylinux_2_28 | Q3 / 2024 | [@0yi0](https://github.com/0yi0) |
| Proposal           | WASI-NN rust (burn) plugin and also added more models support | Q3 / 2024 | [@CaptainVincent](https://github.com/CaptainVincent) |

## Previous Roadmap

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
