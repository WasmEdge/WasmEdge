# SPIFFE/SPIRE Compatibility and Non-Human Identity in WasmEdge

## Summary

This document describes how WasmEdge can be deployed in environments that use SPIFFE/SPIRE for workload identity. It clarifies what the WasmEdge runtime provides today, how identity is managed for non-human workloads, and what security boundaries apply.

## Current Compatibility Statement

WasmEdge does not implement SPIFFE or SPIRE directly in this repository. Instead, WasmEdge is compatible with SPIFFE/SPIRE deployments by running as a workload inside an environment that already provides SPIFFE identities. In that model, WasmEdge and the Wasm modules it runs can consume SPIFFE SVIDs and related identity materials that are supplied by the host platform.

## Non-Human Identity Model in WasmEdge

WasmEdge runs WebAssembly modules inside an operating system process. The default identity boundary is the OS process or container that hosts the WasmEdge runtime. If per-module identity isolation is required, run each module in its own WasmEdge process or container, and assign it a distinct SPIFFE identity via the host platform.

## Access to Identity Materials

SPIFFE/SPIRE typically delivers identity materials (such as SVIDs) through host mechanisms. WasmEdge can access those materials when the host explicitly makes them available to the runtime and to the module. For example:

- Files or sockets exposed by the SPIRE agent can be mounted into the WasmEdge process and provided to the module via preopened paths.
- Environment variables or arguments that point to identity locations can be passed to the module.

WasmEdge does not bypass host policy. If the host does not preopen a path or otherwise expose the identity channel, the module cannot access it.

## Monitoring and Security Filtering

WasmEdge provides a sandbox for WebAssembly execution. Access to host resources is mediated by the runtime and configured by the host. For example, the CLI supports restricting filesystem access by preopening specific directories and marking them read-only using `--dir guest_path:host_path:readonly`. This allows hosts to expose only the minimum necessary identity-related files while keeping other paths inaccessible.

## Deployment Guidance

To integrate WasmEdge with SPIFFE/SPIRE:

- Deploy WasmEdge as a workload that is already enrolled with SPIRE.
- Configure the host to expose identity materials to WasmEdge only through the minimal required interfaces.
- Use one WasmEdge process per security boundary when a distinct identity is required.

## References

- SPIFFE: https://spiffe.io/
- SPIRE: https://spiffe.io/spire/
- wasmCloud SPIFFE adoption: https://wasmcloud.com/blog/2025-03-04-why-were-adopting-spiffe-for-webassembly-workload-identity/

Last updated: 2026-02-05
