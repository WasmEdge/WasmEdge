# WASI Interface Validation and Security Guidance

## Overview

WebAssembly System Interface (WASI) provides a standardized set of POSIX-like system calls allowing WebAssembly bytecode to interact with host operating system resources (filesystem, clocks, random numbers, environment variables, and network sockets). Because WebAssembly execution relies entirely on host-provided capability imports, the host runtime's WASI implementation serves as the primary security boundary protecting host environments from untrusted or multi-tenant WebAssembly guest code.

This document serves as the comprehensive security architecture, interface validation specification, and operational hardening guide for WasmEdge's WASI implementation. It directly addresses recommendations from the CNCF TAG-Security Joint Assessment regarding:

* Validation of WASI interface implementations
* Security architecture of provided interfaces (specifically preopened directory mechanisms)
* Prevention of path traversal, escape, and race condition vulnerabilities
* Secure handling of symbolic links and mounted directory namespaces
* User guidance, deployment best practices, and common security pitfalls

---

## Threat Model and Security Guarantees

WasmEdge's WASI implementation operates under a capability-based zero-trust security model:

| Vector | Assumption / Threat | WasmEdge Security Guarantee |
| :--- | :--- | :--- |
| **Ambient Authority** | Untrusted guest code attempts to access arbitrary host files or sockets | **Zero ambient authority**: Guests have zero filesystem or network access unless explicitly preopened via configuration flags (`--dir`, `--env`). |
| **Path Traversal (`../`)** | Guest uses relative path traversal sequences to escape mounted root boundaries | **Strict subtree containment**: Path resolution tokenizes components, enforces depth limits, and immediately terminates escaping traversals with `__WASI_ERRNO_PERM`. |
| **Absolute Path Attacks** | Guest attempts to reference absolute host paths (e.g., `/etc/passwd`) | **Mandatory relative paths**: All filesystem system calls require a valid directory file descriptor. Any path with a leading `/` is rejected with `__WASI_ERRNO_PERM`. |
| **Symlink Arbitrary Traversal** | Guest creates or resolves symlinks pointing outside the preopen sandbox | **Escape detection**: Symlink creation verifies target resolution statically; absolute symlinks and symlinks escaping the preopen root are rejected. |
| **Symlink Recursion / DoS** | Circular or excessively nested symlinks consume host CPU or stack space | **Bounded resolution**: Symlink traversals are capped at `kMaxNestedLinks` (returning `__WASI_ERRNO_LOOP`). |
| **Buffer & Memory Corruption** | Malicious parameters cause out-of-bounds reads/writes in host memory | **Span bounds checking**: All guest pointers and buffer lengths are strictly validated against guest linear memory bounds before copying or writing. |
| **Unauthorized Write / Mutation** | Guest modifies configuration, binaries, or assets mounted as read-only | **Rights bitmask enforcement**: When `:readonly` is configured, mutating capability rights are stripped from descriptors, blocking write/create/unlink calls with `__WASI_ERRNO_NOTCAPABLE`. |

---

## Capability-Based Security Architecture

WasmEdge implements WASI Preview 1 (`wasi_snapshot_preview1`) based on object capabilities rather than ambient authority. In this model, holding a file descriptor represents the capability to perform operations on the underlying kernel resource, subject to fine-grained permission bitmasks.

### Rights Representation and Inheritance

Every open file descriptor in WasmEdge is managed through an abstract virtual node (`VINode`) containing two 64-bit rights bitmasks:

1. **`FsRightsBase` (`__wasi_rights_t`)**: Specifies the exact operations permissible directly on this descriptor (e.g., `FD_READ`, `FD_WRITE`, `FD_SEEK`, `PATH_OPEN`, `PATH_UNLINK_FILE`).
2. **`FsRightsInheriting` (`__wasi_rights_t`)**: Specifies the maximum rights that may be inherited by any new file descriptor opened through this directory descriptor.

```
                    +------------------------------------+
                    |        Preopened Directory         |
                    |        (CLI: --dir guest:host)     |
                    |  FsRightsBase: All or Read-Only    |
                    |  FsRightsInheriting: Filtered Base |
                    +------------------------------------+
                                      |
                                  path_open
                                      v
                    +------------------------------------+
                    |       Derived Child Descriptor     |
                    |       (e.g., opened child file)    |
                    |  Rights = Requested & Inheriting   |
                    +------------------------------------+
```

Before delegating any call to the underlying operating system kernel, WasmEdge evaluates descriptor rights using the `can()` assertion:

```cpp
// Example rights verification pattern in WasmEdge host implementations
if (!Fd->can(__WASI_RIGHTS_PATH_UNLINK_FILE)) {
  return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
}
```

If the requested right is not present in `FsRightsBase`, WasmEdge aborts the call immediately and returns `__WASI_ERRNO_NOTCAPABLE` without invoking the underlying host kernel API.

### Preopened File Descriptors

In compliance with the WASI standard, standard streams reserve initial file descriptors:
* Descriptor `0`: Standard Input (`stdin`)
* Descriptor `1`: Standard Output (`stdout`)
* Descriptor `2`: Standard Error (`stderr`)

Preopened directory trees configured by the host administrator begin at file descriptor `3`. Guest applications discover available preopens by enumerating descriptors starting at `3` using `fd_prestat_get` and `fd_prestat_dir_name`. A guest has no access to any host directory or file unless that directory (or an ancestor directory) was preopened.

---

## Validation of WASI Interface Implementations

WasmEdge validates WASI interface usage through multiple distinct layers:

### 1. Pre-Execution Bytecode and Signature Validation

During the module loading and validation phase (prior to runtime instantiation), WasmEdge's Validator validates all imported functions:
* Imports declaring the `wasi_snapshot_preview1` module namespace are checked against known WASI function signatures (parameter types and return types).
* Import type mismatches result in validation failure, preventing malformed or mismatched modules from executing.
* Exported memory requirements are checked: WASI host functions require an exported memory instance named `memory` to read and write guest buffers.

### 2. Memory Boundary Validation and Buffer Safety

All guest memory operations in WASI functions pass through WasmEdge's memory management layer:
* **Linear Memory Bounds**: Pointers passed from guest code (`uint32_t` offsets) are converted to host spans (`Span<uint8_t>`). Every read and write operation is verified against the size of the WebAssembly linear memory.
* **Vectorized IO (`iovec`) Validation**: Functions such as `fd_read` and `fd_write` take arrays of `__wasi_iovec_t` structures. WasmEdge validates both the outer array buffer and each referenced inner buffer segment.
* **Overflow Protection**: All pointer arithmetic and buffer offset calculations use checked integer arithmetic to prevent 32-bit wrap-around vulnerabilities.

### 3. Parameter Sanitization and Null-Byte Injection Prevention

Paths supplied to WASI filesystem APIs (`path_open`, `path_create_directory`, `path_unlink_file`, `path_rename`, `path_symlink`) are validated before path resolution:
* **Null-Byte Injection**: System calls check that paths do not contain embedded null characters (`\0`):
  ```cpp
  static bool isPathValid(std::string_view Path) noexcept {
    return Path.find('\0') == std::string_view::npos;
  }
  ```
  If a null byte is found, the system call fails immediately with `__WASI_ERRNO_INVAL`.

---

## Path Resolution and Traversal Prevention

The most critical security requirement of a WASI implementation is preventing directory escape attacks (e.g., path traversal sequences like `../../etc/passwd` attempting to break out of a preopened directory).

WasmEdge implements strict component-by-component canonical path resolution in `VINode::resolvePath` located in `lib/host/wasi/vinode.cpp`:

### Path Traversal Defense Mechanisms

1. **Absolute Path Rejection**:
   WASI path APIs require paths to be resolved relative to a base directory descriptor. Any path with a leading slash (`/`) is rejected immediately:
   ```cpp
   if (!Path.empty() && Path[0] == '/') {
     return WasiUnexpect(__WASI_ERRNO_PERM);
   }
   ```
2. **Component Tokenization**:
   Paths are decomposed into individual segments separated by `/`. Redundant slashes (e.g., `a//b`) and current-directory tokens (`.`) are skipped.
3. **Tracking Preopen Descent with `PartFds`**:
   WasmEdge maintains a stack of open virtual node descriptors (`PartFds`) representing the directory descent depth below the preopened root descriptor.
4. **Parent Directory (`..`) Escape Prevention**:
   When a parent directory token (`..`) is encountered:
   * If the descent stack `PartFds` is empty, the traversal is attempting to ascend above the preopened root directory.
   * WasmEdge immediately aborts path resolution and returns `__WASI_ERRNO_PERM`:
   ```cpp
   if (Part.size() == 2 && Part[1] == '.') {
     if (PartFds.empty()) {
       return WasiUnexpect(__WASI_ERRNO_PERM);
     }
     Fd = std::move(PartFds.back());
     PartFds.pop_back();
     Path = Remain;
     continue;
   }
   ```
   This guarantees that a guest module cannot navigate above the preopened base directory under any sequence of `..` segments.

---

## Symlink Handling and Directory Namespaces

Symbolic links introduce potential breakout vectors if target paths refer to files outside the mounted preopened root. WasmEdge implements multi-stage symlink validation:

### 1. Static Escape Validation on Link Creation

When a guest calls `path_symlink` to create a new symbolic link, WasmEdge inspects the link target (`OldPath`):
* **Absolute Targets Forbidden**: Symlink targets beginning with `/` are rejected with `__WASI_ERRNO_PERM`.
* **Relative Target Escape Check**: WasmEdge calculates the creation directory's depth below the preopened root (`ParentDepth`). It evaluates the target string using `isSymlinkTargetEscaping`:
  ```cpp
  static bool isSymlinkTargetEscaping(std::string_view Target, uint32_t Depth) noexcept {
    int64_t Level = static_cast<int64_t>(Depth);
    // Evaluates each segment: increments Level on descent, decrements on '..'
    // If Level drops below 0, the target escapes the base preopen root.
  }
  ```
  If the link target escapes the base preopen root, link creation is rejected with `__WASI_ERRNO_PERM`.

### 2. Bounded Symlink Recursion

During path resolution (`resolvePath`), encountering a symbolic link causes WasmEdge to read the link contents via `pathReadlink`, prepend any remaining path components, and restart traversal. To prevent denial-of-service from infinite symlink loops, WasmEdge bounds nested link traversals:
* Recursion counter `LinkCount` is incremented for every resolved symlink.
* If `LinkCount >= kMaxNestedLinks`, resolution terminates with `__WASI_ERRNO_LOOP`.

### 3. Strict Symlink Lookup Flags

WASI system calls accept `__wasi_lookupflags_t`. WasmEdge follows symlinks for the trailing path component only when `__WASI_LOOKUPFLAGS_SYMLINK_FOLLOW` is explicitly set. Operations such as `path_readlink` and `path_unlink_file` operate directly on the symbolic link itself without following the target.

---

## Preopened Directory Configuration (`--dir`)

WasmEdge allows administrators to mount host directories into the WebAssembly virtual namespace using the `--dir` CLI flag or C API bindings.

### CLI Syntax and Modes

```sh
# Read-write mount: guest can read, write, create, and delete within host_dir
wasmedge --dir guest_path:host_dir my_app.wasm

# Read-only mount: mutating rights are completely stripped
wasmedge --dir guest_path:host_dir:readonly my_app.wasm
```

### Read-Only Enforcement

When `:readonly` is specified, WasmEdge removes mutating rights during preopen descriptor instantiation. The resulting descriptor will have:
* `__WASI_RIGHTS_FD_WRITE` cleared
* `__WASI_RIGHTS_FD_ALLOCATE` cleared
* `__WASI_RIGHTS_PATH_CREATE_DIRECTORY` cleared
* `__WASI_RIGHTS_PATH_CREATE_FILE` cleared
* `__WASI_RIGHTS_PATH_LINK_SOURCE` and `PATH_LINK_TARGET` cleared
* `__WASI_RIGHTS_PATH_RENAME_SOURCE` and `PATH_RENAME_TARGET` cleared
* `__WASI_RIGHTS_PATH_UNLINK_FILE` and `PATH_REMOVE_DIRECTORY` cleared
* `__WASI_RIGHTS_PATH_FILESTAT_SET_TIMES` and `PATH_FILESTAT_SET_SIZE` cleared

Attempts by guest applications to perform mutating operations on read-only mounts fail at the capability check with `__WASI_ERRNO_NOTCAPABLE` or `__WASI_ERRNO_ROFS`.

---

## Secure Deployment Guidance and Best Practices

To ensure secure execution in production, multi-tenant, and containerized deployments, follow these guidelines:

### 1. Apply the Principle of Least Privilege

* **Scope Directory Mounts Narrowly**: Mount only the specific subdirectories required by the application. Never mount broad parent directories such as `/`, `/usr`, `/var`, `/home`, or system configuration directories (`/etc`).
* **Enforce Read-Only Mounts by Default**: If an application only needs to read model weights, configuration files, or templates, always append `:readonly` (e.g., `--dir /models:/var/models:readonly`).

### 2. Environment Variable Hygiene

* WASI allows applications to read environment variables via `environ_get` and `environ_sizes_get`.
* WasmEdge does not inherit host environment variables by default.
* Explicitly pass only necessary variables using `--env KEY=VALUE`. Do not expose host credentials, tokens, or private endpoints unless required.

### 3. Resource Limiting and Denial-of-Service Defenses

* **Gas / Instruction Counting**: Enable statistics or cost measuring (`--enable-all-statistics` or API cost limit) to prevent runaway loops or CPU exhaustion in multi-tenant environments.
* **Time Limits**: Set execution deadlines where supported to protect against hanging host calls.
* **Memory Constraints**: Configure memory page limits to prevent unbounded WebAssembly linear memory growth.

### 4. Socket and Network Hardening

* When using WasmEdge WASI socket extensions, ensure socket listeners bind to loopback addresses (`127.0.0.1`) rather than `0.0.0.0` unless external network ingress is strictly necessary.
* Use container network namespaces or host firewalls to enforce network egress filtering.

---

## Common Security Pitfalls and Mitigations

| Anti-Pattern / Pitfall | Security Risk | Recommended Mitigation |
| :--- | :--- | :--- |
| Mounting root or system directory (`--dir /:/`) | Entire host filesystem exposed to guest; vulnerability in guest permits reading/tampering with host files | Never mount `/` or system paths. Mount isolated scratch directories (e.g., `--dir /workspace:/tmp/scratch`). |
| Omission of `:readonly` on static assets | Guest can overwrite application binaries, static configuration, or model weights | Always append `:readonly` to directories containing static assets or configuration. |
| Insecure symlink targets in host directories | Pre-existing symlinks in host directories pointing outside the mount may be followed if permissions permit | Audit host directories before mounting; ensure no pre-existing links cross security boundaries. |
| Leaking sensitive environment variables via `--env` | Guest bytecode reads credentials or API keys that are unnecessary for its function | Whitelist specific environment variables; never dump the host environment wholesale into the guest. |
| Missing memory and instruction limits | Malicious guest enters infinite loop or allocates maximum memory (4 GB) causing host OOM | Enforce memory page limits and monitor CPU consumption via container or process cgroups. |

---

## References

* [WasmEdge Security Policy](https://github.com/WasmEdge/WasmEdge/blob/master/SECURITY.md)
* [WasmEdge Security Self-Assessment](https://github.com/WasmEdge/WasmEdge/blob/master/docs/self-assessment.md)
* [WasmEdge Technical Review](https://github.com/WasmEdge/WasmEdge/blob/master/docs/technical-review.md)
* [WASI Specification (WebAssembly System Interface)](https://wasi.dev/)
* [CNCF TAG-Security Cloud Native Security Tenets](https://tag-security.cncf.io/)
